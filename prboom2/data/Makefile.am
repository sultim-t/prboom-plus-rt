#
# Install, and make if needed, prboom.wad
#

waddir=$(DOOMWADDIR)
wad_DATA=prboom.wad
EXTRA_DIST=prboom.wad prboom.txt

prboom.wad      :       prboom.txt $(wildcard graphics/*.ppm sprites/*.ppm lumps/*.lmp sounds/*.wav)
	-rm -f $@
	deutex -make prboom.txt $@
