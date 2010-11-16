## Process this file with automake to produce Makefile.in
#
# Makefile.am for LxDoom doc directory
#
#

docdir=$(prefix)/share/doc/@PACKAGE@-@VERSION@
dist_man_MANS = prboom.6 prboom.cfg.5 prboom-game-server.6
dist_doc_DATA = boom.txt MBF.txt MBFFAQ.txt README.demos README.compat

README.command-line: prboom.6 prboom.cfg.5 prboom-game-server.6
	(echo ".ll +12" && cat *.5 *.6) | groff -Tascii -mandoc | perl -p -e 's/.\x08//g' > README.command-line

