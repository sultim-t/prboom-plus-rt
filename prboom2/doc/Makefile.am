## Process this file with automake to produce Makefile.in
#
# Makefile.am for LxDoom doc directory
#
# $Id: Makefile.am,v 1.5 2000/08/20 13:27:00 cph Exp $
#

docdir=$(prefix)/share/doc/@PACKAGE@
man_MANS=prboom.6 boom.cfg.5 prboom-game-server.6
doc_DATA=boom.txt MBF.txt MBFFAQ.txt README.demos README.compat
EXTRA_DIST=boom.txt prboom.6 boom.cfg.5 prboom-game-server.6 \
	MBF.txt MBFFAQ.txt README.demos README.compat

README.command-line: prboom.6 boom.cfg.5 prboom-game-server.6
	cat *.6 *.5 | groff -Thtml -mandoc > doc.html
	lynx -nolist -dump doc.html > README.command-line

