#
# Top level automake Makefile.am for LxDoom
#
#

# From "GNU autoconf, automake and libtool" Vaughan, Elliston, 
#  Tromey and Taylor, publisher New Riders, p.134
# They made me do it! :-)
AUX_DIST_GEN =				\
	$(ac_aux_dir)/config.guess	\
	$(ac_aux_dir)/config.sub	\
	$(ac_aux_dir)/install-sh	\
	$(ac_aux_dir)/missing

MAINTAINERCLEANFILES =  $(AUX_DIST_GEN)

docdir=$(prefix)/share/doc/@PACKAGE@-@VERSION@
SUBDIRS = doc data src ICONS VisualC8 VisualC6
EXTRA_DIST = prboom.spec.in prboom.spec config.h bootstrap $(AUX_DIST_GEN)
doc_DATA = README COPYING AUTHORS NEWS

