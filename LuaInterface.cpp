/* ----------------------------------------------------------------------------
  Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  LuaInterface.cpp
  Provide an interface to a lua processor

  Provides an API for:

  Updating the prompt
  Processing file completions
  Processing the history
-----------------------------------------------------------------------------*/


#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <exception>

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

#include "LuaInterface.h"
#include "ShellData.h"
#include "Utilities.h"


// -- Macros and types --
#define CrabShell_metatable "Crabshell"


LuaInterface **GetLuaInterface(lua_State *L)
{
    lua_getglobal(L, "ShellInterface");
    if (lua_isuserdata(L, -1)) {
        LuaInterface **lua = static_cast<LuaInterface**>(lua_touserdata(L, -1));
        // pop the lua interface
        lua_pop(L, 1);
        return lua;
    }

    return nullptr;
}

int RegisterFunc(lua_State* L)
{
    if (!(lua_isstring(L, -1))) {
        std::cerr << "Invalid call to RegisterFunc. Should be RegisterFunc(name)\n";
        return 0;
    }
    std::string name = lua_tostring(L, -1);

    auto ty = lua_getglobal(L, "ShellInterface");
    if (lua_isuserdata(L, -1)) {
        LuaInterface **lua = static_cast<LuaInterface**>(lua_touserdata(L, -1));
        // (*lua)->AddFunc(name);
        lua_pop(L, 1);
    }

    return 1;
}

int RegisterHook(lua_State* L)
{
    if (!(lua_isstring(L, -1))) {
        std::cerr << "Invalid call to RegisterFunc. Should be RegisterFunc(name)\n";
        return 0;
    }
    std::string hook = lua_tostring(L, -2);
    std::string func = lua_tostring(L, -1);

    LuaInterface **lua = GetLuaInterface(L);
    if (lua) {
        (*lua)->AddHook(hook, func);
    }
    return 1;
}


int ParseString(lua_State* L)
{
    // Parse the string at the top of the stack and return array of components
    if (!(lua_isstring(L, -1))) {
        std::cout << "Invalid call to ParseString. Should be ParseString(line)\n";
        return 0;
    }
    std::string line = lua_tostring(L, -1);

    Utilities::CmdClass cmds;
    cmds.ParseLine(line, false);

    const std::vector<Utilities::CmdToken> &words = cmds.GetTokens();

    lua_newtable(L);
    for(int i = 0; i < words.size(); i++) {
        lua_pushinteger(L, i+1);
        lua_pushstring(L, words[i].cmd.c_str());
        lua_settable(L, -3);    // table is index -3
    }

    // table is at the top of the stack    
    return 1;
}

int AddAlias(lua_State* L)
{
    if (!(lua_isstring(L, -1)) and !(lua_isstring(L, -2))) {
        std::cout << "Invalid call to AddAlias. Should be AddAlias(alias, cmd)\n";
        return 0;
    }
    std::string name = lua_tostring(L, -2);
    std::string cmd = lua_tostring(L, -1);

    LuaInterface **lua = GetLuaInterface(L);
    if (lua) {
        (*lua)->shell->AddAlias(name, cmd);
    }

    return 1;
}


int SetVar(lua_State* L)
{
    // Set a controlling Crabshell variable
    if (!lua_isstring(L, -1) || !lua_isstring(L, -2)) {
        std::cout << "Invalid call to SetVar. Should be SetVar(var, val)\n";
        return 0;
    }
    std::string var = lua_tostring(L, -2);
    std::string val = lua_tostring(L, -1);

    LuaInterface **lua = GetLuaInterface(L);
    if (lua) {
//        (*lua)->shell->SetVariable(var, val);
    }

    return 1;
}

int DoCD(lua_State* L)
{
    if (!(lua_isstring(L, -1))) {
        std::cout << "Invalid call to DoCD. Should be DoDC(dir)\n";
        return 0;
    }
    std::string dir = lua_tostring(L, -1);

    LuaInterface **lua = GetLuaInterface(L);
    if (lua) {
        (*lua)->shell->DoCD(dir);
    }

    return 1;
}


bool AddFunction(lua_State *L, const std::string& name, int(func)(lua_State* L))
{
    lua_pushcfunction(L, func);
    lua_setglobal(L, name.c_str());
    return true;
}


LuaInterface::LuaInterface(ShellDataClass *sh) : shell(sh)
{

    L = luaL_newstate();
    if (!L) {
        return;
    }

    luaL_openlibs(L);

    // AddFunction(L, "RegisterFunc", RegisterFunc);
    AddFunction(L, "RegisterHook", RegisterHook);
    AddFunction(L, "ParseString", ParseString);
    AddFunction(L, "AddAlias", AddAlias);
    AddFunction(L, "DoCD", DoCD);
    AddFunction(L, "SetVar", SetVar);

    int sz = sizeof(this);
    LuaInterface **luaPtr = reinterpret_cast<LuaInterface**>(lua_newuserdata(L, sz));
    (*luaPtr) = this;
    lua_setglobal(L, "ShellInterface");
}


void LuaInterface::AddHook(const std::string &hook, const std::string &func)
{
    hooks[hook] = func;
}

bool LuaInterface::LoadFile(const std::string &f)
{
    // Need to use dofile to allow calls to register
    bool res = luaL_dofile(L, f.c_str()) == LUA_OK;
    if (!res) {
        // error message is on the stack
        std::cerr << "Error running " << f << "\n";
        if (lua_isstring(L, -1)) {
            const char *msg = lua_tostring(L, -1);
            std::cerr << msg << "\n";
        }
    }
    return res;
}

bool LuaInterface::RunCommand(const std::vector<std::string> &args)
{
    std::string cmd = args[0];

    for (std::vector<std::string>::const_iterator it = plugins.begin(); it != plugins.end(); it++) {
      if (*it == cmd) {

          // combine the arguments
          std::string combArgs;
          for (int i = 1; i < args.size(); i++) {
            combArgs += args[i];
            if (i < args.size()-1) {
                combArgs += " ";
            }
          }

          lua_getglobal(L, cmd.c_str());

          /* Run the plugin's run function providing it with the text. */
          lua_getfield(L, -1, "run");
          lua_pushstring(L, combArgs.c_str());
          if (lua_pcall(L, 1, 0, 0) != 0) {
            std::cout << "Error running function: " << cmd << "\n";
            return false;
          }
          return true;
      }
    }
    return false;
}

bool LuaInterface::LoadPlugins()
{

    try {

        fs::path configFolder(Utilities::GetConfigFolder());
        fs::path pluginDir = (configFolder / "Plugins");
        if (not fs::exists(pluginDir)) {
            return false;
        }
        
        for(auto const& dir_entry: fs::directory_iterator{pluginDir}) {
            auto name = dir_entry.path().string();

            if (name.substr(name.size() - 4, 4) == ".lua") {

                /* Load the plugin. */
                if (luaL_dofile(L, name.c_str())) {
                    std::cerr << "Could not load plugin " << name <<  "\n";
                    continue;
                }

                /* Get and check the plugin has a name*/
                lua_getfield(L, -1, "name");
                if (lua_isnil(L, -1)) {
                    std::cerr << "Plugin " << name << " missing\n";
                    lua_pop(L, 2);
                    continue;
                }
                std::string pName = lua_tostring(L, -1);
                lua_pop(L, 1);

                /* Set the loaded plugin to a global using it's name. */
                lua_setglobal(L, pName.c_str());

                plugins.push_back(pName);

            }
        }
        return true;
    } catch (std::exception &exc) {
        std::cerr << "Error loading plugin folder " << exc.what() << "\n";
        return false;
    }
}


#ifdef HAVEMAIN

int main (int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: GFlowLua luafile\n";
        return 0;
    }

    std::shared_ptr<ShellDataClass> shell = std::make_shared<ShellDataClass>();

    LuaInterface lua(shell);    // = LuaInterface::GetLua();

    lua.LoadFile(argv[1]);
    lua.RunCommand("ListDir");

    return 0;
}

#endif
