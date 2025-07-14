/* ----------------------------------------------------------------------------
  Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  Config.h
  Custom configuration file, superceded by Lua
-----------------------------------------------------------------------------*/

#pragma once

#include <map>
#include <string>
#include <vector>


typedef std::map<std::string, std::vector<std::vector<std::string>>> ConfigMap;

bool LoadConfig(const std::string &inFile, ConfigMap &outVals);
