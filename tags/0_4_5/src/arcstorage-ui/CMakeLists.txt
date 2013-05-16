add_definitions(-DQT_NO_KEYWORDS)

include(${QT_USE_FILE})

set(arcstorageui_UIS
    arcstoragewindow.ui
    filepropertiesdialog.ui
    srmsettingsdialog.ui
    transferlistwindow.ui
    applicationsettings.ui
    filepropertyinspector.ui
    renamedialog.ui
    ../common/proxywindow.ui
    ../common/helpwindow.ui
)

set(arcstorageui_SRCS
    main.cpp
    arcstoragewindow.cpp
    fileserver.cpp
    arcfileelement.cpp
    dragdropabletreewidget.cpp
    fileserverfactory.cpp
    filepropertiesdialog.cpp
    arcfileserver.cpp
    srmsettingsdialog.cpp
    settings.cpp
    filetransfer.cpp
    filetransferlist.cpp
    transferlistwindow.cpp
    globalstateinfo.cpp
    applicationsettings.cpp
    filepropertyinspector.cpp
    renamedialog.cpp
    ../common/arctools.cpp
    ../common/arcproxy-utils.cpp
    ../common/arcproxy-utils-functions.cpp
    ../common/proxywindow.cpp
    ../common/helpwindow.cpp
)

set(arcstorageui_MOC_HDRS
    arcstoragewindow.h
    filepropertiesdialog.h
    arcfileserver.h
    srmsettingsdialog.h
    filetransfer.h
    filetransferlist.h
    transferlistwindow.h
    globalstateinfo.h
    applicationsettings.h
    filepropertyinspector.h
    renamedialog.h
    ../common/arctools.h
    ../common/arcproxy-utils.h
    ../common/proxywindow.h
    ../common/arcproxy-utils-functions.h
    ../common/helpwindow.h
)

SET(arcstorageui_RCS
  arcstorage-ui-resources.qrc
)

# After this call, foo_MOC_SRCS = moc_Class1.cxx moc_Class2.cxx moc_Class3.cxx.
QT4_WRAP_CPP(arcstorageui_MOC_SRCS ${arcstorageui_MOC_HDRS} )
QT4_WRAP_UI(arcstorageui_UIS_H ${arcstorageui_UIS} )
QT4_ADD_RESOURCES(arcstorageui_RC_SRCS ${arcstorageui_RCS} )

IF(APPLE)
	add_executable(arcstorage-ui 
		MACOSX_BUNDLE 
		${arcstorageui_SRCS} 
		${arcstorageui_MOC_SRCS} 
		${arcstorageui_RC_SRCS} 
		${arcstorageui_UIS_H} 
		../../images/arcstorage_icon.icns 
	)

	install(TARGETS arcstorage-ui
   		BUNDLE
   		DESTINATION bin
	)

	SET( MACOSX_BUNDLE_ICON_FILE arcstorage_icon.icns )
	SET( MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1 )
	SET( MACOSX_BUNDLE_VERSION 0.1 )
	SET( MACOSX_BUNDLE_LONG_VERSION_STRING 0.1 )

	SET_SOURCE_FILES_PROPERTIES(
        ../../images/arcstorage_icon.icns
		PROPERTIES
		MACOSX_PACKAGE_LOCATION Resources
	)

   	set_target_properties(
   		arcstorage-ui
 		PROPERTIES BUILD_WITH_INSTALL_RPATH OFF
		INSTALL_NAME_DIR "@executable_path"
   	)

ELSE(APPLE)
        add_executable(arcstorage-ui ${arcstorageui_SRCS} ${arcstorageui_MOC_SRCS} ${arcstorageui_RC_SRCS} ${arcstorageui_UIS_H} )
ENDIF(APPLE)


include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/common ${Arc_INCLUDE_DIRS} ${GlibmmConfig_INCLUDE_DIR} ${Glibmm_INCLUDE_DIR} ${GlibConfig_INCLUDE_DIR} ${Glib_INCLUDE_DIR} ${SigC++Config_INCLUDE_DIR} ${SigC++_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} ${NSS_INCLUDE_DIR} ${NSPR_INCLUDE_DIR})
target_link_libraries (arcstorage-ui ${Arc_LIBRARIES} ${Glibmm_LIBRARY} ${Glib_LIBRARIES} ${SigC++_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_SSL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES} ${NSS_LIBRARY} ${NSPR_LIBRARY})


if (LINUX)
	install(TARGETS arcstorage-ui DESTINATION bin)
	install(FILES arcstorage-ui.desktop DESTINATION share/applications)
endif(LINUX)
