// Provide an interface to a lua processor

#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

#include "LuaInterface.h"


// -- Macros and types --
#define CrabShell_metatable "Crabshell"



int RegisterFunc(lua_State* L)
{
    if (!(lua_isstring(L, -1))) {
        std::cout << "Invalid call to RegisterFunc. Should be RegisterFunc(name)\n";
        return 0;
    }
    std::string name = lua_tostring(L, -1);
    LuaInterface *lua = LuaInterface::GetLua();
    lua->AddFunc(name);

    return 1;
}


bool AddFunction(lua_State *L, const std::string& name, int(func)(lua_State* L))
{
    lua_pushcfunction(L, func);
    lua_setglobal(L, name.c_str());
    return true;
}


LuaInterface::LuaInterface()
{

    L = luaL_newstate();
    if (!L) {
        return;
    }

    luaL_openlibs(L);

    AddFunction(L, "RegisterFunc", RegisterFunc);
}


bool LuaInterface::LoadFile(const std::string &f)
{
    // Need to use dofile to allow calls to register
    bool res = luaL_dofile(L, f.c_str()) == LUA_OK;
    if (!res) {
        // error message is on the stack
        std::cerr << "Error running " << f << "\n";
        if (lua_isstring(L, 1)) {
            const char *msg = lua_tostring(L, 1);
            std::cerr << msg << "\n";
        }
    }
    return res;
}

bool LuaInterface::RunCommand(const std::string &cmd)
{
    for (std::vector<std::string>::const_iterator it = regFuncs.begin(); it != regFuncs.end(); it++) {
      if (*it == cmd) {
          lua_getglobal(L, cmd.c_str());
          /* do the call (0 arguments, 0 result) */
    
          if (lua_pcall(L, 0, 0, 0) != 0) {
            std::cout << "Error running function: " << cmd << "\n";
            return false;
          }

          return true;
      }
    }
    return false;
}


LuaInterface *LuaInterface::GetLua()
{
    static LuaInterface instance;  // Guaranteed to be destroyed.
                                   // Instantiated on first use.
    return &instance;
}


#ifdef HAVEMAIN

int main (int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: GFlowLua luafile\n";
        return 0;
    }

    LuaInterface *lua = LuaInterface::GetLua();

    lua->LoadFile(argv[1]);
    lua->RunCommand("ListDir");
    return 0;
}

#endif
