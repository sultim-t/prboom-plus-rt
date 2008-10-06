BOOM.CFG(5)                                                        BOOM.CFG(5)



NAME
       boom.cfg, glboom.cfg - Configuration file for PrBoom v2.1.0 onwards

USAGE
       When  a  version  of PrBoom is run, it  searches for this configuration
       file to modify its default  settings.   Every  time  PrBoom  exits,  it
       rewrites  the  configuration file, updating any settings that have been
       changed using the in-game menus.

       PrBoom  expects  the  config  file  to   be    ~/.prboom/boom.cfg,   or
       ~/.prboom/glboom.cfg if compiled with GL support. Alternatively, it can
       be made to look elsewhere by using a command-line parameter:

       {prboom,glboom} [-config myconf]

FORMAT
       boom.cfg consists of a number of variables and values. Each line is  of
       the following format:

       [  { {{#,;,[} comment_text , variable {decimal_integer, 0x hex_integer,
       "string_text"}}]

       Any line beginning with a non-alphabetic character is treated as a com-
       ment  and  ignored;  for future compatibility you should start comments
       with a #, ; or [.  Note however that when PrBoom rewrites  boom.cfg  it
       does not preserve user added comments.

       Any  line  beginning with an alphabetic character is treated as a vari-
       able-value pair.  The first word (sequence  of  non-whitespace  charac-
       ters) is the variable name, and everything after the following block of
       whitespace is taken to be the value assigned to the variable.

       Variables not recognised by PrBoom, or which are given an invalid value
       or  a value of an inappropriate type, are ignored. Warning messages are
       given where relevant.

       The variables recognised by PrBoom are  described  per-section  in  the
       following  sections.  The  sections  are  informal however; when PrBoom
       rewrites the config file it writes in section headings and  puts  vari-
       ables into the relevant sections, but when reading these are ignored.


MISC SETTINGS
       compatibility_level
              PrBoom  is  capable of behaving in a way compatible with earlier
              versions of Doom and Boom/PrBoom. The value given  here  selects
              the  version  to  be compatible with when doing new games/demos.
              See README.compat for details.

       realtic_clock_rate
              Selects the speed that PrBoom runs at, as a percentage of normal
              game speed.  Leave at 0 unless you want to experiment. Note that
              it is considered `cheating' to use this at any setting  below  0
              (or above?).

       max_player_corpse
              Sets the maximum number of player corpses to leave lying around.
              If this limit would be exceeded, an old corpse is removed.  Use-
              ful  for  big/long  Deathmatch  games, where the sheer number of
              corpses could slow the game down.

       flashing_hom
              Flag indicating whether a flashing red background  is  drawn  to
              highlight HOM errors in levels (for level developers)

       demo_insurance
              Selects a method of protecting demos against `going out of sync'
              (where the player seems to lose control and behave madly, but in
              fact  the  players  original  instructions as stored in the demo
              have got out of sync with the game he was playing). 0=No protec-
              tion,  1=Full  protection,  2=Only while recording demos. Safest
              when left set to 2.

       endoom_mode
              This parameter specifies options controlling the display of  the
              credits  screen  when  Doom  exits. Currently it is the sum of 3
              options: add 1 for colours, 2 for  non-ASCII  characters  to  be
              displayed, and 4 for the last line to be skipped so the top line
              doesn't scroll off screen.

       level_precache
              If set, when loading a new level PrBoom precaches all the graph-
              ics  the  level  is likely to need in memory. This makes it much
              slower to load the level, but reduces disk  activity  and  slow-
              downs  reading  data  during  play. Most systems are fast enough
              that precaching is not needed.


FILES SETTINGS
       wadfile_1, wadfile_2
              The names of 2 .wad files to be automatically loaded when PrBoom
              is started.  A blank string means unused.


       dehfile_1, dehfile_2
              The  names  of  2 patch files (.deh or .bex) to be automatically
              loaded when PrBoom is started (empty string for none).


GAME SETTINGS
       default_skill
              The default skill level when starting a new game.

       weapon_recoil
              Enables recoil from weapon fire.

       doom_weapon_toggles
              Flag indicating whether pressing 3 or  1  when  that  weapon  is
              already selected causes the selected shotgun or fist/chainsaw to
              be toggled, as in original Doom. Some people  prefer  to  use  a
              number for each weapon alone.

       player_bobbing
              Enables player bobbing (view randomly moving up/down slightly as
              the player runs).

       monsters_remember
              Makes monsters remember their previous enemy after killing their
              current target.

       monster_infighting
              Whether  monsters  will  fight  each other when they injure each
              other accidentally.

       monster_backing
              Whether monsters without close combat  weapons  will  back  away
              from close combat (unlike original Doom).

       monster_avoid_hazards
              Whether monsters avoid crushing ceilings.

       monkeys
              Whether monsters will climb steep stairs.

       monster_friction
              Whether  monsters  are  affected by changed floor friction (they
              should be, but weren't in Boom)

       help_friends
              Whether monsters will help out injured monsters by  aiding  them
              against their attacker.

       player_helpers
              The number of helper dogs to spawn.

       friend_distance
              Distance within which friends will generally stay.

       dog_jumping
              Whether dogs will jump.

       sts_always_red
              PrBoom  can  make  the colour of the text displays on the status
              bar  reflect  your  current  status  (red=low,   yellow=average,
              green=good, blue=super-charged).  This option if set selects the
              traditional Doom behavior of always-red status bar display;  set
              to 0 to allow the coloured display.

       sts_pct_always_gray
              See  above,  this  makes  just  the  percent  signs always gray,
              instead of changing colour.

       sts_traditional_keys
              Doom and PrBoom have two types of  keys;  PrBoom  will  normally
              display  both  keys  of  a  given  colour if you have both. This
              option, if enabled, instead makes PrBoom only ever  display  one
              key of each colour, in the same way Doom did.

       traditional_menu
              Changes  PrBoom's  menu ordering to be the same as original Doom
              if enabled.

       show_messages
              When enabled, text messages are displayed in the top left corner
              of  the  screen describing events in the game. Can be toggled in
              the game, this is just to preserve the setting.

       autorun
              Makes the player always run, without having to hold down  a  run
              key.  Can  be  toggled in the game, this just preserves the set-
              ting.


SOUND SETTINGS
       sound_card
              Selects whether sound effects are  enabled  (non-zero  enables).
              For  compatibility  reasons  with  Boom,  a  range of values are
              accepted.

       music_card
              Selects whether in-game music is enabled (non-zero enables). For
              compatibility reasons a range of values are accepted.

       pitched_sounds
              If  enabled  by  this  variable, this enables `pitching' (making
              pitch adjustments to the playing sounds) for 16 bit sound cards.

       samplerate
              The  samplerate  for soundmixing and timidity. The sound quality
              is much better at higher samplerates, but if  you  use  timidity
              then  higher samplerates need much more CPU power. Useful values
              are 11025, 22050, 44100 and 48000.

       sfx_volume
              Sound effects volume. This is best adjusted in the game.

       music_volume
              Music volume. This is best adjusted in the game.

       mus_pause_opt
              Selects what PrBoom does to the music when a  games  is  paused.
              0=stop  the  music, 1=pause the music (stop it playing, but when
              resumed resume it at the same place - not  implemented),  2=con-
              tinue playing.

       sounddev, snd_channels, soundsrv, musicsrv
              These  variables  are no longer used by PrBoom, but are kept for
              compatibility reasons.


COMPATIBILITY SETTINGS
       These are settings that let you choose whether the normal game  mechan-
       ics  are  used,  or whether various quirks, bugs and limitations of the
       original Doom game are emulated.


VIDEO SETTINGS
       screen_width, screen_height
              For versions of PrBoom which support high-res, these specify the
              default  screen  or  window  size for PrBoom. These settings are
              ignored and preserved by versions of  PrBoom  which  do  not  do
              high-res (they assume 320x200).

       use_fullscreen
              If  set,  this causes PrBoom to try to go full screen. Depending
              on your video driver and mode, this may include changing  screen
              resolution to better match the game's screen resolution.

       use_doublebuffer
              Use double buffering to reduce tearing. On some machines this is
              even faster than the normal method, but  on  others  this  makes
              problems, so you have to try out which setting works best.

       translucency
              Causes PrBoom to display certain objects as translucent.

       tran_filter_pct
              Selects  how  translucent objects are when they are translucent.
              Play with this and see for yourself.

       screenblocks
              Selects a reduced screen size  inside  the  PrBoom  window  (the
              player's view is surrounded by a border). Normally this is unde-
              sirable, but it can help speed up the game. Can  be  changed  in
              the  game  with  the +/- keys, this variable is just to preserve
              that setting.

       usegamma
              Selects a level of gamma correction (extra  screen  brightening)
              to  correct  for  a  dark  monitor or light surroundings. Can be
              selected in the game with the F11 key, this  config  entry  pre-
              serves that setting.


OPENGL SETTINGS
       If you are knowledgeable about OpenGL, you can tweak various aspects of
       the GL rendering engine.

       gl_nearclip
              The near clipping plane *100.

       gl_colorbuffer_bits
              The bit depth for the framebuffer. (16, 24 or 32 bits)

       gl_depthbuffer_bits
              The bit depth for the z-buffer. (16, 24 or 32 bits)

       gl_tex_filter_string
              A string, one of the  following:  GL_NEAREST  or  GL_LINEAR  (no
              mipmapping),     or     one     of    GL_NEAREST_MIPMAP_NEAREST,
              GL_NEAREST_MIPMAP_LINEAR,   GL_LINEAR_MIPMAP_NEAREST,    GL_LIN-
              EAR_MIPMAP_LINEAR with mipmapping.

       gl_tex_format_string
              One of the following strings: GL_RGBA - means format selected by
              driver (not so good), GL_RGBA2 - means 2 bits for each component
              (bad),  GL_RGBA4 - means 4 bits for each component (like GL_RGBA
              on most cards), GL_RGB5_A1 - means 5 bits for each color  compo-
              nent  1  bit for the alpha channel (default), GL_RGBA8 - means 8
              bits for each component (best quality, but  only  a  little  bit
              better than GL_RGB5_A1 and slower on most cards)

       gl_drawskys
              If  0,  disables  drawing  skies,  which may be needed with some
              problematic 3D cards.

       gl_sortsprites
              Experimental option, possibly faster but less reliable.


MOUSE SETTINGS
       This section specifies settings for using a mouse  with  PrBoom.  There
       are  several  settings  that  control button bindings (what action each
       button causes in the game); these are  easiest  set  from  the  in-game
       menus, these config entries are to preserve the settings between games.

       use_mouse
              Enable or disable the use of a mouse with PrBoom.

       mouse_sensitivity_horiz, mouse_sensitivity_vert
              Sets the sensitivity of the mouse in PrBoom. Easier set from the
              in-game menus.


KEY BINDINGS
       These  specify  the  keys  that  trigger various actions in PrBoom. The
       codes used for keys are internal to PrBoom, though many keys are repre-
       sented  by their ASCII codes. It is easiest to modify these via the in-
       game menus (OPTIONS->SETUP->KEY BINDINGS). These  config  file  entries
       preserve the settings from this menu between game sessions.


JOYSTICK SETTINGS
       There  are the trigger variables here, which are calculated during joy-
       stick calibration (the values received from the kernel  driver  outside
       of  which  movement  is caused in the game). Also there are the button-
       bindings, again best adjusted using the in-game menus.

       use_joystick
              This selects the number of the joystick to use, or 0 selects  no
              joystick.  You  have to have the relevant device files (/dev/js0
              etc) and the kernel driver loaded.


CHAT MACROS
       These are pre-written text strings for quick transmission to players in
       a  network  game (consult your Doom documentation). Easiest set via the
       in-game menus (OPTIONS->SETUP->CHAT MACROS).


AUTOMAP SETTINGS
       These are settings related to the automap. These are easiest  set  from
       within the game.


HEADS_UP DISPLAY SETTINGS
       These  are  settings  related to the heads-up display, that is messages
       received while playing and the heads-up display of your current  status
       obtained by pressing + while the view is full-screen in PrBoom. See the
       Boom documentation for details. All controlled  best  from  within  the
       game.


WEAPON PREFERENCES
       Here   are   the   settings   from   the   Weapons  menu  in  the  game
       (OPTIONS->SETUP->WEAPONS).


ALSO SEE
       prboom(6), PrBoom's documentation (including the Boom and MBF  documen-
       tation) and your Doom documentation.


AUTHOR
       See the file AUTHORS included with PrBoom for a list of contributors to
       PrBoom.   This  config  file  reference   written   by   Colin   Phipps
       (cph@moria.org.uk).

PRBOOM-GAME-SERVER(6)                                    PRBOOM-GAME-SERVER(6)



NAME
       prboom-game-server - Server for network games of PrBoom.

SYNOPSIS
       prboom-game-server  [ -adfnrv ] [ -e epis ] [ -l level ] [ -t ticdup ]
       [ -x xtics ] [ -p port ] [ -s skill ] [ -N players ]  [ -c conffilename
       ]  [ -w wadname[,dl_url ]]

DESCRIPTION
       PrBoom  is a version of the 3D shoot'em'up Doom, originally by id soft-
       ware.  It includes, amongst other things, the ability to play with sev-
       eral  players  connected by a tcp/ip network. prboom-game-server is the
       `server', that is the program that passes data  between  the  different
       players in the game.

       To  start  a  network  game (often abbreviated to `netgame'), first the
       server is started. prboom-game-server  accepts  various  parameters  to
       control  the type of game (the skill level, number of players, level to
       play, optional WAD file(s) to load, etc).

       Then each player that wishes to participate runs prboom -net  hostname,
       where  hostname  is the name of the machine on which the server is run-
       ning. Each copy of prboom retrieves information about the game from the
       server,  and when the specified number of players have joined, the game
       begins.


Options
       -N players
              Specifies the number of players in the  game  (default  2).  The
              server  will  wait for this many players to join before starting
              the game.

       -e epis
              The episode to play (default 1).  Unless you are playing Doom  1
              or  Ultimate  Doom,  and wish to play one of the later episodes,
              you do not need to change this.

       -l level
              The level to play (default 1).

       -s skill
              Specify the skill level to play (1-5).

       -d     Set game mode to (old) deathmatch (default is cooperative).  See
              the  original Doom docs for information about the different net-
              work game modes.

       -a     Set game mode to `altdeath' (v2 deathmatch) (default is coopera-
              tive). See the original Doom docs for information about the dif-
              ferent network game modes.

       -f     Select fast mode (monsters move faster).

       -n     Selects nomonsters mode, i.e. there are no monsters in the game.

       -r     Respawn  mode. If you don't know what this is, you don't want to
              ;-).

       -c conffilename
              Specifies a configuration file to read which sets parameters for
              the game. This is in the same format as the PrBoom configuration
              file (in fact, you can ask it to read your normal PrBoom config-
              uration  file  if  you want). Only certain settings are acknowl-
              edged: default_skill, default_compatibility_level, the  compati-
              bility options and some of the game settings (use -v to have the
              server print the options as it recognises them).

       -w wadname[,dl_url]
              Specifies a WAD file to play. This is added to the internal list
              that  the server keeps. When a client connects, the server sends
              the list of WADs; PrBoom will then add this to the list of  WADs
              specified  on  its command line.  Optionally, an url to the file
              can be given too; if when PrBoom connects  it  cannot  find  the
              named  WAD,  it will attempt to retrieve the file from the given
              url, extracting it if necessary. See prboom(1)  for  information
              about the supported url types and compression formats.

       -t ticdup
              Reserved.

       -x xtics
              This  causes  extra  information  to  be  sent with each network
              packet; this will help on networks with high  packet  loss,  but
              will use more bandwidth.

       -p port
              Tells  prboom-game-server  what  port  number to communicate via
              (default 5030).  Note that if you change this from the  default,
              then  all the clients will also need to specify this number when
              they try to connect (the default programmed into prboom is  also
              5030).

       -v     Increases   verbosity  level;  causes  more  diagnostics  to  be
              printed, the more times -v is specified.

More Information
       prboom(6), boom.cfg(5)

       For more information, see the README that came with PrBoom.

       Doom is a  registered  trademark  of  id  software  (http://www.idsoft-
       ware.com/).

Author
       See  the  file AUTHORS included with the PrBoom distribution.  This man
       page was written by Colin Phipps (cph@moria.org.uk).



PRBOOM(6)                                                            PRBOOM(6)



NAME
       prboom - PrBoom, a version of Doom for Unix, Linux and Windows systems

SYNOPSIS
       prboom [ -complevel lvl ]  [ -width w ] [ -height h ] [ -viewangle n  ]
       [ -vidmode gl ] [ -[no]fullscreen ] [ -[no]window ]  [ -iwad iwadname ]
       [ -file wad1 ... ]  [  -deh  deh_file  ]  [  -noload  ]    [  -loadgame
       {0,1,2,3,4,5,6,7}  ] [ -warp { map | epis level } -skill {1,2,3,4,5} ]
       [ {-fastdemo,-timedemo,-playdemo} demofile [ -ffmap num ] ]  [  -record
       demofile ] [ -solonet ]  [ -net hostname[:port] ] [ -deathmatch [ -alt-
       death ] ] [ { -timer mins | -avg }] ]  [ -nosound ] [ -nosfx ] [ -nomu-
       sic] [ -nojoy ] [ -nomouse ] [ -noaccel ] [ -nodraw ]  [ -config myconf
       ] [ -save savedir ]  [ -bexout bexdbg ] [  -debugfile  debug_file  ]  [
       -devparm ] [ -noblit ] [ -nodrawers ]

DESCRIPTION
       PrBoom  is a version of the 3D shoot'em'up Doom, originally by iD soft-
       ware.  It is based on Boom,  a  version  of  Doom  adapted  by  TeamTNT
       (http://www.teamtnt.com/) for DOS. PrBoom uses the SDL library, meaning
       it can run on a variety of different systems, including  Windows,  X11,
       Linux/SVGALib.

Options
       -complevel lvl
              This  sets  the  compatibility  mode that PrBoom runs in. If you
              need to change this, see README.compat.


Video Options
       -width w
              Specifies the width of the PrBoom window, in pixels. Default  is
              320, the width must be greater than 320..

       -height h
              Specifies the height of the PrBoom window, in pixels. Default is
              200, the height must be greater than 200.

       -viewangle n
              Causes the player view to be rotated by a given  offset  (speci-
              fied in 45degree increments, in the range 0..7) from the way the
              player is facing.

       -vidmode gl
              Use the OpenGL video mode. The default is to  use  the  software
              video mode.

       -fullscreen, -nofullscreen
              These options toggle fullscreen mode. The default is fullscreen.

       -window, -nowindow
              This pair of options also toggle fullscreen mode. They only take
              effect  for this prboom session and do not alter your configura-
              tion file.

WAD Options
       -iwad iwadname
              Specifies the location of the IWAD file, typically  doom.wad  or
              doom2.wad (or doom2f.wad). This tells prboom where the main .wad
              file that came with the version of Doom that you own is.

       -file wad1 ...
              Specifies a list of PWAD files to load in addition to  the  IWAD
              file. PWAD files modify the existing Doom game, by adding levels
              or new sounds or graphics. PWAD files are widely  available  for
              download; try ftp.cdrom.com/pub/idgames for starters.

       -deh deh_file
              Tells PrBoom to load the dehacked patch deh_file.

Game Options
       -loadgame {0,1,2,3,4,5,6,7}
              Instructs PrBoom to load the specified saved game immediately.

       -warp { map | epis level }
              Tells  PrBoom  to  begin  a  new game immediately. For Doom 1 or
              Ultimate Doom, you must specify the episode and level number  to
              begin  at  (epis is 1 for Knee-Deep in the Dead, 2 for Shores of
              Hell, 3 for Inferno, 4 for Ultimate Doom; level is between 1 and
              9). For Doom ][ or Final Doom, you must specify the map to begin
              at, which is between 1 and 2 (0 for German Doom ][).

       -skill n
              Tells PrBoom to begin the game at skill level n (1 for ITYTD,  2
              for  Not  Too Rough, 3 for Hurt Me Plenty, 4 for Ultraviolent, 5
              for Nightmare).

       -respawn
              Tells PrBoom that monsters that die should respawn (come back to
              life) after a while. Not for the inexperienced.

       -fast  Tells  PrBoom  to  make all the monsters move  react faster. Not
              for the inexperienced.

       -nomonsters
              Tells PrBoom to include no monsters in the game.

Multiplayer Options
       -net hostname[:port]
              Specifies that a TCP/IP network game is to be started.  hostname
              is  the  name of the machine on which the network game server is
              running (prboom-game-server). For more information  about  this,
              see  prboom-game-server(6) and the README that came with prboom.
              port is the port number on the remote machine to which  to  con-
              nect;  if  not  specified,  the  default  of  5030 (which is the
              default for prboom-game-server(6) ) is assumed.  The server will
              configure your PrBoom settings, so that all the players have the
              same game settings (skill, map etc).

       Also, the server may specify additional PWAD files to play with; if you
       do  not  have  the required .WAD file, PrBoom will ask the server for a
       download path, and attempt to use wget(1) and if necessary unzip(1)  to
       download and extract the required WAD.

       -port portnum
              Specifies  the  local port to use to communicate with the server
              in a netgame.

       -deathmatch
              No longer used. Tells PrBoom to begin  a  deathmatch  game,  but
              this is overridden by the server's settings. Only works for sin-
              gle play (!).

       -altdeath
              Similar to -deathmatch, but implies a different set of rules for
              the deathmatch game. No longer used (specified by the server).

       -timer mins
              No  longer  used. Specifies that levels will end after mins min-
              utes of play if the level is still being played, but is overrid-
              den  by  the  server  in a netgame. Not really useful for single
              play.

       -avg   Equivalent to -timer 20

       -solo-net
              Used to run a single-player network game, without a network game
              server.  This enables network game items & options for an other-
              wise single-player game; some demos are recorded like this.

Demo (LMP) Options
       -record demofile
              Instructs PrBoom to begin recording a  demo,  to  be  stored  in
              demofile.lmp.  You  should specify game options to specify which
              level and skill to record at.

       -playdemo demofile
              Play the recorded demo demofile.lmp

       -timedemo demofile
              Play the recorded demo demofile.lmp, reporting information about
              the length of the demo (in gametics) afterwards.

       -fastdemo demofile
              Play  the recorded demo demofile.lmp as fast as possible. Useful
              for benchmarking PrBoom, as compared to other versions of  Doom.

       -ffmap num
              Fast forward the demo (play at max speed) until reaching map num
              (note that this takes just a number,  not  a  map  name,  so  so
              -ffmap 7 to go fast until MAP07 or ExM7).

I/O Options
       -nosound
              Disables  all sound effects and in-game music. This prevents the
              sound server loading, which lets the game run a little faster.

       -nosfx Disables sound effects during the game. This does not  stop  the
              sound  server  loading,  however,  so  for  best performance use
              -nosound.

       -nomusic
              Disables playing of music in the game.

       -nojoy Disables joystick support.

       -nomouse
              Prevents the mouse being grabbed by the prboom window.

       -noaccel
              For prboom, this prevents it using the MITShm  server  extension
              for  passing the screen data to the X server. This option may be
              required if the X server is not local. For  lsdoom,  this  tells
              lsdoom  not  to  use  the  accelerated  graphics  functions that
              SVGALib provides even when they are  supported  for  your  video
              card (normally this is autodetected).

       -1, -2, -3
              Specifies  the  scale factor by which to enlarge the window. The
              default, -1, displays the normal 320x200 pixel Doom  screen  (or
              whatever  size is specified by the -width and -height parameters
              or in the config file for prboom).  If this window is too small,
              try  using -2 or -3 to enlarge the window.  -nodraw Suppress all
              graphical display. Only for debugging & demo testing.

Configuration
       -config myconf
              Loads an  alternative  configuration  file,  named  myconf.  The
              default  is boom.cfg(5), taken from the same directory as PrBoom
              was run from, except when running with OpenGL, then the  default
              is glboom.cfg(5).

       -save savedir
              Causes  prboom  to  save  games  in  the  directory specified by
              savedir instead of ~/.prboom/.

Debugging/Profiling Options
       -devparm
              Development mode. Mostly redundant these days, but it does force
              non-lazy  generation  of texture lookups which can be useful for
              level authors debugging PWADs.

       -debugfile debug_file
              Causes  some  debugging  information,  mainly  network  info   I
              believe, to be written to the named file as prboom runs.

       -nodrawers
              Causes no rendering to be done. The only conceivable use of this
              is (a) a multiplayer server (b) to test the speed of  the  other
              routines in the program, when combined with -timedemo.

       -noblit
              Causes  no copying to the screen from the rendering buffer to be
              performed. The only conceivable use of this is (a) a multiplayer
              server  (b)  to test the speed of the other routines in the pro-
              gram, when combined with -timedemo.

       -bexout bexdbg
              Causes diagnostics related to bex and dehacked  file  processing
              to be written to the names file.

More Information
       wget(1), unzip(1), boom.cfg(5), prboom-game-server(6)

       For  more  information,  see the README that came with PrBoom, the Boom
       documentation, and your original Doom documentation.

       Doom is a  registered  trademark  of  id  software  (http://www.idsoft-
       ware.com/).

Author
       See the file AUTHORS included with the PrBoom distribution.



                                     local                           PRBOOM(6)
