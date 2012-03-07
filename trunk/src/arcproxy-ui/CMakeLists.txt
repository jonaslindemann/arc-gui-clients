add_definitions(-DQT_NO_KEYWORDS)

include(${QT_USE_FILE})

SET(arcproxyui_UIS
    mainwindow.ui
)

SET(arcproxyui_SRCS
  arcproxy-utils.cpp
  mainwindow.cpp
  main.cpp
)

SET(arcproxyui_MOC_HDRS
  arcproxy-utils.h
  mainwindow.h
)

SET(arcproxyui_RCS 
  arcproxy-ui.qrc
)

# After this call, foo_MOC_SRCS = moc_Class1.cxx moc_Class2.cxx moc_Class3.cxx.
QT4_WRAP_CPP(arcproxyui_MOC_SRCS ${arcproxyui_MOC_HDRS} )
QT4_WRAP_UI(arcproxyui_UIS_H ${arcproxyui_UIS} )
QT4_ADD_RESOURCES( arcproxyui_RC_SRCS ${arcproxyui_RCS} )


add_executable(arcproxy-ui ${arcproxyui_SRCS} ${arcproxyui_MOC_SRCS}
${arcproxyui_RC_SRCS} ${arcproxyui_UIS_H} )

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${Arc_INCLUDE_DIRS} ${GlibmmConfig_INCLUDE_DIR} ${Glibmm_INCLUDE_DIR} ${GlibConfig_INCLUDE_DIR} ${Glib_INCLUDE_DIR} ${SigC++Config_INCLUDE_DIR} ${SigC++_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})

target_link_libraries (arcproxy-ui ${Arc_LIBRARIES} ${Glibmm_LIBRARY} ${Glib_LIBRARIES} ${SigC++_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_LIBRARIES})
