// lua interface to FPlot
#pragma once

#include <string>
#include <vector>
#include <memory>

class lua_State;
class ShellDataClass;

class LuaInterface {
protected:        
    lua_State *L;
    std::vector<std::string> plugins;

public:
    LuaInterface(ShellDataClass *sh);
    bool LoadFile(const std::string &f);
    bool RunCommand(const std::vector<std::string> &args); 

    bool LoadPlugins();

    ShellDataClass *shell;

    // void AddFunc(const std::string &f) {
    //     regFuncs.push_back(f);
    // }
};

