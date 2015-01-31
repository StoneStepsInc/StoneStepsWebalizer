@echo off

rem
rem grab the script location, so we can run the script from anywhere
rem
set dir=%~d0%~p0

rem
rem Set up robocopy options to mirror patches
rem
set options=/DCOPY:T /NS /NC /NFL /NDL /NJS /NJH /NP /PURGE 

rem 
rem Copy all patch files and the series file
rem

echo.

echo Copying Berkeley DB...
robocopy %options% "%dir%..\..\common\db\.hg\patches" "%dir%db" *.patch series 

echo Copying FreeType...
robocopy %options% "%dir%..\..\common\freetype\.hg\patches" "%dir%freetype" *.patch series %options%

echo Copying GD...
robocopy %options% "%dir%..\..\common\gd\.hg\patches" "%dir%gd" *.patch series %options%

echo Copying PNG...
robocopy %options% "%dir%..\..\common\libpng\.hg\patches" "%dir%libpng" *.patch series %options%

echo Copying MaxMindDB...
robocopy %options% "%dir%..\..\common\GeoIP\.hg\patches" "%dir%maxminddb" *.patch series %options%

echo Copying ZLIB...
robocopy %options% "%dir%..\..\common\zlib\.hg\patches" "%dir%zlib" *.patch series %options%

echo.
