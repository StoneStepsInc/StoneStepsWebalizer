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
robocopy /NC /NS /NFL /NDL /NJS /NJH /NP /PURGE %dir%..\..\common\db\.hg\patches\ %dir%db\ *.patch series

echo Copying FreeType...
robocopy /NC /NS /NFL /NDL /NJS /NJH /NP /PURGE %dir%..\..\common\freetype\.hg\patches\ %dir%freetype\ *.patch series

echo Copying GD...
robocopy /NC /NS /NFL /NDL /NJS /NJH /NP /PURGE %dir%..\..\common\gd\.hg\patches\ %dir%gd\ *.patch series

echo Copying PNG...
robocopy /NC /NS /NFL /NDL /NJS /NJH /NP /PURGE %dir%..\..\common\libpng\.hg\patches\ %dir%libpng\ *.patch series

echo Copying MaxMindDB...
robocopy /NC /NS /NFL /NDL /NJS /NJH /NP /PURGE %dir%..\..\common\GeoIP\.hg\patches\ %dir%maxminddb\ *.patch series

echo Copying ZLIB...
robocopy /NC /NS /NFL /NDL /NJS /NJH /NP /PURGE %dir%..\..\common\zlib\.hg\patches\ %dir%zlib\ *.patch series

echo.
