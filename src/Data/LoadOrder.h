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
#include "Utilities/utils.h"

namespace shse
{

class LoadOrder {
public:
	static LoadOrder& Instance();
	LoadOrder();
	bool Analyze(void);
	RE::FormID GetFormIDMask(const std::string& modName) const;
	bool IncludesMod(const std::string& modName) const;
	bool ModPrecedesSHSE(const std::string& modName) const;
	bool ModOwnsForm(const std::string& modName, const RE::FormID formID) const;
	void AsJSON(nlohmann::json& j) const;
	void UpdateFrom(const nlohmann::json& j);
	RE::TESForm* RehydrateCosaveForm(const RE::FormID cosaveID) const;
	template <typename T>
	T* RehydrateCosaveFormAs(const RE::FormID cosaveID) const
	{
		RE::TESForm* form(RehydrateCosaveForm(cosaveID));
		return form ? form->As<T>() : nullptr;
	}

	inline RE::FormID AsMask(const RE::FormID formID) const
	{
		constexpr RE::FormID ESPFERawMask = 0x00000FFF;
		if ((formID & ESPFETypeMask) == ESPFETypeMask)
			return formID & ESPFEMask;
		return formID & ESPMask;
	}
	inline RE::FormID AsRaw(const RE::FormID formID) const
	{
		constexpr RE::FormID ESPFERawMask = 0x00000FFF;
		if ((formID & ESPFETypeMask) == ESPFETypeMask)
			return formID & ESPFERawMask;
		return formID & FullRawMask;
	}

	struct LoadInfo {
		inline bool operator==(const LoadInfo& rhs) const
		{
			return m_mask == rhs.m_mask && m_priority == rhs.m_priority;
		}
		RE::FormID m_mask;
		int m_priority;
	};

private:
	static constexpr RE::FormID LightFormIDSentinel = 0xfe000000;
	static constexpr RE::FormID LightFormIDMask = 0xfefff000;
	static constexpr RE::FormID RegularFormIDMask = 0xff000000;
	// no lock as all public functions are const once loaded
	static std::unique_ptr<LoadOrder> m_instance;
	mutable RecursiveLock m_loadLock;

	std::unordered_map<std::string, LoadInfo> m_loadInfoByName;
	std::unordered_map<std::string, LoadInfo> m_cosaveLoadInfoByName;
	std::unordered_map <RE::FormID, std::string> m_cosaveModNameByMask;
	int m_shsePriority;
	int m_cosaveShsePriority;
	bool m_coSaveLoadOrderDiffers;
};

inline bool operator<(const LoadOrder::LoadInfo& lhs, const LoadOrder::LoadInfo& rhs) { return lhs.m_priority < rhs.m_priority; }

void to_json(nlohmann::json& j, const LoadOrder& p);

}
