# CrabShell

This is a hobby project to develop a cross-platform shell with some of the useful properties of the fish shell on Windows.

I have been using a custom version of PyCmd for the last few years, but wanted to improve some of the completion options.

This is a very first alpha version - it compiles and works on my system. It uses a custom version of crossline, which I have forked (https://github.com/Kiwi4242/Crossline-cpp).

![](https://github.com/Kiwi4242/CrabShell/blob/main/Install/ScreenShot.gif)

## Releases
You can find alpha releases under "Releases"

## Compiling

You need a recent c++ compiler, CMake and the lua library.

First you need to compile crossline-cpp using CMake.

To compile CrabShell using CMake you need to provide the locations to the crossline-cpp library and lua. 
