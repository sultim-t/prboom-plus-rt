# UMAPINFO Specification Rev 2.1
Contents:
- [Map Entry](#map-entry)
- [Keys](#keys)
- [Default Handling](#default-handling)
- [Example](#example)
- [Revisions](#revisions)

## Map entry
```
MAP mapname
{
    key = value
    key = value1, value2,...
    ...
}
```
Values can be strings, enclosed in quotation marks (`"`), numbers or identifiers.
Identifiers are case insensitive names that start with a letter and may only
contain letters, numbers or the underscore (`_`).

The map names are limited to the format of the currently loaded IWAD, i.e. Doom
2 only supports MAPxx entries and Doom 1 only ExMy entries. The numbers x and y
can exceed their original limits, though, so MAP50, E5M6 or even MAP100 or
E1M10 are valid map names for their respective game. This limit comes from the
game using numeric variables 'gameepisode' and 'gamemap' to identify a level.
It may later be decided to lift the naming restriction but this cannot be done
without some extensive refactoring which simply exceeds the scope of the
initial implementation.

## Keys
Currently the following keys are supported. If not given, the hardcoded default
will be used, unless the following list says differently.

### Levelname
`levelname = "name"`  
Specifies the readable name of the level (e.g. "Hangar") This will be used in
the automap and also on the status screen if no name patch is specified.

### Label
`label = "name"`  
Specifies the string to prepend to the levelname on the automap. If not
specified the mapname will be used by default followed by a colon and a space
character (e.g. "E1M1: ").

`label = clear`  
Only print the levelname on the automap.

### LevelPic
`levelpic = "graphic"`  
Specifies the patch that is used on the status screen for 'entering'
and 'finished'. This can be omitted, in that case the levelname will be printed
with the HUD font.

### Next
`next = "mapname"`  
Specifies the map the regular exit leads to. In Doom 1 this may cross episodes.

### NextSecret
`nextsecret = "mapname"`  
Specifies the map the secret exit leads to. In Doom 1 this may cross episodes.

### SkyTexture
`skytexture = "texture"`  
Specifies the sky texture to be used for this map.

### Music
`music = "song"`  
Specifies the music to be played on this map.

### ExitPic
`exitpic = "graphic"`  
Specifies the background for the 'level finished' screen. This can override
Doom's animated screens for E1-3.

### EnterPic
`enterpic = "graphic"`  
Specifies the background for the 'entering level' screen. This can override
Doom's animated screens for E1-3.

### ParTime
`partime = seconds`  
Specifies the level's par time.

### EndGame
`endgame = false`  
Overrides a default map exit (e.g. ExM8 or MAP30)

`endgame = true`  
Ends the game after this level, showing the default post-game screen for the
current episode. Skips the 'entering level' screen.

### EndPic
`endpic = "graphic"`  
Ends the game after this level, showing the specified graphic as an end screen.
Skips the 'entering level' screen.

### EndBunny
`endbunny = true`  
Ends the game after this level, showing the bunny scroller. Skips the 'entering
level' screen.

### EndCast
`endcast = true`  
Ends the game after this level, showing the cast call. Skips the 'entering
level' screen.

### NoIntermission
`nointermission = true/false`  
Currently only working for levels that end the game: When true skips the 'level
finished' screen.

### InterText
`intertext = "text"`  
Shows an intermission text screen after the level is exited through the regular
exit. "text" can be multiple lines, for ease of reading they can be specified
as multiple parameters over several lines (see example below.)

`intertext = clear`  
Disables default intermission text for the given map (e.g. to go from MAP06 to
MAP07 without a text showing up.)

### InterTextSecret
`intertextsecret = "text"`  
Shows an intermission text screen after the level is exited through the secret
exit. This will never default to 'intertext'. If not given, the defaults will
be used.

`intertextsecret = clear`  
Disables default intermission text for the given map's secret exit.

### InterBackdrop
`interbackdrop = "graphic"`  
Backdrop to be used for intertext and intertextsecret. If it does not specify a
valid flat, it will be drawn as a patch instead. If not specified and there is
no default for current map, the FLOOR4_8 flat will be used.

### InterMusic
`intermusic = "song"`  
Music to be used for intertext and intertextsecret. If not specified
D_VICTOR/D_READ_M will be used, depending on the IWAD.

### Episode
`episode = "patch", "name", "key"`  
Defines an entry for the episode menu. If all defined episodes define a valid
patch, those will be shown, otherwise the names will be used with the HUD font.
At most 8 episodes can be defined.

`episode = clear`  
Clears the episode menu of all previous entries. Should be used on the first map
if a mod wants to define its own episodes. Doom 2 and Chex Quest have no
episodes by default.

### BossAction
`bossaction = thingtype, linespecial, tag`  
Defines a boss death action, overriding any map default actions. Tag 0 is not
allowed except for level exits. Shoot triggers, teleporters and locked doors
are not supported. A map may define multiple death actions. Thingtype uses
ZDoom's class names (see list below.)

`bossaction = clear`  
Disables any previously-defined boss actions (including map defaults) for the
given map.

## Default handling

Normally, if some information cannot be found, the engine will fall back to its
hard coded implementation, with a few exceptions:

- `nextsecret` If not present, it will use the normal exit's map if the current
  map has a MAPINFO record. This also applies to maps which by default have a
  secret exit!

- `enterpic` If the map that was just left has an exitpic entry and the map to
  be entered has no enterpic entry, the previous map's exitpic entry will be
  used for both screens.

- `levelpic` If not given, the status screen will instead print the map's name
  with a suitable font (PrBoom uses STFxxx) to ensure that the proper name is
  used.

## Example
```
MAP E1M7
{
    levelname = "The Hidden Cave"
    skytexture =  "sky2"
    intertext = "You have beaten the shit",
        "out of those big barons",
        "and now must continue the fight."
}
```

## Thingtypes
```
    DoomPlayer
    ZombieMan
    ShotgunGuy
    Archvile
    ArchvileFire
    Revenant
    RevenantTracer
    RevenantTracerSmoke
    Fatso
    FatShot
    ChaingunGuy
    DoomImp
    Demon
    Spectre
    Cacodemon
    BaronOfHell
    BaronBall
    HellKnight
    LostSoul
    SpiderMastermind
    Arachnotron
    Cyberdemon
    PainElemental
    WolfensteinSS
    CommanderKeen
    BossBrain
    BossEye
    BossTarget
    SpawnShot
    SpawnFire
    ExplosiveBarrel
    DoomImpBall
    CacodemonBall
    Rocket
    PlasmaBall
    BFGBall
    ArachnotronPlasma
    BulletPuff
    Blood
    TeleportFog
    ItemFog
    TeleportDest
    BFGExtra
    GreenArmor
    BlueArmor
    HealthBonus
    ArmorBonus
    BlueCard
    RedCard
    YellowCard
    YellowSkull
    RedSkull
    BlueSkull
    Stimpack
    Medikit
    Soulsphere
    InvulnerabilitySphere
    Berserk
    BlurSphere
    RadSuit
    Allmap
    Infrared
    Megasphere
    Clip
    ClipBox
    RocketAmmo
    RocketBox
    Cell
    CellPack
    Shell
    ShellBox
    Backpack
    BFG9000
    Chaingun
    Chainsaw
    RocketLauncher
    PlasmaRifle
    Shotgun
    SuperShotgun
    TechLamp
    TechLamp2
    Column
    TallGreenColumn
    ShortGreenColumn
    TallRedColumn
    ShortRedColumn
    SkullColumn
    HeartColumn
    EvilEye
    FloatingSkull
    TorchTree
    BlueTorch
    GreenTorch
    RedTorch
    ShortBlueTorch
    ShortGreenTorch
    ShortRedTorch
    Stalagtite
    TechPillar
    CandleStick
    Candelabra
    BloodyTwitch
    Meat2
    Meat3
    Meat4
    Meat5
    NonsolidMeat2
    NonsolidMeat4
    NonsolidMeat3
    NonsolidMeat5
    NonsolidTwitch
    DeadCacodemon
    DeadMarine
    DeadZombieMan
    DeadDemon
    DeadLostSoul
    DeadDoomImp
    DeadShotgunGuy
    GibbedMarine
    GibbedMarineExtra
    HeadsOnAStick
    Gibs
    HeadOnAStick
    HeadCandles
    DeadStick
    LiveStick
    BigTree
    BurningBarrel
    HangNoGuts
    HangBNoBrain
    HangTLookingDown
    HangTSkull
    HangTLookingUp
    HangTNoBrain
    ColonGibs
    SmallBloodPool
    BrainStem
    PointPusher
    PointPuller
    MBFHelperDog
    PlasmaBall1
    PlasmaBall2
    EvilSceptre
    UnholyBible
    MusicChanger
    Deh_Actor_145
    [...]
    Deh_Actor_249
```
## Revisions

Rev 2.1 (@rfomin, June 22 2021)
 * Lookup for default backdrop if `interbackdrop` is not specified.

Rev 2 (@fabiangreffrath, May 11 2021)
 * Introduce the new `label` field to allow for overriding of the string that
   gets prepended to the `levelname` on the automap.

Rev 1.6 (@fabiangreffrath, Apr 19 2021)
 * Skip the 'entering level' screen if one of `endgame`, `endpic`, `endbunny` or
   `endcast` is set.

Rev 1.5 (@Shadow-Hog, Apr 17 2021)
 * Add in all the additional actor names for DEHEXTRA.

Rev 1.4 (@rfomin, Mar 23 2021)
 * Clarify the `episode` field in the case of Doom 2 and Chex Quest.

Rev 1.3 (@fabiangreffrath, Mar 5 2021)
 * If `interbackdrop` does not specify a valid flat, draw it as a patch instead.

Rev 1.2 (@JadingTsunami, Feb 12 2021)
 * Fix typo in ZDoom-style Actor name.

Rev 1.1 (@XaserAcheron, Jan 24 2021)
 * Updates to the UMAPINFO docs to disambiguate the `bossaction` field a bit.

Rev 1 (@coelckers, Jul 10 2017)
 * Updated UMAPINFO spec to curly brace syntax.

Rev 0 (@coelckers, Apr 22 2017)
 * Initial implementation.
