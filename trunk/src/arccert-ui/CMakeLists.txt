add_definitions(-DQT_NO_KEYWORDS)

include(${QT_USE_FILE})

SET(arccertui_UIS
    certconvertwindow.ui
)

SET(arccertui_SRCS
  certconvertwindow.cpp
  main.cpp
)

SET(arccertui_MOC_HDRS
  certconvertwindow.h
)

#SET(arccertui_RCS 
#  arcproxy-ui.qrc
#)

# After this call, foo_MOC_SRCS = moc_Class1.cxx moc_Class2.cxx moc_Class3.cxx.
QT4_WRAP_CPP(arccertui_MOC_SRCS ${arccertui_MOC_HDRS} )
QT4_WRAP_UI(arccertui_UIS_H ${arccertui_UIS} )
QT4_ADD_RESOURCES( arccertui_RC_SRCS ${arccertui_RCS} )

IF(APPLE)
	ADD_EXECUTABLE(
		arccert-ui
		MACOSX_BUNDLE
		${arccertui_SRCS} ${arccertui_MOC_SRCS} ${arccertui_RC_SRCS}
		${arccertui_UIS_H}
		../../images/arcproxy_icon.icns
	)

	install(TARGETS arccert-ui
		BUNDLE
		DESTINATION bin
	)

	SET( MACOSX_BUNDLE_ICON_FILE arcproxy_icon.icns )
	SET( MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1 )
	SET( MACOSX_BUNDLE_VERSION 0.1 )
	SET( MACOSX_BUNDLE_LONG_VERSION_STRING 0.1 )
	SET_SOURCE_FILES_PROPERTIES(
			../../images/arcproxy_icon.icns
			PROPERTIES
			MACOSX_PACKAGE_LOCATION Resources
	)
	
ELSE(APPLE)
        add_executable(arccert-ui 
        	${arccertui_SRCS} ${arccertui_MOC_SRCS} ${arccertui_RC_SRCS}
        	${arccertui_UIS_H} 
        )
ENDIF(APPLE)

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/src/common ${Arc_INCLUDE_DIRS} ${GlibmmConfig_INCLUDE_DIR} ${Glibmm_INCLUDE_DIR} ${GlibConfig_INCLUDE_DIR} ${Glib_INCLUDE_DIR} ${SigC++Config_INCLUDE_DIR} ${SigC++_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})

target_link_libraries (arccert-ui ${Arc_LIBRARIES} ${Glibmm_LIBRARY} ${Glib_LIBRARIES} ${SigC++_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_SSL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES})

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
install(TARGETS arccert-ui DESTINATION bin)
install(FILES arccert-ui.desktop DESTINATION share/applications)
endif()
