add_definitions(-DQT_NO_KEYWORDS)

include(${QT_USE_FILE})

SET(arcsubui_UIS
    jobdefinitionwindow.ui
)

SET(arcsubui_SRCS
  jobdefinitionwindow.cpp
  jobdefinitions.cpp
  main.cpp
)

SET(arcsubui_MOC_HDRS
  jobdefinitions.h
  jobdefinitionwindow.h
)

#SET(arccertui_RCS 
#  arcproxy-ui.qrc
#)

# After this call, foo_MOC_SRCS = moc_Class1.cxx moc_Class2.cxx moc_Class3.cxx.
QT4_WRAP_CPP(arcsubui_MOC_SRCS ${arcsubui_MOC_HDRS} )
QT4_WRAP_UI(arcsubui_UIS_H ${arcsubui_UIS} )
QT4_ADD_RESOURCES( arcsubui_RC_SRCS ${arcsubui_RCS} )

IF(APPLE)
ADD_EXECUTABLE(
	arcsub-ui 
	MACOSX_BUNDLE
	${arcproxyui_SRCS} ${arcproxyui_MOC_SRCS} ${arcproxyui_RC_SRCS} ${arcproxyui_UIS_H}
    ../../images/arccert_icon.icns
)

install(TARGETS arcsub-ui
   BUNDLE
   DESTINATION .
)

SET( MACOSX_BUNDLE_ICON_FILE arcsub_icon.icns )
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
        ../../images/arcsub_icon.icns
	PROPERTIES
	MACOSX_PACKAGE_LOCATION Resources
)
ELSE(APPLE)
	add_executable(arcsub-ui ${arcsubui_SRCS} ${arcsubui_MOC_SRCS} ${arcsubui_RC_SRCS} ${arcsubui_UIS_H} )
ENDIF(APPLE)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${Arc_INCLUDE_DIRS} ${GlibmmConfig_INCLUDE_DIR} ${Glibmm_INCLUDE_DIR} ${GlibConfig_INCLUDE_DIR} ${Glib_INCLUDE_DIR} ${SigC++Config_INCLUDE_DIR} ${SigC++_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})

target_link_libraries (arcsub-ui ${Arc_LIBRARIES} ${Glibmm_LIBRARY} ${Glib_LIBRARIES} ${SigC++_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_SSL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES})

#if(LINUX)
install(TARGETS arcsub-ui DESTINATION bin)
install(FILES arcsub-ui.desktop DESTINATION share/applications)
#endif(LINUX)
