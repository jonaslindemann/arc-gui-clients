cmake_minimum_required (VERSION 3.0)

project(arc-gui-clients)

# --- Apple fixes

if (APPLE)

    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -headerpad_max_install_names" )
    set(CMAKE_SKIP_INSTALL_RPATH ON )

    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        # No rpath on Darwin. Setting it will only cause trouble.
    else(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        option(USE_RPATH "Use -rpath when linking libraries, executables" ON)
    endif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")

endif (APPLE)

# --- Custom CMake modules

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(CPack)
include(CheckFileOffsetBits)

# --- Check if running on Linux

if(UNIX AND NOT APPLE)
   set(LINUX TRUE)
endif()

# --- Check for packages requirements

find_package(OpenSSL REQUIRED)
find_package(Qt4 COMPONENTS QtMain QtCore QtGui QtSql QtXml QtNetwork QtWebKit REQUIRED)
find_package(Glib REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Glibmm REQUIRED)
find_package(Arc REQUIRED)

# --- Handle Linux quirks

if (LINUX)

    find_program(LSB_RELEASE_EXEC lsb_release)

    # Find Linux release name

    execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
      OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Find Linux release version

    execute_process(COMMAND ${LSB_RELEASE_EXEC} -rs
      OUTPUT_VARIABLE LSB_RELEASE_VERSION_SHORT
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Split into major/minor/patch

    string(REPLACE "." ";" VERSION_LIST ${LSB_RELEASE_VERSION_SHORT})
    list(GET VERSION_LIST 0 LSB_RELEASE_VERSION_MAJOR)
    list(GET VERSION_LIST 1 LSB_RELEASE_VERSION_MINOR)
    list(GET VERSION_LIST 2 LSB_RELEASE_VERSION_PATCH)

    # Make sure we use the right ABI version for CentOS 7

    if (LSB_RELEASE_ID_SHORT STREQUAL "CentOS")
      if (LSB_RELEASE_VERSION_MAJOR STREQUAL "7")
         message("-- Fixing CentOS 7 ABI compatibility.")
         add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
      endif()
    endif()
endif()

# --- Configure arc-gui-config

configure_file(${CMAKE_SOURCE_DIR}/src/common/arc-gui-config.h.in ${CMAKE_SOURCE_DIR}/src/common/arc-gui-config.h)

# --- Required Qt stuff

include(${QT_USE_FILE})

# --- Go an build in src

subdirs(src)

set(CPACK_GENERATOR "TGZ;RPM")
set(CPACK_PACKAGE_NAME "arc-gui-clients")
set(CPACK_PACKAGE_VENDOR "lunarc.lu.se")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Graphical clients for the ARC middleware")
set(CPACK_PACKAGE_VERSION "0.5.0")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "5")
set(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_BINARY_RPM "ON")
set(CPACK_RPM_PACKAGE_VENDOR "LUNARC, Lund University")
set(CPACK_RPM_PACKAGE_LICENSE "Apache License V2.0")
set(CPACK_RPM_PACKAGE_DESCRIPTION "Graphical clients for the ARC middleware")
set(CPACK_RPM_PACKAGE_REQUIRES "nordugrid-arc-plugins-globus >= 5.0, nordugrid-arc >= 5.0, nordugrid-arc-plugins-needed >= 5.0, nordugrid-arc-client >= 5.0, qt >= 4.6, qt-x11 >= 4.6, openssl >= 1.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CPack Component Example")
set(CPACK_SOURCE_GENERATOR "TGZ;RPM")
set(CPACK_SOURCE_IGNORE_FILES
        /CVS
        /.svn
        ~$
        .\#
        \#
        .DS_Store
        .project
        ./LLNL-bin
        ./Makefile
        xcodeproj$
        ./builddir/
        ./bin/
)
set(CPACK_RPM_PACKAGE_DEBUG 1)

check_file_offset_bits()

if(__WITH_FILE_OFFSET_BITS_64)
    add_definitions(-D_FILE_OFFSET_BITS=64)
endif(__WITH_FILE_OFFSET_BITS_64)
