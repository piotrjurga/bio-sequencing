@echo off

set VCVARSPATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"

REM if cl command is not found, run vcvarsall
where cl >nul 2>nul
if %ERRORLEVEL% neq 0 call %VCVARSPATH%

mkdir build >nul 2>nul
cl /nologo /Zi /MT /O2 /Oi /F10485760 /openmp main.cpp
move *.obj build >nul 2>nul
move *.pdb build >nul 2>nul
move *.ilk build >nul 2>nul
