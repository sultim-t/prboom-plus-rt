## Process this file with automake to produce Makefile.in
#
# Makefile.am for LxDoom doc directory
#
#

docdir=$(prefix)/share/doc/@PACKAGE@-@VERSION@
dist_doc_DATA=boom.txt MBF.txt MBFFAQ.txt README.demos README.compat \
	prboom-plus-history.txt \
	prboom-plus-history.html \
	prboom-plus-usage.txt

dist_man5_MANS = prboom-plus.cfg.5
dist_man6_MANS = prboom-plus.6 prboom-plus-game-server.6

README.command-line: prboom-plus.cfg.5 prboom-plus.6 prboom-plus-game-server.6
	(echo ".ll +12" && cat *.5 *.6) | groff -Tascii -mandoc | perl -p -e 's/.\x08//g' > README.command-line

