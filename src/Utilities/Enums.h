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
#pragma once

#include "Looting/ObjectType.h"

namespace shse
{

// object glow reasons, in descending order of precedence
enum class GlowReason {
	LockedContainer = 0,
	BossContainer,
	QuestObject,
	Collectible,
	Valuable,
	EnchantedItem,
	PlayerProperty,
	SimpleTarget,
	None
};

inline GlowReason CycleGlow(const GlowReason current)
{
	int next(int(current) + 1);
	if (next > int(GlowReason::SimpleTarget))
		return GlowReason::LockedContainer;
	return GlowReason(next);
}

inline std::string GlowName(const GlowReason glow)
{
	switch (glow) {
	case GlowReason::LockedContainer:
		return "Locked";
	case GlowReason::BossContainer:
		return "Boss";
	case GlowReason::QuestObject:
		return "Quest";
	case GlowReason::Collectible:
		return "Collectible";
	case GlowReason::Valuable:
		return "Valuable";
	case GlowReason::EnchantedItem:
		return "Enchanted";
	case GlowReason::PlayerProperty:
		return "PlayerOwned";
	case GlowReason::SimpleTarget:
		return "SimpleTarget";
	default:
		return "Unknown";
	}
}

enum class LootingType {
	LeaveBehind = 0,
	LootAlwaysSilent = 1,
	LootAlwaysNotify = 2,
	LootIfValuableEnoughSilent = 3,
	LootIfValuableEnoughNotify = 4,
	LootOreVeinIfNotBYOH = LootAlwaysSilent,
	LootOreVeinAlways = LootAlwaysNotify,
	MAX = 5
};

inline bool LootingRequiresNotification(const LootingType lootingType)
{
	return lootingType == LootingType::LootIfValuableEnoughNotify || lootingType == LootingType::LootAlwaysNotify;
}

inline LootingType LootingTypeFromIniSetting(const double iniSetting)
{
	uint32_t intSetting(static_cast<uint32_t>(iniSetting));
	if (intSetting >= static_cast<int32_t>(LootingType::MAX))
	{
		return LootingType::LeaveBehind;
	}
	return static_cast<LootingType>(intSetting);
}

enum class CollectibleHandling {
	Leave = 0,
	Take,
	Glow,
	Print,
	MAX
};

constexpr std::pair<bool, CollectibleHandling> NotCollectible = { false, CollectibleHandling::Leave };

enum class SpecialObjectHandling {
	DoNotLoot = 0,
	DoLoot,
	GlowTarget,
	MAX
};

enum class QuestObjectHandling {
	NoAction = 0,
	GlowTarget,
	MAX
};

inline CollectibleHandling UpdateCollectibleHandling(const CollectibleHandling initial, const CollectibleHandling next)
{
	// update if new is more permissive
	if (next == CollectibleHandling::Take)
	{
		return next;
	}
	else if (next == CollectibleHandling::Print)
	{
		return initial == CollectibleHandling::Take ? initial : next;
	}
	else if (next == CollectibleHandling::Glow)
	{
		return initial == CollectibleHandling::Take || initial == CollectibleHandling::Print ? initial : next;
	}
	else
	{
		// leave - this is the least permissive - initial cannot be any less so
		return initial;
	}
}

inline bool IsSpecialObjectLootable(const SpecialObjectHandling specialObjectHandling)
{
	return specialObjectHandling == SpecialObjectHandling::DoLoot;
}

inline bool CanLootCollectible(const CollectibleHandling collectibleHandling)
{
	return collectibleHandling == CollectibleHandling::Take;
}

inline bool CollectibleHistoryNeeded(const CollectibleHandling collectibleHandling)
{
	return collectibleHandling == CollectibleHandling::Take || collectibleHandling == CollectibleHandling::Glow;
}

inline std::string CollectibleHandlingString(const CollectibleHandling collectibleHandling)
{
	switch (collectibleHandling) {
	case CollectibleHandling::Take:
		return "take";
	case CollectibleHandling::Glow:
		return "glow";
	case CollectibleHandling::Print:
		return "print";
	case CollectibleHandling::Leave:
	default:
		return "leave";
	}
}

inline CollectibleHandling ParseCollectibleHandling(const std::string& action)
{
	if (action == "take")
		return CollectibleHandling::Take;
	if (action == "glow")
		return CollectibleHandling::Glow;
	if (action == "print")
		return CollectibleHandling::Print;
	return CollectibleHandling::Leave;
}

inline CollectibleHandling CollectibleHandlingFromIniSetting(const double iniSetting)
{
	uint32_t intSetting(static_cast<uint32_t>(iniSetting));
	if (intSetting >= static_cast<int32_t>(CollectibleHandling::MAX))
	{
		return CollectibleHandling::Leave;
	}
	return static_cast<CollectibleHandling>(intSetting);
}

inline SpecialObjectHandling SpecialObjectHandlingFromIniSetting(const double iniSetting)
{
	uint32_t intSetting(static_cast<uint32_t>(iniSetting));
	if (intSetting >= static_cast<int32_t>(SpecialObjectHandling::MAX))
	{
		return SpecialObjectHandling::DoNotLoot;
	}
	return static_cast<SpecialObjectHandling>(intSetting);
}

inline QuestObjectHandling QuestObjectHandlingFromIniSetting(const double iniSetting)
{
	uint32_t intSetting(static_cast<uint32_t>(iniSetting));
	if (intSetting >= static_cast<int32_t>(SpecialObjectHandling::MAX))
	{
		return QuestObjectHandling::NoAction;
	}
	return static_cast<QuestObjectHandling>(intSetting);
}

inline bool LootingDependsOnValueWeight(const LootingType lootingType, ObjectType objectType)
{
	if (objectType == ObjectType::septims ||
		objectType == ObjectType::key ||
		objectType == ObjectType::oreVein ||
		objectType == ObjectType::lockpick)
		return false;
	if (lootingType != LootingType::LootIfValuableEnoughNotify && lootingType != LootingType::LootIfValuableEnoughSilent)
		return false;
	return true;
}

enum class DeadBodyLooting {
	DoNotLoot = 0,
	LootExcludingArmor,
	LootAll,
	MAX
};

inline DeadBodyLooting DeadBodyLootingFromIniSetting(const double iniSetting)
{
	uint32_t intSetting(static_cast<uint32_t>(iniSetting));
	if (intSetting >= static_cast<int32_t>(DeadBodyLooting::MAX))
	{
		return DeadBodyLooting::DoNotLoot;
	}
	return static_cast<DeadBodyLooting>(intSetting);
}

// the approximate compass direction to take to reach a target
enum class CompassDirection {
	North = 0,
	NorthEast,
	East,
	SouthEast,
	South,
	SouthWest,
	West,
	NorthWest,
	AlreadyThere,	// no movement needed to get to target
	MAX
};

inline std::string CompassDirectionName(const CompassDirection direction)
{
	switch (direction) {
	case CompassDirection::North:
		return "north";
	case CompassDirection::NorthEast:
		return "northeast";
	case CompassDirection::East:
		return "east";
	case CompassDirection::SouthEast:
		return "southeast";
	case CompassDirection::South:
		return "south";
	case CompassDirection::SouthWest:
		return "southwest";
	case CompassDirection::West:
		return "west";
	case CompassDirection::NorthWest:
		return "northwest";
	default:
		return "";
	}
}

enum class OwnershipRule {
	AllowCrimeIfUndetected = 0,
	DisallowCrime,
	Ownerless,
	MAX
};

inline OwnershipRule OwnershipRuleFromIniSetting(const double iniSetting)
{
	uint32_t intSetting(static_cast<uint32_t>(iniSetting));
	if (intSetting >= static_cast<int32_t>(OwnershipRule::MAX))
	{
		return OwnershipRule::Ownerless;
	}
	return static_cast<OwnershipRule>(intSetting);
}

// Cell Ownership - lower values are less restrictive
enum class CellOwnership {
	NoOwner = 0,
	Player,
	PlayerFaction,
	OtherFaction,
	NPC,
	MAX
};

inline bool IsPlayerFriendly(const CellOwnership ownership)
{
	return ownership != CellOwnership::OtherFaction && ownership != CellOwnership::NPC;
}

inline std::string CellOwnershipName(const CellOwnership ownership)
{
	switch (ownership) {
	case CellOwnership::NoOwner:
		return "NoOwner";
	case CellOwnership::Player:
		return "Player";
	case CellOwnership::PlayerFaction:
		return "PlayerFaction";
	case CellOwnership::OtherFaction:
		return "OtherFaction";
	case CellOwnership::NPC:
		return "NPC";
	default:
		return "";
	}
}

enum class Lootability {
	Lootable = 0,
	BaseObjectBlocked,
	CannotRelootFirehoseSource,
	ContainerPermanentlyOffLimits,
	CorruptArrowPosition,
	CannotMineTwiceInSameCellVisit,
	ReferenceBlacklisted,
	UnnamedReference,
	ReferenceIsPlayer,
	ReferenceIsLiveActor,
	FloraHarvested,
	PendingHarvest,
	ContainerLootedAlready,
	DynamicReferenceLootedAlready,
	NullReference,
	InvalidFormID,
	NoBaseObject,
	LootDeadBodyDisabled,
	DeadBodyIsPlayerAlly,
	DeadBodyIsSummoned,
	DeadBodyIsEssential,
	DeadBodyDelayedLooting,
	DeadBodyPossibleDuplicate,
	LootContainersDisabled,
	HarvestLooseItemDisabled,
	PendingProducerIngredient,
	ObjectTypeUnknown,
	ManualLootTarget,
	BaseObjectOnBlacklist,
	CannotLootQuestTarget,
	ObjectIsInBlacklistCollection,
	CannotLootValuableObject,
	CannotLootEnchantedObject,
	CannotLootAmmo,
	PlayerOwned,
	CrimeToLoot,
	CellOrItemOwnerPreventsOwnerlessLooting,
	PopulousLocationRestrictsLooting,
	ItemInBlacklistCollection,
	CollectibleItemSetToGlow,
	LawAbidingSoNoWhitelistItemLooting,
	ItemIsBlacklisted,
	ItemTypeIsSetToPreventLooting,
	HarvestDisallowedForBaseObjectType,
	ValueWeightPreventsLooting,
	ItemTheftTriggered,
	HarvestOperationPending,
	ContainerHasNoLootableItems,
	ContainerIsLocked,
	ContainerIsBossChest,
	ContainerHasQuestObject,
	ContainerHasValuableObject,
	ContainerHasEnchantedObject,
	ReferencesBlacklistedContainer,
	CannotGetAshPile,
	ProducerHasNoLootable,
	ContainerBlacklistedByUser,
	DeadBodyBlacklistedByUser,
	NPCExcludedByDeadBodyFilter,
	NPCIsInBlacklistCollection,
	MAX
};

inline bool LootOwnedItemIfCollectible(const Lootability lootability)
{
	return lootability == Lootability::PlayerOwned ||
		lootability == Lootability::CellOrItemOwnerPreventsOwnerlessLooting;
}

std::string LootabilityName(const Lootability lootability);

enum class PlayerAffinity
{
	Unaffiliated = 0,
	TeamMate,
	FollowerFaction,
	Player,
	MAX
};

enum class PartyUpdateType
{
	Joined = 0,
	Departed,
	Died,
	MAX
};

enum class ReferenceScanType
{
	Loot = 0,
	NoLoot,
	Calibration,
	MAX
};

enum class SerializationRecordType
{
	LoadOrder = 0,
	Collections,
	PlacesVisited,
	PartyUpdates,
	Victims,
	Adventures,
	MAX
};

inline std::string SerializationRecordName(const SerializationRecordType recordType)
{
	switch (recordType) {
	case SerializationRecordType::LoadOrder:
		return "LORD";
	case SerializationRecordType::Collections:
		return "COLL";
	case SerializationRecordType::PlacesVisited:
		return "PLAC";
	case SerializationRecordType::PartyUpdates:
		return "PRTY";
	case SerializationRecordType::Victims:
		return "VCTM";
	case SerializationRecordType::Adventures:
		return "ADVN";
	default:
		return "????";
	}
}

}
