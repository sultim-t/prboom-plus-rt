=====================================================================
                    README.TXT    04/09/2000
                    currently not up to date
=====================================================================

PRBOOM v2.03 --- an port to WIN32 of the released BOOM source
GLBOOM v2.03 --- an port to OpenGL of the released BOOM source

-This port is based upon BOOM v2.02 and MBF
-This port was written by Florian "Proff" Schulze, member of TeamTNT
 http://www.teamtnt.com
-DOOM is copyright ID software (www.idsoftware.com)
-This port is not endorsed by id software, so don't bug them about it
-I am not responsible for any damage that may be done by this program

=====================================================================

Contents

Section 1. Installing PRBOOM
Section 2. Configuring PRBOOM
Section 3. Playing PRBOOM in Single Player mode
Section 4. Playing PRBOOM in Serial/Network mode
Section 5. Editing for PRBOOM
Section 6. Differences between PRBOOM and BOOM
Section 7. Files and Directories in the PRBOOM Distribution
Section 8. How to report bugs in PRBOOM
Section 9. Appendix

=====================================================================
-----------------------------
Section 1. Installing PRBOOM
-----------------------------

Requirements:
-------------

PRBOOM requires a minimum of a 486DX/33 with 16M of RAM running
Windows 95 / 98 / Windows NT 4.0 (+SP3 or better).

PRBOOM requires a copy of DOOM, DOOM II, Ultimate DOOM, or Final DOOM already
       installed on your system. In the installation instructions below we use
       C:\DOOM2 to denote the directory it is installed in. Substitute the
       path to DOOM on your system wherever that appears.

PRBOOM requires a sound card, it will not play sounds over the PC speaker. If
you do not have a sound card you should run it with the -nosound option on the
command line. It requires DirectX 2.0 or better if you want to use sound and
fullscreen-drawing.

If you want to run PRBOOM in a window you need 32768 (15 bit) or more to get a
playable view.

To run it fullscreen you need 256 colors. No other color-modes are supported.

Suggested installation procedure:
---------------------------------

1) Unzip the download archive in a new directory, which we will call \PRBOOM.

2) Type PRBOOM -iwad C:\DOOM2, the game will start as usual. To avoid having
   to type -iwad C:\DOOM2, add the line below to your AUTOEXEC.BAT and
   reboot:

   SET DOOMWADDIR=C:\DOOM2

   Alternately you can unzip the PRBOOM archive in your DOOM directory and
   avoid needing -iwad or the change to your AUTOEXEC.BAT file.

   You can also simply copy DOOM1.WAD, DOOM.WAD, or DOOM2.WAD to the PRBOOM
   directory.

3) PRBOOM provides support for mouse, but in the current release the mouse
   is sometimes choppy if you use it in the window-mode, I had no problems
   in fullscreen-mode.

   If you don`t use the mouse you should set use_mouse to 0 in the CFG-File,
   because the game runs a little bit faster.

4) PRBOOM supports different ways to play music. You can choose it with
   the mus_card variable in the cfg-file. You have the following options:
    0 No Music
    1 MCI-Midi, this uses the mci-interface and needs a temporary
      file called prboom.mid
    2 Stream-Midi, this uses the streaming midi-interface and needs
      no more temp-file, but it could make problems.
   Stream-Midi is used by default.

   You can't set the volume of the Midi-Music with the option in the game, with
   it you can only turn the music on and off. You have to set the volume of the
   Midi-Music with the windows-mixer.

   I had some problems with my PCI-Soundcard. I have solved them by setting
   the hardware acceleration to one tick lower than 100%. I have found this
   setting in the control panel under multimedia settings.

   Under Windows 98 I had to install the newest driver for my PCI-Soundcard
   to get MIDI-Music.

   You can change the sampling parameters in the CFG-File.
    snd_channels : This is the number of sound-channels used. A higher value
                   means you can hear more sounds at once, but it also uses
                   more CPU-Power.
    snd_frequency: The sampling-rate, useful values are 11025,
                   22050 and 44100.
    snd_bits     : The bitdepth, allowed values are 8 and 16.
    snd_stereo   : If you have a mono-soundcard you can set this to 0.

  If you have problems with the midi-music like crashes, you should try to set
  "mus_card 1" to select MCI-Midi. The Stream-Midi functions seem to have a bug.

5) If sound and music sound ok, and the mouse behaves properly, you're done!

------------------------------
Section 2. Configuring PRBOOM
------------------------------

It's the same as in original BOOM, see boom.txt in the txts/boom directory.

------------------------------------------------
Section 3. Playing PRBOOM in Single Player mode
------------------------------------------------

It's the same as in original BOOM, see boom.txt in the txts/boom directory.

In the game 'ALT-ENTER' switches to fullscreen-mode and all window-keys like
'ALT-TAB' work the same way as in any other windows-program.

Command line parameters
------------------------

*Configuration Options

I have added the following command line parameters:

-condump
  This dumps the output to the files disp.txt and error.txt

-2
  In window mode:
   The gamescreen is stretched by 2 with StretchDIBits
  In fullscreen mode:
   If your display driver is only able to display 640x480 like in the most
   NT drivers, you can use this option to stretch it up to fullscreen.

-m2
  In window mode:
   The gamescreen is stretched by 2 with my own routine (this 
   is normally faster on NT)
  In fullscreen mode:
   If your display driver is only able to display 640x480 like in most
   NT drivers, you can use this option to stretch it up to fullscreen.

-fullscr
  Forces fullscreen-mode at startup if it's available

-nofullscr
  Forces window-mode at startup

-width
-height
  Specifies the resolution for the display. The maximum is 1600x1200.
  Usefull values are:

  width    x    height
  --------------------
    320    x      200
    320    x      240
    400    x      300
    512    x      384
    640    x      400
    640    x      480
    800    x      600
    960    x      720
   1024    x      768
   1280    x     1024
   1600    x     1200

  Default is 320x200. If you go higher than 640x480 the game will be
  much slower on most Systems, this has to do with the size of the
  L1-Cache and the L2-Cache.

-noddraw
  Don't use DirectDraw. You will not be able to use fullscreen with
  this option.

-------------------------------------------------
Section 4. Playing PRBOOM in Serial/Network mode
-------------------------------------------------

This is completely different to the original BOOM version, but its
similar to the Linux version of DOOM.

The following command line parameters are for use in netgames:

-net
  After this comes the player number of the machine. Then comes a list
  of host addresses or IP addresses. If you use IP addresses they have to
  begin with a ".". If one of the other machines has another portnumber,
  you can add " :PORTNUMBER" after the address (note that there MUST be
  a blank between the address and the colon).

-port
  If you want to use another port for a machine you can specify it with
  this command.

Here are some samples:

2 Players:
  1. Machine: -net 1 .192.168.1.2
  2. Machine: -net 2 .192.168.1.1

3 Players with changed port on the 2. Machine:
  1. Machine: -net 1 .192.168.1.2 :26001 .192.168.1.3
  2. Machine: -net 2 .192.168.1.1 .192.168.1.3 -port 26001
  3. Machine: -net 3 .192.168.1.1 .192.168.1.2 :26001

The -port command is also useful if you want to run a net game on ONE machine:
2 Players:
  1. Command: -net 1 .192.168.1.1 :26001 -port 26000
  2. Command: -net 2 .192.168.1.1 :26000 -port 26001

*Multiplayer Options

It's the same as in original BOOM, see boom.txt in the txts/boom directory.

------------------------------
Section 5. Editing for PRBOOM
------------------------------

It's the same as in original BOOM, see boom.txt in the txts/boom directory.

-----------------------------------------------
Section 6. Differences between PRBOOM and BOOM
-----------------------------------------------

Whats implemented:
  -mouse
  -keyboard
  -graphics   (using StretchDIBits and DirectDraw)
  -sound fx   (using DirectSound)
  -music      (using Windows-MIDI-Functions)
  -networking (using TCP/IP)

Whats not implemented:
  -joystick

Whats new:
  High-resolutions

-----------------------------------------------------------
Section 7. Files and Directories in the PRBOOM Distribution
-----------------------------------------------------------

PRBOOM executable archive PRBOOM202.ZIP

prboom.exe    The executable for Windows 95 / 98 / NT 4.0
txts          Docs and infos
txts/boom     Docs and infos of the original BOOM distribution
prbstuff      Progs for the Win32-Version. These are from me

PRBOOM source archive PRBOOM202S.ZIP

prboom.dsp            VC50 Project File
prboom.dsw            VC50 Workspace File
src                   All the source files
txts                  Docs and infos
txts/boom             Docs and infos of the original BOOM distribution
txts/doom             Docs and infos of the original DOOM source distribution
makfiles/makefile.dos The DJGPP makefile
makfiles/makefile.c32 The CYGWIN32 makefile
makfiles/makefile.m32 The MINGW32 makefile
boomstuf              Progs for the DOS-Version. These are from BOOM

----------------------------------------
Section 8. How to report bugs in PRBOOM
----------------------------------------

If you find a bug in PRBOOM you should visit

http://prboom.sourceforge.net

and follow the directions there for reporting it.

For more information on reporting a bug, see boom.txt in the same section.

--------------------
Section 9. Appendix
--------------------

You can get the PRBOOM source at:
http://prboom.sourceforge.net

You can get the original linux-doom source at:
ftp://ftp.idsoftware.com/source/doomsrc.zip

You can get the BOOM source at:
http://www.teamtnt.com

Thanks go to (in no particular order):
    ID Software for the release of LinuxDOOM

    TeamTNT for BOOM

    Andy Bay for ATBDOOM

    The DOSDOOM-Team for DOSDOOM

    Randy Heit for ZDOOM

    Ryan Haksi for Dynamic OpenGL Loading (opengl.*)
    
-------------------------------------------------------------------------
