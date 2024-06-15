'''
  Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  Scons build script
'''
import os

tools = {'win32': 'mingw', 'posix': 'default'}
buildDirs = {'win32': 'build_mingw', 'posix': 'build_linux'}

vars = Variables(None, ARGUMENTS)
debugVar = BoolVariable("DEBUG", "Set to 1 to build a debug release", 0)

vars.AddVariables(debugVar)

defEnv = DefaultEnvironment(variables=vars)

debug = defEnv.get('DEBUG', 0)

platform = defEnv['PLATFORM']


if (debug):
    OPT = ['-g']
else:
    OPT = ['-O2']

DEFINES = ['-std=c++17']

srcDir = '../'
cppInc = [os.path.join(srcDir, 'isocline-pp/include')]
ldLibs = [os.path.join(srcDir, 'isocline-pp/build_mingw')]
libs = ['isocline']

if (platform == "win32"):
    libs += ['shell32', 'kernel32', 'user32', 'shlwapi', 'ole32', 'uuid']

buildDir = buildDirs[platform]

env = Environment(tools=[tools[platform]], CPPPATH=cppInc, CPPFLAGS=OPT+DEFINES)


##################################################
# Start of scons statements
VariantDir(buildDir, '.', duplicate=0)

# the programs
progs = {'CrabShell': ['CrabShell.cpp', 'History.cpp', 'Utilities.cpp', 'Config.cpp']}

srcObj = {}
for p in progs:
    obj = env.Object([os.path.join(buildDir, f) for f in progs[p]])
    env.Program(target=p, source=obj, LINKFLAGS=OPT+DEFINES, LIBPATH=ldLibs, LIBS=libs)


