add_definitions(-DQT_NO_KEYWORDS)

include(${QT_USE_FILE})

SET(arcstatui_UIS
    jobstatuswindow.ui
)

SET(arcstatui_SRCS
  JmBase.cpp
  jobstatuswindow.cpp
  main.cpp
  arcjobcontroller.cpp
)

SET(arcstatui_MOC_HDRS
  JmBase.h
  jobstatuswindow.h
  arcjobcontroller.h
)

SET(arcstatui_RCS 
  arcstat-ui.qrc
)

# After this call, foo_MOC_SRCS = moc_Class1.cxx moc_Class2.cxx moc_Class3.cxx.
QT4_WRAP_CPP(arcstatui_MOC_SRCS ${arcstatui_MOC_HDRS} )
QT4_WRAP_UI(arcstatui_UIS_H ${arcstatui_UIS} )
QT4_ADD_RESOURCES( arcstatui_RC_SRCS ${arcstatui_RCS} )

IF(APPLE)
	add_executable(arcstat-ui 
	MACOSX_BUNDLE 
	${arcstatui_SRCS} ${arcstatui_MOC_SRCS} ${arcstatui_RC_SRCS} ${arcstatui_UIS_H} 
	../../images/arcstat_icon.icns )

install(TARGETS arcstat-ui
   BUNDLE
   DESTINATION .
)

SET( MACOSX_BUNDLE_ICON_FILE arcstat_icon.icns )
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
        ../../images/arcstat_icon.icns
	PROPERTIES
	MACOSX_PACKAGE_LOCATION Resources
)
ELSE(APPLE)
	add_executable(arcstat-ui ${arcstatui_SRCS} ${arcstatui_MOC_SRCS} ${arcstatui_RC_SRCS} ${arcstatui_UIS_H} )
ENDIF(APPLE)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${Arc_INCLUDE_DIRS} ${GlibmmConfig_INCLUDE_DIR} ${Glibmm_INCLUDE_DIR} ${GlibConfig_INCLUDE_DIR} ${Glib_INCLUDE_DIR} ${SigC++Config_INCLUDE_DIR} ${SigC++_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})

target_link_libraries (arcstat-ui ${Arc_LIBRARIES} ${Glibmm_LIBRARY} ${Glib_LIBRARIES} ${SigC++_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_LIBRARIES})

#if (LINUX)
install(TARGETS arcstat-ui DESTINATION bin)
install(FILES arcstat-ui.desktop DESTINATION share/applications)
#endif(LINUX)
