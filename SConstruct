'''
  Copyright (c) 2024, John Burnell
  This is free software; you can redistribute it and/or modify it
  under the terms of the MIT License. A copy of the license can be
  found in the "LICENSE" file at the root of this distribution.

  Scons build script
'''
import os


vars = Variables(None, ARGUMENTS)
debugVar = BoolVariable("DEBUG", "Set to 1 to build a debug release", 0)

vars.AddVariables(debugVar)

defEnv = DefaultEnvironment(variables=vars)

debug = defEnv.get('DEBUG', 0)

if (debug):
    OPT = ['-g']
else:
    OPT = ['-O2']

DEFINES = ['-std=c++17']

srcDir = '../'
cppInc = [os.path.join(srcDir, 'isocline-pp/include')]
ldLibs = [os.path.join(srcDir, 'isocline-pp/build_mingw')]
libs = ['isocline']

buildDir = 'build_mingw'

env = Environment(tools=['mingw'], CPPPATH=cppInc, CPPFLAGS=OPT)

##################################################
# Start of scons statements
VariantDir(buildDir, '.', duplicate=0)

# the programs
progs = {'CrabShell': ['CrabShell.cpp', 'History.cpp', 'Utilities.cpp', 'Config.cpp']}

srcObj = {}
for p in progs:
    obj = env.Object([os.path.join(buildDir, f) for f in progs[p]])
    env.Program(target=p, source=obj, LINKFLAGS=OPT+DEFINES, LIBPATH=ldLibs, LIBS=libs)


