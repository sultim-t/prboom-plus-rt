## Process this file with automake to produce Makefile.in
#
# Makefile.am for LxDoom doc directory
#
# $Id: Makefile.am,v 1.6 2000/08/27 16:25:29 cph Exp $
#

docdir=$(prefix)/share/doc/@PACKAGE@
man_MANS=prboom.6 boom.cfg.5 prboom-game-server.6
doc_DATA=boom.txt MBF.txt MBFFAQ.txt README.demos README.compat
EXTRA_DIST=boom.txt prboom.6 boom.cfg.5 prboom-game-server.6 \
	MBF.txt MBFFAQ.txt README.demos README.compat

README.command-line: prboom.6 boom.cfg.5 prboom-game-server.6
	cat *.6 *.5 | groff -Tascii -mandoc | perl -p -e 's/.\x08//g' > README.command-line

