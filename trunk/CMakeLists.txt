cmake_minimum_required (VERSION 2.6)
project(arc-gui-clients)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
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


