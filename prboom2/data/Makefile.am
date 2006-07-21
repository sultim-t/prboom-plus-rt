#
#
# Install boomlump.wad to /usr/local/share/doom
#

waddir=$(datadir)/games/doom
wad_DATA=prboom.wad
EXTRA_DIST=prboom.wad prboom.txt

prboom.wad      :       prboom.txt $(wildcard graphics/*.ppm sprites/*.ppm lumps/*.lmp sounds/*.wav)
	-rm -f $@
	deutex -make prboom.txt $@
