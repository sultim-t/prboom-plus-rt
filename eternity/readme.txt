*********************-= Team Eternity Presents =-*********************

                         The Eternity Engine

                            Version: 3.31
                        Release: Public Beta 7

      Coded by: James "Quasar" Haley and Steven "SoM" McGranahan

**********************************************************************

This is the source code for the Eternity Engine v3.31 public beta 6,
released under the terms of the GNU General Public License (GPL). The
terms of this license can be found in the file "Copying" included 
inside  the archive, but what it basically entails is that you are 
free to modify it, but if that you intend to release any such 
modification,  you are obligated to provide the source for that 
modification as well.

Some code in this distribution is under terms specified by the zdoom
source distribution license, which is included in the archive. This
license is considered less restrictive than the GPL, and has been
determined to be legally compatible with it. Therefore, that code
can also be considered to be covered by the above license for purposes
of modification and distribution with this archive.

Team Eternity will enforce this license to the best of its ability.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
General Public License for more details.

Contact James Haley at haleyjd@hotmail.com with any questions
regarding this source code.

You will need the Eternity Engine binary distribution in order to use
an executable compiled from this source code. It should be available
at the same location. If not, please contact me.

**** COPYRIGHT ****

Copyright holders in this work, along with the source of their code,
include (but may not be limited to) the following:

id Software ............ GPL DOOM source distribution
Chi Hoang .............. Original DosDOOM port
Lee Killough ........... TeamTNT's BOOM, MBF
Jim Flynn .............. BOOM
Rand Phares ............ BOOM
Ty Halderman ........... BOOM
Stan Gula .............. BOOM
Simon Howard ........... SMMU
Colin Phipps ........... PRBOOM
Florian Schulze ........ PRBOOM
Randy Heit ............. ZDOOM
Martin Hedenfalk ....... libConfuse cfg parser library
DJ Delorie ............. DJGPP libc
James Haley ............ Original code
Steven McGranahan ...... Original code

If you believe you own code in this work and have not been given
credit, please contact the author. No one has been intentionally
omitted for any reason personal or otherwise.


=== Instructions for Compilation =====================================

DOS Build

The DOS build, which uses DJGPP, can be constructed by changing into
the source sub-directory and typing "make" or "make debug" -- the
resulting objects and executable will be built into the obj or
objdebug directories respectively. You will need Allegro v3.x -- see 
the file allegro.h for more information on what versions of this library 
are compatible.  This version was built using an older version of
DJGPP, with GCC v2.81.  It should be possible to build it with newer
versions, but some small modifications might be necessary.


Windows Build -- Microsoft Visual C++

Requires Visual C++ 6.0 or later.

Just load the provided workspace and use the "Build" menu as always.
You will need SDL 1.2.5 and SDL Mixer 1.2.5 or later, and you will also 
need to change the "Additional Include Directories" under project 
options. The project expects the SDL library files to be in your VC++ 
lib directory, although this can be changed as well by editing the
"Additional library directories" Link option.


Windows Build -- MinGW

SDL and SDL Mixer should be fully built and installed. Switch into
the Source subdirectory in MSYS and use the makefile.mingw makefile
to build. Debug build works the same here as for DJGPP. Note that the 
makefile for this platform does not currently contain any dependency 
information, and therefore you will need to do a clean build every 
time. This problem should be addressed in the near future. Executables
will be built in the mingw_obj and mingw_objdebug directories.

Eternity expects to find SDL and SDL mixer files in the following
MSYS locations for the MinGW build. This can be changed by editing the
makefile's command-line option settings, but these locations are where
the files are normally placed by the SDL build scripts.

Include files: /usr/local/include/SDL
Library files: /usr/local/lib

Support for MinGW is still young, so there may be problems. Please 
report any you might encounter.

======================================================================
