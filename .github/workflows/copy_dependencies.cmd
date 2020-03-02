@echo off

if [%1]==[] goto usage
if [%2]==[] goto usage

md %2\bin
copy %1\bin\FLAC.* %2\bin
copy %1\bin\dumb.* %2\bin
copy %1\bin\FLAC.* %2\bin
copy %1\bin\glib-2.* %2\bin
copy %1\bin\libcharset.* %2\bin
copy %1\bin\libfluidsynth-2.* %2\bin
copy %1\bin\libiconv.* %2\bin
copy %1\bin\libintl.* %2\bin
copy %1\bin\libmpg123.* %2\bin
copy %1\bin\libpng16.* %2\bin
copy %1\bin\modplug.* %2\bin
copy %1\bin\ogg.* %2\bin
copy %1\bin\opus.* %2\bin
copy %1\bin\pcre.* %2\bin
copy %1\bin\pcreposix.* %2\bin
copy %1\bin\portmidi.* %2\bin
copy %1\bin\SDL2*.* %2\bin
copy %1\bin\vorbis*.* %2\bin
copy %1\bin\zlib*.* %2\bin

md %2\include
xcopy /s /i %1\include\FLAC %2\include\FLAC
xcopy /s /i %1\include\fluidsynth %2\include\fluidsynth
xcopy /s /i %1\include\libmodplug %2\include\libmodplug
xcopy /s /i %1\include\libpng16 %2\include\libpng16
xcopy /s /i %1\include\ogg %2\include\ogg
xcopy /s /i %1\include\opus %2\include\opus
xcopy /s /i %1\include\SDL2 %2\include\SDL2
xcopy /s /i %1\include\vorbis %2\include\vorbis
copy %1\include\dumb.h %2\include
copy %1\include\fluidsynth.h %2\include
copy %1\include\fmt123.h %2\include
copy %1\include\mad.h %2\include
copy %1\include\mpg123.h %2\include
copy %1\include\pcre.h %2\include
copy %1\include\pcreposix.h %2\include
copy %1\include\png.h %2\include
copy %1\include\pngconf.h %2\include
copy %1\include\pnglibconf.h %2\include
copy %1\include\portmidi.h %2\include
copy %1\include\porttime.h %2\include
copy %1\include\zconf.h %2\include
copy %1\include\zlib.h %2\include

md %2\lib
copy %1\lib\dumb.lib %2\lib
copy %1\lib\FLAC.lib %2\lib
copy %1\lib\fluidsynth.lib %2\lib
copy %1\lib\glib-2.0.lib %2\lib
copy %1\lib\libmpg123.lib %2\lib
copy %1\lib\libpng16.lib %2\lib
copy %1\lib\mad.lib %2\lib
copy %1\lib\modplug.lib %2\lib
copy %1\lib\ogg.lib %2\lib
copy %1\lib\opus.lib %2\lib
copy %1\lib\opusfile.lib %2\lib
copy %1\lib\pcre.lib %2\lib
copy %1\lib\pcreposix.lib %2\lib
copy %1\lib\portmidi.lib %2\lib
copy %1\lib\SDL2.lib %2\lib
copy %1\lib\SDL2_image.lib %2\lib
copy %1\lib\SDL2_mixer.lib %2\lib
copy %1\lib\SDL2_net.lib %2\lib
copy %1\lib\manual-link\SDL2main.lib %2\lib
copy %1\lib\vorbis.lib %2\lib
copy %1\lib\vorbisenc.lib %2\lib
copy %1\lib\vorbisfile.lib %2\lib
copy %1\lib\zlib.lib %2\lib

goto :eof

:usage
echo Usage: %0 prefix-path target-path
exit /b 1
