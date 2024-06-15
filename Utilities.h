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

  bool ParseLine(const std::string &line, std::vector<std::string> &args, const bool stripQuotes);
  
  bool FixupPath(std::string &path);
  bool StripString(std::string &cmd);

  std::string ToLower(const std::string &st);

  std::string GetEnvVar(const std::string &var);
  std::string GetHome();
  std::string GetConfigFolder();

  void SetupLogging(const bool doLog);
  void LogMessage(const std::string &msg);
  void LogError(const std::string &msg);
  bool HasError(std::string &msg);


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