cmake_minimum_required (VERSION 2.8)
project(arc-gui-clients)

if (APPLE)
SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -headerpad_max_install_names" )
SET( CMAKE_SKIP_INSTALL_RPATH ON )
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
   # No rpath on Darwin. Setting it will only cause trouble.
else(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
   option(USE_RPATH "Use -rpath when linking libraries, executables" ON)
endif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
endif (APPLE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(OpenSSL REQUIRED)
set(QT_MAKE_EXECUTABLE "/opt/local/bin/qmake")
find_package(Qt4 COMPONENTS QtMain QtCore QtGui QtSql QtXml QtNetwork QtWebKit REQUIRED)
find_package(Glib REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Glibmm REQUIRED)
find_package(Arc REQUIRED)
find_package(NSS REQUIRED)
find_package(NSPR REQUIRED)

configure_file(${CMAKE_SOURCE_DIR}/src/common/arc-gui-config.h.in ${CMAKE_SOURCE_DIR}/src/common/arc-gui-config.h)

include(${QT_USE_FILE})

subdirs(src)

set(CPACK_GENERATOR "TGZ;RPM")
set(CPACK_PACKAGE_NAME "arc-gui-clients")
set(CPACK_PACKAGE_VENDOR "lunarc.lu.se")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Graphical clients for the ARC middleware")
set(CPACK_PACKAGE_VERSION "0.4.6")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "4")
set(CPACK_PACKAGE_VERSION_PATCH "6")
SET(CPACK_BINARY_RPM "ON")
set(CPACK_RPM_PACKAGE_VENDOR "Lunarc, Lund University")
set(CPACK_RPM_PACKAGE_LICENSE "Apache License V2.0")
set(CPACK_RPM_PACKAGE_DESCRIPTION "Graphical clients for the ARC middleware")
set(CPACK_RPM_PACKAGE_REQUIRES "nordugrid-arc-plugins-globus >= 2.0, nordugrid-arc >= 2.0, nordugrid-arc-plugins-needed >= 2.0, nordugrid-arc-client >= 2.0, qt >= 4.6, qt-x11 >= 4.6, openssl >= 1.0")
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
SET(CPACK_RPM_PACKAGE_DEBUG 1)
include(CPack)

include(CheckFileOffsetBits)
CHECK_FILE_OFFSET_BITS()
IF(__WITH_FILE_OFFSET_BITS_64)
ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)
ENDIF(__WITH_FILE_OFFSET_BITS_64)
