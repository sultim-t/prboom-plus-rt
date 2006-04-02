***********************-= Team Eternity Presents =-***********************

                           The Eternity Engine

                            Version: 3.33.02

        Coded by: James "Quasar" Haley and Steven "SoM" McGranahan
                    
      Based on SMMU - "Smack My Marine Up" by Simon "fraggle" Howard

                 Start map by Derek "Afterglow" Mac Donald         
      
          Special Thanks To:  Lee Killough, the TeamTNT BOOM Team,
             DooMWiz, Colin Phipps, fraggle, Nemesis, Carnevil,
           Randy Heit, Julian, Joel Murdoch, schepe, SargeBaldy,
                   and all Eternity Engine beta testers.
                   
**************************************************************************

**** COPYRIGHT ***********************************************************

The Eternity Engine is distributed under the terms of the GNU General 
Public License. For more information, see the file "COPYING" included in 
the distribution archive.

Copyright holders in this work, along with the source of their code,
include (but may not be limited to) the following:

id Software ............ GPL DOOM source distribution, Quake 2
Chi Hoang .............. Original DosDOOM port
Lee Killough ........... BOOM, MBF
Jim Flynn .............. BOOM
Rand Phares ............ BOOM
Ty Halderman ........... BOOM
Stan Gula .............. BOOM
Simon Howard ........... SMMU
Colin Phipps ........... PrBoom
Florian Schulze ........ PrBoom
Randy Heit ............. zdoom
Martin Hedenfalk ....... libConfuse library
DJ Delorie ............. DJGPP libc
James Haley ............ Original code
Steven McGranahan ...... Original code

If you believe you own code in this work and have not been given credit,
please contact the author. No one has been intentionally omitted for any
reason personal or otherwise.

Eternity also makes use of the following, which do not impact its copy-
right:

Sam Lantinga et al. .... SDL and SDL Mixer libraries (http://www.libsdl.org)
  * Source code for these libraries is available via CVS or tar.gz from this
    site.

ITB CompuPhase, Inc. ... Small AMX and compiler


**** DISCLAIMER ****

This software is covered by NO WARRANTY, implicit or otherwise, including
the implied warranties of merchantability and fitness for a particular
purpose. The authors of this software accept no responsibility for any ill
effects caused by its use and will not be held liable for any damages, 
even if they have been made aware of the possibility of such damages.

Bug reports and feature requests will be appreciated.

Please email haleyjd@hotmail.com with any concerns.


**** DESCRIPTION *********************************************************

Eternity is the newest source port to inherit the banner of BOOM. 
Descended from MBF, it attempts to carry on the effort to put editability, 
reliability, and compatibility first. From SMMU it also gains the momentum
to give the player new options rather than to dictate DOOM purity.

Eternity includes full BOOM compatibility, as well as most features added 
in MBF and SMMU. In addition, Eternity adds many more. Check out the ex-
tensive documentation package for full information.


**** Features New to Version 3.33.02 *************************************

These include some of the major features added in the latest version:

* EDF 1.5

  * New TerrainTypes system. TerrainTypes now absorbed under the umbrella
    of EDF. New features include low mass splashes, terrain damage,
    particle interaction, and complete customization capabilities. Old
    binary TERTYPES lump is still supported within new system. Terrain
    definitions can be placed both within the root EDF and within ETERRAIN
    lump(s). Existing terrain objects standardized for editor use.

  * ifenabled function improved to operate on an arbitrary number of
    parameters. Added ifenabledany, ifdisabled, ifdisabledany, ifgametype,
    and ifngametype functions for more flexible conditional parsing of
    definitions.

  * Sounds may now provide custom distance attenuation parameters.
  
  * Added support for independently parsed EDF lump chains, including the
    ESTRINGS and ETERRAIN lumps.
  
  * String values now supported for thingtype "mod" field.
    
  * Default fallback loading fixed.
    
  * Many error messages further improved.

* 3D Object Clipping Enhancement
  
  All objects in DOOM can now be toggled to use a height corrected for
  3D clipping through the comp_theights option. Decorative objects bearing
  the 3DDECORATION flag will always clip missiles using their original
  height in order to perfectly preserve the playability of old maps even
  when comp_theights is on.
  
* Particle System Enhancements

  * Parameterized drip object (#5007) can create particle drips of any
    size and color at any frequency with optional terrain interaction. See
    the documentation for ExtraData and EDF for full information.
    
  * New FLIESONDEATH effect almost perfectly emulates Quake 2's corpse
    flies. Old FLIES effect is persistent, and now provides an appropriate
    sound effect.
    
  * Some Small scripting interaction added; much more to come.
  
  * Particles with fall-to-ground style will now stop at deep water floors
    defined by linedef 242.
  
* Compatibility and Efficiency

  * Crusher desync issue from 3.33.01 repaired; crushers now descend to
    floor again.
    
  * "Are you sure?" message restored to Nightmare skill selection through
     menu.
     
  * More memory usage improvements.
  
  * Quad column buffer code further streamlined for drastically fewer
    calls on the most critical line of execution.
      
**** COMING SOON *********************************************************

These are features planned to debut in future versions of the Eternity 
Engine:

* Priority *

* EDF for Weapons and Player Classes
* Complete Heretic support
* Improved Particle Effects (implementation has begun)
* More standard TerrainTypes
* Working hub system

* Long-Term Goals *

* Sound sequences
* Polyobjects
* Hexen Support
* Strife Support
* Network code and massive changes needed to support it

**** Revision History ****************************************************

3.33.02 -- 10/01/05

* A very respectable amount of progress hopefully offsets the terrible 
  delay experienced for this release. 3D object clipping improvement,
  new EDF TerrainTypes engine, and various EDF improvements are the
  most significant features.

3.33.01 -- 06/24/05

* Minor fixes, some user-requested and CQIII-needed features implemented,
  and HUD scripting functions for Small.

3.33.00 -- 05/26/05

* Small scripting finally complete and available to the user. Lots of
  fixes, tweaks, and feature additions on top of that.

3.31.10 -- 01/19/05

* More Small support, ExtraData, global MapInfo, and tons of other new 
  features and bug fixes make this an extremely critical release.

3.31 Public Beta 7 04/11/04

* High resolution support enabled via generalization of the screen 
  drawing code. Some minor fixes to portals, and a new codepointer just 
  for good measure.

3.31 Public Beta 6 02/29/04

* A huge amount of progress toward all current goals for the engine.
  Portals, the brain child of SoM, are the major stars of this release.
  Small and ExtraData are on the verge of becoming available to users 
  for editing.

3.31 Public Beta 5 12/17/03

* Several minor bug fixes over beta 4, including 3D object clipping and
  file drag-and-drop. Incremental improvements to EDF and to several
  parameterized codepointers.

3.31 Public Beta 4 11/29/03

* Most effort put into improving EDF, largely via user feature requests.
  Major progress has been made toward working ExtraData as well. The
  Heretic status bar is now implemented.

3.31 Public Beta 3 08/08/03

* A long time in coming, and a ton of features to show for the delay.
  EDF steals the show for this release.

3.31 Public Beta 2 03/05/03

* New BFGs, object z clipping, movable 3DMidTex lines, tons more Heretic
  support, and some really nasty bugs fixed. Also first version to use
  SDL_mixer for better sound.

3.31 Public Beta 1 09/11/02

* Prelim. Heretic support, 3DMIDTEX flag, zdoom-style translucency, and
  some other stuff now in. And of course, bug fixes galore.

3.29 "Gamma" 07/04/02

* Final release for v3.29. Massive changes all over. First version to
  include Windows support. SoM is now co-programmer.

3.29 Development Beta 5 10/02/01

* Minor release to demonstrate dialog, primarily

3.29 Public Beta 4 06/30/01

* Fixed a good number of bugs once again. Improved portability with 
  Julian's help, major feature is working demo recording. See info below
  and changelog for more info, as usual.

3.29 Public Beta 3 05/10/01

* Fixed a large number of bugs, added several interesting features. See
  bug list below, and the changelog, for more information.

3.29 Public Beta 2 01/09/01

* Fixed mouse buttons - problem was leftover code in I_GetEvent() that
  caused the mouse button variables to never be asserted - code was
  unreachable because of an assignment of the variable 'buttons' to 
  'lastbuttons'.
  
3.29 Public Beta 1 01/08/01

* Initial Release


**** Bugs Fixed, Known Issues ********************************************

* Major Bugs Fixed:

+ Garbage being drawn at ends of sprite columns when looking at certain
  y-shear angles was tracked to discrepancies in use of centery vs.
  centeryfrac in the column drawers; repaired fully. Change also makes
  y-shear motion appear smoother.
  
+ All player messages now use the player's configured message color,
  unless the strings themselves override that color with embedded color
  codes.
  
+ Erasing of text HUD widgets made more thorough.

+ whistle console command can no longer teleport the whistled object into
  inert objects.  

  Please see the Eternity changelog for full information on all bugs that
  have been fixed.

* Known Issues in 3.33.01:

- Volume sliders may permanently affect desktop volume levels. This is due
  to bugs/limitations/oversights in SDL_mixer and NOT because of any
  problem inherent in Eternity. There may be ways to fix this, at least
  for MIDI, but the fixes will not be implemented until at least the
  next version.

- Network code is incomplete and totally broken and will require change to
  thousands of lines of code (a complete engine overhaul). Minor progress
  has begun by isolating portions of the code that will be affected with
  various verbose *_FIXME tagged comments.

