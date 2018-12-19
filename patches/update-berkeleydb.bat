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

set BDBSRC=%basedir%..\common\db\build_windows\
set BDBDEST=%basedir%temp\packages\berkeleydb\build\native\

rem
rem Make sure all directories exist (or xcopy gets confused whether it's a file or a directory)
rem
for %%d in (include, ^
   bin\Win32\Debug, bin\Win32\Release, bin\x64\Debug, bin\x64\Release, ^
   lib\Win32\Debug, lib\Win32\Release, lib\x64\Debug, lib\x64\Release) do (
	if NOT EXIST "%BDBDEST%%%~d" mkdir "%BDBDEST%%%~d\\"
)

rem
rem Delete all source, binaries and symbols from the package tree
rem
FOR %%f in (h dll lib pdb) do del /Q /S "%BDBDEST%*.%%~f"

rem
rem Copy all include files
rem
xcopy %XCOPYOPTS% "%BDBSRC%*.h" "%BDBDEST%include"

rem
rem Copy libraries
rem
xcopy %XCOPYOPTS% "%BDBSRC%Win32\Debug\libdb181d.lib" "%BDBDEST%lib\Win32\Debug"
xcopy %XCOPYOPTS% "%BDBSRC%Win32\Release\libdb181.lib" "%BDBDEST%lib\Win32\Release"
xcopy %XCOPYOPTS% "%BDBSRC%\x64\Debug\libdb181d.lib" "%BDBDEST%lib\x64\Debug"
xcopy %XCOPYOPTS% "%BDBSRC%\x64\Release\libdb181.lib" "%BDBDEST%lib\x64\Release"

rem
rem Copy DLLs and debug symbols
rem
xcopy %XCOPYOPTS% "%BDBSRC%Win32\Debug\libdb181d.dll" "%BDBDEST%bin\Win32\Debug"
xcopy %XCOPYOPTS% "%BDBSRC%Win32\Debug\libdb181d.pdb" "%BDBDEST%bin\Win32\Debug"

xcopy %XCOPYOPTS% "%BDBSRC%Win32\Release\libdb181.dll" "%BDBDEST%bin\Win32\Release"
xcopy %XCOPYOPTS% "%BDBSRC%Win32\Release\libdb181.pdb" "%BDBDEST%bin\Win32\Release"

xcopy %XCOPYOPTS% "%BDBSRC%x64\Debug\libdb181d.dll" "%BDBDEST%bin\x64\Debug"
xcopy %XCOPYOPTS% "%BDBSRC%x64\Debug\libdb181d.pdb" "%BDBDEST%bin\x64\Debug"

xcopy %XCOPYOPTS% "%BDBSRC%x64\Release\libdb181.dll" "%BDBDEST%bin\x64\Release"
xcopy %XCOPYOPTS% "%BDBSRC%x64\Release\libdb181.pdb" "%BDBDEST%bin\x64\Release"
