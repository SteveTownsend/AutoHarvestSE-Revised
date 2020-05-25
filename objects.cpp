#include "PrecompiledHeaders.h"

#include "iniSettings.h"
#include "utils.h"
#include "objects.h"
#include "dataCase.h"
#include "basketfile.h"
#include "containerLister.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "ExtraDataListHelper.h"

#include "CommonLibSSE/include/RE/BGSProjectile.h"

bool IsBossContainer(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;

	const RE::ExtraDataList* extraList = &refr->extraList;

	if (!extraList)
		return nullptr;

	const RE::ExtraLocationRefType* exLocRefType = extraList->GetByType<RE::ExtraLocationRefType>();
	return exLocRefType && exLocRefType->locRefType->formID == 0x0130F8;
}

// this logic is essential - TESObjectREFR::IsLocked() is not reliable
bool IsContainerLocked(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;

	const RE::ExtraDataList* extraList = &refr->extraList;
	if (!extraList)
		return false;
		
	const RE::ExtraLock* exLock(extraList->GetByType<RE::ExtraLock>());
	return exLock && (exLock->lock->flags & RE::REFR_LOCK::Flag::kLocked) == RE::REFR_LOCK::Flag::kLocked;
}

// This appears safe to call during combat - does not introspect an in-flux ExtraDataList, just a bitfield
bool HasAshPile(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;

	const RE::ExtraDataList* extraList = &refr->extraList;
	if (!extraList)
		return false;

	return extraList->HasType(RE::ExtraDataType::kAshPileRef);
}

// This is unsafe to call during combat - used only during deferred looting of Dead Bodies
RE::TESObjectREFR* GetAshPile(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return nullptr;

	const RE::ExtraDataList* extraList = &refr->extraList;
	if (!extraList)
		return nullptr;

	RE::ObjectRefHandle ashHandle = const_cast<RE::ExtraDataList*>(extraList)->GetAshPileRefHandle();
	return ashHandle.get().get();
}

TESObjectREFRHelper::TESObjectREFRHelper(const RE::TESObjectREFR* ref) : m_ref(ref), m_lootable(nullptr)
{
	m_objectType = ClassifyType(m_ref);
	m_typeName = GetObjectTypeName(m_objectType);
}

double TESObjectREFRHelper::GetPosValue()
{
	if (!m_ref)
		return -1.0;
	double dx = m_ref->GetPositionX();
	double dy = m_ref->GetPositionY();
	double dz = m_ref->GetPositionZ();
	return (dx*dx + dy*dy + dz*dz);
}

bool TESObjectREFRHelper::IsQuestItem(const bool requireFullQuestFlags)
{
	if (!m_ref)
		return false;

	RE::RefHandle handle;
	RE::CreateRefHandle(handle, const_cast<RE::TESObjectREFR*>(m_ref));

	RE::NiPointer<RE::TESObjectREFR> targetRef;
	RE::LookupReferenceByHandle(handle, targetRef);

	if (!targetRef)
		targetRef.reset(const_cast<RE::TESObjectREFR*>(m_ref));

	ExtraDataListHelper extraListEx(&targetRef->extraList);
	if (!extraListEx.m_extraData)
		return false;

	return extraListEx.IsQuestObject(requireFullQuestFlags);
}

std::vector<RE::TESObjectREFR*> TESObjectREFRHelper::GetLinkedRefs(RE::BGSKeyword* keyword)
{
	std::vector<RE::TESObjectREFR*> result;
	const RE::ExtraLinkedRef* exLinkRef(m_ref->extraList.GetByType<RE::ExtraLinkedRef>());
	if (!exLinkRef)
		return result;

	for (const auto pair : exLinkRef->linkedRefs)
	{
		if (!pair.refr || pair.keyword != keyword)
			continue;
		result.push_back(pair.refr);
	}
	return result;
}

RE::TESContainer* TESObjectREFRHelper::GetContainer() const
{
	if (!m_ref->data.objectReference)
		return nullptr;

	RE::TESContainer *container = nullptr;
	if (m_ref->data.objectReference->formType == RE::FormType::Container ||
	    m_ref->data.objectReference->formType == RE::FormType::NPC)
		container = m_ref->data.objectReference->As<RE::TESContainer>();

	return container;
}

bool TESObjectREFRHelper::IsPlayerOwned()
{
	const RE::TESForm* owner = m_ref->GetOwner();
	if (owner)
	{
		if (owner->formType == RE::FormType::NPC)
		{
			const RE::TESNPC* npc = owner->As<RE::TESNPC>();
			RE::TESNPC* playerBase = RE::PlayerCharacter::GetSingleton()->GetActorBase();
			return (npc && npc == playerBase);
		}
		else if (owner->formType == RE::FormType::Faction)
		{
			const RE::TESFaction* faction = owner->As<RE::TESFaction>();
			if (faction)
			{
				return RE::PlayerCharacter::GetSingleton()->IsInFaction(faction);
			}
		}
	}
	return false;
}

RE::TESForm* TESObjectREFRHelper::GetLootable() const
{
	return m_lootable;
}

void TESObjectREFRHelper::SetLootable(RE::TESForm* lootable)
{
	m_lootable = lootable;
}

double TESObjectREFRHelper::GetWorth(void) const
{
	TESFormHelper itemEx(m_lootable ? m_lootable : m_ref->data.objectReference);
	return itemEx.GetWorth();
}

double TESObjectREFRHelper::GetWeight(void) const
{
	TESFormHelper itemEx(m_lootable ? m_lootable : m_ref->data.objectReference);
	return itemEx.GetWeight();
}

const char* TESObjectREFRHelper::GetName() const
{
	return m_ref->GetName();
}

UInt32 TESObjectREFRHelper::GetFormID() const
{
	return m_ref->data.objectReference->formID;
}

SInt16 TESObjectREFRHelper::GetItemCount()
{
	if (!m_ref)
		return 1;
	if (!m_ref->data.objectReference)
		return 1;
	if (m_lootable)
		return 1;
	if (m_objectType == ObjectType::oreVein)
	{
		// limit ore harvesting to constrain Player Home mining
		return static_cast<SInt16>(INIFile::GetInstance()->GetInstance()->GetSetting(INIFile::autoharvest, INIFile::config, "maxMiningItems"));
	}

	const RE::ExtraCount* exCount(m_ref->extraList.GetByType<RE::ExtraCount>());
	return (exCount) ? exCount->count : 1;
}

RE::NiTimeController* TESObjectREFRHelper::GetTimeController() const
{
	const RE::NiAVObject* node = m_ref->Get3D2();
	return (node && node->GetControllers()) ? node->GetControllers() : nullptr;
}

bool ActorHelper::IsSneaking() const
{
	return (m_actor && m_actor->IsSneaking());
}

bool ActorHelper::IsPlayerAlly() const
{
	if (!m_actor)
		return false;
	if (m_actor->IsPlayerTeammate())
	{
		return true;
	}
	static const RE::TESFaction* followerFaction = RE::TESForm::LookupByID(CurrentFollowerFaction)->As<RE::TESFaction>();
	return (followerFaction) ? m_actor->IsInFaction(followerFaction) : false;
}

bool ActorHelper::IsEssential() const 
{
	if (!m_actor)
		return false;
	RE::TESActorBaseData* baseData(m_actor->As<RE::TESActorBaseData>());
	if (!baseData)
		return false;
	return baseData->IsEssential();
}

bool ActorHelper::IsSummoned(void) const
{
	return m_actor ? m_actor->IsSummoned() : false;
}

ObjectType ClassifyType(const RE::TESObjectREFR* refr, bool ignoreUserlist)
{
	if (!refr || !refr->data.objectReference)
		return ObjectType::unknown;

	if (refr->formType == RE::FormType::ActorCharacter)
	{
		return ObjectType::actor;
	}
	else if (refr->data.objectReference->formType == RE::FormType::Activator && HasAshPile(refr))
	{
		return ObjectType::unknown;
	}
	else if (refr->formType == RE::FormType::ProjectileArrow)
	{
		return ObjectType::ammo;
	}
	else if (refr->data.objectReference->formType == RE::FormType::SoulGem)
	{
		return ObjectType::soulgem;
	}

	return ClassifyType(refr->data.objectReference, ignoreUserlist);
}

ObjectType ClassifyType(RE::TESForm* baseForm, bool ignoreUserlist)
{
	// Leveled items typically redirect to their contents
	baseForm = DataCase::GetInstance()->ConvertIfLeveledItem(baseForm);
	if (!baseForm)
		return ObjectType::unknown;

	if (!ignoreUserlist && BasketFile::GetSingleton()->IsinList(BasketFile::USERLIST, baseForm))
	{
		return ObjectType::userlist;
	}

	DataCase* data = DataCase::GetInstance();
	ObjectType objectType(data->GetObjectTypeForForm(baseForm));
	if (objectType != ObjectType::unknown)
	{
    	return objectType;
	}
	if (baseForm->formType == RE::FormType::Misc)
	{
		const RE::TESObjectMISC* miscObject = baseForm->As<RE::TESObjectMISC>();
		if (!miscObject)
			return ObjectType::unknown;

		return objectType;
	}
	else if (baseForm->formType == RE::FormType::Book)
	{
		const RE::TESObjectBOOK* book = baseForm->As<RE::TESObjectBOOK>();
		if (!book || !book->CanBeTaken())
			return ObjectType::unknown;
		bool isRead = book->IsRead();
		if (book->TeachesSkill())
			return (isRead) ? ObjectType::skillbookRead : ObjectType::skillbook;
		else if (book->TeachesSpell())
			return (isRead) ? ObjectType::spellbookRead : ObjectType::spellbook;
		return (isRead) ? ObjectType::booksRead : ObjectType::books;
	}
	else if (baseForm->formType == RE::FormType::Ammo || baseForm->formType == RE::FormType::Light)
	{
		if (baseForm->GetPlayable())
			return objectType;
	}
	else if (baseForm->formType == RE::FormType::Weapon)
	{
		const static RE::FormID Artifacts1 = 0x0A8668;
		const static RE::FormID Artifacts2 = 0x0917E8;

		const RE::TESObjectWEAP* weapon = baseForm->As<RE::TESObjectWEAP>();
		if (!weapon || !weapon->GetPlayable())
			return ObjectType::unknown;

		if (weapon->HasKeyword(Artifacts1) || weapon->HasKeyword(Artifacts2))
			return ObjectType::unknown;

		return (weapon->formEnchanting) ? ObjectType::enchantedWeapon : ObjectType::weapon;
	}
	else if (baseForm->formType == RE::FormType::Armor || objectType == ObjectType::jewelry)
	{
		const RE::TESObjectARMO* armor = baseForm->As<RE::TESObjectARMO>();
		if (!armor || !armor->GetPlayable())
			return ObjectType::unknown;

		if (objectType == ObjectType::jewelry)
			return (armor->formEnchanting) ? ObjectType::enchantedJewelry : ObjectType::jewelry;
		return (armor->formEnchanting) ? ObjectType::enchantedArmor : ObjectType::armor;
	}
	return ObjectType::unknown;
}

std::string GetObjectTypeName(SInt32 num)
{
	ObjectType objType = static_cast<ObjectType>(num);
	return GetObjectTypeName(objType);
}

std::string GetObjectTypeName(const RE::TESObjectREFR* refr)
{
	ObjectType objType = ClassifyType(refr);
	return GetObjectTypeName(objType);
}

std::string GetObjectTypeName(RE::TESForm* pForm)
{
	ObjectType objType = ClassifyType(pForm);
	return GetObjectTypeName(objType);
}

const std::unordered_map<ObjectType, std::string> nameByType({
	{ObjectType::unknown, "unknown"},
	{ObjectType::flora, "flora"},
	{ObjectType::critter, "critter"},
	{ObjectType::ingredients, "ingredients"},
	{ObjectType::septims, "septims"},
	{ObjectType::gems, "gems"},
	{ObjectType::lockpick, "lockpick"},
	{ObjectType::animalHide, "animalhide"},
	{ObjectType::animalParts, "animalparts"},
	{ObjectType::oreIngot, "oreingot"},
	{ObjectType::soulgem, "soulgem"},
	{ObjectType::keys, "keys"},
	{ObjectType::clutter, "clutter"},
	{ObjectType::light, "light"},
	{ObjectType::books, "books"},
	{ObjectType::spellbook, "spellbook"},
	{ObjectType::skillbook, "skillbook"},
	{ObjectType::booksRead, "booksread"},
	{ObjectType::spellbookRead, "spellbookread"},
	{ObjectType::skillbookRead, "skillbookread"},
	{ObjectType::scrolls, "scrolls"},
	{ObjectType::ammo, "ammo"},
	{ObjectType::weapon, "weapon"},
	{ObjectType::enchantedWeapon, "enchantedweapon"},
	{ObjectType::armor, "armor"},
	{ObjectType::enchantedArmor, "enchantedarmor"},
	{ObjectType::jewelry, "jewelry"},
	{ObjectType::enchantedJewelry, "enchantedjewelry"},
	{ObjectType::potion, "potion"},
	{ObjectType::poison, "poison"},
	{ObjectType::food, "food"},
	{ObjectType::drink, "drink"},
	{ObjectType::oreVein, "orevein"},
	{ObjectType::userlist, "userlist"},
	{ObjectType::container, "container"},
	{ObjectType::actor, "actor"},
	{ObjectType::ashPile, "ashpile"},
	{ObjectType::manualLoot, "manualloot"}
	});

std::string GetObjectTypeName(ObjectType type)
{
	const auto result(nameByType.find(type));
	if (result != nameByType.cend())
		return result->second;
	return "unknown";
}

ObjectType GetObjectTypeByTypeName(const std::string& name)
{
	// normalize from BSFixedString
	std::string lcName;
	std::transform(name.cbegin(), name.cend(), std::back_inserter(lcName), [](const char& c) { return std::tolower(c); });
	for (const auto& nextPair : nameByType)
	{
		if (nextPair.second == lcName)
		{
#if _DEBUG
			_DMESSAGE("Mapped name %s to ObjectType %d", name.c_str(), nextPair.first);
#endif
			return nextPair.first;
		}
	}
#if _DEBUG
	_DMESSAGE("Unmapped ObjectType name %s", name.c_str());
#endif
	return ObjectType::unknown;
}

