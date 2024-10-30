// Class for interacting with the OS

#pragma once

#include <string>
#include <vector>
#include <map>
#include <filesystem>
namespace fs = std::filesystem;

#include "Utilities.h"

class LuaInterface;

class HookData {
public:
  std::string hookType;
  HookData();
  virtual bool SetupLuaFunc(LuaInterface *lua);
};

// This class is the interface with the OS
class ShellDataClass {
protected:

  std::string currentDir;    
  std::string root;

  std::vector<std::string> pushDirs;

  std::string currentPrompt;
  int maxPrompt;

  fs::path configFolder;
  int pid;
  bool doLog;  

  std::map<std::string, std::string> aliases;
  std::string startDir;

  typedef bool (*CmdFunc)(const std::vector<std::string> &args, ShellDataClass &shell);
  std::map<std::string, CmdFunc> funcs;

  LuaInterface *lua;

  bool RunCommand(const std::vector<Utilities::CmdToken> &args);

public:
  ShellDataClass(const bool useLog=false, const std::string &configFolder="") ;
  
  ~ShellDataClass();

  // various hooks for lua
  bool RunHook(const HookData &data);

  bool DoCD(const std::string &dir, const bool push=false);
  bool PopDir();
  int SetDrive(const char d);
  
  // Get the drive number corresponding to a letter
  static int GetDriveNo(const char d);

  const std::string &GetCurrentDir() const {return currentDir;}

  const std::string GetPrompt();

  // Update path information
  int GetPaths();
  
  bool MSWSystem(const std::vector<std::string> &args);
  bool ProcessCommand(const std::string &commandLineArg);

  void AddAlias(const std::string &alias, const std::string &cmd);


};
