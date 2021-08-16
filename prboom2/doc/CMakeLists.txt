set(DOC_FILES
    ../NEWS
    boom.txt
    DeePBSPV4specs.txt
    MBFFAQ.txt
    MBF.txt
    prboom-plus-usage.txt
    README.command-line
    README.compat
    README.demos
    umapinfo.txt
)
set(MAN5_FILES
    prboom-plus.cfg.5
)
set(MAN6_FILES
    prboom-plus.6
    prboom-plus-game-server.6
)

if(${CMAKE_VERSION} VERSION_LESS "3.14.0")
	install(FILES ${DOC_FILES} DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT "Documentation")
else()
	install(FILES ${DOC_FILES} TYPE DOC COMPONENT "Documentation")
endif()
install(FILES ${MAN5_FILES} DESTINATION "${CMAKE_INSTALL_MANDIR}/man5" COMPONENT "Manpages")
install(FILES ${MAN6_FILES} DESTINATION "${CMAKE_INSTALL_MANDIR}/man6" COMPONENT "Manpages")
