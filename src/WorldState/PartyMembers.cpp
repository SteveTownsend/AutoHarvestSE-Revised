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

#include "WorldState/PartyMembers.h"
#include "WorldState/LocationTracker.h"
#include "WorldState/Saga.h"
#include "Data/LoadOrder.h"

namespace shse
{

PartyUpdate::PartyUpdate(const RE::Actor* follower, const PartyUpdateType eventType, const float gameTime) :
	m_follower(follower), m_eventType(eventType), m_gameTime(gameTime)
{
}

std::string PartyUpdate::AsString() const
{
	std::ostringstream stream;
	if (m_eventType == PartyUpdateType::Joined)
	{
		stream << m_follower->GetName() << " traveled with me.";
	}
	else if (m_eventType == PartyUpdateType::Departed)
	{
		stream << m_follower->GetName() << " bade me farewell.";
	}
	if (m_eventType == PartyUpdateType::Died)
	{
		stream << m_follower->GetName() << " died while accompanying me.";
	}
	return stream.str();
}

void PartyUpdate::AsJSON(nlohmann::json& j) const
{
	j["follower"] = StringUtils::FromFormID(m_follower->GetFormID());
	j["event"] = int(m_eventType);
	j["time"] = m_gameTime;
}

void to_json(nlohmann::json& j, const PartyUpdate& partyUpdate)
{
	partyUpdate.AsJSON(j);
}

std::unique_ptr<PartyMembers> PartyMembers::m_instance;

PartyMembers& PartyMembers::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<PartyMembers>();
	}
	return *m_instance;
}

void PartyMembers::Reset()
{
	RecursiveLockGuard guard(m_partyLock);
	m_followers.clear();
}

void PartyMembers::RecordUpdate(const PartyUpdate& partyUpdate)
{
	m_partyUpdates.push_back(partyUpdate);
	Saga::Instance().AddEvent(partyUpdate);
}

void PartyMembers::AdjustParty(const Followers& followers, const float gameTime)
{
	RecursiveLockGuard guard(m_partyLock);
	// lists of followers do not get very large, just brute force it
	bool updated(false);
	for (const auto newFollower : followers)
	{
		if (m_followers.find(newFollower) == m_followers.cend())
		{
			updated = true;
			RecordUpdate(PartyUpdate(newFollower, PartyUpdateType::Joined, gameTime));
			DBG_MESSAGE("Follower {}/0x{:08x} joined party at {:0.3f}", newFollower->GetName(), newFollower->GetFormID(), gameTime);
		}
	}
	for (const auto existingFollower : m_followers)
	{
		if (followers.find(existingFollower) == followers.cend())
		{
			updated = true;
			PartyUpdateType updateType(existingFollower->IsDead(true) ? PartyUpdateType::Died : PartyUpdateType::Departed);
			RecordUpdate(PartyUpdate(existingFollower, updateType, gameTime));
			// Ensure Location is recorded
			LocationTracker::Instance().RecordCurrentPlace(gameTime);
			DBG_MESSAGE("Follower {}/0x{:08x} left party at {:0.3f}", existingFollower->GetName(), existingFollower->GetFormID(), gameTime);
		}
	}
	if (updated)
	{
		m_followers = followers;
	}
}

void PartyMembers::AsJSON(nlohmann::json& j) const
{
	nlohmann::json updates(nlohmann::json::array());
	for (const auto& update : m_partyUpdates)
	{
		updates.push_back(update);
	}
	j["updates"] = updates;
	nlohmann::json followers(nlohmann::json::array());
	for (const auto follower : m_followers)
	{
		followers.push_back(StringUtils::FromFormID(follower->GetFormID()));
	}
	j["followers"] = followers;
}

void PartyMembers::UpdateFrom(const nlohmann::json& j)
{
	REL_MESSAGE("Cosave Party Members\n{}", j.dump(2));
	RecursiveLockGuard guard(m_partyLock);
	m_partyUpdates.clear();
	m_partyUpdates.reserve(j["updates"].size());
	for (const nlohmann::json& update : j["updates"])
	{
		RE::FormID followerID(StringUtils::ToFormID(update["follower"].get<std::string>()));
		PartyUpdateType updateType(static_cast<PartyUpdateType>(update["event"].get<int>()));
		const float gameTime(update["time"].get<float>());
		RE::Actor* actor(LoadOrder::Instance().RehydrateCosaveFormAs<RE::Actor>(followerID));
		if (!actor)
		{
			REL_WARNING("Historic Follower (Actor) 0x{:08x} not found", followerID);
			continue;
		}
		RecordUpdate(PartyUpdate(actor, updateType, gameTime));
	}
	m_followers.clear();
	for (const nlohmann::json& follower : j["followers"])
	{
		RE::FormID followerID(StringUtils::ToFormID(follower.get<std::string>()));
		RE::Actor* actor(LoadOrder::Instance().RehydrateCosaveFormAs<RE::Actor>(followerID));
		if (!actor)
		{
			REL_WARNING("Current Follower (Actor) 0x{:08x} not found", followerID);
			continue;
		}
		m_followers.insert(actor);
	}
}

void to_json(nlohmann::json& j, const PartyMembers& partyMembers)
{
	partyMembers.AsJSON(j);
}

}