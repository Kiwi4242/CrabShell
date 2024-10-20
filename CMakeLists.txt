# -----------------------------------------------------------------------------
# Copyright (c) 2024, John Burnell
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.10)
project(CrabShell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

set(cs_version "0.2")

option(CL_USE_CROSSLINE           "Build with Crossline" ON)

# -----------------------------------------------------------------------------
# Initial definitions
# -----------------------------------------------------------------------------
set(cs_cxxflags)
set(cs_cxxdefs)
set(cs_install_dir)
set(cl_dir)

find_path(LUA_H NAMES lua/lua.h)
find_library(LUA_LIB NAMES liblua)

if(CL_USE_CROSSLINE)
  find_path(RD_H NAMES crossline.h PATHS ../Crossline-cpp)
  find_library(RD_LIB NAMES libcrossline PATHS ../Crossline-cpp)
  set(RD_H_INCLUDE ${RD_H})
  list(APPEND cs_cxxdefs USE_CROSSLINE)
else()  
  find_path(RD_H NAMES isocline_pp.h PATHS ../../isocline-pp)
endif()

list(APPEND _sources 
            History.h
            History.cpp
            Utilities.h
            Utilities.cpp
            Config.h
            Config.cpp
            LuaInterface.cpp
    )


add_executable(CrabShell CrabShell.cpp ${_sources})
target_compile_options(CrabShell PRIVATE ${cs_cxxflags})
target_compile_definitions(CrabShell PRIVATE ${cs_cxxdefs})

#add_library(LIB_LIB STATIC IMPORTED)
#add_library(LUA_LIB STATIC IMPORTED)
target_link_libraries(CrabShell ${RD_LIB})
target_link_libraries(CrabShell ${LUA_LIB})

# set_property(TARGET LIB_LIB PROPERTY IMPORTED_LOCATION ${RD_LIB})
target_include_directories(CrabShell PUBLIC ${RD_H_INCLUDE} ${LUA_H})

