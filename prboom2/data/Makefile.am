#
# $Id: Makefile.am,v 1.4 2001/11/13 21:38:02 cph Exp $
#
# Install boomlump.wad to /usr/local/share/doom
#

waddir=$(datadir)/games/doom
wad_DATA=prboom.wad
EXTRA_DIST=prboom.wad prboom.txt

prboom.wad      :       prboom.txt $(wildcard graphics/*.ppm sprites/*.ppm lumps/*.lmp sounds/*.wav)
	-rm -f $@
	deutex -make prboom.txt $@

#
# $Log: Makefile.am,v $
# Revision 1.4  2001/11/13 21:38:02  cph
# Must distribute prboom.txt so the make can pass prboom.wad
#
# Revision 1.3  2001/07/19 20:38:37  cph
# Generate prboom.wad from a DeuTex tree
# Must be the biggest CVS commit I've ever made :-)
#
# Revision 1.2  2000/05/18 09:36:11  cph
# Import new data WAD with Boom, MBF and PrBoom stuff
#
# Revision 1.1.1.1  2000/05/04 07:54:32  proff_fs
# initial login on sourceforge as prboom2
#
# Revision 1.1  1999/10/01 08:13:39  cphipps
# Initial revision
#
#
