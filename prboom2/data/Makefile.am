#
#
# Install boomlump.wad to /usr/local/share/doom
#

waddir=$(datadir)/games/doom
wad_DATA=prboom-plus.wad
EXTRA_DIST=prboom-plus.wad prboom.txt

prboom-plus.wad: prboom.txt $(wildcard graphics/*.ppm sprites/*.ppm lumps/*.lmp sounds/*.wav)
	-rm -f $@
	deutex -make prboom.txt $@
