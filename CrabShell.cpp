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
#include <csignal>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;
#include <iostream>
#include <sstream>
#include <chrono>
using namespace std::chrono_literals;

#include <isocline_pp.h>

#include "History.h"
#include "Utilities.h"
#include "Config.h"

extern char ** environ;


class ShellDataClass {
protected:

  std::string currentDir;    
  std::string root;

  std::vector<std::string> pushDirs;

  std::string currentPrompt;
  int maxPrompt;

  fs::path configFolder;
  bool doLog;  

  std::map<std::string, std::string> aliases;
  std::string startDir;

  typedef bool (*HookFunc)(const std::vector<std::string> &args, ShellDataClass &shell);
  std::map<std::string, HookFunc> hooks;

public:
  ShellDataClass(bool useLog=false) ;
  
  ~ShellDataClass() {
    // if (log)
    //   log->close();
  }


  bool DoCD(const std::string &dir, const bool push=false);

  bool PopDir();
  
  int SetDrive(const char d);
  
  // Get the drive number corresponding to a letter
  static int GetDriveNo(const char d);

  const std::string &GetCurrentDir() const {return currentDir;}

  const std::string GetPrompt();

  // Update path information
  int GetPaths();
  
  //  bool MSWSystem1(const std::string &cmd);
  bool MSWSystem(const std::string &cmd);
  bool ProcessCommand(const std::string &commandLineArg);

  bool RunHooks(std::vector<std::string> &args);

};


class ReadLineClass : public IsoclinePP {
protected:
  std::shared_ptr<ShellDataClass> shell;
  ShellHistoryClass history;


  std::chrono::high_resolution_clock hintClock;
  int hintDelayMS;
  std::chrono::time_point<std::chrono::high_resolution_clock> lastHint;

public:
    ReadLineClass(std::shared_ptr<ShellDataClass> sh);

    // Complete after tab called
    bool Completer(const std::string &inp, std::vector<CompletionItem> &completions);

    // Provide a hint after a new character is entered
    bool Hint(const std::string &inp, CompletionItem &hint, const bool atEnd);

    using IsoclinePP::AddHistory;
    virtual void AddHistory(const std::string &statement, const std::string &folder, const bool write);

    void ReadHistory(const std::string &name);

    virtual int HistoryCount();
    virtual std::string GetHistoryItem(const ssize_t n);
    virtual void HistoryDelete(const ssize_t ind, const ssize_t n);
    virtual void HistoryAdd(const std::string &st);

};



ReadLineClass::ReadLineClass(std::shared_ptr<ShellDataClass> sh) : IsoclinePP(), shell(sh) 
{
    EnableHint(true);
    EnableMultiLine(false);
    UseCustomHistory();
    hintDelayMS = 300;
    lastHint = hintClock.now();
}

// -------------------------------------------------------------------------------
// Completion
// -------------------------------------------------------------------------------

bool ReadLineClass::Hint(const std::string &inp, CompletionItem &hint, const bool atEnd)
{
    hint.comp = "";
    hint.delBefore = 0;

    if (!atEnd) {
      return false;      
    }

    auto t1 = hintClock.now();
    int delta = (t1 - lastHint) / 1ms;
    if (delta < hintDelayMS) {
      return false;
    }
    lastHint = t1;
    
    // need to search backwards for latest
    std::string folder = shell->GetCurrentDir();
    auto items = history.GetFolderItems(folder);
    int no = items.size();
    for (int i = no-1; i >= 0; i--) {
      auto it = items[i];
      if (Utilities::StartsWith(it->cmd, inp, false)) {
        hint.delBefore = inp.length();
        hint.comp = it->cmd;
        return true;
      }
    }
    items = history.GetNoFolderItems();
    for (int i = items.size()-1; i >= 0; i--) {
      auto it = items[i];
      if (Utilities::StartsWith(it->cmd, inp, false)) {
        hint.delBefore = inp.length();
        hint.comp = it->cmd;
        return true;
      }
    }

    return false;
}


bool ReadLineClass::Completer(const std::string &inp, std::vector<CompletionItem> &completions)
{
  try {
    Utilities::GetFileMatches(inp, completions);

    Utilities::LogMessage("Completions for " + inp);
    for (size_t i = 0; i < completions.size(); i++) {
      Utilities::LogMessage("  " + completions[i].comp);
    }
  } catch (std::exception &e) {
  }

  return completions.size() > 0;
}


void ReadLineClass::AddHistory(const std::string &statement, const std::string &folder, const bool write)
{
  // IsoclinePP::AddHistory(statement);

  std::chrono::high_resolution_clock clock;
  auto t1 = clock.now();
  const auto t2 = std::chrono::system_clock::to_time_t(t1);
  char nowSt[128];
  std::strftime(nowSt, 128, "%Y-%m-%d %H:%M:%S", std::localtime(&t2));
  // std::string nowSt = std::put_time(std::localtime(&t2), "%F %T.\n");

  history.Append(statement, folder, nowSt, write);
}


void ReadLineClass::ReadHistory(const std::string &name)
{
    fs::path inPath = fs::path(Utilities::GetConfigFolder()) / name;

   history.Clear();
   history.Load(inPath.string());

#ifdef OLD
   std::vector<std::string> hisItems;
   for (unsigned int i = 0; i < history.GetNoHistory(); i++) {
      const HistoryItemPtr &it = history.Get(i);
      hisItems.push_back(it->cmd);
    }
    AddHistory(hisItems);
#endif    
}


int ReadLineClass::HistoryCount() 
{
    return history.GetNoHistory();
}


std::string ReadLineClass::GetHistoryItem(const ssize_t n)
{
  // return in reverse order
  int no = history.GetNoHistory();
  int ind = n;
  if (n < 0) {
    ind = no + n;
  }
  if (ind == 0) {
    return "";
  }
  if (ind <= no) {    // the call starts at 1
    std::string st = history.Get(no-ind)->cmd;
    std::ostringstream msg;
    msg << "Returning item " << n << " " << ind << " " << no << " " << st;
    Utilities::LogMessage(msg.str());
    return st;
  }
  return "";
}

void ReadLineClass::HistoryDelete(const ssize_t ind, const ssize_t n)
{}

void ReadLineClass::HistoryAdd(const std::string &st)
{}

int ShellDataClass::GetPaths()
{
  // set currentDir and root
  currentDir = Utilities::GetCurrentDirectory();
  fs::path curPath(currentDir);
  root = curPath.root_name().string();
  
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
  std::string cur = Utilities::GetCurrentDirectory();
  Utilities::FixupPath(dir);
  if (not Utilities::SetCurrentDirectory(dir)) {
    return false;
  }

  if (push) {
    pushDirs.push_back(cur);
  }

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



bool ShellDataClass::RunHooks(std::vector<std::string> &args)
{
  if (args.size() == 0) {
    return false;
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

}


bool ShellDataClass::ProcessCommand(const std::string &commandLineArg)
{
  if (commandLineArg.empty()) 
    return 1;
  
  std::vector<std::string> args;
  std::string lastTok;
  
  bool lastBlank = Utilities::ParseLine(commandLineArg, args, false);
  std::string cmd = args[0];

  Utilities::StripString(cmd);

  unsigned cmdLen = cmd.length();
  
  // command of the form c:
  if (cmdLen == 2 && cmd[1] == ':') {
    return DoCD(cmd);
  }

  // update aliases
  if (aliases.count(cmd) > 0) {
    args[0] = aliases[cmd];
  }

  bool res = RunHooks(args);
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

  return true;
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

  bool PWD(const std::vector<std::string> &args, ShellDataClass &shell) {
    std::string currentPath = Utilities::GetCurrentDirectory();
    std::cout << currentPath << std::endl; 
    return true;
  }

  bool PopDir(const std::vector<std::string> &args, ShellDataClass &shell) {
    return shell.PopDir();
  }


  std::string ExpandVars(const std::string &st) {
    // expand any environment variables in st
#ifdef __WIN32__
    std::string newSt;
    bool inVar = false;
    int pos = st.find("%");
    int curPos = 0;
    int len = st.size();
    while (pos != st.npos) {
      if (inVar) {
        auto var = st.substr(curPos, pos-curPos);
        var = Utilities::GetEnvVar(var);
        newSt += var;
        inVar = false;
      } else {
        newSt += st.substr(curPos, pos-curPos);
        inVar = true;
      }
      curPos = pos + 1;
      if (curPos >= len-1) {
        break;
      }
      pos = st.find("%", curPos);
    }
    if (curPos < len) {
      newSt += st.substr(curPos);
    }
    return newSt;
#else
#endif    
  }


  bool SetEnv(const std::vector<std::string> &args, ShellDataClass &shell) {
    if (args.size() > 1) {
      const std::string &cmd = args[1];
      int pos = cmd.find('=');
      if (pos != cmd.npos) {
        std::string var = cmd.substr(0, pos);
        std::string val = cmd.substr(pos+1);
        val = ExpandVars(val);
        _putenv_s(var.c_str(), val.c_str());
        // SetEnvironmentVariable(var.c_str(), val.c_str());
        return 1;
      }
    } else {
      char **env = environ;
      for (env; *env; ++env) {
        std::cout << *env << "\n";
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

  configFolder = Utilities::GetConfigFolder();
  doLog = useLog;  

  hooks["exit"] = &ShellFuncs::ExitFunc;
  hooks["cd"] = &ShellFuncs::CD;
  hooks["pwd"] = &ShellFuncs::PWD;
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


namespace SignalHandling {
  // Signal handler for SIGINT (ie Ctrl+C) - used in system call
  int sig = 0;
  void Handler(int signal)
  {
    sig = signal;
  }
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

  Utilities::SetupLogging(doLog);

  try {
    std::shared_ptr<ShellDataClass> shell = std::make_shared<ShellDataClass>(doLog);
    ReadLineClass readLine(shell);

    setlocale(LC_ALL,"C.UTF-8");  // we use utf-8 in this example

    // set signal handler
    std::signal(SIGINT, SignalHandling::Handler);

    // use StyleDef to use bbcode's for markup
    readLine.StyleDef("kbd","gray underline");     // you can define your own styles
    readLine.StyleDef("ic-prompt","cadetblue");     // you can define your own styles
    // ic_style_def("ic-prompt","ansi-maroon");  // or re-define system styles
    
    readLine.Printf( "[b]Welcome to CrabShell[/b]\n\n", "");
    
    // enable history; use a NULL filename to not persist history to disk
    readLine.ReadHistory("history.dat");

    // enable syntax highlighting with a highlight function
    // ic_set_default_highlighter(highlighter, NULL);

    // try to auto complete after a completion as long as the completion is unique
    readLine.EnableAutoTab(false);

    // inline hinting is enabled by default
    readLine.EnableHint(true);

    // run until empty input
    char* input;

    while(true) {
      std::string curDir = shell->GetCurrentDir();    // get the folder for the history, as the cmd may be a cd
      std::string input = readLine.ReadLine(shell->GetPrompt());   // ctrl-d returns NULL (as well as errors)
      try {
        // readLine.Printf("%s\n", input.c_str());
        bool res = shell->ProcessCommand(input);
        if (input == "exit") {
            break;
        } 

        if (input.length() > 0)  {
          readLine.AddHistory(input, curDir, true);
        }
        if (not res) {
          std::string err;
          if (Utilities::HasError(err)) {
            readLine.Printf(err);
          }
        }
      } catch (std::exception &e) {
        // catch and continue
        std::string err;
        if (Utilities::HasError(err)) {
          readLine.Printf(err, "");
        } else {
          readLine.Printf("Error: with the command\n","");
        }
      }

    }

    readLine.Printf("Goodbye\n", "");

  } catch (std::exception &e) {
    std::cerr << "Error starting CrabShell " << e.what() << "\n";
    return 1;
  }


  return 0;
}
