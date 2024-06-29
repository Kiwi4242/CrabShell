# CrabShell

This is a hobby project to develop a cross-platform shell with some of the useful properties of the fish shell on Windows.

I have been using a custom version of PyCmd for the last few years, but wanted to improve some of the completion options.

This is a very first alpha version - it compiles and works on my system. It uses a custom version of isocline, which I have forked (https://github.com/Kiwi4242/isocline-pp).

![](https://github.com/Kiwi4242/CrabShell/blob/main/Install/ScreenShot.gif)

## Releases
You can find alpha releases under "Releases"

## Compiling

Yu need a recent c++ compiler.

First you need to compile isocline_pp (needs CMake).

To compile CrabShell you need the scons build system. Edit the SConstruct file and change the location to the isocline_pp folder. 
