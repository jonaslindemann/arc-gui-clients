cmake_minimum_required (VERSION 2.6)
project(arc-gui-clients)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(OpenSSL REQUIRED)
find_package(Qt4 COMPONENTS QtMain QtCore QtGui QtSql QtXml QtNetwork REQUIRED)
find_package(Glib REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Glibmm REQUIRED)
find_package(Arc REQUIRED)

include(${QT_USE_FILE})
#message(${QT_USE_FILE})

subdirs(src)

set(CPACK_GENERATOR "TGZ;RPM")
set(CPACK_PACKAGE_NAME "arc-gui-clients")
set(CPACK_PACKAGE_VENDOR "lunarc.lu.se")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Graphical clients for the ARC middleware")
set(CPACK_PACKAGE_VERSION "0.4.0")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "4")
set(CPACK_PACKAGE_VERSION_PATCH "0")
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
include(CPack)

include(CheckFileOffsetBits)
CHECK_FILE_OFFSET_BITS()
IF(__WITH_FILE_OFFSET_BITS_64)
ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)
ENDIF(__WITH_FILE_OFFSET_BITS_64)
