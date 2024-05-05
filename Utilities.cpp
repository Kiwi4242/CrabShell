/* Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  Utilities.cpp
  Various utility functions
*/


#include <fstream>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

#include <isocline_pp.h>
#include "Utilities.h"
 
namespace Utilities {


#ifdef WIN32  
  static const bool isWindows = true;
#else
  static const bool isWindows = false;
#endif


  class LogClass {
  protected:
    std::ofstream out;
    bool doLog;

  public:
      static LogClass *GetLog() {
        static LogClass instance;  // Guaranteed to be destroyed.
                                   // Instantiated on first use.
        return &instance;
      }

  private:
      LogClass() 
      {
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

  };


  void SetupLogging(const bool doLog) 
  {
    LogClass *log = LogClass::GetLog();
    log->SetupLogging(doLog);
  }

  void LogMessage(const std::string &msg) {
    LogClass::GetLog()->LogMessage(msg);
  }


  std::string GetConfigFolder()
  {
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
    std::string home = GetEnvVar("HOME");
    std::string pathSep = std::string(1, Utilities::pathSep);
    if (home.length() == 0) {
      if (isWindows) {
        // HOMEDRIVE and HOMEPATH
        std::string homeDir = GetEnvVar("HOMEDRIVE");
        std::string homePath = GetEnvVar("HOMEPATH");
        if (homeDir.length() > 0) {
          if (homePath.length() > 0) {
            return homeDir + homePath;
          }
          return homeDir;
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


  bool StripString(std::string &cmd)
  /* remove white space at start of cmd */
  {
    int pos = cmd.find_first_not_of(" \t", 0);
    if (pos != cmd.npos && pos != 0) {
      cmd.erase(0, pos);
    }
    return true;
  }


  bool FixupPath(std::string &path)
  {
    /* remove quotes from start and end
       change any / to \
       */
    // remove blanks at start of line
    StripString(path);

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


  void SetCurrentDirectory(const std::string &d)
  {
    fs::path dirPath(d);
    fs::current_path(dirPath);
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



  bool ParseLine(const std::string &line, std::vector<std::string> &tokens, const bool stripQuotes)
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


  bool GetFileMatches(const std::string &line, std::vector<CompletionItem> &matches)
  {

    std::vector<std::string> args;
    bool lastBlank = ParseLine(line, args, true);

    matches.clear();

    std::string fileSt = args.back();
    if (lastBlank > 0) {
      args.pop_back();
    }

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

    // first remove the last entry - do we have anything left
    fs::path fullPath(fileSt);
    // remove filename
    fs::path pathNoName = fullPath.parent_path();
    fs::path fName = fullPath.filename();

    std::string prepend;
    int replaceLen = 0;
    if (pathNoName.empty()) {
      // just a file name, so search current dir
      searchDir = fs::current_path();
      searchName = fileSt;
    } else {
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
        pathSt = pathSt.substr(0, tdPos) + home + pathSt.substr(tdPos+1);
        pathNoName = fs::path(pathSt);
        prepend = pathSt;
      }
      searchDir = pathNoName;
      searchName = fullPath.filename().string();
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

          // Add quotes if spaces in name
          if (name.find(" ") != name.npos) {
            name = quote + name + quote;
          }
          // check if a folder
          if (fs::is_directory(ent) and name.back() != pathSep) {
            name += pathSep;
          }
          if (prepend.length() > 0) {
            prefLen += replaceLen;
            name = prepend + name;
          }
          matches.push_back(CompletionItem(name, prefLen, 0));
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

}


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