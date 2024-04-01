#pragma once

#include <map>
#include <string>
#include <vector>


typedef std::map<std::string, std::vector<std::vector<std::string>>> ConfigMap;

bool LoadConfig(const std::string &inFile, ConfigMap &outVals);
