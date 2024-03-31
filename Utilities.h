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

class CompletionItem;


namespace Utilities {

#ifdef WIN32  
  static const char pathSep = '\\';
#else
  static const char pathSep = '/';
#endif

  void SplitString(const std::string &st, const std::string &sep, std::vector<std::string> &res);

  bool StartsWith(const std::string &mainStr, const std::string &start, const bool ignoreCase);
  bool GetFileMatches(const std::string &line, std::vector<CompletionItem> &matches);

  std::string AbbrevPath(const std::string &path, const int maxLen);

  void SetCurrentDirectory(const std::string &d);
  std::string GetCurrentDirectory();

  bool ParseLine(const std::string &line, std::vector<std::string> &args);
  
  bool FixupPath(std::string &path);
  bool StripString(std::string &cmd);

}