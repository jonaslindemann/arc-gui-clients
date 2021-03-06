add_definitions(-DQT_NO_KEYWORDS)

include(${QT_USE_FILE})

SET(arcproxyui_UIS
    ../common/proxywindow.ui
    ../common/infodialog.ui
    ../common/helpwindow.ui
)

SET(arcproxyui_SRCS
  ../common/arctools.cpp
  ../common/arcproxy-utils-functions.cpp
  ../common/arcproxy-utils.cpp
  ../common/infodialog.cpp
  ../common/proxywindow.cpp
  ../common/helpwindow.cpp
  main.cpp
)

SET(arcproxyui_MOC_HDRS
  ../common/arctools.h
  ../common/arcproxy-utils-functions.h
  ../common/arcproxy-utils.h
  ../common/proxywindow.h
  ../common/infodialog.h
  ../common/helpwindow.h
)

SET(arcproxyui_RCS 
  arcproxy-ui.qrc
)

# After this call, foo_MOC_SRCS = moc_Class1.cxx moc_Class2.cxx moc_Class3.cxx.
QT4_WRAP_CPP(arcproxyui_MOC_SRCS ${arcproxyui_MOC_HDRS} )
QT4_WRAP_UI(arcproxyui_UIS_H ${arcproxyui_UIS} )
QT4_ADD_RESOURCES( arcproxyui_RC_SRCS ${arcproxyui_RCS} )

IF(APPLE)
	ADD_EXECUTABLE(
		arcproxy-ui 
		MACOSX_BUNDLE
		${arcproxyui_SRCS} ${arcproxyui_MOC_SRCS} ${arcproxyui_RC_SRCS}
		${arcproxyui_UIS_H}
		../../images/arcproxy_icon.icns
	)

	install(TARGETS arcproxy-ui
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
	add_executable(arcproxy-ui ${arcproxyui_SRCS} ${arcproxyui_MOC_SRCS} ${arcproxyui_RC_SRCS} ${arcproxyui_UIS_H} )
ENDIF(APPLE)

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/common ${Arc_INCLUDE_DIRS} ${GlibmmConfig_INCLUDE_DIR} ${Glibmm_INCLUDE_DIR} ${GlibConfig_INCLUDE_DIR} ${Glib_INCLUDE_DIR} ${SigC++Config_INCLUDE_DIR} ${SigC++_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} ${NSS_INCLUDE_DIR} ${NSPR_INCLUDE_DIR})

target_link_libraries (arcproxy-ui ${Arc_LIBRARIES} ${Glibmm_LIBRARY} ${Glib_LIBRARIES} ${SigC++_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_SSL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES} ${NSS_LIBRARY} ${NSPR_LIBRARY})

#if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
install(TARGETS arcproxy-ui DESTINATION bin)
install(FILES arcproxy-ui.desktop DESTINATION share/applications)
#endif()
