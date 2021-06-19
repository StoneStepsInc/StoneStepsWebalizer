@echo off

rem
rem This script copies GD binaries from their build directories
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

rem
rem Set up source and destination directories
rem 
set GDSRC=%basedir%..\common\gd\
set GDDEST=%basedir%temp\packages\gd\

rem
rem GD
rem
for %%d in (include, ^
   bin\Win32\Debug, bin\Win32\Release, bin\x64\Debug, bin\x64\Release, ^
   lib\Win32\Debug, lib\Win32\Release, lib\x64\Debug, lib\x64\Release) do (
if NOT EXIST "%GDDEST%build\native\%%d\" mkdir "%GDDEST%build\native\%%d\"
)

xcopy %XCOPYOPTS% "%GDSRC%src\*.h" "%GDDEST%build\native\include\"

xcopy %XCOPYOPTS% "%GDSRC%windows\Debug\gd2.dll" "%GDDEST%build\native\bin\Win32\Debug\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Debug\gd2.lib" "%GDDEST%build\native\lib\Win32\Debug\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Debug\gd2.pdb" "%GDDEST%build\native\bin\Win32\Debug\"

xcopy %XCOPYOPTS% "%GDSRC%windows\Release\gd2.dll" "%GDDEST%build\native\bin\Win32\Release\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Release\gd2.lib" "%GDDEST%build\native\lib\Win32\Release\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Release\gd2.pdb" "%GDDEST%build\native\lib\Win32\Release\"

xcopy %XCOPYOPTS% "%GDSRC%windows\Debug64\gd2.dll" "%GDDEST%build\native\bin\x64\Debug\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Debug64\gd2.lib" "%GDDEST%build\native\lib\x64\Debug\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Debug64\gd2.pdb" "%GDDEST%build\native\lib\x64\Debug\"

xcopy %XCOPYOPTS% "%GDSRC%windows\Release64\gd2.dll" "%GDDEST%build\native\bin\x64\Release\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Release64\gd2.lib" "%GDDEST%build\native\lib\x64\Release\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Release64\gd2.pdb" "%GDDEST%build\native\lib\x64\Release\"
