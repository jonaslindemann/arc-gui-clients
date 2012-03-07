add_definitions(-DQT_NO_KEYWORDS)

include(${QT_USE_FILE})

SET(arcstatui_UIS
    mainwindow.ui
)

SET(arcstatui_SRCS
  mainwindow.cpp
  main.cpp
  arcjobcontroller.cpp
)

SET(arcstatui_MOC_HDRS
  mainwindow.h
  arcjobcontroller.h
)

SET(arcstatui_RCS 
  arcstat-ui.qrc
)

# After this call, foo_MOC_SRCS = moc_Class1.cxx moc_Class2.cxx moc_Class3.cxx.
QT4_WRAP_CPP(arcstatui_MOC_SRCS ${arcstatui_MOC_HDRS} )
QT4_WRAP_UI(arcstatui_UIS_H ${arcstatui_UIS} )
QT4_ADD_RESOURCES( arcstatui_RC_SRCS ${arcstatui_RCS} )


add_executable(arcstat-ui ${arcstatui_SRCS} ${arcstatui_MOC_SRCS} ${arcstatui_RC_SRCS} ${arcstatui_UIS_H} )

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${Arc_INCLUDE_DIRS} ${GlibmmConfig_INCLUDE_DIR} ${Glibmm_INCLUDE_DIR} ${GlibConfig_INCLUDE_DIR} ${Glib_INCLUDE_DIR} ${SigC++Config_INCLUDE_DIR} ${SigC++_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})

target_link_libraries (arcstat-ui ${Arc_LIBRARIES} ${Glibmm_LIBRARY} ${Glib_LIBRARIES} ${SigC++_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_LIBRARIES})
