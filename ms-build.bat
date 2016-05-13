@echo off

rem
rem Make sure we have at least one argument
rem
if "%~1" == "" goto :help

rem
rem Check if help is requested
rem
if "%~1" == "help" goto help
if "%~1" == "?" goto help

rem
rem Check if Visual Studio is configured for this command prompt
rem
if "%VCINSTALLDIR%" == "" goto :novs12

rem
rem Keep all variables created in this batch file local
rem
setlocal

rem
rem If the platform is not supplied, use Win32 by default
rem
set platform=%~1

rem
rem If there was no build command supplied, use Build by default
rem
if "%~2" == "" (
set buildcmd=Build 
) else (
set buildcmd=%2
)

rem
rem msbuild reports that it canot create a log file if the log directory doesn't
rem exist, even if it is created during the build. Create the output directory 
rem for the requested platform if it doesn't exist.
rem
if not exist build\%platform%\ mkdir build\%platform%\
if not exist build\%platform%\Release\ mkdir build\%platform%\Release\

rem
rem Make a release build of the project for the specified platform
rem
rem /nr         - msbuild node reuse 
rem /fl         - use file ogger
rem /flp        - file logger property
rem /clp        - console loggger parameters
rem /noconlog   - do not log to the console window
rem
rem Console logger is configured as Quiet. Use msbuild.log in each build directory to 
rem troubleshoot build issues.
rem
echo Building Stone Steps Webalizer for %platform%
msbuild /nologo /target:%buildcmd% /property:Configuration=Release;Platform=%platform% /nr:false /fl /clp:DisableMPLogging;DisableConsoleColor;NoSummary;NoItemAndPropertyList;Verbosity=Quiet /flp:LogFile=build\%platform%\Release\msbuild-webalizer.log src\webalizer.vcxproj

rem
rem Check if msbuild generated any errors
rem 
if ERRORLEVEL 1 goto :msbuild_error

rem
rem Check if there's an executable available 
rem
if NOT EXIST "build\%platform%\Release\webalizer.exe" goto :exit

rem
rem If the executable we just built can run on this machine, run it and grab the 
rem version from the standard output. Otherwise, generate a string with a random 
rem number.
rem
set runnable=0
if "%PROCESSOR_ARCHITECTURE%" == "x86" (
   if "%platform%" == "Win32" set runnable=1
) else if  "%PROCESSOR_ARCHITECTURE%" == "x64" (
   set runnable=1
)

if %runnable% == 1 (
   for /F %%v in ('"build\%platform%\Release\webalizer" -Q -V') do set version=%%v
) else (
   set version=_v_%RANDOM%
)

rem
rem Create the directory structure for the final package
rem
if not exist build\%platform%\%version% mkdir build\%platform%\%version%
if not exist build\%platform%\%version%\lang mkdir build\%platform%\%version%\lang
if not exist build\%platform%\%version%\ofcharts mkdir build\%platform%\%version%\ofcharts

rem
rem Copy all binaries into the versioned package directory we just created
rem
echo Copying binaries to build\%platform%\%version%\
msbuild /nologo /target:%buildcmd% /property:Configuration=Release;Platform=%platform%;OutDir=..\build\%platform%\%version%\ /nr:false /fl /clp:DisableMPLogging;DisableConsoleColor;NoSummary;NoItemAndPropertyList;Verbosity=Quiet /flp:LogFile=build\%platform%\Release\msbuild-package.log src\package.vcxproj

rem
rem Check if msbuild generated any errors
rem 
if ERRORLEVEL 1 goto :msbuild_error

rem
rem Copy all files to the version directory
rem

echo Copying XSL templates
for %%f in (index.xsl usage.xsl webalizer.xsl graphs-amc.xsl graphs-amc-line.xml graphs-amc-pie.xml graphs-ofc.xsl) do xcopy /Q /Y src\%%f build\%platform%\%version%\ > nul

echo Copying Flash charts
xcopy /Q /Y /S /E ..\common\ofcharts\* build\%platform%\%version%\ofcharts > nul

echo Done

goto :exit

:help
echo.
echo Usage:
echo.
echo  ms-build help
echo  ms-build platform [build-command]
echo.
echo    platform        - Win32 or x64 (default: Win32)
echo    build-command   - Build, Rebuild, Clean (default: Build)
echo.

goto :exit

:novs12
echo.
echo Visual Studio is not configured for this command prompt window. Run 
echo the appropriate vsvars*.bat batch file for your platform from this 
echo directory prior to launching ms-build.bat.
echo.
echo %VS120COMNTOOLS%
echo. 

goto :exit

:msbuild_error
echo.
echo Failed to %buildcmd% the project for %platform%
echo.
goto :exit

:exit
