/* Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  Utilities.h
  Utilities functions
*/

#pragma once

#include <string>
#include <vector>
#include <memory>

#include <isocline_pp.h>


namespace Utilities {

#ifdef __WIN32__
  static const char pathSep = '\\';
#else
  static const char pathSep = '/';
#endif

  struct CompletionInfo {
    CompletionItem item;
    bool NeedsQuotes;
  };
  
  bool IsWindows();
  
  void SplitString(const std::string &st, const std::string &sep, std::vector<std::string> &res);
  bool StartsWith(const std::string &mainStr, const std::string &startIn, const bool ignoreCase=false);

  bool StartsWith(const std::string &mainStr, const std::string &start, const bool ignoreCase);
  bool GetFileMatches(const std::string &line, std::vector<CompletionItem> &matches);

  std::string AbbrevPath(const std::string &path, const int maxLen);

  bool SetCurrentDirectory(const std::string &d);
  std::string GetCurrentDirectory();

  bool FixupPath(std::string &path);
  bool StripString(std::string &cmd);

  std::string ToLower(const std::string &st);

  std::string GetEnvVar(const std::string &var);
  std::string GetHome();

  void SetConfigFolder(const std::string &fld);
  std::string GetConfigFolder();

  bool FileExists(const std::string &f);

  void SetupLogging(const bool doLog);
  void LogMessage(const std::string &msg);
  void LogError(const std::string &msg);
  bool HasError(std::string &msg);


  // bool ParseLine(const std::string &line, std::vector<std::string> &args, const bool stripQuotes);
  
  class CmdClass {
      // class to store the elements of a command line
      // Simple:      "jed fred.txt"
      // Redirection: "echo python PlotNSP.py > plotP.bat"
      // Pipes:       "type fred.txt | grep hello"

  protected:
      bool ParseTokens(const std::vector<std::string> &toks, const int tok);
      std::string cmdToks[3];

  public:
      enum CmdType {
          PlainCmd,
          Pipe,
          Redirection
      };

      CmdClass();

      CmdType type;
      std::shared_ptr<CmdClass> preCmd;    // before a > or |
      std::shared_ptr<CmdClass> postCmd;   // after a > or |
      std::vector<std::string> tokens;     // broken into tokens
      int numQuotes;                        // number of quotes in line

      bool ParseLine(const std::string &line, const bool stripQuotes);

      int GetNoArgs() const;
      std::string GetArg(const int n) const;
      void SetArg(const int n, const std::string &c);

      void Print(std::ostream &out);
  };

  class FileLock {
  protected:
    bool hasLock;
    std::string lckFile;
  public:
    FileLock(const std::string &fileName);
    ~FileLock();
    bool HasLock() const;
  };

}