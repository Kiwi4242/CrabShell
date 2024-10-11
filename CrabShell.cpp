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
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>

// break some of the Utilities routines
# ifdef GetCurrentDirectory
# undef GetCurrentDirectory
# undef SetCurrentDirectory
# endif

#endif

#ifdef USE_CROSSLINE
#include <crossline.h>
#else
#include <isocline_pp.h>
#endif

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


#ifdef USE_CROSSLINE
class ReadLineClass : public Crossline {
#else
class ReadLineClass : public IsoclinePP {
#endif
protected:
  std::shared_ptr<ShellDataClass> shell;
  ShellHistoryClass history;


  std::chrono::high_resolution_clock hintClock;
  int hintDelayMS;
  std::chrono::time_point<std::chrono::high_resolution_clock> lastHint;

  bool debug;
public:
    ReadLineClass(std::shared_ptr<ShellDataClass> sh, const bool debug);

    // Complete after tab called
#ifdef USE_CROSSLINE      
    bool Completer(const std::string &inp, const int pos, CrosslineCompletions &completions);
#else    
    bool Completer(const std::string &inp, std::vector<CompletionItem> &completions);
#endif

    // Provide a hint after a new character is entered
    bool Hint(const std::string &inp, CompletionItem &hint, const bool atEnd);

#ifndef USE_CROSSLINE
    using IsoclinePP::AddHistory;
#endif    
    virtual void AddHistory(const std::string &statement, const std::string &folder, const bool write);

    void ReadHistory(const std::string &name);

    virtual int HistoryCount();
#ifdef USE_CROSSLINE
    virtual HistoryItemPtr GetHistoryItem(const ssize_t n, const bool forward);
#else    
    virtual std::string GetHistoryItem(const ssize_t n);
#endif    
    virtual void HistoryDelete(const ssize_t ind, const ssize_t n);
    virtual void HistoryAdd(const std::string &st);

};


#ifdef USE_CROSSLINE
ReadLineClass::ReadLineClass(std::shared_ptr<ShellDataClass> sh, const bool dbg) : Crossline(), shell(sh) 
#else
ReadLineClass::ReadLineClass(std::shared_ptr<ShellDataClass> sh, const bool dbg) : IsoclinePP(), shell(sh) 
#endif
{
#ifndef USE_CROSSLINE  
    EnableHint(true);
    EnableMultiLine(false);
    UseCustomHistory();
#endif    
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
    return false;
}


#ifdef USE_CROSSLINE      
bool ReadLineClass::Completer(const std::string &inp, const int pos, CrosslineCompletions &completions)
{
  // complete file name
  try {
    std::vector<CompletionItem> comp;
    int startPos;  // the start of the word being matched
    // just pass the portion up to pos
    std::string stIn = inp.substr(0, pos);
    Utilities::GetFileMatches(stIn, comp, startPos);
    completions.Setup(startPos, pos);

    Utilities::LogMessage("Completions for " + inp);
    for (size_t i = 0; i < comp.size(); i++) {
      auto cmd = comp[i];
      std::string &cmp = cmd.comp;
      std::ostringstream msg;
      msg << "\t" << cmp << " " << completions.start;
      Utilities::LogMessage(msg.str());
      completions.Add(cmp, "", cmd.needQuotes);
    }
  } catch (std::exception &e) {
  }

  return completions.Size() > 0;
}
#else
bool ReadLineClass::Completer(const std::string &inp, std::vector<CompletionItem> &completions)
{
  // complete file name
  try {
    
    Utilities::GetFileMatches(inp, completions);

    Utilities::LogMessage("Completions for " + inp);
    for (size_t i = 0; i < completions.size(); i++) {
      const CompletionItem &cmp = completions[i];
      std::ostringstream msg;
      msg << "\t" << cmp.comp << " " << cmp.delBefore;
      Utilities::LogMessage(msg.str());
    }
  } catch (std::exception &e) {
  }

  return completions.size() > 0;
}
#endif

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
}


int ReadLineClass::HistoryCount() 
{
    return history.GetNoHistory();
}


#ifdef USE_CROSSLINE
HistoryItemPtr ReadLineClass::GetHistoryItem(const ssize_t n, const bool forward)
#else
std::string ReadLineClass::GetHistoryItem(const ssize_t n)
#endif
{
  int no = history.GetNoHistory();
  int ind = n;
  // interpret < 0 as from end
  if (!forward) {
    ind = no - 1 - n;  
  } else if (n < 0) {
    ind = no + n;
  }
  if (ind < 0) {
#ifdef USE_CROSSLINE
    return std::make_shared<HistoryItem>("", "", "");
#else    
    return "";
#endif    
  }
  if (ind <= no) {    // the call starts at 1
#ifdef USE_CROSSLINE
    HistoryItemPtr ptr = history.Get(ind);
    std::string st = ptr->item;
#else    
    std::string st = history.Get(ind)->cmd;
#endif    
    std::ostringstream msg;
    msg << "Returning history item " << n << " " << ind << " " << no << " " << st;
    Utilities::LogMessage(msg.str());
#ifdef USE_CROSSLINE
    return ptr;
#else    
    return st;
#endif    
  }
#ifdef USE_CROSSLINE
    return std::make_shared<HistoryItem>("", "", "");
#else    
  return "";
#endif  
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


bool ShellDataClass::RunHooks(const std::vector<Utilities::CmdToken> &args)
{
  if (args.size() == 0) {
    return false;
  }

  std::vector<std::string> argSts(args.size());
  for (int i = 0; i < args.size(); i++) {
    argSts[i] = args[i].cmd;
  }

  // run any lua functions
  if (lua->RunCommand(argSts)) {
    return true;
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

  // hooks should be lowercase
  std::string cmd = Utilities::ToLower(argSts[0]);
  std::map<std::string, HookFunc>::iterator iter = hooks.find(cmd);
  if (iter != hooks.end()) {
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

  if (cmdInfo.type == Utilities::CmdClass::PlainCmd) {
    // at the moment deal with pipes or redirects using the system command
    
    std::string cmd = cmdInfo.GetArg(0);

    Utilities::StripString(cmd);

    unsigned cmdLen = cmd.length();
    
    // command of the form c:
    if (cmdLen == 2 && cmd[1] == ':') {
      return DoCD(cmd);
    }

    // update aliases
    if (aliases.count(cmd) > 0) {
      cmdInfo.SetArg(0, aliases[cmd]);
    }

    bool res = RunHooks(cmdInfo.GetTokens());
    if (res) {
      return res;
    }
  }

  // Reconstruct the command line - potoentially with alias
  std::string cmdLine;
  const std::vector<Utilities::CmdToken> &args = cmdInfo.GetTokens();
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


ShellDataClass::ShellDataClass(const bool useLog, const std::string &localConfig) 
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

  std::string configFile;
  if (localConfig.size() > 0) {
    configFile = localConfig;
  } else {
    configFile = (configFolder / "Config.dat").string();
  }


#ifdef USELUA
  lua = new LuaInterface(this);
  lua->LoadFile(configFile);
  lua->LoadPlugins();

#else  
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
#endif

  GetPaths();
}


ShellDataClass::~ShellDataClass()
{
  if (lua != nullptr) {
    delete lua;
  }
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

  Utilities::SetupLogging(doLog);

  try {
    std::shared_ptr<ShellDataClass> shell = std::make_shared<ShellDataClass>(doLog, "");

    ReadLineClass readLine(shell, debug);

    setlocale(LC_ALL,"C.UTF-8");  // we use utf-8 in this example

    // set signal handler
    std::signal(SIGINT, SignalHandling::Handler);

#ifdef USE_CROSSLINE
    readLine.crossline_prompt_color_set(CROSSLINE_FGCOLOR_BLUEGREEN);

    readLine.crossline_color_set (CROSSLINE_FGCOLOR_BLUE);
    readLine.NewPrint( "Welcome to CrabShell\n\n");
    readLine.crossline_color_set (CROSSLINE_FGCOLOR_DEFAULT);

    readLine.AllowESCCombo(false);

#else    
    // use StyleDef to use bbcode's for markup
    readLine.StyleDef("kbd","gray underline");     // you can define your own styles
    readLine.StyleDef("ic-prompt","cadetblue");     // you can define your own styles
    // ic_style_def("ic-prompt","ansi-maroon");  // or re-define system styles

    // enable syntax highlighting with a highlight function
    // ic_set_default_highlighter(highlighter, NULL);

    // try to auto complete after a completion as long as the completion is unique
    readLine.EnableAutoTab(false);

    // inline hinting is enabled by default
    readLine.EnableHint(true);

    readLine.NePrint( "[b]Welcome to CrabShell[/b]\n\n");
#endif

    
    readLine.HistorySetup(true);
    // enable history; use a NULL filename to not persist history to disk
    readLine.ReadHistory("history.dat");

    if (readLine.HistoryCount() > 16*1024) {
      std::ostringstream msg;
      msg << "[bold][blue]Have " << readLine.HistoryCount() << " history items. Suggest running CleanHistory[/][/]\n\n";
      readLine.NewPrint(msg.str());
    }


    // run until empty input
    char* input;

    while(true) {
      std::string curDir = shell->GetCurrentDir();    // get the folder for the history, as the cmd may be a cd
      std::string prompt = shell->GetPrompt();
      if (debug) {
        prompt = "Deb: " + prompt;
      }
#ifdef USE_CROSSLINE
      prompt += "> ";
#endif      
      std::string input = readLine.ReadLine(prompt);   // ctrl-d returns NULL (as well as errors)
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
            readLine.NewPrint(err);
          }
        }
      } catch (std::exception &e) {
        // catch and continue
        std::string err;
        if (Utilities::HasError(err)) {
          readLine.NewPrint(err);
        } else {
          readLine.NewPrint("Error: with the command\n");
        }
      }

    }

    readLine.NewPrint("Goodbye\n");

  } catch (std::exception &e) {
    std::cerr << "Error starting CrabShell " << e.what() << "\n";
    return 1;
  }


  return 0;
}
