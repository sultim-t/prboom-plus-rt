PrBoom demo support
===================

PrBoom has lots of demo options compared to most Doom ports, so I decided to
write a brief document outlining the features. If you're thinking of
recording demos, you're strongly advised to read this.

To play demos, you of course use "prboom -playdemo demoname" (the .lmp is
optional). To the best of my knowledge, PrBoom should play all MBF and new
PrBoom demos perfectly. It also plays most Doom v1.9 demos (significantly more
than Boom or MBF); almost all demos recorded with ultimate doom or doom2.exe
version v1.9 should work (please let me know if you find ones that don't), but
there are still problems with doom95 and final doom though. It should play most
Boom v2.01 and v2.02 demos, although I don't know of that many to test with. It
will also try to play older Doom demos, LxDoom demos, and Boom v2.00 demos.

Recording demos is done with "prboom -record demoname". **IMPORTANT** PrBoom
will record to a number of demo formats, namely Doom v1.9, Boom v2.02, MBF,
and PrBoom - which format it records is determined by your current
compatibility level, see README.compat. The purpose of this feature is so
people like me that won't let DOS anywhere near their computers can record
demos compatible with other engines.

PrBoom supports the -recordfrom parameter, which starts recording a demo
from a given savegame. "-recordfrom demoname n" is a synonym for "-record
demoname -loadgame n", which records a demo from savegame number n.

People doing speedrun demos may be interested in the comp_moveblock option,
see README.compat.

PrBoom supports loading and saving games during demo recording and playback
(but only for new PrBoom demos, not for older demo formats, obviously).
Don't worry, saves in demo playback are written with different names, they
don't overwrite your own saves. Saves when recording, do. You can load games
during recording, but they'd better be games saved during the same
recording, otherwise it won't play back.

PrBoom supports continuation of recorded demos. This is the scenario this
feature is designed for: say you're beta testing a level for someone, and
you want to send them a *complete* demo of how the level went (saves, deaths
and reloads and all). You start with "prboom test.wad -record test", play
for half an hour, save and exit. Next day, you go to play again, with
"prboom test.wad -record test" again. PrBoom will scan the existing
test.lmp, find the savegame command, and insert a loadgame command to reload
that game; it will reload the game and continue recording of the demo. This
is an experimental feature, but hopefully it works :-).

**IMPORTANT** The observant will have noticed from the above that PrBoom
won't overwrite existing demos anymore. If you want to record over a demo,
delete it first.

I think that's all for now.

- Colin <doom@cph.demon.co.uk>
