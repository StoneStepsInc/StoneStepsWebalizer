@echo off

rem
rem Keep all variables created in this batch file local
rem
setlocal

rem
rem Set up a packages directory relative to the script location
rem
set pkgdir=%~d0%~p0..\temp\packages\

rem
rem Create Nuget packages for all 3rd-party libraries
rem
nuget pack %pkgdir%berkeleydb\StoneStepsWebalizer.OracleBerkeleyDB.nuspec -OutputDirectory %pkgdir%
nuget pack %pkgdir%gd\StoneStepsWebalizer.GD.nuspec -OutputDirectory %pkgdir%
nuget pack %pkgdir%maxminddb\StoneStepsWebalizer.MaxMindDB.nuspec -OutputDirectory %pkgdir%
nuget pack %pkgdir%zlib\StoneStepsWebalizer.ZLib.nuspec -OutputDirectory %pkgdir%
