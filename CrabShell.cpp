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

#ifdef __WIN32__
# include <shlobj.h>
# include <shlwapi.h>
# include <objbase.h>

// break some of the Utilities routines
# ifdef GetCurrentDirectory
# undef GetCurrentDirectory
# undef SetCurrentDirectory
# endif

#endif

#include <crossline.h>

#include "ShellData.h"
#include "History.h"
#include "Utilities.h"
#include "Config.h"

#define USELUA
#ifdef USELUA
#  include "LuaInterface.h"
#endif


// need to get and set environment variables
extern char ** environ;


class ReadLineClass : public Crossline {
protected:
  std::shared_ptr<ShellDataClass> shell;
  // ShellHistoryClass history;


  std::chrono::high_resolution_clock hintClock;
  int hintDelayMS;
  std::chrono::time_point<std::chrono::high_resolution_clock> lastHint;

  bool debug;
public:
    ReadLineClass(std::shared_ptr<ShellDataClass> sh, const bool debug);

    // Provide a hint after a new character is entered
    bool Hint(const std::string &inp, CompletionItem &hint, const bool atEnd);

    virtual void AddHistory(const std::string &statement, const std::string &folder, const bool write);

    void ReadHistory(const std::string &name);

    virtual int HistoryCount();
    virtual HistoryItemPtr GetHistoryItem(const ssize_t n) const;
    virtual void HistoryDelete(const ssize_t ind, const ssize_t n);
    virtual void HistoryAdd(const std::string &st);
};


ReadLineClass::ReadLineClass(std::shared_ptr<ShellDataClass> sh, const bool dbg) : 
  Crossline(new Utilities::FileCompleter(), new ShellHistoryClass()), shell(sh) 
{
    hintDelayMS = 300;
    lastHint = hintClock.now();
    debug = dbg;
}

// -------------------------------------------------------------------------------
// Completion
// -------------------------------------------------------------------------------

bool ReadLineClass::Hint(const std::string &inp, CompletionItem &hint, const bool atEnd)
{
#ifndef USE_CROSSLINE  
    // return hint for potential completion
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
#endif
    // need to rewrite hints for Crossline
    return false;
}


void ReadLineClass::AddHistory(const std::string &statement, const std::string &folder, const bool write)
{

  std::chrono::high_resolution_clock clock;
  auto t1 = clock.now();
  const auto t2 = std::chrono::system_clock::to_time_t(t1);
  char nowSt[128];
  std::strftime(nowSt, 128, "%Y-%m-%d %H:%M:%S", std::localtime(&t2));
  // std::string nowSt = std::put_time(std::localtime(&t2), "%F %T.\n");

  ShellHistoryClass *his = dynamic_cast<ShellHistoryClass*>(history);
  his->Append(statement, folder, nowSt, write);
}


void ReadLineClass::ReadHistory(const std::string &name)
{
    fs::path inPath = fs::path(Utilities::GetConfigFolder()) / name;

   ShellHistoryClass *his = dynamic_cast<ShellHistoryClass*>(history);
   his->Clear();
   his->Load(inPath.string());
}


int ReadLineClass::HistoryCount() 
{
    return history->Size();
}


HistoryItemPtr ReadLineClass::GetHistoryItem(const ssize_t n) const
{
  int no = history->Size();
  int ind = n;
  // interpret < 0 as from end
  if (n < 0) {
    ind = no + n;
  }
  if (ind < 0) {
    return std::make_shared<CrabHistoryItem>("", "", "");
  }
  if (ind <= no) {    // the call starts at 1
    HistoryItemPtr ptr = history->GetHistoryItem(ind);
    std::string st = ptr->item;

    std::ostringstream msg;
    msg << "Returning history item " << n << " " << ind << " " << no << " " << st;
    Utilities::LogMessage(msg.str());
    return ptr;
  }
  return std::make_shared<CrabHistoryItem>("", "", "");
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


void ShellDataClass::AddAlias(const std::string &alias, const std::string &cmd)
{
  aliases[alias] = cmd;
}


// various hooks for lua
bool ShellDataClass::RunHook(const HookData &data)
{
  return true;
}


bool ShellDataClass::RunCommand(const std::vector<Utilities::CmdToken> &args)
{
  if (args.size() == 0) {
    return false;
  }

  std::vector<std::string> argSts(args.size());
  for (int i = 0; i < args.size(); i++) {
    argSts[i] = args[i].cmd;
  }

#ifdef USELUA
  // run any lua functions
  if (lua->RunCommand(argSts)) {
    return true;
  }
#endif

  // hooks should be lowercase
  std::string cmd = Utilities::ToLower(argSts[0]);
  std::map<std::string, CmdFunc>::iterator iter = funcs.find(cmd);
  if (iter != funcs.end()) {
    return (iter->second)(argSts, *this);
  }
  return 0;

}


#ifdef __WIN32__
bool ShellDataClass::MSWSystem(const std::vector<std::string> &cmdArgs)
{
    SHELLEXECUTEINFO ShExecInfo;

    if (cmdArgs.size() == 0) {
        return false;
    }

    TCHAR args[MAX_PATH];
    args[0] = 0;
    for (int i = 1; i < cmdArgs.size(); i++)  {
        strcat(args, cmdArgs[i].c_str());
        strcat(args, " ");
    }

    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOASYNC |
                      SEE_MASK_FLAG_NO_UI |
                      SEE_MASK_NO_CONSOLE;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = cmdArgs[0].c_str();
    ShExecInfo.lpParameters = args;
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOWDEFAULT;
    ShExecInfo.hInstApp = NULL;

    return ShellExecuteEx(&ShExecInfo);
}
#endif


bool ShellDataClass::ProcessCommand(const std::string &commandLineArg)
{
  if (commandLineArg.empty()) 
    return 1;
  
  // std::vector<std::string> args;
  
  Utilities::LogMessage("Running command " + commandLineArg);

  Utilities::CmdClass cmdInfo;
  // bool lastBlank = Utilities::ParseLine(commandLineArg, args, false);
  cmdInfo.ParseLine(commandLineArg, false);

  // at the moment deal with pipes or redirects using the system command
  if (cmdInfo.type == Utilities::CmdClass::PlainCmd ||
      cmdInfo.type == Utilities::CmdClass::Pipe ||
      cmdInfo.type == Utilities::CmdClass::Redirection) {
    
    std::string cmd = cmdInfo.GetArg(0);

    Utilities::StripStringBegin(cmd);

    unsigned cmdLen = cmd.length();
    
    // command of the form c:
    if (cmdLen == 2 && cmd[1] == ':') {
      return DoCD(cmd);
    }

    // update aliases
    if (aliases.count(cmd) > 0) {
      cmdInfo.SetArg(0, aliases[cmd]);
    }

    bool res = RunCommand(cmdInfo.GetTokens());
    if (res) {
      return res;
    }
  }

  // Reconstruct the command line - potoentially with alias
  std::string cmdLine;
  const std::vector<Utilities::CmdToken> &args = cmdInfo.GetTokens();
  if (args.size() == 0) {
    return false;
  }
  for (int i = 0; i < args.size()-1; i++) {
    cmdLine += args[i].cmd + " ";
  }
  cmdLine += args.back().cmd;

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
    std::string newSt;
    bool inVar = false;
    int pos = st.find("$");
    int curPos = 0;
    int len = st.size();
    while (pos != st.npos) {
      std::string nextSearch;
      if (inVar) {
        auto var = st.substr(curPos, pos-curPos);
        var = Utilities::GetEnvVar(var);
        newSt += var;
        inVar = false;
        nextSearch = "$";
      } else {
        newSt += st.substr(curPos, pos-curPos);
        inVar = true;
        nextSearch = ":";
      }
      curPos = pos + 1;
      if (curPos >= len-1) {
        break;
      }
      pos = st.find(nextSearch, curPos);
    }
    if (curPos < len) {
      newSt += st.substr(curPos);
    }
    return newSt;
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
        // _putenv_s(var.c_str(), val.c_str());
        std::string cmd = var + "=" + val;
        putenv(const_cast<char*>(cmd.c_str()));
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


ShellDataClass::ShellDataClass(const bool useLog, const std::string &localConfig) 
{

  configFolder = Utilities::GetConfigFolder();
  doLog = useLog;  

  funcs["exit"] = &ShellFuncs::ExitFunc;
  funcs["cd"] = &ShellFuncs::CD;
  funcs["pwd"] = &ShellFuncs::PWD;
  funcs["pushd"] = &ShellFuncs::PushDir;
  funcs["popd"] = &ShellFuncs::PopDir;
  funcs["setcolour"] = &ShellFuncs::SetColour;
  funcs["set"] = &ShellFuncs::SetEnv;
  maxPrompt = 25;

  std::string configFile;
  if (localConfig.size() > 0) {
    configFile = localConfig;
  } else {
    configFile = (configFolder / "Config.dat").string();
  }

  bool hasConfig = true;
  if (!fs::exists(configFile)) {
    std::cerr << "Cannot find " << configFile << "\n";
    hasConfig = false;
  }

#ifdef USELUA
  lua = new LuaInterface(this);
  if (hasConfig) {
    lua->LoadFile(configFile);
    lua->LoadPlugins();
  }

#else  
  ConfigMap configData;
  if (hasConfig && LoadConfig(configFile, configData)) {
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
#endif
  
  GetPaths();
}


ShellDataClass::~ShellDataClass()
{
#ifdef USELUA
  if (lua != nullptr) {
    delete lua;
  }
#endif  
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
  bool debug = false;
  bool err = false;

  int i = 1;
  while (i < argc) {
    std::string arg(argv[i]);
    if (arg == "-l") {
      doLog = true;
    } else if (arg == "-c") {
      if (i+1 < argc) {
        std::string configFolder = argv[i+1];
        Utilities::SetConfigFolder(configFolder);
        i++;
      } else {
        err = true;
      }
    } else if (arg == "-d") {
      debug = true;
    } else {
      std::cerr << "Unknown argument " << arg << "\n";
      err = true;
    }
    i++;
  }

  if (err) {
    std::cerr << "Usage: CrabShell [-l] [-c configFile]\n";
    std::cerr << "   -l: write information to log\n";
    std::cerr << "   -c: alternative configuration location\n";
    std::cerr << "   -d: activate a debugging mode\n";
    return 1;
  }

  if (!Utilities::SetupConfigFolder()) {
    std::cerr << "Could not find or setup the configuration folder " << 
      Utilities::GetConfigFolder() << "\n";
      return 1;
  }

  Utilities::SetupLogging(doLog);

  try {
    std::shared_ptr<ShellDataClass> shell = std::make_shared<ShellDataClass>(doLog, "");

    ReadLineClass readLine(shell, debug);

    setlocale(LC_ALL,"C.UTF-8");  // we use utf-8 in this example

    // set signal handler
    std::signal(SIGINT, SignalHandling::Handler);

    // readLine.crossline_prompt_color_set(CROSSLINE_FGCOLOR_BLUEGREEN);
    readLine.PromptColorSet(CROSSLINE_FGCOLOR_CYAN);

    readLine.ColorSet (CROSSLINE_FGCOLOR_CYAN | CROSSLINE_FGCOLOR_BRIGHT);
    readLine.PrintStr( "Welcome to CrabShell\n\n");
    readLine.ColorSet (CROSSLINE_FGCOLOR_DEFAULT);

    readLine.PagingSet(false);

#  ifdef __WIN32__
    readLine.AllowESCCombo(false);
#  endif    

    // Set the maximum number of search items to return
    readLine.HistorySetSearchMaxCount(12);

    readLine.HistorySetup(true);
    // enable history; use a NULL filename to not persist history to disk
    readLine.ReadHistory("history.dat");

    if (readLine.HistoryCount() > 20*1024) {
      std::ostringstream msg;
      msg << "Have " << readLine.HistoryCount() << " history items. Suggest running CleanHistory\n\n";
      readLine.PrintStr(msg.str());
    }


    while(true) {
      std::string curDir = shell->GetCurrentDir();    // get the folder for the history, as the cmd may be a cd
      std::string prompt = shell->GetPrompt();
      if (debug) {
        prompt = "Deb: " + prompt;
      }
      prompt += "> ";

      std::string input;
      if (readLine.ReadLine(prompt, input)) {   // ctrl-d returns NULL (as well as errors)
        try {
          // readLine.Printf("%s\n", input.c_str());
          bool res = shell->ProcessCommand(input);
          if (input == "exit") {
              break;
          } 

          if (input.length() > 0)  {
            readLine.AddHistory(input, curDir, !debug);
          }
          if (not res) {
            std::string err;
            if (Utilities::HasError(err)) {
              readLine.PrintStr(err);
            }
          }
        } catch (std::exception &e) {
          // catch and continue
          std::string err;
          if (Utilities::HasError(err)) {
            readLine.PrintStr(err);
          } else {
            readLine.PrintStr("Error: with the command\n");
          }
        }
      }
    }

    readLine.PrintStr("Goodbye\n");

  } catch (std::exception &e) {
    std::cerr << "Error starting CrabShell " << e.what() << "\n";
    return 1;
  }


  return 0;
}
