﻿/*************************************************************************
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
#include "PluginFacade.h"

#include "Utilities/utils.h"
#include "Utilities/version.h"
#include "VM/papyrus.h"
#include "Data/dataCase.h"
#include "Data/LoadOrder.h"
#include "Collections/CollectionManager.h"
#include "WorldState/VisitedPlaces.h"
#include "WorldState/PartyMembers.h"
#include "WorldState/ActorTracker.h"

#include <shlobj.h>
#include <sstream>
#include <KnownFolders.h>
#include <filesystem>

if 0
constexpr const char* LORDFILE("LORD.compressed.json");
constexpr const char* COLLFILE("COLL.compressed.json");
constexpr const char* PLACFILE("PLAC.compressed.json");
constexpr const char* PRTYFILE("PRTY.compressed.json");
constexpr const char* VCTMFILE("VCTM.compressed.json");
#endif

void SaveCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Save hook called");
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Serialization Save hook");
#endif
	// Serialize JSON and compress per https://github.com/google/brotli
#if 0
	// output LoadOrder
	{
		nlohmann::json j(shse::LoadOrder::Instance());
		DBG_MESSAGE("Wrote %s :\n%s", LORDFILE, j.dump().c_str());
		std::string compressed(CompressionUtils::EncodeBrotli(j));
		std::ofstream saveData(LORDFILE, std::ios::out | std::ios::binary);
		saveData.write(compressed.c_str(), compressed.length());
		saveData.close();
	}
	// output Collection Groups - Definitions and Members
	{
		nlohmann::json j(shse::CollectionManager::Instance());
		DBG_MESSAGE("Wrote %s :\n%s", COLLFILE, j.dump().c_str());
		std::string compressed(CompressionUtils::EncodeBrotli(j));
		std::ofstream saveData(COLLFILE, std::ios::out | std::ios::binary);
		saveData.write(compressed.c_str(), compressed.length());
		saveData.close();
	}
	// output Location history
	{
		nlohmann::json j(shse::VisitedPlaces::Instance());
		DBG_MESSAGE("Wrote %s :\n%s", PLACFILE, j.dump().c_str());
		std::string compressed(CompressionUtils::EncodeBrotli(j));
		std::ofstream saveData(PLACFILE, std::ios::out | std::ios::binary);
		saveData.write(compressed.c_str(), compressed.length());
		saveData.close();
	}
	// output Followers-in-Party history
	{
		nlohmann::json j(shse::PartyMembers::Instance());
		DBG_MESSAGE("Wrote %s :\n%s", PRTYFILE, j.dump().c_str());
		std::string compressed(CompressionUtils::EncodeBrotli(j));
		std::ofstream saveData(PRTYFILE, std::ios::out | std::ios::binary);
		saveData.write(compressed.c_str(), compressed.length());
		saveData.close();
	}
	// output Party Kills history
	{
		nlohmann::json j(shse::ActorTracker::Instance());
		DBG_MESSAGE("Wrote %s :\n%s", VCTMFILE, j.dump().c_str());
		std::string compressed(CompressionUtils::EncodeBrotli(j));
		std::ofstream saveData(VCTMFILE, std::ios::out | std::ios::binary);
		saveData.write(compressed.c_str(), compressed.length());
		saveData.close();
	}
#else
	// output LoadOrder
	std::string lordRecord(CompressionUtils::EncodeBrotli(shse::LoadOrder::Instance()));
	if (!a_intfc->WriteRecord('LORD', 1, lordRecord.c_str(), static_cast<UInt32>(lordRecord.length())))
	{
		REL_ERROR("Failed to serialize LORD");
	}
	// output Collection Groups - Definitions and Members
	std::string collRecord(CompressionUtils::EncodeBrotli(shse::CollectionManager::Instance()));
	if (!a_intfc->WriteRecord('COLL', 1, collRecord.c_str(), static_cast<UInt32>(collRecord.length())))
	{
		REL_ERROR("Failed to serialize COLL");
	}
	// output Location history
	std::string placRecord(CompressionUtils::EncodeBrotli(shse::VisitedPlaces::Instance()));
	if (!a_intfc->WriteRecord('PLAC', 1, placRecord.c_str(), static_cast<UInt32>(placRecord.length())))
	{
		REL_ERROR("Failed to serialize PLAC");
	}
	// output Followers-in-Party history
	std::string prtyRecord(CompressionUtils::EncodeBrotli(shse::PartyMembers::Instance()));
	if (!a_intfc->WriteRecord('PRTY', 1, prtyRecord.c_str(), static_cast<UInt32>(prtyRecord.length())))
	{
		REL_ERROR("Failed to serialize PRTY");
	}
	std::string vctmRecord(CompressionUtils::EncodeBrotli(shse::ActorTracker::Instance()));
	if (!a_intfc->WriteRecord('VCTM', 1, vctmRecord.c_str(), static_cast<UInt32>(vctmRecord.length())))
	{
		REL_ERROR("Failed to serialize VCTM");
	}
#endif
}

void LoadCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Load hook called");
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Serialization Load hook");
#endif
#if 0
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(LORDFILE));
		std::ifstream readData(LORDFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read %s:\n%s", LORDFILE, jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("Load error on %s: %s", LORDFILE, exc.what());
	}
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(COLLFILE));
		std::ifstream readData(COLLFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read %s:\n%s", COLLFILE, jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("Load error on %s: %s", COLLFILE, exc.what());
	}
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(PLACFILE));
		std::ifstream readData(PLACFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read %s:\n%s", PLACFILE, jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("Load error on %s: %s", PLACFILE, exc.what());
	}
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(PRTYFILE));
		std::ifstream readData(PRTYFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read %s:\n%s", PRTYFILE, jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("Load error on %s: %s", PRTYFILE, exc.what());
	}
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(VCTMFILE));
		std::ifstream readData(VCTMFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read %s:\n%s", VCTMFILE, jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("Load error on %s: %s", VCTMFILE, exc.what());
	}
#else
	UInt32 readType;
	UInt32 version;
	UInt32 length;
	std::unordered_map<shse::SerializationRecordType, nlohmann::json> records;
	std::string saveData;
	while (a_intfc->GetNextRecordInfo(readType, version, length)) {
		saveData.resize(length);
		if (!a_intfc->ReadRecordData(const_cast<char*>(saveData.c_str()), length))
		{
			REL_ERROR("Failed to load record %d", readType);
		}
		shse::SerializationRecordType recordType(shse::SerializationRecordType::MAX);
		switch (readType) {
		case 'LORD':
			// Load Order
			recordType = shse::SerializationRecordType::LoadOrder;
			break;
		case 'COLL':
			// Collection Groups - Definitions and Members
			recordType = shse::SerializationRecordType::Collections;
			break;
		case 'PLAC':
			// Visited Places
			recordType = shse::SerializationRecordType::PlacesVisited;
			break;
		case 'PRTY':
			// Party Membership
			recordType = shse::SerializationRecordType::PartyUpdates;
			break;
		case 'VCTM':
			// Party Victims
			recordType = shse::SerializationRecordType::Victims;
			break;
		default:
			REL_ERROR("Unrecognized signature type %d", readType);
			break;
		}
		if (recordType != shse::SerializationRecordType::MAX)
		{
			records.insert({ recordType, CompressionUtils::DecodeBrotli(saveData) });
		}
	}
#endif
}
void SKSEMessageHandler(SKSE::MessagingInterface::Message* msg)
{
	static bool scanOK(true);
	switch (msg->type)
	{
	case SKSE::MessagingInterface::kDataLoaded:
		DBG_MESSAGE("Loading Papyrus");
		SKSE::GetPapyrusInterface()->Register(papyrus::RegisterFuncs);
		REL_MESSAGE("Registered Papyrus functions!");
		break;

	case SKSE::MessagingInterface::kPreLoadGame:
		REL_MESSAGE("Game load starting");
		shse::PluginFacade::Instance().PrepareForReload();
		break;

	case SKSE::MessagingInterface::kNewGame:
	case SKSE::MessagingInterface::kPostLoadGame:
		REL_MESSAGE("Game load done, initializing Tasks");
		// if checks fail, abort scanning
		if (!shse::PluginFacade::Instance().Init())
		{
			REL_FATALERROR("SearchTask initialization failed - no looting");
			return;
		}
		REL_MESSAGE("Initialized SearchTask, looting available");
		shse::PluginFacade::Instance().AfterReload();
		break;
	}
}

extern "C"
{

bool SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
	std::wostringstream path;
	path << L"/My Games/Skyrim Special Edition/SKSE/" << std::wstring(L_SHSE_NAME) << L".log";
	std::wstring wLogPath(path.str());
	SKSE::Logger::OpenRelative(FOLDERID_Documents, wLogPath);
	SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kDebugMessage);
	SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kDebugMessage);
	SKSE::Logger::UseLogStamp(true);
	SKSE::Logger::UseTimeStamp(true);
	SKSE::Logger::UseThreadID(true);
#if _DEBUG
	SKSE::Logger::HookPapyrusLog(true);
#endif
	REL_MESSAGE("%s v%s", SHSE_NAME, VersionInfo::Instance().GetPluginVersionString().c_str());

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = SHSE_NAME;
	a_info->version = VersionInfo::Instance().GetVersionMajor();

	if (a_skse->IsEditor()) {
		REL_FATALERROR("Loaded in editor, marking as incompatible!\n");
		return false;
	}
	SKSE::Version runtimeVer(a_skse->RuntimeVersion());
	if (runtimeVer < SKSE::RUNTIME_1_5_73)
	{
		REL_FATALERROR("Unsupported runtime version %08x!\n", runtimeVer);
		return false;
	}

	return true;
}

bool SKSEPlugin_Load(const SKSE::LoadInterface * skse)
{
	REL_MESSAGE("%s plugin loaded", SHSE_NAME);
	if (!SKSE::Init(skse)) {
		return false;
	}
	SKSE::GetMessagingInterface()->RegisterListener("SKSE", SKSEMessageHandler);

	auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID('SHSE');
	serialization->SetSaveCallback(SaveCallback);
	serialization->SetLoadCallback(LoadCallback);

	return true;
}

};
