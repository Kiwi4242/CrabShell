/* ----------------------------------------------------------------------------
  Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  Crabshell.cpp
  Main classes and function
-----------------------------------------------------------------------------*/

#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <locale.h>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;
#include <iostream>


#ifdef OLD_HISTORY
#include <nlohmann/json.hpp>
// for convenience
using json = nlohmann::json;
#endif


#include <isocline_pp.h>

#include "History.h"
#include "Utilities.h"
#include "Config.h"


class ShellDataClass {
protected:

#ifdef OLD_DRIVE
  static const int MAX_DRIVES = 26;
  char currentDrive;  // store the current drive - maybe as a:
  std::string currentDir[MAX_DRIVES];
#else
  std::string currentDir;    
  std::string root;
#endif  

  std::vector<std::string> pushDirs;

  std::string currentPrompt;
  int maxPrompt;

  fs::path configFolder;
  std::ofstream *log;

  std::map<std::string, std::string> aliases;
  std::string startDir;

  typedef bool (*HookFunc)(const std::vector<std::string> &args, ShellDataClass &shell);
  std::map<std::string, HookFunc> hooks;

public:
  ShellDataClass(bool useLog=false) ;
  
  ~ShellDataClass() {
    if (log)
      log->close();
  }


  bool DoCD(const std::string &dir, const bool push=false);

  bool PopDir();
  
  int SetDrive(const char d);
  
  // Get the drive number corresponding to a letter
  static int GetDriveNo(const char d);

#ifdef OLD_DRIVE
  // Get the current directory
  const std::string &GetDriveDir() {return currentDir[GetDriveNo(currentDrive)];}

  // Set the current directory
  void SetDriveDir(const std::string &dir) {
    currentDir[GetDriveNo(currentDrive)] = dir;
  }
  
  // Get the current dir associated with the drive driveLet (a, b, c etc)
  const std::string &GetDriveDir(const char driveLet) {
    return currentDir[GetDriveNo(driveLet)];
  }
#endif

  const std::string GetPrompt();

  // Update path information
  int GetPaths();
  
  //  bool MSWSystem1(const std::string &cmd);
  bool MSWSystem(const std::string &cmd);
  int ProcessCommand(const std::string &commandLineArg);

  int RunHooks(std::vector<std::string> &args);

  void LogMessage(const std::string &msg) {
    if (log) {
      *log << msg << "\n";
      log->flush();
    }
  }

  const fs::path &GetConfigFolder() const {
    return configFolder;
  }
};


class ReadLineClass : public IsoclinePP {
protected:
  std::shared_ptr<ShellDataClass> shell;
#ifdef OLD_HISTORY  
  StVec history;
#else
  ShellHistoryClass history;
#endif

public:
    ReadLineClass(std::shared_ptr<ShellDataClass> sh);

    // Complete after tab called
    bool Completer(const std::string &inp, std::vector<CompletionItem> &completions);

    // Provide a hint after a new character is entered
    bool Hint(const std::string &inp, CompletionItem &hint);

    using IsoclinePP::AddHistory;
    virtual void AddHistory(const std::string &statement, const bool write);

    void ReadHistory(const std::string &name);
};



ReadLineClass::ReadLineClass(std::shared_ptr<ShellDataClass> sh) : IsoclinePP(), shell(sh) 
{
    EnableHint(true);
    EnableMultiLine(false);
}

// -------------------------------------------------------------------------------
// Completion
// -------------------------------------------------------------------------------

bool ReadLineClass::Hint(const std::string &inp, CompletionItem &hint)
{
#ifdef OLD_HISTORY    
    for (StVec::const_reverse_iterator it=history.rbegin(); it!=history.rend(); it++) {
      if (Utilities::StartsWith(*it, inp, false)) {
        pref = inp;
        hint = *it;
        break;
      }
    }
#else    
    int no = history.GetNoHistory();
    hint.comp = "";
    hint.delBefore = 0;
    for (int i = no-1; i >= 0; i--) {
      const HistoryItem &it = history.Get(i);
      if (Utilities::StartsWith(it.cmd, inp, false)) {
        hint.delBefore = inp.length();
        hint.comp = it.cmd;
        break;
      }
    }
#endif    
    return true;
}

bool ReadLineClass::Completer(const std::string &inp, std::vector<CompletionItem> &completions)
{
  try {
    Utilities::GetFileMatches(inp, completions);

    shell->LogMessage("Completions for " + inp);
    for (size_t i = 0; i < completions.size(); i++) {
      shell->LogMessage("  " + completions[i].comp);
    }
  } catch (std::exception &e) {
  }

  return completions.size() > 0;
}

void ReadLineClass::AddHistory(const std::string &statement, const bool write)
{
  IsoclinePP::AddHistory(statement);
#ifdef OLD_HISTORY  
  history.push_back(statement);
#else  
  history.Append(statement, write);
#endif  
}

void ReadLineClass::ReadHistory(const std::string &name)
{
    fs::path inPath = shell->GetConfigFolder() / name;

#ifdef OLD_HISTORY
    history.clear();
    std::ifstream inStr(inPath.string());
    if (!inStr.good()) {
      Printf("Unable to open ", name);
      return;
    }
    json hisJson;
    inStr >> hisJson;

    if (hisJson.find("History") == hisJson.end()) {
      return;
    }

   auto his = hisJson["History"];
   for (json::iterator it = his.begin(); it != his.end(); ++it) {
      history.push_back((*it)["Cmd"]);
   }
    
  inStr.close();
  AddHistory(history);
#else 
   history.Clear();
   history.Load(inPath.string());

   std::vector<std::string> hisItems;
   for (unsigned int i = 0; i < history.GetNoHistory(); i++) {
      const HistoryItem &it = history.Get(i);
      hisItems.push_back(it.cmd);
    }
    AddHistory(hisItems);
#endif  
}


#if 0
int ShellDataClass::GetDriveNo(const std::string &d)
{
  int result;
  if (d[0] < 'a')
    result = d[0] - 'A';
  else
    result = d[0] - 'a';
  if (result < 0 || result >= MAX_DRIVES)
    result = 0;
  return result;
}
#endif

#ifdef OLD_DRIVE
int ShellDataClass::GetDriveNo(const char d)
{
  if (d < 'a')
    return d - 'A';
  else
    return d - 'a';
}

#endif

int ShellDataClass::GetPaths()
{
  // set currentDir and root

#ifdef OLD_DRIVE  

  std::string currentPath, tmpPath;
#ifdef __MINGW32__  
  char drive[16], dir[PATH_MAX], file[PATH_MAX], ext[PATH_MAX];
#else
  char *drive, *dir, *file;
#endif
  int len;

  //  getcwd(currentPath, PATH_MAX + 1);
  currentPath = Utilities::GetCurrentDirectory();
#ifdef __MINGW32__  
  _splitpath(currentPath.c_str(), drive, dir, file, ext);
#else
  _splitpath2(currentPath.c_str(), tmpPath, &drive, &dir, &file, NULL);
#endif

  currentDrive = drive[0];

  std::string cd;
#ifdef __MINGW32__
  int dirLen = strlen(dir);
  if (dirLen && dir[dirLen-1] == Utilities::pathSep)
    dir[dirLen-1] = 0;
  cd = std::string(dir) + std::string(1, Utilities::pathSep) + std::string(file) + std::string(ext);
  //  sprintf(cd, "%s%s%s", dir, file, ext);
#else
  cd = std::string(dir) + std::string(Utilities::pathSep) + file;
  sprintf(cd, "%s\\%s", dir, file);
#endif

  len = cd.length();
  if (cd[len-1] == Utilities::pathSep)
    cd.erase(len-1);
  SetDriveDir(cd);

#else
  currentDir = Utilities::GetCurrentDirectory();
  fs::path curPath(currentDir);
  root = curPath.root_name().string();
#endif

  
  return 1;
}

const std::string ShellDataClass::GetPrompt()
{
  std::string pathSep = std::string(1, Utilities::pathSep);

  std::string curPath = currentDir;
  std::string pre;                   // a string to prepend - usually d: for windows

  // fs::path fsPath(currentDir);

  std::string curDir = currentDir;
#ifdef __WIN32__
  // remove the root (usually d:)
  if (root.length() == 2) {
    curDir = currentDir.substr(2);
    pre = root + pathSep;
  }
#endif

  currentPrompt = root + pathSep + curDir;

  // If current path is too long then cut out some of the path
  int len = curDir.length();
  if (len <= maxPrompt) {
    return currentPrompt;
  }

  std::vector<std::string> folders;
  Utilities::SplitString(curDir, pathSep, folders);

  int noFolders = folders.size();
  int preLen = (noFolders - 1) * 2;  // should add drive
  int maxLen = maxPrompt - preLen;
  if (maxLen <= 3) {
    maxLen = 5;
  }

  currentPrompt = folders.back();
  bool useOne = false;
  for (int i = folders.size()-2; i >= 0; i--) {
    std::string cur = folders[i];
    if (currentPrompt.length() + cur.length() > maxLen) {
      cur = cur.substr(0, 1);
    }
    currentPrompt = cur + pathSep + currentPrompt;
  }

  return pre + currentPrompt;

}


bool ShellDataClass::DoCD(const std::string &d, const bool push)
{
  std::string dir = d;

  // strip any quotes from the directory as SetCurrentDirectory doesn't
  // like them
  if (push) {
    pushDirs.push_back(Utilities::GetCurrentDirectory());
  }
  Utilities::FixupPath(dir);
  Utilities::SetCurrentDirectory(dir);

  GetPaths();

  return true;
}

bool ShellDataClass::PopDir()
{
  if (pushDirs.size() > 0) {
    const std::string &dir = pushDirs.back();
    Utilities::SetCurrentDirectory(dir);
    pushDirs.pop_back();
    GetPaths();
  }
  return true;
}

#ifdef OLD_DRIVE
int ShellDataClass::SetDrive(const char d)
{
#ifdef NOTUSED  
  unsigned int total;
  int drive;
  drive = GetDriveNo(d) + 1;
#ifdef __MINGW32__  
  _chdrive(drive);
#else
  _dos_setdrive(drive, &total);
#endif

  if (currentDir[drive-1].length())
    DoCD(currentDir[drive-1]);
  else
    DoCD("\\");
#endif  

  // can set the drive with "cd d:"
  std::string drive = std::string(1, d) + ":";
  Utilities::SetCurrentDirectory(drive);
  GetPaths();
  return true;
}
#endif


int ShellDataClass::RunHooks(std::vector<std::string> &args)
{
  if (args.size() == 0) {
    return 0;
  }


#ifdef TODO
  // run any function in the cln file
  int entry = interp.FindFunction(args[0].c_str());
  if (entry != -1) {
    int noArgs = args.size() - 1;
    InterpVar *vars = new InterpVar[noArgs];
    for(int i = 0; i < noArgs; i++)
      vars[i] = InterpVar(args[i+1]);
    int result =  interp.ExecuteFunction(args[0].c_str(), noArgs, vars);
    delete [] vars;
    return result;
  }
#endif

  std::map<std::string, HookFunc>::iterator iter = hooks.find(args[0]);
  if (iter != hooks.end()) {
    return (iter->second)(args, *this);
  }
  return 0;

#if 0
  if (args[0] == "exit")
    exit(1);
  if (args[0] == "cd" && args.size() > 1)
    return DoCD(args[1].c_str());
  return 0;
#endif
}


int ShellDataClass::ProcessCommand(const std::string &commandLineArg)
{
  if (commandLineArg.empty()) 
    return 1;
  
  std::vector<std::string> args;
  std::string lastTok;
  
  bool lastBlank = Utilities::ParseLine(commandLineArg, args);
  std::string cmd = args[0];

  Utilities::StripString(cmd);

  unsigned cmdLen = cmd.length();
  
  // command of the form c:
  if (cmdLen == 2 && cmd[1] == ':') {
    // std::string drive = GetDriveDir(cmd[0]);
#ifdef OLD_DRIVE    
    SetDrive(cmd[0]);
#else
    Utilities::SetCurrentDirectory(cmd);
#endif 
    return 1;
  }

#ifdef OLD_DRIVE
  bool resetCWD = 0;
  std::string currentPath;
  // command of the form c:fred
  if (cmdLen > 1 && cmd[1] == ':') {
    currentPath = Utilities::GetCurrentDirectory();    
    resetCWD = 1;
    std::string drive = GetDriveDir(cmd[0]);
    Utilities::SetCurrentDirectory(drive);
  }
#endif

  // update aliases
  if (aliases.count(cmd) > 0) {
    args[0] = aliases[cmd];
  }

  int res = RunHooks(args);
  if (res) {
    return res;
  }

  // Reconstruct the command line
  std::string cmdLine;
  for (int i = 0; i < args.size()-1; i++) {
    cmdLine += args[i] + " ";
  }
  cmdLine += args.back();

  std::system(cmdLine.c_str());

#ifdef OLD_DRIVE
  if (resetCWD) {
    Utilities::SetCurrentDirectory(currentPath);
  }
#endif

  return 1;
}


namespace ShellFuncs {

  bool ExitFunc(const std::vector<std::string> &args, ShellDataClass &shell) {
    exit(1);
    return 1;
  }
  
  bool CD(const std::vector<std::string> &args, ShellDataClass &shell) {
    if (args.size() > 1) {
      return shell.DoCD(args[1]);
    }
    return 0;
  }

  bool PushDir(const std::vector<std::string> &args, ShellDataClass &shell) {
    if (args.size() > 1) {
      return shell.DoCD(args[1], true);
    }
    return 0;
  }

  bool PopDir(const std::vector<std::string> &args, ShellDataClass &shell) {
    return shell.PopDir();
  }


  bool SetEnv(const std::vector<std::string> &args, ShellDataClass &shell) {
    if (args.size() > 1) {
      const std::string &cmd = args[1];
      int pos = cmd.find('=');
      if (pos != cmd.npos) {
        std::string var = cmd.substr(0, pos);
        std::string val = cmd.substr(pos+1);
        _putenv_s(var.c_str(), val.c_str());
        // SetEnvironmentVariable(var.c_str(), val.c_str());
        return 1;
      }
    }
    return 0;
  }
  
  bool SetColour(const std::vector<std::string> &args, ShellDataClass &shell) {
    return 1;
  }

}


ShellDataClass::ShellDataClass(bool useLog) 
{

  std::string home = Utilities::GetEnvVar("HOME");
  configFolder = fs::path(home) / ".crabshell";
  if (useLog) {
    log = new std::ofstream(configFolder / "CrabShell.log");
  } else {
    log = nullptr;
  }

  hooks["exit"] = &ShellFuncs::ExitFunc;
  hooks["cd"] = &ShellFuncs::CD;
  hooks["pushd"] = &ShellFuncs::PushDir;
  hooks["popd"] = &ShellFuncs::PopDir;
  hooks["setcolour"] = &ShellFuncs::SetColour;
  hooks["set"] = &ShellFuncs::SetEnv;
  maxPrompt = 25;

  std::string configFile = (configFolder / "Config.dat").string();

  ConfigMap configData;
  if (LoadConfig(configFile, configData)) {
    if (configData.count("Aliases") > 0) {
      std::vector<std::vector<std::string>> vals = configData["Aliases"];
      for (int i = 0; i < vals.size(); i++) {
        if (vals[i].size() == 2) {
          aliases[vals[i][0]] = vals[i][1];
        }
      }
    }
    if (configData.count("Start Dir")) {
      std::vector<std::vector<std::string>> vals = configData["Start Dir"];
      if (vals.size() == 1 and vals[0].size() == 2) {
        startDir = vals[0][1];
        DoCD(startDir);
      }
    }
  }
  GetPaths();
}


// main program
int main(int argc, char* argv[]) 
{

  // check args
  bool doLog = false;

  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-l") {
      doLog = true;
    }
  }

  try {
    std::shared_ptr<ShellDataClass> shell = std::make_shared<ShellDataClass>(doLog);
    ReadLineClass readLine(shell);

    setlocale(LC_ALL,"C.UTF-8");  // we use utf-8 in this example

    // use `ic_print` functions to use bbcode's for markup
    readLine.StyleDef("kbd","gray underline");     // you can define your own styles
    // ic_style_def("ic-prompt","ansi-maroon");  // or re-define system styles
    
    readLine.Printf( "[b]Welcome to CrabShell[/b]\n\n", "");
    
    // enable history; use a NULL filename to not persist history to disk
  #ifdef OLD_HISTORY
    readLine.ReadHistory("history.json");
  #else
    readLine.ReadHistory("history.dat");
  #endif  

    // enable syntax highlighting with a highlight function
    // ic_set_default_highlighter(highlighter, NULL);

    // try to auto complete after a completion as long as the completion is unique
    readLine.EnableAutoTab(false);

    // inline hinting is enabled by default
    readLine.EnableHint(true);

    // run until empty input
    char* input;

    while(true) {
      std::string input = readLine.ReadLine(shell->GetPrompt());   // ctrl-d returns NULL (as well as errors)
      try {
        // readLine.Printf("%s\n", input.c_str());
        shell->ProcessCommand(input);
        if (input == "exit") {
            break;
        } 

        if (input.length() > 0)  {
          readLine.AddHistory(input, true);
        }
      } catch (std::exception &e) {
        // catch and continue
        readLine.Printf("Error: with command\n","");
      }

    }

    readLine.Printf("Goodbye\n", "");

  } catch (std::exception &e) {
    std::cerr << "Error starting CrabShell " << e.what() << "\n";
    return 1;
  }


  return 0;
}
