include(vcpkg_common_functions)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO kode54/dumb
    REF 2.0.3
    SHA512 18b10a507d69a754cdf97fbeae41c17f211a6ba1f166a822276bdb6769d3edc326919067a3f4d1247d6715d7a5a8276669d83b9427e7336c6d111593fb7e36cf
    HEAD_REF master
    PATCHES
        msvc-build-fixes.patch
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS
        -DBUILD_EXAMPLES=OFF
        -DBUILD_ALLEGRO4=OFF
)

vcpkg_install_cmake()

# Remove unnecessary files
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

if(VCPKG_LIBRARY_LINKAGE STREQUAL static)
    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/bin ${CURRENT_PACKAGES_DIR}/debug/bin)
endif()

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/dumb RENAME copyright)
