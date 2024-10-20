/* Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  History.cpp
  Class to manage shell history
*/


#include <fstream>
#include <iostream>
#include <stdexcept>
#include <map>
#include <algorithm>
#include <chrono>
using namespace std::chrono_literals;

#include <filesystem>
namespace fs = std::filesystem;

#include <ctime>

#include "History.h"
#include "Utilities.h"


HistoryItem::HistoryItem(const std::string &c, const std::string &d, const std::string &f)
{
    item = c;
    date = d;
    folder = f;    
}


std::ostream& operator<<(std::ostream &out, const HistoryItem& item) 
{
    out << item.item << ", " << item .date << ", " << item.folder;
    return out;
}


ShellHistoryClass::ShellHistoryClass()
{
}


const HistoryItemPtr &ShellHistoryClass::Get(const unsigned int n) const
{
    if (n < 0 or n > history.size()) {
        throw std::out_of_range("Range error in History::Get()");
    }
    return history[n];
}


bool ShellHistoryClass::Load(const std::string &inFile)
{
    fileName = inFile;

    if (not Utilities::FileExists(inFile)) {
        std::ofstream ofs(inFile);
        if (ofs) {
            ofs << "History:\n";
        }
        ofs.close();
    }

    // load the history from a yaml file. The format is simple so just use a custom parser
    std::ifstream inp(inFile);
    // search start - "History:"
    bool found = false;
    for (std::string line; std::getline(inp, line);) {
        Utilities::StripStringEnd(line);
        if (line == "History:") {
            found = true;
            break;
        }
    }

    if (!found) {
        return false;
    }

    std::string keys[] = {"- Cmd: ", "Date: ", "Folder: "};
    std::string entries[3];
    std::string line;
    while (std::getline(inp, line)) {
        // 3 records - Cmd, Date, Folder
        Utilities::StripStringEnd(line);
        if (line.find(keys[0]) != line.npos) {
            // have found record
            for (int i = 0; i < 3; i++) {
                int pos = line.find(keys[i]);
                if (pos != line.npos) {
                     std::string st = line.substr(pos+keys[i].size());
                     // remove quotes in yaml file
                     if (st.find("'") == 0) {
                        st.erase(0, 1);
                        // find last
                        int pos1 = st.find("'");
                        if (pos1 != st.npos) {
                            st.erase(pos1, 1);
                        }
                     }
                     entries[i] = st;
                }
                if (i < 2 and !std::getline(inp, line)) {
                    return history.size() > 0;
                }
            }
            HistoryItemPtr item = std::make_shared<HistoryItem>(entries[0], entries[1], entries[2]);
            history.push_back(item);
            // historyMap[entries[0]] = item;
            if (entries[2].size() > 0) {
                folderMap[entries[2]].push_back(item);
            } else {
                noFolderMap.push_back(item);
            }
        }
    }

    std::ostringstream msg;
    msg << "Read history with " << GetNoHistory() << " items\n";
    Utilities::LogMessage(msg.str());

    return true;
}

/*
bool ShellHistoryClass::GetMatch(const std::string &pref)
{
    return true;
}
*/

void ShellHistoryClass::Clear()
{
    history.clear();
}

typedef std::vector<HistoryItemPtr>::iterator HistIter;
int ShellHistoryClass::RevFind(const std::string &cmd, const int beg, const int end)
{
    // Search from fin to begin for cmd
    HistIter start = history.begin() + beg;
    HistIter fin = history.end() + end;
    for (int i = end; i >= beg; i--) {
        if (history[i]->item == cmd) {
            return i;
        }
    }
    return -1;
}


// constexpr auto t20{20ms};
void ShellHistoryClass::Append(const std::string &cmd, const std::string &folder, 
                               const std::string &tm, const bool appendToFile)
{
    HistoryItemPtr item = std::make_shared<HistoryItem>(cmd, tm, folder);

    bool add = true;
    // remove any old entries in the last 50 
    int noFind = std::min(50, int(history.size()));
    int startInd = history.size() - noFind;

    int fin = history.size()-1;
    auto it = RevFind(cmd, startInd, fin);
    if (it >= 0) {
        if (it == fin) {
            add = false;
        } else {
            std::ostringstream msg;
            msg << "Erasing history item " << history[it]->item;
            Utilities::LogMessage(msg.str());
            history.erase(history.begin()+it);
        }
    }
    if (add) {
        history.push_back(item);
    }

    // Assign to a folder
    if (folder.size() > 0) {
        if (folderMap.count(folder) > 0) {
            // check if already present, just the last 20 entries
            std::vector<HistoryItemPtr> &foldVec = folderMap[folder];
            int no = foldVec.size();
            int start = std::max(0, no-20);
            bool add = true;
            for(int i = no-1; i >= start; i--) {
                if (foldVec[i]->item == cmd) {
                    if (i == no-1) {
                        add = false;
                    } else {
                        foldVec.erase(foldVec.begin()+i);
                    }
                    break;
                }
            }
        }
        if (add) {
            folderMap[folder].push_back(item);
        }
    } else {
        // this shouldn't be called here
        noFolderMap.push_back(item);
    }

    if (appendToFile and add) {
        if (fileName.length() > 0) {
            // write a lock file to prevent other instances from writing

            Utilities::FileLock lck(fileName);
            if (lck.HasLock()) {

                std::ofstream ofs(fileName, std::ios::app);
                if (ofs) {
                    ofs << "- Cmd: " << cmd << "\n";
                    ofs << "  Date: '" << tm << "'\n";
                    ofs << "  Folder: " << folder << "\n";
                }
                ofs.close();
            }
        }
    }
}


std::vector<HistoryItemPtr> ShellHistoryClass::GetFolderItems(const std::string &folder)
{
    std::string st;
    for (auto it : folderMap) {
        st = it.first; 
    }
    auto item = folderMap.find(folder);
    if (item != folderMap.end()) {
        return item->second;
    }
    return std::vector<HistoryItemPtr>();
}


const std::vector<HistoryItemPtr> &ShellHistoryClass::GetNoFolderItems() 
{
    return noFolderMap;
}


#ifdef MAIN

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        std::cout << "Usage: History history_file\n";
        return 0;
    }

    ShellHistoryClass history;
    history.Load(argv[1]);

    unsigned int no = history.GetNoHistory();
    std::cout << "Have read history with " << no << " items\n";

    for (unsigned int i = no-10; i < no; i++) {
        std::cout << "Entry " << i << " " << history.Get(i) << "\n";
    }

    return 0;
}

#endif