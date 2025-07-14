/* ----------------------------------------------------------------------------
  Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  Utilities.cpp
  Various utility functions
-----------------------------------------------------------------------------*/


#include <fstream>
#include <iostream>
#include <thread>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

#include <chrono>
using namespace std::chrono_literals;

#include <algorithm>
#include <cctype>

#include "Utilities.h"
 
namespace Utilities {


#ifdef __WIN32__
  static const bool isWindows = true;
#else
  static const bool isWindows = false;
#endif

  static std::string customConfigFolder;

  class LogClass {
  protected:
    std::ofstream out;
    std::string error;
    bool doLog;

  public:
      static LogClass *GetLog() {
        static LogClass instance;  // Guaranteed to be destroyed.
                                   // Instantiated on first use.
        return &instance;
      }

  private:
      LogClass() {
        doLog = false;
      }


      // C++ 11
      // =======
      // We can use the better technique of deleting the methods
      // we don't want.
  public:
      LogClass(LogClass const&)  = delete;
      void operator=(LogClass const&)  = delete;

      // Note: Scott Meyers mentions in his Effective Modern
      //       C++ book, that deleted functions should generally
      //       be public as it results in better error messages
      //       due to the compilers behavior to check accessibility
      //       before deleted status

  public:

    void SetupLogging(const bool doL) {
        doLog = doL;
        if (doLog) {
          fs::path configFolder(Utilities::GetConfigFolder());
          fs::path logFile = configFolder / "CrabShell.log";
          out.open(logFile);
        }
      }


    void LogMessage(const std::string &msg) {
      if (doLog) {
        out << msg << "\n";
      }
    }

    void LogError(const std::string &msg) {
      error = msg;
      if (doLog) {
        out << msg << "\n";
      }
    }

    std::string GetError() const {
      return error;
    }

  };


  void SetupLogging(const bool doLog) 
  {
    LogClass *log = LogClass::GetLog();
    log->SetupLogging(doLog);
  }

  void LogMessage(const std::string &msg) {
    LogClass::GetLog()->LogMessage(msg);
  }

  void LogError(const std::string &msg) {
    LogClass::GetLog()->LogError(msg);
  }


  bool HasError(std::string &msg) {
    std::string st = LogClass::GetLog()->GetError();
    if (st.length() > 0) {
      msg = st;
      return true;
    }
    return false;
  }

  bool IsWindows() {
    return isWindows;
  }


  bool SetupConfigFolder()
  {
      fs::path folder(GetConfigFolder());
      if (!fs::exists(folder)) {
        if (!fs::create_directory(folder)) {
          return false;
        }
      }
      return true;
  }

  void SetConfigFolder(const std::string &fld)
  {
      customConfigFolder = fld;
  }

  std::string GetConfigFolder()
  {
    if (customConfigFolder.size() > 0) {
      return customConfigFolder;
    }
    std::string home = Utilities::GetHome();
    fs::path p = fs::path(home) / ".crabshell";
    return p.string();
  }


  std::string GetEnvVar(const std::string &var)
  {
    const char * env = std::getenv(var.c_str());
    if (env) {
      return env;
    }
    return "";
  }

  std::string GetHome()
  {
    std::string pathSep = std::string(1, Utilities::pathSep);
    std::string home = GetEnvVar("HOME");
    if (home.length() == 0) {
      if (isWindows) {
        // try userprofile
        std::string profDir = GetEnvVar("USERPROFILE");
        if (profDir.length() > 0) {
          return profDir;
        } else {
          // HOMEDRIVE and HOMEPATH
          std::string homeDir = GetEnvVar("HOMEDRIVE");
          std::string homePath = GetEnvVar("HOMEPATH");
          if (homeDir.length() > 0) {
            if (homePath.length() > 0) {
              return homeDir + homePath;
            }
            return homeDir;
          }
        }
        return pathSep;   // set to root
      }
      return pathSep;     // set to root
    }
    return home;
  }

  std::string ToLower(const std::string &st)
  {
      // need to reserve space in lower
      std::string lower(st);
      std::transform(st.begin(), st.end(), lower.begin(), ::tolower);
      return lower;
  }


  bool StartsWith(const std::string &mainStr, const std::string &startIn, const bool ignoreCase)
  {
      std::string main = mainStr;
      std::string start = startIn;

      if (ignoreCase) {
        // Convert toMatch to lower case
        main = ToLower(main);
        start = ToLower(start);
      }

      if(main.find(start) == 0)
          return true;
      else
          return false;
  }


  bool StripStringBegin(std::string &cmd)
  /* remove white space at start of cmd */
  {
    int pos = cmd.find_first_not_of(" \t", 0);
    if (pos != cmd.npos && pos != 0) {
      cmd.erase(0, pos);
    }
    return true;
  }

  bool StripStringEnd(std::string &cmd)
  /* remove white space at start of end */
  {
    int pos = cmd.find_last_not_of(" \t\r");
    if (pos != cmd.npos && pos != 0) {
      cmd.erase(pos+1);
    }
    return true;
  }

  bool FixupPath(std::string &path)
  {
    /* remove quotes from start and end
       change any / to \
       */
    // remove blanks at start of line
    StripStringBegin(path);

    int pos;
    while ((pos = path.find('"', 0)) != path.npos) {
      path.erase(pos, 1);
    }

    while ((pos = path.find('/', 0)) != path.npos) {
      path.replace(pos, 1, 1, '\\');
    }

    return true;
  }


  bool ExpandPath(std::string &path)
  {
    /* Expand ~ at start of path and run FixupPath
    */
    FixupPath(path);
    if (path[0] == '~' && path[1] == pathSep) {
      std::string home = GetHome();
      if (home.length()) {
        path.replace(0, 1, home);
      }
    }
    
    return true;
  }


  bool FileExists(const std::string &f)
  {
      return fs::exists(f);
  }

  bool SetCurrentDirectory(const std::string &d)
  {
    try {
      fs::path dirPath(d);
      fs::current_path(dirPath);
      return true;
    } catch(const fs::filesystem_error &e) {
      if (!fs::exists(d)) {
        LogError("Error: Directory " + d + " does not exist");
      } else {
        LogError("Error: cannot change directory to: " + d);
      }
    } catch(const std::exception &e) {
        LogError("Error: cannot change directory to: " + d);
    }
    return false;
  }

  std::string GetCurrentDirectory()
  {
    fs::path curDir = fs::current_path();
    return curDir.string();
  }

  void SplitString(const std::string &st, const std::string &sep, std::vector<std::string> &res)
  {
    // split the string st using sep
    std::string::size_type len = st.length();
    std::string::size_type pos = 0;
    unsigned int sepLen = sep.length();
    while (pos < len) {
      std::string::size_type pos1 = st.find(sep, pos);
      if (pos1 == st.npos) {
        res.push_back(st.substr(pos));
        return;
      }
      if (pos1 > pos) {
        res.push_back(st.substr(pos, pos1-pos));
      }
      pos = pos1 + sepLen;
    }
  }


int ReplaceAll(std::string &str, const std::string &from, const std::string &to)
  {
      // replace all occurrences of from with to in str 
      // return count of changes
      size_t start_pos = 0;
      int n = 0;
      while ((start_pos = str.find(from, start_pos)) != str.npos)
      {
          str.replace(start_pos, from.length(), to);
          n++;
          start_pos += to.length();       // Handles case where 'to' is a substring of 'from'
      }
      return n;
  }


  int CountAll(std::string &str, const std::string &sub)
  {
      // count all occurrences of sub in str 
      size_t start_pos = 0;
      int n = 0;
      while ((start_pos = str.find(sub, start_pos)) != str.npos) {
          n++;
          start_pos += sub.length();
      }
      return n;
  }



  bool ParseLine_nu(const std::string &line, std::vector<std::string> &tokens, const bool stripQuotes)
  {

      std::stringstream ss(line);
      std::string token;
      char delim = ' ';
      bool inQuotes = false;


      bool lastBlank = false;
      std::string whiteSpace(" \t");

      // find last non-whitespace
      size_t lastPos = line.find_last_not_of(whiteSpace);
      if (lastPos == line.npos) {
          // no non-whitespace
          return true;
      }
      lastBlank = lastPos < line.length()-1;

      while (std::getline(ss, token, delim)) {
          size_t quotPos, lastPos;
          // remove all quotes
          bool inQuotes2 = inQuotes;
          lastPos = 0;
          while ((quotPos = token.find('"', lastPos)) != std::string::npos) {
            if (stripQuotes) {
              token.erase(quotPos, 1);
            } else {
              lastPos = quotPos + 1;
            }
            inQuotes2 = !inQuotes2;
          }

          if (!inQuotes) {
              tokens.push_back(token);
          } else {
              tokens.back() += delim + token;
          }

          inQuotes = inQuotes2;
          // if (token.find('"') != std::string::npos) {
          //     inQuotes = !inQuotes;
          // }
      }

      return lastBlank;
  }


  std::string BestPath(const std::string &p1, const std::string &relPath)
  {
    // Compare two versions of a path to find the "best"
    // e.g.  ~/Home vs ..  - .. is best
    //       ~/Home vs ../../../ ~/Home is best

    if (relPath.length() == 0) {
      return p1;
    } else if (p1.length() == 0) {
      return relPath;
    }

    // count number of ..
    int n = 0;
    size_t pos = relPath.find("..");
    while (pos != relPath.npos) {
      n++;
      pos = relPath.find(pos+2);
    }
    if (n > 2) {
      return p1;
    }

    if (p1.length() > relPath.length()) {
      return relPath;
    }
    return p1;
  }


  bool GetFileMatches(const std::string &line, std::vector<CompletionItem> &matches, int &startPos)
  {

    CmdClass cmds;
    bool lastBlank = cmds.ParseLine(line, true);
    // std::vector<std::string> &args = cmds.LastToken();

    matches.clear();

    CmdToken lastTok = cmds.LastToken();
    std::string fileSt = lastTok.cmd;    // args.back();

    // do this?
    // if (lastBlank > 0) {
    //   cmds.PopBack();
    // }

    startPos = lastTok.startPos;

    bool ignoreCase = isWindows;
    if (isWindows) {
      ReplaceAll(fileSt, "/", std::string(1, Utilities::pathSep));
    }

    // Have 3 cases:
    // searching for a file in a subfolder - foldera/fil
    // searching for a file in another folder - ../fold
    // searching for a file in the current folder fil

    // the folder we will search
    fs::path searchDir;
    std::string searchName;

    fs::path fullPath(fileSt);

    // split string
    fs::path pathNoName = fullPath.parent_path();
    fs::path fName = fullPath.filename();

    // variables for building the path
    // fs::path relPath;
    // bool useRelPath = false;
    std::string prepend;
    int replaceLen = 0;

    if (pathNoName.empty()) {
      // just a file name, so search current dir
      searchDir = ".";   // fs::current_path();
      searchName = fileSt;
    } else {
      // pathNoName is a folder
      // first replace ~ with Home
      int tdPos;
      std::string pathSt = pathNoName.string();
      if ((tdPos = pathSt.find("~")) != std::string::npos) {
        std::string home = GetHome();
        if (home.back() != Utilities::pathSep) {
          home += pathSep;
        }
        replaceLen = 1;
        if ((tdPos < pathSt.length()-1 and pathSt[tdPos+1] == Utilities::pathSep)
            or (!fullPath.empty())) {
          replaceLen++;
        }
        // update pathNoName
        pathSt = pathSt.substr(0, tdPos) + home + pathSt.substr(tdPos+1);
        pathNoName = fs::path(pathSt);
        //prepend = pathSt;
      }

      searchDir = pathNoName;
      searchName = fullPath.filename().string();

      // useRelPath = true;
      fs::path relPath = fs::relative(searchDir, GetCurrentDirectory());
      prepend = BestPath(searchDir.string(), relPath.string());

      if (prepend.back() != Utilities::pathSep) {
        prepend += Utilities::pathSep;
      }

      // if (relPath.string().find("..") == std::string::npos) {
      //   // have no .. in path, so it is a full path. Add the root is there
      //   relPath = searchDir;
      // } else {
      //   prepend.clear();
      // }
      // if (relPath.empty() || relPath == ".") {
      //   useRelPath = false;
      // }
    }

    LogMessage("Searching for " + searchName + " in " + searchDir.string());

    std::string quote(1, '"');
  
    std::string name;
    size_t searchLen = searchName.length();
    for(auto const& dir_entry: fs::directory_iterator{searchDir}) {
        auto ent = dir_entry.path();
        name = ent.filename().string();
        if (lastBlank or searchLen == 0 or StartsWith(name, searchName, ignoreCase)) {
          int prefLen = searchLen;

          // check if a folder
          if (fs::is_directory(ent) and name.back() != pathSep) {
            name += pathSep;
          }
          if (prepend.length() > 0) {
            prefLen += replaceLen;
            name = prepend + name;
          }

          
          bool needQuotes = false;
          // Add quotes if spaces in name
          if (name.find(" ") != name.npos) {
            prefLen = fileSt.size();
            // std::string prefix = searchDir.string() + pathSep;
            // name = quote + prefix + name + quote;
            // name = quote + name + quote;
            needQuotes = true;
          }
          // if (useRelPath) {
          //   name = relPath.string() + pathSep + name;
          // }
#ifdef USE_CROSSLINE          
          matches.push_back({name, needQuotes});
#else          
          matches.push_back(CompletionItem(name, prefLen, 0));
#endif          
        }
    }

    return true;
  }

  std::string AbbrevPath(const std::string &path, const int maxLen)
  {
      /*
      This function has been taken from PyCmd (https://sourceforge.net/projects/pycmd/) by Horea Haitonic

      Abbreviate a full path to make it shorter, yet still unambiguous.

      This function takes a directory path and tries to abbreviate it as much as
      possible while making sure that the resulting shortened path is not
      ambiguous: a path element is only abbreviated if its shortened form is
      unique in its directory (in other words, if a sybling would have the same
      abbreviation, the original name is kept).

      The abbreviation is performed by keeping only the first letter of each
      "word" composing a path element. "Words" are defined by CamelCase,
      underscore_separation or "whitespace separation".

      Try to keep the path under 25 characters
      d:\...\GFlowGui\Tests\Awi-10>
      */

      if (path.length() < maxLen) {
          return path;
      }

/*

      // origPath = path
      drive, path = os.path.splitdrive(path);    // split c:/path into c: /path

      // Split the path into folders using os.path.split - this give the tails
      std::string st = path;
      std::vector<std::string> tails;
      int pathLen = 0;
      while (st.length() > 1) {
          head, tail = os.path.split(st);
          st = head;
          tails.append(tail);
          pathLen += len(tail);
      }
      if (len(st) > 0) {
          tails.append(st);
      }
      tails.reverse();

      pathAbbrev = drive;

      // Take first element of path
      for tail in tails[:-1] {
          if (pathLen < elemLen):
              pathAbbrev = os.path.join(pathAbbrev, tail)  # use whole path
          else:
              pathAbbrev = os.path.join(pathAbbrev, tail[0:1])  # first letter only
      }

      tail = tails[-1];
      if (len(tail) > elemLen) {
          pathAbbrev = os.path.join(pathAbbrev, '...')
          pathAbbrev += tail[-elemLen:]
      } else {
          pathAbbrev = os.path.join(pathAbbrev, tail);
        }

      return pathAbbrev;
  */

    fs::path fullPath(path);
    auto root = fullPath.root_name();

    // build the path from the back

    // find the last folder name
    int pos = path.rfind(pathSep);
    if (pos == path.npos) {
      // this shouldn't happen
      pos = 0; 
    }
    std::string res = path.substr(pos);
    pos--;

    bool useFull = true;
    while (pos > 0) {
      if (res.length() >= maxLen-4) {
        useFull = false;
        break;
      }
      int pos1 = path.rfind(pathSep, pos);
      if (pos1 == path.npos) {
        pos1 = 0;
      }
      if (useFull) {
        res = path.substr(pos1, pos-pos1) + pathSep + res;
      } else {
        res = path.substr(pos1, 1) + pathSep + res;
      }
      pos = pos1-1;
    }

    res = root.string() + pathSep + res;

    return res;

    // now we need to abbreviate path[pos:-1] - take the last maxLen chars
    // return std::string("...") + path.substr(pos);

  }


  FileLock::FileLock(const std::string &fullName)
  {
    fs::path fullPath(fullName);
    std::string fileName = fullPath.filename().string();

    fs::path configFolder(Utilities::GetConfigFolder());
    fs::path lckFilePath = configFolder / (fileName + ".lck");
    lckFile = lckFilePath.string();

    hasLock = true;
    std::chrono::high_resolution_clock clock;
    constexpr auto t20{20ms};
    auto t1 = clock.now();
    while (fs::exists(lckFile)) {
        std::this_thread::sleep_for(t20);
        std::chrono::duration<double> timeSpan 
            = std::chrono::duration_cast<std::chrono::duration<double>>(clock.now() - t1);
        if (timeSpan.count() > 1.0) {
            std::cout << "Waiting too long for file lock\n";
            hasLock = false;
            return;
        }
    }

    std::ofstream lck(lckFile);
    lck << "Locked\n";
    lck.close();
  }

  FileLock::~FileLock()
  {
    fs::remove(lckFile);
  }

  bool FileLock::HasLock() const
  {
    return hasLock;  
  }


  CmdToken::CmdToken(const std::string tok, const int st, const int qp) {
      cmd = tok;
      startPos = st;
      hasQuotes = qp >= 0;
  }

 CmdClass::CmdClass() 
 {
    cmdToks[1] = "|"; cmdToks[2] = ">";
    type = PlainCmd;
 }


  bool CmdClass::IdentifyTokens(const std::vector<CmdToken> &toks, const int tokInd)
  {
    // parse line into  cmd arg <tok> right
    std::string tok = cmdToks[tokInd];

    bool hasTok = false;
    std::vector<CmdToken> preToks;
    std::vector<CmdToken> postToks;

    for (auto it=toks.begin(); it!=toks.end(); it++) {
        if (it->cmd == tok) {
            hasTok = true;
            type = CmdType(tokInd);
        } else if (hasTok) {
            postToks.push_back(*it);
        } else {
            preToks.push_back(*it);
        }
    }

    if (tokInd > 1) {
        if (hasTok) {
            preCmd = std::make_shared<CmdClass>();
            preCmd->IdentifyTokens(preToks, tokInd-1);
            postCmd = std::make_shared<CmdClass>();
            postCmd->IdentifyTokens(postToks, tokInd);
            tokens = toks;
        } else {
            tokens = preToks;
        }
    } else {
        tokens = preToks;
    }

    return true;
  }


  void CmdClass::Print(std::ostream &out)
  {
    out << "Type: " << type << "\n";
    if (type == PlainCmd) {
        for (int i = 0; i < tokens.size(); i++) {
            out << tokens[i].cmd << " ";
        }
        out << "\n";
    } else {
        preCmd->Print(out);
        postCmd->Print(out);
    }

  }

  bool CmdClass::ParseLine(const std::string &line, const bool stripQuotes)
  {
      // parse line into tokens, returns true if the last character is a blank
      // the tokens honor quotes
      // if stripQuotes then remove quotes

      // std::stringstream ss(line);
      char delim = ' ';


      bool lastBlank = false;
      std::string whiteSpace(" \t");

      // find last non-whitespace
      size_t lastPos = line.find_last_not_of(whiteSpace);
      if (lastPos == line.npos) {
          // no non-whitespace
          return true;
      }
      lastBlank = lastPos < line.length()-1;

      bool inQuotes = false;
      std::string token;
      std::vector<CmdToken> stToks;
      size_t delimInd;
      size_t delimPos = 0;
      size_t lineLen = line.length();
      while (delimPos < lineLen) {
          delimInd = std::min(line.find(delim, delimPos), lineLen);
          token = line.substr(delimPos, delimInd-delimPos);

          size_t quotPos, lastPos;
          // if we have an open quote we need to add the token, but note that we are closing the quote
          bool inQuotes2 = inQuotes;
          lastPos = 0;
          quotPos = -1;
          int quotStart = -1;
          while ((quotPos = token.find('"', lastPos)) != std::string::npos) {
              // remove all quotes
            if (quotStart < 0) {
                quotStart = quotPos;
            }
            if (stripQuotes) {
              token.erase(quotPos, 1);
            } else {
              lastPos = 1;
            }
            lastPos += quotPos;
            inQuotes2 = !inQuotes2;
          }

          if (!inQuotes) {
              stToks.push_back(CmdToken(token, delimPos, quotStart));
          } else {
              stToks.back().cmd += delim + token;
          }

          inQuotes = inQuotes2;
          delimPos = delimInd + 1;
          // if (token.find('"') != std::string::npos) {
          //     inQuotes = !inQuotes;
          // }
      }

      // have the tokens so identify type
      IdentifyTokens(stToks, CmdClass::Redirection);

      return lastBlank;
  }


  int CmdClass::GetNoArgs() const
  {
    return tokens.size();
  }

  std::string CmdClass::GetArg(const int n) const
  {
    return tokens[n].cmd;
  }

  void CmdClass::SetArg(const int n, const std::string &c)
  {
    tokens[n].cmd = c;
  }

  const std::vector<CmdToken> &CmdClass::GetTokens() const
  {
    return tokens;
  }

  CmdToken CmdClass::LastToken() const
  {
    return tokens.back();
  }

  void CmdClass::PopBack()
  {
    tokens.pop_back();
  }

}  // end namespace


#ifdef MAIN

#include <iostream>

int main(int argc, char const *argv[])
{
  
  if (argc < 2) {
    std::cout <<  "Usage: Utilities line\n";
    return 0;
  }
  std::string line = argv[1];
  std::vector<std::string> toks;

  std::cout << "Processing " << line << "\n";

  Utilities::ParseLine(line, toks, true);

  return 0;
}

#endif