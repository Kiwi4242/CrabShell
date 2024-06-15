// lua interface to FPlot
#pragma once

#include <string>
#include <vector>


class lua_State;

// Singleton class for connecting with Lua

class LuaInterface {
protected:        
    lua_State *L;
    std::vector<std::string> regFuncs;
public:
    LuaInterface();
    bool LoadFile(const std::string &f);
    bool RunCommand(const std::string &cmd);   // , const std::vector<std::string> &args);

   static LuaInterface *GetLua();

   void AddFunc(const std::string &f) {
    regFuncs.push_back(f);
   }
};

