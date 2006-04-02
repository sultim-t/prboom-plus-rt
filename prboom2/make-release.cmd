mkdir release
set CYGWIN=c:\cygwin
copy VISUALC6\Release\prboom.exe release
copy VISUALC6\ReleaseGL\glboom.exe release
copy VISUALC6\ReleaseServer\prboom_server.exe release
copy data\prboom.wad release
%CYGWIN%\bin\man2html doc\boom.cfg.5 > release\boom.cfg.html
%CYGWIN%\bin\man2html doc\prboom.6 > release\prboom.html
%CYGWIN%\bin\man2html doc\prboom-game-server.6 > release\prboom-game-server.html
copy AUTHORS release\AUTHORS.txt
copy COPYING release\COPYING.txt
copy FAQ release\FAQ.txt
copy NEWS release\NEWS.txt
copy README release\README.txt
copy doc\boom.txt release\boom.txt
copy doc\MBF.txt release\MBF.txt
copy doc\MBFFAQ.txt release\MBFFAQ.txt
copy doc\README.command-line release\README.command-line.txt
copy doc\README.compat release\README.compat.txt
copy doc\README.demos release\README.demos.txt
