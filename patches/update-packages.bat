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

rem
rem Set up source and destination directories
rem 
set BDBSRC=%basedir%..\common\db\
set GDSRC=%basedir%..\common\gd\
set GEOIPSRC=%basedir%..\common\GeoIP\
set ZLIBSRC=%basedir%..\common\zlib\

set BDBDEST=%basedir%temp\packages\berkeleydb\
set GDDEST=%basedir%temp\packages\gd\
set GEOIPDEST=%basedir%temp\packages\maxminddb\
set ZLIBDEST=%basedir%temp\packages\zlib\

rem
rem Berkeley DB
rem
for %%d in (include, ^
   bin\Win32\Debug, bin\Win32\Release, bin\x64\Debug, bin\x64\Release, ^
   lib\Win32\Debug, lib\Win32\Release, lib\x64\Debug, lib\x64\Release) do (
if NOT EXIST "%BDBDEST%build\native\%%d\" mkdir "%BDBDEST%build\native\%%d\"
)

xcopy %XCOPYOPTS% "%BDBSRC%build_windows\*.h" "%BDBDEST%build\native\include\"

xcopy %XCOPYOPTS% "%BDBSRC%build_windows\Win32\Debug\libdb60d.dll" "%BDBDEST%build\native\bin\Win32\Debug\"
xcopy %XCOPYOPTS% "%BDBSRC%build_windows\Win32\Release\libdb60.dll" "%BDBDEST%build\native\bin\Win32\Release\"
xcopy %XCOPYOPTS% "%BDBSRC%build_windows\x64\Debug\libdb60d.dll" "%BDBDEST%build\native\bin\x64\Debug\"
xcopy %XCOPYOPTS% "%BDBSRC%build_windows\x64\Release\libdb60.dll" "%BDBDEST%build\native\bin\x64\Release\"

xcopy %XCOPYOPTS% "%BDBSRC%build_windows\Win32\Debug\libdb60d.lib" "%BDBDEST%build\native\lib\Win32\Debug\"
xcopy %XCOPYOPTS% "%BDBSRC%build_windows\Win32\Release\libdb60.lib" "%BDBDEST%build\native\lib\Win32\Release\"
xcopy %XCOPYOPTS% "%BDBSRC%build_windows\x64\Debug\libdb60d.lib" "%BDBDEST%build\native\lib\x64\Debug\"
xcopy %XCOPYOPTS% "%BDBSRC%build_windows\x64\Release\libdb60.lib" "%BDBDEST%build\native\lib\x64\Release\"

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
xcopy %XCOPYOPTS% "%GDSRC%windows\Release\gd2.dll" "%GDDEST%build\native\bin\Win32\Release\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Debug64\gd2.dll" "%GDDEST%build\native\bin\x64\Debug\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Release64\gd2.dll" "%GDDEST%build\native\bin\x64\Release\"

xcopy %XCOPYOPTS% "%GDSRC%windows\Debug\gd2.lib" "%GDDEST%build\native\lib\Win32\Debug\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Release\gd2.lib" "%GDDEST%build\native\lib\Win32\Release\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Debug64\gd2.lib" "%GDDEST%build\native\lib\x64\Debug\"
xcopy %XCOPYOPTS% "%GDSRC%windows\Release64\gd2.lib" "%GDDEST%build\native\lib\x64\Release\"

rem
rem MaxMind DB
rem
for %%d in (include, ^
   lib\Win32\Debug, lib\Win32\Release, lib\x64\Debug, lib\x64\Release) do (
if NOT EXIST "%GEOIPDEST%build\native\%%d\" mkdir "%GEOIPDEST%build\native\%%d\"
)

xcopy %XCOPYOPTS% "%GEOIPSRC%include\*.h" "%GEOIPDEST%build\native\include\"

xcopy %XCOPYOPTS% "%GEOIPSRC%projects\VS14\Debug\libmaxminddbd.lib" "%GEOIPDEST%build\native\lib\Win32\Debug\"
xcopy %XCOPYOPTS% "%GEOIPSRC%projects\VS14\Release\libmaxminddb.lib" "%GEOIPDEST%build\native\lib\Win32\Release\"
xcopy %XCOPYOPTS% "%GEOIPSRC%projects\VS14\x64\Debug\libmaxminddbd.lib" "%GEOIPDEST%build\native\lib\x64\Debug\"
xcopy %XCOPYOPTS% "%GEOIPSRC%projects\VS14\x64\Release\libmaxminddb.lib" "%GEOIPDEST%build\native\lib\x64\Release\"

rem
rem ZLib
rem
for %%d in (include, ^
   lib\Win32\Debug, lib\Win32\Release, lib\x64\Debug, lib\x64\Release) do (
if NOT EXIST "%ZLIBDEST%build\native\%%d\" mkdir "%ZLIBDEST%build\native\%%d\"
)

xcopy %XCOPYOPTS% "%ZLIBSRC%*.h" "%ZLIBDEST%build\native\include\"

xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\vc14\x86\ZlibDebugStatic\zlib12.lib" "%ZLIBDEST%build\native\lib\Win32\Debug\"
xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\vc14\x86\ZlibReleaseStatic\zlib12.lib" "%ZLIBDEST%build\native\lib\Win32\Release\"
xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\vc14\x64\ZlibDebugStatic\zlib12.lib" "%ZLIBDEST%build\native\lib\x64\Debug\"
xcopy %XCOPYOPTS% "%ZLIBSRC%contrib\vstudio\vc14\x64\ZlibReleaseStatic\zlib12.lib" "%ZLIBDEST%build\native\lib\x64\Release\"

