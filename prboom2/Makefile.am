#
# Top level automake Makefile.am for LxDoom
#
# $Id: Makefile.am,v 1.1 2000/05/04 07:53:10 proff_fs Exp $
#

docdir=$(prefix)/share/doc/@PACKAGE@
SUBDIRS = doc data src
EXTRA_DIST = ChangeLog lxdoom.spec.in
doc_DATA = ChangeLog README COPYING AUTHORS

rpm:	lxdoom.spec dist
	mv @PACKAGE@-@VERSION@.tar.gz /usr/src/redhat/SOURCES
	rpm -bb lxdoom.spec

#
# $Log: Makefile.am,v $
# Revision 1.1  2000/05/04 07:53:10  proff_fs
# Initial revision
#
# Revision 1.5  2000/03/19 16:28:37  cph
# Fix documentation directory
#
# Revision 1.4  2000/03/16 20:14:08  cph
# Add docs to make install, add RPM building
#
# Revision 1.3  1999/10/12 17:48:37  cphipps
# Remove CREDITS
#
# Revision 1.2  1999/10/01 20:38:00  cphipps
# Added ChangeLog to distributed files
#
# Revision 1.1  1999/09/10 20:08:16  cphipps
# Initial revision
#
#
