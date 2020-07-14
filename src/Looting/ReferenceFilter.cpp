/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2020

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses
>>> END OF LICENSE >>>
*************************************************************************/
#include "PrecompiledHeaders.h"

#include "Data/dataCase.h"
#include "Utilities/utils.h"
#include "Looting/tasks.h"
#include "WorldState/ActorTracker.h"
#include "WorldState/LocationTracker.h"
#include "Looting/ReferenceFilter.h"

namespace shse
{

ReferenceFilter::ReferenceFilter(DistanceToTarget& refs, IRangeChecker& rangeCheck, const bool respectDoors, const size_t limit) :
	m_refs(refs), m_rangeCheck(rangeCheck), m_respectDoors(respectDoors), m_nearestDoor(0.), m_limit(limit)
{
}

/*
Lootability can be subjective and/or time-sensitive. Dynamic forms (FormID 0xffnnnnnn) may be deleted from under our feet.
Current hypothesis is that this is safe so long as the base object is not itself dynamic.
Example from play-testing CTD logs where we crashed getting FormID for a pending-loot dead body.

Line 3732034: 0x8e70 (2020 - 05 - 23 07:24 : 16.910) J : \GitHub\SmartHarvestSE\tasks.cpp(1034) : [MESSAGE] Process REFR 0xff0024e9 with base object Ancient Nord Arrow / 0x0003be1a
Line 3735016 : 0x8e70 (2020 - 05 - 23 07:24 : 17.800) J : \GitHub\SmartHarvestSE\tasks.cpp(1034) : [MESSAGE] Process REFR 0xff0024e9 with base object Ancient Nord Arrow / 0x0003be1a
Line 3737998 : 0x8e70 (2020 - 05 - 23 07:24 : 18.700) J : \GitHub\SmartHarvestSE\tasks.cpp(1034) : [MESSAGE] Process REFR 0xff0024e9 with base object Ancient Nord Arrow / 0x0003be1a
Line 3785563 : 0x8e70 (2020 - 05 - 23 07:24 : 36.095) J : \GitHub\SmartHarvestSE\tasks.cpp(1034) : [MESSAGE] Process REFR 0xff0024eb with base object Restless Skeleton / 0xff0024ed
Line 3785564 : 0x8e70 (2020-05-23 07:24:36.095) J:\GitHub\SmartHarvestSE\tasks.cpp(277): [DEBUG] Enqueued dead body to loot later 0xff0024eb
Line 3785565 : 0x8e70 (2020-05-23 07:24:36.095) J:\GitHub\SmartHarvestSE\utils.cpp(211): [MESSAGE] TIME(Process Auto-loot Candidate Restless Skeleton/0xff0024ed)=114 micros

When we come to process this dead body a little later, we get CTD. The FormID has morphed, meaning the REFR was junked.

Line 3797443 : 0x8e70 (2020-05-23 07:24:39.705) J:\GitHub\SmartHarvestSE\tasks.cpp(295): [DEBUG] Process enqueued dead body 0x10000000
Line 3797443 : 0x8e70 (2020-05-23 07:24:40.707) J:\GitHub\SmartHarvestSE\LogStackWalker.cpp(7): [MESSAGE] Callstack dump :
...snip...
J:\GitHub\CommonLibSSE\src\RE\TESForm.cpp (110): RE::TESForm::GetFormID
J:\GitHub\SmartHarvestSE\tasks.cpp (1033): SearchTask::DoPeriodicSearch
J:\GitHub\SmartHarvestSE\tasks.cpp (690): SearchTask::ScanThread
J:\GitHub\SmartHarvestSE\tasks.cpp (705): `SearchTask::Start'::`2'::<lambda_1>::operator()
C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.25.28610\include\type_traits (1610): std::_Invoker_functor::_Call<`SearchTask::Start'::`2'::<lambda_1> >
C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.25.28610\include\type_traits (1610): std::invoke<`SearchTask::Start'::`2'::<lambda_1> >
C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.25.28610\include\thread (44): std::thread::_Invoke<std::tuple<`SearchTask::Start'::`2'::<lambda_1> >,0>

The failing REFR is a dynamic form referencing a dynamic base form. Earlier REFRs dynamic -> non-dynamic worked OK.

Dynamic forms may be deleted by script. This means we must be especially wary in handling them. 
For now, choose to flat-out ignore any REFR to a Dynamic Base - manual looting is still possible if REFR is not deleted. Filtering also includes
never recording a Dynamic REFR or Base Form in our filter lists, as Dynamic REFR FormIDs are recycled.
*/

bool ReferenceFilter::CanLoot(const RE::TESObjectREFR* refr) const
{
	DataCase* data = DataCase::GetInstance();
	if (data->IsFormBlocked(refr->GetBaseObject()))
	{
		DBG_VMESSAGE("skip REFR 0x%08x, blocked base form 0x%08x", refr->formID, refr->GetBaseObject() ? refr->GetBaseObject()->GetFormID() : InvalidForm);
		return false;
	}

	if (data->IsReferenceBlocked(refr))
	{
		DBG_VMESSAGE("skip blocked REFR for object/container 0x%08x", refr->formID);
		return false;
	}

	// check blacklist early - this may be a malformed REFR e.g. GetBaseObject() blank, 0x00000000 FormID
	// as observed in play testing
	if (data->IsReferenceOnBlacklist(refr))
	{
		DBG_VMESSAGE("skip blacklisted REFR 0x%08x", refr->GetFormID());
		return false;
	}

	const RE::TESFullName* fullName = refr->GetBaseObject()->As<RE::TESFullName>();
	if (!fullName || fullName->GetFullNameLength() == 0)
	{
		data->BlacklistReference(refr);
		DBG_VMESSAGE("blacklist REFR with blank name 0x%08x, base form 0x%08x", refr->formID, refr->GetBaseObject()->GetFormID());
		return false;
	}

	if (refr == RE::PlayerCharacter::GetSingleton())
	{
		DBG_VMESSAGE("skip PlayerCharacter %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
		return false;
	}

	// Actor derives from REFR
	if (refr->GetFormType() == RE::FormType::ActorCharacter)
	{
		if (!refr->IsDead(true))
		{
			DBG_VMESSAGE("skip living Actor/NPC %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
			shse::ActorTracker::Instance().RecordLiveSighting(refr);
			return false;
		}
	}

	if ((refr->GetBaseObject()->formType == RE::FormType::Flora || refr->GetBaseObject()->formType == RE::FormType::Tree) &&
		((refr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested) == RE::TESObjectREFR::RecordFlags::kHarvested))
	{
		DBG_VMESSAGE("skip REFR 0x%08x to harvested Flora %s/0x%08x", refr->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		return false;
	}

	if (SearchTask::IsLockedForHarvest(refr))
	{
		DBG_VMESSAGE("skip REFR, harvest pending %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
		return false;
	}
	if (SearchTask::IsLootedContainer(refr))
	{
		DBG_VMESSAGE("skip looted container %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
		return false;
	}

	// FormID can be retrieved using pointer, but we should not dereference the pointer as the REFR may have been recycled
	RE::FormID dynamicForm(SearchTask::LootedDynamicContainerFormID(refr));
	if (dynamicForm != InvalidForm)
	{
		DBG_VMESSAGE("skip looted dynamic container at %p with Form ID 0x%08x", refr, dynamicForm);
		return false;
	}

	DBG_VMESSAGE("lootable candidate 0x%08x", refr->formID);
	return true;
}

bool ReferenceFilter::IsLootCandidate(const RE::TESObjectREFR* refr) const
{
	DataCase* data = DataCase::GetInstance();
	// check blacklist early - this may be a malformed REFR e.g. GetBaseObject() blank, 0x00000000 FormID
	// as observed in play testing
	if (data->IsReferenceOnBlacklist(refr))
	{
		DBG_VMESSAGE("skip blacklisted REFR 0x%08x", refr->GetFormID());
		return false;
	}

	if (refr->formType == RE::FormType::ActorCharacter)
	{
		if (refr == RE::PlayerCharacter::GetSingleton())
		{
			DBG_VMESSAGE("skip PlayerCharacter %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
			return false;
		}
	}

	// FormID can be retrieved using pointer, but we should not dereference the pointer as the REFR may have been recycled
	RE::FormID dynamicForm(SearchTask::LootedDynamicContainerFormID(refr));
	if (dynamicForm != InvalidForm)
	{
		DBG_VMESSAGE("skip looted dynamic container at %p with Form ID 0x%08x", refr, dynamicForm);
		return false;
	}
	const RE::TESFullName* fullName = refr->GetBaseObject()->As<RE::TESFullName>();
	if (!fullName || fullName->GetFullNameLength() == 0)
	{
		data->BlacklistReference(refr);
		DBG_VMESSAGE("blacklist REFR with blank name 0x%08x", refr->formID);
		return false;
	}

	DBG_VMESSAGE("permissive lootable candidate 0x%08x", refr->formID);
	return true;
}

void ReferenceFilter::FindLootableReferences()
{
	m_predicate = std::bind(&ReferenceFilter::CanLoot, this, std::placeholders::_1);
	FilterNearbyReferences();
}

void ReferenceFilter::FindAllCandidates()
{
	m_predicate = std::bind(&ReferenceFilter::IsLootCandidate, this, std::placeholders::_1);
	FilterNearbyReferences();
}

// update list with all possibly lootable REFRs in cell
void ReferenceFilter::RecordCellReferences(const RE::TESObjectCELL* cell)
{
	// Do not scan reference list until cell is attached
	if (!cell->IsAttached())
		return;

	DBG_MESSAGE("Filter %d REFRS in CELL 0x%08x", cell->references.size(), cell->GetFormID());
	for (const RE::TESObjectREFRPtr& refptr : cell->references)
	{
		RE::TESObjectREFR* refr(refptr.get());
		if (refr)
		{
			if (!refr->GetBaseObject())
			{
				DBG_VMESSAGE("null base object for REFR 0x%08x", refr->GetFormID());
				continue;
			}

			// if 3D not loaded do not measure
			if (!refr->Is3DLoaded())
			{
				DBG_VMESSAGE("skip REFR 0x%08x, 3D not loaded %s/0x%08x", refr->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
				continue;
			}

			// If Looting through Doors is not allowed, check distance and record if this is the nearest so far
			RE::FormType formType(refr->GetBaseObject()->formType);
			if (formType == RE::FormType::Door)
			{
				if (m_respectDoors && m_rangeCheck.IsValid(refr) && (m_nearestDoor == 0. || m_rangeCheck.Distance() < m_nearestDoor))
				{
					DBG_VMESSAGE("New nearest Door 0x%08x(%s) at distance %.2f", refr->GetFormID(), refr->GetBaseObject()->GetName(), m_rangeCheck.Distance());
					m_nearestDoor = m_rangeCheck.Distance();
					m_rangeCheck.SetRadius(m_nearestDoor);
				}
				else
				{
					DBG_VMESSAGE("skip Door 0x%08x(%s)", refr->GetFormID(), refr->GetBaseObject()->GetName());
				}
				continue;
			}
			// skip other invalid form types
			if (formType == RE::FormType::Furniture ||
				formType == RE::FormType::Hazard ||
				formType == RE::FormType::IdleMarker ||
				formType == RE::FormType::MovableStatic ||
				formType == RE::FormType::Static)
			{
				DBG_VMESSAGE("invalid formtype %d for 0x%08x(%s)", formType, refr->GetFormID(), refr->GetBaseObject()->GetName());
				continue;
			}
			if (refr->formType == RE::FormType::ActorCharacter && refr->As<RE::Actor>()->GetActorBase() && !refr->IsDead(true))
			{
				// TODO hook this up to allow safe stealing
				EventPublisher::Instance().TriggerCheckDetectedBy(refr);
			}

			if (!m_rangeCheck.IsValid(refr))
			{
				DBG_VMESSAGE("omit out of range REFR 0x%08x(%s)", refr->GetFormID(), refr->GetBaseObject()->GetName());
				continue;
			}
			if (!m_predicate(refr))
			{
				DBG_VMESSAGE("omit ineligible REFR 0x%08x(%s)", refr->GetFormID(), refr->GetBaseObject()->GetName());
				continue;
			}
			DBG_VMESSAGE("add REFR 0x%08x(%s), distance %.2f, formtype %d", refr->GetFormID(),
				refr->GetBaseObject()->GetName(), m_rangeCheck.Distance(), formType);
			m_refs.emplace_back(m_rangeCheck.Distance(), refr);
		}
	}
}

void ReferenceFilter::FilterNearbyReferences()
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Filter loot candidates in/near cell");
#endif
	const RE::TESObjectCELL* cell(LocationTracker::Instance().PlayerCell());
	if (!cell)
		return;

	// For exterior cells, also check directly adjacent cells for lootable goodies. Restrict to cells in the same worldspace.
	// If current cell fills the list then ignore others.
	RecordCellReferences(cell);
	if (!LocationTracker::Instance().IsPlayerIndoors())
	{
		DBG_VMESSAGE("Scan cells adjacent to 0x%08x", cell->GetFormID());
		for (const auto& adjacentCell : LocationTracker::Instance().AdjacentCells())
		{
			// sanity checks
			if (!adjacentCell || !adjacentCell->IsAttached())
			{
				DBG_VMESSAGE("Adjacent cell null or unattached");
				continue;
			}
			DBG_VMESSAGE("Check adjacent cell 0x%08x", adjacentCell->GetFormID());
			RecordCellReferences(adjacentCell);
		}
	}

	// Postprocess the list into ascending distance order and truncate if too long
	// We must confirm distance is valid because the Radius may update adjusted on the fly as Doors are processed
	DBG_MESSAGE("Sort and truncate %d eligible REFRs", m_refs.size());

	// This logic needs to reliably handle load spikes. We do not commit to process more than N references. The rest will get processed on future passes.
	// A spike of 200+ in a second makes the VM dump stacks, so pick N accordingly. Prefer closer references, so partition the list by distance order so we handle
	// no more than N. std::nth_element does precisely what we need.
	// End-of-range iterator remains valid as the container is processed in situ by each algorithms
	auto endOfRange(m_refs.begin() + std::min(m_limit, m_refs.size()));
	std::nth_element(m_refs.begin(), endOfRange, m_refs.end(),
		[&](const TargetREFR& a, const TargetREFR& b) ->bool { return a.first < b.first; });
	std::sort(m_refs.begin(), endOfRange, [&](const TargetREFR& a, const TargetREFR& b) ->bool { return a.first < b.first; });
	// "Nearest Door" restriction can adjust the range downwards during the scan - re-check here
	if (m_respectDoors)
	{
		auto tooFarAway(std::find_if(m_refs.begin(), endOfRange, [&](const auto& target) -> bool
		{
			return target.first > m_rangeCheck.Radius();
		}));

		DBG_MESSAGE("Erase %d out-of-range REFRs", std::distance(tooFarAway, m_refs.end()));
		m_refs.erase(tooFarAway, m_refs.end());
	}
}

}