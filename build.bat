@echo off
cd build
cmake -G "MinGW Makefiles" ../src
mingw32-make
cd ..
