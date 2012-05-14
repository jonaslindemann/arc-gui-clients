add_definitions(-DQT_NO_KEYWORDS)

include(${QT_USE_FILE})

set(arcstorageui_UIS
    mainwindow.ui
    filepropertiesdialog.ui
    srmsettingsdialog.ui
)

set(arcstorageui_SRCS
    main.cpp
    mainwindow.cpp
    fileserver.cpp
    localfileserver.cpp
    arcfileelement.cpp
    ftpfileserver.cpp
    dragdropabletreewidget.cpp
    fileserverfactory.cpp
    filepropertiesdialog.cpp
    srmfileserver.cpp
    srmsettingsdialog.cpp
    settings.cpp
    filetransfer.cpp
)

set(arcstorageui_MOC_HDRS
    mainwindow.h
    ftpfileserver.h
    filepropertiesdialog.h
    srmfileserver.h
    srmsettingsdialog.h
)

#SET(arcexplorerui_RCS 
#  arcexplorer-ui.qrc
#)

# After this call, foo_MOC_SRCS = moc_Class1.cxx moc_Class2.cxx moc_Class3.cxx.
QT4_WRAP_CPP(arcstorageui_MOC_SRCS ${arcstorageui_MOC_HDRS} )
QT4_WRAP_UI(arcstorageui_UIS_H ${arcstorageui_UIS} )
#QT4_ADD_RESOURCES( arcstorageui_RC_SRCS ${arcstorageui_RCS} )

IF(APPLE)
	add_executable(arcstorage-ui 
	MACOSX_BUNDLE ${arcstorageui_SRCS} ${arcstorageui_MOC_SRCS} ${arcstorageui_RC_SRCS} ${arcstorageui_UIS_H} 
	../../images/arcstorage_icon.icns )

install(TARGETS arcstorage-ui
   BUNDLE
   DESTINATION .
)

SET( MACOSX_BUNDLE_ICON_FILE arcstorage_icon.icns )
SET( MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1 )
SET( MACOSX_BUNDLE_VERSION 0.1 )
SET( MACOSX_BUNDLE_LONG_VERSION_STRING 0.1 )

#SET_SOURCE_FILES_PROPERTIES(
#        ../../bin/release/brushes/rbrush16.rgb
#        ../../bin/release/brushes/rbrush32.rgb
#        ../../bin/release/brushes/rbrush4.rgb
#        ../../bin/release/brushes/rbrush64.rgb
#        ../../bin/release/brushes/rbrush8.rgb
#        ../../bin/release/brushes/sbrush16.rgb
#        ../../bin/release/brushes/sbrush32.rgb
#        ../../bin/release/brushes/sbrush4.rgb
#        ../../bin/release/brushes/sbrush64.rgb
#        ../../bin/release/brushes/sbrush8.rgb
#	PROPERTIES
#	MACOSX_PACKAGE_LOCATION MacOS/brushes
#)
#SET_SOURCE_FILES_PROPERTIES(
#        ../../bin/release/colormaps/colormap1.map
#        ../../bin/release/colormaps/colormap2.map
#	PROPERTIES
#	MACOSX_PACKAGE_LOCATION MacOS/colormaps
#)
SET_SOURCE_FILES_PROPERTIES(
        ../../images/arcstorage_icon.icns
	PROPERTIES
	MACOSX_PACKAGE_LOCATION Resources
)
ELSE(APPLE)
	add_executable(arcstorage-ui ${arcstorageui_SRCS} ${arcstorageui_MOC_SRCS} ${arcstorageui_UIS_H} )
ENDIF(APPLE)


include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${Arc_INCLUDE_DIRS} ${GlibmmConfig_INCLUDE_DIR} ${Glibmm_INCLUDE_DIR} ${GlibConfig_INCLUDE_DIR} ${Glib_INCLUDE_DIR} ${SigC++Config_INCLUDE_DIR} ${SigC++_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})

target_link_libraries (arcstorage-ui ${Arc_LIBRARIES} ${Glibmm_LIBRARY} ${Glib_LIBRARIES} ${SigC++_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_LIBRARIES})

#if (LINUX)
install(TARGETS arcstorage-ui DESTINATION bin)
#endif(LINUX)