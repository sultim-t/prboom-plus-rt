#
# Install, and make if needed, prboom.wad
#

noinst_PROGRAMS = rdatawad
rdatawad_SOURCES = rd_main.c
rdatawad_SOURCES += rd_util.c rd_util.h
rdatawad_SOURCES += rd_output.c rd_output.h
rdatawad_SOURCES += rd_sound.c rd_sound.h
rdatawad_SOURCES += rd_palette.c rd_palette.h
rdatawad_SOURCES += rd_graphic.c rd_graphic.h

waddir=$(DOOMWADDIR)
wad_DATA=prboom.wad
EXTRA_DIST=prboom.wad prboom.txt
MAINTAINERCLEANFILES=prboom.wad

prboom.wad      :       prboom.txt $(wildcard graphics/*.ppm sprites/*.ppm lumps/*.lmp sounds/*.wav)
	-rm -f $@
	deutex -make prboom.txt $@
