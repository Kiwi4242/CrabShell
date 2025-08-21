/* ----------------------------------------------------------------------------
  Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  History.h
  Class to manage shell history
-----------------------------------------------------------------------------*/

  
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include <crossline.h>


class CrabHistoryItem : public HistoryItem {
public:
    std::string date;
    std::string folder;

    CrabHistoryItem(){}
    CrabHistoryItem(const std::string &c, const std::string &d, const std::string &f);

};


typedef std::shared_ptr<CrabHistoryItem> CrabHistoryItemPtr;


class ShellHistoryClass : public HistoryClass {
protected:
    // std::vector<HistoryItemPtr> history;            // store all the history
    std::unordered_map<std::string, std::vector<HistoryItemPtr>> folderMap;    // map commands per folder
    std::vector<HistoryItemPtr> noFolderMap;                                   // any commands without a folder
    std::string fileName;

    int RevFind(const std::string &cmd, const int beg, const int end);
public:
    ShellHistoryClass();

    bool Load(const std::string &inFile);

    // bool GetMatch(const std::string &pref);

    unsigned int GetNoHistory() const {
        return Size();
    }

    std::vector<HistoryItemPtr> GetFolderItems(const std::string &folder);
    const std::vector<HistoryItemPtr> &GetNoFolderItems();

    void Append(const std::string &cmd, const std::string &folder, const std::string &t, const bool appendToFile);
};

