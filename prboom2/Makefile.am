#
# Top level automake Makefile.am for LxDoom
#
# $Id: Makefile.am,v 1.5 2000/11/05 14:05:31 cph Exp $
#

docdir=$(prefix)/share/doc/@PACKAGE@
SUBDIRS = doc data src ICONS VISUALC5 VISUALC6
EXTRA_DIST = ChangeLog prboom.spec.in config.h
doc_DATA = README COPYING AUTHORS NEWS

rpm:	prboom.spec dist
	mv @PACKAGE@-@VERSION@.tar.gz /usr/src/redhat/SOURCES
	rpm -bb prboom.spec

#
# $Log: Makefile.am,v $
# Revision 1.5  2000/11/05 14:05:31  cph
# Update changelog, include in RPM/install
#
# Revision 1.4  2000/09/17 10:59:12  cph
# Include all the windows stuff in the distribution
# Fix src/Makefile.am so it includes all the sources
#
# Revision 1.3  2000/09/16 22:39:43  cph
# Don't include out-of-date Changelog
#
# Revision 1.2  2000/07/22 14:55:02  cph
# Add prboom RPM spec file to build scripts
#
# Revision 1.1.1.1  2000/05/04 07:53:10  proff_fs
# initial login on sourceforge as prboom2
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
