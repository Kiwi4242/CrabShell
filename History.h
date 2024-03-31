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


class HistoryItem {
public:
    std::string cmd;
    std::string date;
    std::string folder;

    HistoryItem(){}
    HistoryItem(const std::string &c, const std::string &d, const std::string &f);


};


class ShellHistoryClass {
protected:
    std::vector<HistoryItem> history;
    std::string fileName;

public:
    ShellHistoryClass();

    bool Load(const std::string &inFile);

    bool GetMatch(const std::string &pref);

    unsigned int GetNoHistory() const {
        return history.size();
    }

    const HistoryItem &Get(const unsigned int n) const;

    void Clear();
    void Append(const std::string &, const bool appendToFile);
};

