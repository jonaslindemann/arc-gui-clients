add_definitions(-DQT_NO_KEYWORDS)

include(${QT_USE_FILE})

SET(arcsubui_UIS
    jobdefinitionwindow.ui
    ../common/proxywindow.ui
    ../common/helpwindow.ui
)

SET(arcsubui_SRCS
    ../common/arctools.cpp
    ../common/arcproxy-utils.cpp
    ../common/arcproxy-utils-functions.cpp
    ../common/proxywindow.cpp
    ../common/helpwindow.cpp  
  arcsubmitcontroller.cpp
  utils.cpp
  jobdefinitionwindow.cpp
  jobdefinitions.cpp
  main.cpp
)

SET(arcsubui_MOC_HDRS
    ../common/arctools.h
    ../common/arcproxy-utils.h
    ../common/proxywindow.h
    ../common/arcproxy-utils-functions.h
    ../common/helpwindow.h
  arcsubmitcontroller.h
  utils.h
  jobdefinitions.h
  jobdefinitionwindow.h
)

SET(arcsubui_RCS
  arcsubui.qrc
)

# After this call, foo_MOC_SRCS = moc_Class1.cxx moc_Class2.cxx moc_Class3.cxx.
QT4_WRAP_CPP(arcsubui_MOC_SRCS ${arcsubui_MOC_HDRS} )
QT4_WRAP_UI(arcsubui_UIS_H ${arcsubui_UIS} )
QT4_ADD_RESOURCES( arcsubui_RC_SRCS ${arcsubui_RCS} )

IF(APPLE)
	add_executable(arcsub-ui
		MACOSX_BUNDLE ${arcsubui_SRCS} ${arcsubui_MOC_SRCS} ${arcsubui_RC_SRCS}
		${arcsubui_UIS_H}
		../../images/arcstat_icon.icns 
	)

	install(TARGETS arcsub-ui
		BUNDLE
		DESTINATION bin
	)

	SET( MACOSX_BUNDLE_ICON_FILE arcstat_icon.icns )
	SET( MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1 )
	SET( MACOSX_BUNDLE_VERSION 0.1 )
	SET( MACOSX_BUNDLE_LONG_VERSION_STRING 0.1 )

	SET_SOURCE_FILES_PROPERTIES(
		../../images/arcstat_icon.icns
		PROPERTIES
		MACOSX_PACKAGE_LOCATION Resources
	)

	set_target_properties(
	arcsub-ui
		PROPERTIES BUILD_WITH_INSTALL_RPATH OFF
		INSTALL_NAME_DIR "@executable_path"
	)

ELSE(APPLE)
        add_executable(arcsub-ui ${arcsubui_SRCS} ${arcsubui_MOC_SRCS} ${arcsubui_RC_SRCS} ${arcsubui_UIS_H} )
ENDIF(APPLE)

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/common ${Arc_INCLUDE_DIRS} ${GlibmmConfig_INCLUDE_DIR} ${Glibmm_INCLUDE_DIR} ${GlibConfig_INCLUDE_DIR} ${Glib_INCLUDE_DIR} ${SigC++Config_INCLUDE_DIR} ${SigC++_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} ${NSS_INCLUDE_DIR} ${NSPR_INCLUDE_DIR})

target_link_libraries (arcsub-ui ${Arc_LIBRARIES} ${Glibmm_LIBRARY} ${Glib_LIBRARIES} ${SigC++_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_SSL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES} ${NSS_LIBRARY} ${NSPR_LIBRARY})


IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
install(TARGETS arcsub-ui DESTINATION bin)
install(FILES arcsub-ui.desktop DESTINATION share/applications)
endif()
