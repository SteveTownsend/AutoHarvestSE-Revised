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

#include "Utilities/Exception.h"
#include "Utilities/utils.h"

PluginError::PluginError(const char* pluginName) : std::runtime_error(std::string(PluginError::ErrorName) + pluginName)
{
}

KeywordError::KeywordError(const char* keyword) : std::runtime_error(std::string(KeywordError::ErrorName) + keyword)
{
}

FileNotFound::FileNotFound(const wchar_t* filename) : std::runtime_error(std::string(FileNotFound::ErrorName) + StringUtils::FromUnicode(filename))
{
}

FileNotFound::FileNotFound(const char* filename) : std::runtime_error(std::string(FileNotFound::ErrorName) + filename)
{
}
