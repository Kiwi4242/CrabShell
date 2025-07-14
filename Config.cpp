/* ----------------------------------------------------------------------------
  Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  Config.cpp
  Custom configuration file, superceded by Lua
-----------------------------------------------------------------------------*/

#include <fstream>
#include <iostream>

#include "Config.h"
#include "Utilities.h"



typedef std::string (*LoadSecFun)(std::ifstream &fs, const std::string &line, std::vector<std::vector<std::string>> &arr);


std::string LoadArrayArray(std::ifstream &fs, const std::string &inLine, std::vector<std::vector<std::string>> &arr)
{
    std::string line;
    while (getline(fs, line)) {
        // starts with '- ['
        size_t arrEnd = line.find("]");
        if (line.find("- [") == line.npos or arrEnd == line.npos) {
            return line;
        }
        std::string valSt = line.substr(3, arrEnd-3);
        std::vector<std::string> vals;
        Utilities::SplitString(valSt, ", ", vals);
        if (vals.size() == 2) {
            arr.push_back(vals);
        }
    }
    return "";
}


std::string LoadKeyVal(std::ifstream &fs, const std::string &line, std::vector<std::vector<std::string>> &arr)
{
    // process key: val
    size_t pos = line.find(":");
    if (pos != line.npos) {
        arr.push_back({line.substr(0, pos), line.substr(pos+1)});
    }
    return "";
}


bool LoadConfig(const std::string &inFile, std::map<std::string, std::vector<std::vector<std::string>>> &outVals)
{
    // load the history from a yaml file. The format is simple so just use a custom parser
    std::ifstream inp(inFile);

    typedef std::map<std::string, LoadSecFun> KeyMap;
    KeyMap keys {{"Aliases",  LoadArrayArray}, {"Start Dir", LoadKeyVal}};
    std::string line;
    bool ok = !std::getline(inp, line).fail();
    while (ok) {
        // std::cout << "Reading line " << line << "\n";
        bool updated = false;
        if (!Utilities::StartsWith(line, "#") and !Utilities::StartsWith(line, "//") and line.length() > 0) {
            // process
            for (KeyMap::const_iterator k = keys.begin(); k != keys.end(); k++) {
                if (Utilities::StartsWith(line, k->first)) {
                    std::vector<std::vector<std::string>> keyVals;
                    line = k->second(inp, line, keyVals);
                    outVals[k->first] = keyVals;
                    updated = true;
                    break;
                }
            }
        }
        if (!updated or line.length() == 0) {
            ok = !std::getline(inp, line).fail();
        }
    }

    return true;
}

#ifdef MAIN

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        std::cout << "Usage: History history_file\n";
        return 0;
    }

    
    typedef std::map<std::string, std::vector<std::vector<std::string>>> ConfigMap;
    ConfigMap outVals;
    LoadConfig(argv[1], outVals);

    size_t no = outVals.size();
    std::cout << "Have read config with " << no << " items\n";

    for (ConfigMap::const_iterator it = outVals.begin(); it != outVals.end(); it++) {
        std::cout << it->first << ":\n";
        const std::vector<std::vector<std::string>> &vec = it->second;
        for (size_t i = 0; i < vec.size(); i++) {
            for (size_t j = 0; j < vec[i].size(); j++) {
                std::cout << vec[i][j] << " ";
            }
            std::cout << "\n";
        }
    }

    return 0;
}

#endif

