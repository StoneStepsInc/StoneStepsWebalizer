@echo off

rem
rem This script copies MaxMind DB binaries from their build directories
rem into Nuget package staging directories.
rem

rem
rem Keep all variables created in this batch file local
rem
setlocal

rem
rem Set up a base directory based on the script location
rem
set basedir=%~d0%~p0..\

rem
rem xcopy arguments
rem
set "XCOPYOPTS=/Y"

set MAXMINDDBSRC=%basedir%..\common\GeoIP\
set MAXMINDDBDEST=%basedir%temp\packages\maxminddb\build\native\

rem
rem Make sure all directories exist (or xcopy gets confused whether it's a file or a directory)
rem
for %%d in (include, ^
    lib\Win32\Debug, lib\Win32\Release, lib\x64\Debug, lib\x64\Release) do (
	if NOT EXIST "%MAXMINDDBDEST%%%~d" mkdir "%MAXMINDDBDEST%%%~d"
)

rem
rem Delete all source, binaries and symbols from the package tree
rem
FOR %%f in (h lib pdb) do del /Q /S "%MAXMINDDBDEST%*.%%~f"

rem
rem Copy all include files
rem
xcopy %XCOPYOPTS% "%MAXMINDDBSRC%include\*.h" "%MAXMINDDBDEST%include"

rem
rem Copy libraries and compile-time debug symbols
rem
xcopy %XCOPYOPTS% "%MAXMINDDBSRC%projects\VS2017\Debug\libmaxminddbd.lib" "%MAXMINDDBDEST%lib\Win32\Debug"
xcopy %XCOPYOPTS% "%MAXMINDDBSRC%projects\VS2017\Debug\libmaxminddbd.pdb" "%MAXMINDDBDEST%lib\Win32\Debug"

xcopy %XCOPYOPTS% "%MAXMINDDBSRC%projects\VS2017\Release\libmaxminddb.lib" "%MAXMINDDBDEST%lib\Win32\Release"
xcopy %XCOPYOPTS% "%MAXMINDDBSRC%projects\VS2017\Release\libmaxminddb.pdb" "%MAXMINDDBDEST%lib\Win32\Release"

xcopy %XCOPYOPTS% "%MAXMINDDBSRC%projects\VS2017\x64\Debug\libmaxminddbd.lib" "%MAXMINDDBDEST%lib\x64\Debug"
xcopy %XCOPYOPTS% "%MAXMINDDBSRC%projects\VS2017\x64\Debug\libmaxminddbd.pdb" "%MAXMINDDBDEST%lib\x64\Debug"

xcopy %XCOPYOPTS% "%MAXMINDDBSRC%projects\VS2017\x64\Release\libmaxminddb.lib" "%MAXMINDDBDEST%lib\x64\Release"
xcopy %XCOPYOPTS% "%MAXMINDDBSRC%projects\VS2017\x64\Release\libmaxminddb.pdb" "%MAXMINDDBDEST%lib\x64\Release"
