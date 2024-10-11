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

#ifdef USE_CROSSLINE
#include <crossline.h>
#else
#include <isocline_pp.h>
#endif

#ifdef USE_CROSSLINE
  struct CompletionItem {
    std::string comp;
    bool needQuotes;
    // int start;
  };
#endif


namespace Utilities {

#ifdef __WIN32__
  static const char pathSep = '\\';
#else
  static const char pathSep = '/';
#endif

  struct CompletionInfo {
    CompletionItem item;
  };
  
  bool IsWindows();
  
  void SplitString(const std::string &st, const std::string &sep, std::vector<std::string> &res);
  bool StartsWith(const std::string &mainStr, const std::string &startIn, const bool ignoreCase=false);

  bool StartsWith(const std::string &mainStr, const std::string &start, const bool ignoreCase);
  bool GetFileMatches(const std::string &line, std::vector<CompletionItem> &matches, int &startPos);

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

  struct CmdToken {
  public:
    std::string cmd;
    bool hasQuotes;
    int startPos;        // the start position of the full token including quotes
    CmdToken(const std::string tok, const int st, const int qp);
  }; 

  class CmdClass {
      // class to store the elements of a command line

  public:
      // Simple:      "jed fred.txt"
      // Redirection: "echo python PlotNSP.py > plotP.bat"
      // Pipes:       "type fred.txt | grep hello"
      enum CmdType {
          PlainCmd,
          Pipe,
          Redirection
      };
      CmdType type;

  protected:
      bool IdentifyTokens(const std::vector<CmdToken> &toks, const int tok);
      std::string cmdToks[3];

      std::shared_ptr<CmdClass> preCmd;    // before a > or |
      std::shared_ptr<CmdClass> postCmd;   // after a > or |
      std::vector<CmdToken> tokens;     // broken into tokens
      int numQuotes;                        // number of quotes in line

  public:

      CmdClass();


      bool ParseLine(const std::string &line, const bool stripQuotes);

      int GetNoArgs() const;
      std::string GetArg(const int n) const;
      void SetArg(const int n, const std::string &c);

      void Print(std::ostream &out);

      const std::vector<CmdToken> &GetTokens() const;
      CmdToken LastToken() const;
      void PopBack();
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