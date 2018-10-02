@echo off
echo.
echo Building program..
echo.
cd build
cmake -G "MinGW Makefiles" ../src

echo.
echo Compiling program..
echo.
mingw32-make
echo.
echo Starting program..
echo.
cd ..
"build\testproject.exe"
