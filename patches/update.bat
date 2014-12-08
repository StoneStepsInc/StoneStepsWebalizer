@echo off

rem
rem grab the script location, so we can run the script from anywhere
rem
set dir=%~d0%~p0

rem 
rem Copy all patch files and the series file
rem

echo.

echo Copying Berkeley DB...
xcopy /Y /Q %dir%..\..\common\db\.hg\patches\*.patch %dir%db\ > nul
xcopy /Y /Q %dir%..\..\common\db\.hg\patches\series %dir%db\ > nul

echo Copying FreeType...
xcopy /Y /Q %dir%..\..\common\freetype\.hg\patches\*.patch %dir%freetype\ > nul
xcopy /Y /Q %dir%..\..\common\freetype\.hg\patches\series %dir%freetype\ > nul

echo Copying GD...
xcopy /Y /Q %dir%..\..\common\gd\.hg\patches\*.patch %dir%gd\ > nul
xcopy /Y /Q %dir%..\..\common\gd\.hg\patches\series %dir%gd\ > nul

echo Copying PNG...
xcopy /Y /Q %dir%..\..\common\libpng\.hg\patches\*.patch %dir%libpng\ > nul
xcopy /Y /Q %dir%..\..\common\libpng\.hg\patches\series %dir%libpng\ > nul

echo Copying MaxMindDB...
xcopy /Y /Q %dir%..\..\common\GeoIP\.hg\patches\*.patch %dir%maxminddb\ > nul
xcopy /Y /Q %dir%..\..\common\GeoIP\.hg\patches\series %dir%maxminddb\ > nul

echo Copying ZLIB...
xcopy /Y /Q %dir%..\..\common\zlib\.hg\patches\*.patch %dir%zlib\ > nul
xcopy /Y /Q %dir%..\..\common\zlib\.hg\patches\series %dir%zlib\ > nul

echo.
