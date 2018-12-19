@echo off

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

set ZLIBSRC=%basedir%..\common\zlib\
set ZLIBDEST=%basedir%temp\packages\zlib\build\native\

rem
rem Make sure all directories exist (or xcopy gets confused whether it's a file or a directory)
rem
for %%d in (include, ^
   lib\Win32\Debug, lib\Win32\Release, lib\x64\Debug, lib\x64\Release) do (
	if NOT EXIST "%ZLIBDEST%%%~d" mkdir "%ZLIBDEST%%%~d"
)

rem
rem Delete all source, binaries and symbols from the package tree
rem
FOR %%f in (h lib pdb) do del /Q /S "%ZLIBDEST%*.%%~f"

rem
rem Copy all include files
rem
xcopy %XCOPYOPTS% "%ZLIBSRC%*.h" "%ZLIBDEST%include"

rem
rem Copy libraries and compile-time debug symbols
rem
xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\VS2017\x86\ZlibStatDebug\zlib12.lib" "%ZLIBDEST%lib\Win32\Debug"
xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\VS2017\x86\ZlibStatDebug\zlib12.pdb" "%ZLIBDEST%lib\Win32\Debug"

xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\VS2017\x86\ZlibStatReleaseWithoutAsm\zlib12.lib" "%ZLIBDEST%lib\Win32\Release"
xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\VS2017\x86\ZlibStatReleaseWithoutAsm\zlib12.pdb" "%ZLIBDEST%lib\Win32\Release"

xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\VS2017\x64\ZlibStatDebug\zlib12.lib" "%ZLIBDEST%lib\x64\Debug"
xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\VS2017\x64\ZlibStatDebug\zlib12.pdb" "%ZLIBDEST%lib\x64\Debug"

xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\VS2017\x64\ZlibStatReleaseWithoutAsm\zlib12.lib" "%ZLIBDEST%lib\x64\Release"
xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\VS2017\x64\ZlibStatReleaseWithoutAsm\zlib12.pdb" "%ZLIBDEST%lib\x64\Release"
