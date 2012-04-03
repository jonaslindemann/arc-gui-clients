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

set(CPACK_PACKAGE_NAME "arc-gui-clients")
set(CPACK_PACKAGE_VENDOR "lunarc.lu.se")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Graphical clients for the ARC middleware")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CPack Component Example")
set(CPACK_SOURCE_GENERATOR TGZ)
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
        README
        xcodeproj$
        ./builddir/
        ./bin/
)
include(CPack)
