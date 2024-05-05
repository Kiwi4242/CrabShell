/* Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  History.h
  classes for managing command history
*/

  
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>


class HistoryItem {
public:
    std::string cmd;
    std::string date;
    std::string folder;

    HistoryItem(){}
    HistoryItem(const std::string &c, const std::string &d, const std::string &f);

};


typedef std::shared_ptr<HistoryItem> HistoryItemPtr;

class ShellHistoryClass {
protected:
    std::vector<HistoryItemPtr> history;
    std::unordered_map<std::string, std::vector<HistoryItemPtr>> folderMap;
    std::vector<HistoryItemPtr> noFolder;
    // std::map<std::string, HistoryItemPtr> historyMap;
    std::string fileName;

public:
    ShellHistoryClass();

    bool Load(const std::string &inFile);

    bool GetMatch(const std::string &pref);

    unsigned int GetNoHistory() const {
        return history.size();
    }

    const HistoryItemPtr &Get(const unsigned int n) const;
    std::vector<HistoryItemPtr> GetFolderItems(const std::string &folder);
    const std::vector<HistoryItemPtr> &GetNoFolderItems();

    void Clear();
    void Append(const std::string &cmd, const std::string &folder, const std::string &t, const bool appendToFile);
};

