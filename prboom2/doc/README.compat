PrBoom compatibility options
============================

Some of this is a bit dry. You might want to skip to the bottom and read the
examples.

Internally, PrBoom maintains a "compatibility level" which it uses to
determine how to behave in various circumstances. The supported levels are
currently:

Number	Name			Description
0	doom_12_compatibility	Emulate Doom v1.2 game details where possible
1	doom_1666_compatibility	Behave like Doom v1.666 as far as possible
2	doom2_19_compatibility	As compatible as possible for playing original 
				Doom demos
3	ultdoom_compatibility	Ultimate Doom made a few changes
4	finaldoom_compatibility	Compatible with Final Doom & Doom95 - needed
				for some demos recorded with Final Doom doom2.exe
5 	dosdoom_compatibility	Emulate early dosdoom versions
				(some TASdoom demos work with this)
6	tasdoom_compatibility	unused
7 	boom_compatibility_compatibility	Emulates Boom's compatibility mode
8	boom_201_compatibility	Emulates Boom v2.01
9	boom_202_compatibility	Emulates Boom v2.02
10	lxdoom_1_compatibility	Emulates LxDoom v1.4.x
11	mbf_compatibility	Emulates MBF
12	prboom_1_compatibility	Emulates PrBoom v2.03beta
13	prboom_2_compatibility	PrBoom v2.1.0
14	prboom_3_compatibility	PrBoom v2.1.1-2.2.6
15	prboom_4_compatibility	PrBoom v2.3.x
16	prboom_5_compatibility	PrBoom v2.4.0
17	prboom_6_compatibility	Current PrBoom

You can cycle through the compatibility levels in the game using the TNTCOMP
cheat. There's also the default_compatibility_level config file option, and
the -complevel command line parameter.

The numbers are subject to change between versions, so if you're doing
elaborate stuff with these things you're advised to check this file each
time you upgrade. Most people should just leave default_compatibility_level
set to -1 in the config file, which means "use most recent" which has most
features, most bug fixes, etc.

But some people want to test the behaviour of levels with older versions, and
will find it helpful to not have to load a dozen games to do so. Some people
like me play a lot of old levels, so need to be able to enable compatibility
with some old bugs. And some people may want to record demos for older game
versions.

When you play a demo, PrBoom sets the compatibility level and settings 
automatically. When you load a savegame, the settings are restored.

When you start a new game, the compatibility level is got from (in order of
preference):
- -complevel parameter
- default_compatibility_level config file param if not -1
- set to the most recent otherwise

If the compatibility level is MBF or better, the detailed compatibility
settings are read from the comp_* config file options, as in MBF. Otherwise,
the settings are put to the defaults for that game version.

You can adjust the compatibility settings during play from the menus
(Options->Setup->Doom Compatibility). 

The sort of people interested in these things will already know the MBF
options, so I'll just list the changes:
- comp_floors now also causes the original Doom behaviour of objects stuck in
a ceiling preventing the floor below lowering.
- new option comp_moveblock enables the old movement clipping bug which
allows things to go through walls sometimes (mancubus shots, and key
grabbing tricks)

PrBoom can also record old demos with some success (demo players see the
comp_moveblock note above):

prboom -complevel 2 -record test

records a doom v1.9 compatible demo

prboom -complevel 9 -record whatever

records a Boom (v2.02) demo

prboom -complevel 11 -record blah

records an MBF demo.

Of course, demo recording is no more reliable than demo playback; original
Doom demos will usually but not always work. MBF support should be perfect.

- Colin <cph@moria.org.uk>

