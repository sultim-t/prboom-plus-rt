#
# Top level automake Makefile.am for LxDoom
#
# $Id: Makefile.am,v 1.6 2000/11/12 18:02:15 cph Exp $
#

# From "GNU autoconf, automake and libtool" Vaughan, Elliston, 
#  Tromey and Taylor, publisher New Riders, p.134
# They made me do it! :-)
AUX_DIST_GEN =				\
	$(ac_aux_dir)/config.guess	\
	$(ac_aux_dir)/config.sub	\
	$(ac_aux_dir)/install-sh	\
	$(ac_aux_dir)/missing		\
	$(ac_aux_dir)/mkinstalldirs

MAINTAINERCLEANFILES =  $(AUX_DIST_GEN)

docdir=$(prefix)/share/doc/@PACKAGE@
SUBDIRS = doc data src ICONS VISUALC5 VISUALC6
EXTRA_DIST = ChangeLog prboom.spec.in config.h bootstrap $(AUX_DIST_GEN)
doc_DATA = README COPYING AUTHORS NEWS

rpm:	prboom.spec dist
	mv @PACKAGE@-@VERSION@.tar.gz /usr/src/redhat/SOURCES
	rpm -bb prboom.spec

