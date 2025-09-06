@echo off
echo Creating ICO file from PNG...

REM This batch file can be used to convert PNG to ICO using ImageMagick if available
REM magick convert "..\assets\img\Glint3DIcon.png" -resize 32x32 glint3d.ico

echo Please manually convert engine/assets/img/Glint3DIcon.png to engine/resources/glint3d.ico
echo You can use online converters like https://convertio.co/png-ico/
echo Or install ImageMagick and run: magick convert ../assets/img/Glint3DIcon.png -resize 32x32 glint3d.ico

pause