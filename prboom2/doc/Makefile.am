## Process this file with automake to produce Makefile.in
#
# Makefile.am for LxDoom doc directory
#
# $Id: Makefile.am,v 1.4 2000/07/22 14:55:21 cph Exp $
#

docdir=$(prefix)/share/doc/@PACKAGE@
man_MANS=prboom.6 boom.cfg.5 prboom-game-server.6
doc_DATA=boom.txt MBF.txt MBFFAQ.txt README.demos README.compat
EXTRA_DIST=boom.txt prboom.6 boom.cfg.5 prboom-game-server.6 \
	MBF.txt MBFFAQ.txt README.demos README.compat

README.command-line: prboom.6 boom.cfg.5 prboom-game-server.6
	cat *.6 *.5 | groff -Tascii -mandoc | perl -e "while (<>) { s/$$/\r/; print; }" > README.command-line

