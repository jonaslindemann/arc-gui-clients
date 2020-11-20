
SET(arcproxytest_SRCS
  arcproxy.h arcproxy.cpp arcproxy_myproxy.cpp arcproxy_proxy.cpp arcproxy_voms.cpp arcproxy_manager.h arcproxy_manager.cpp
)

add_executable(arcproxy-test ${arcproxytest_SRCS} )

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/common ${Arc_INCLUDE_DIRS} ${GlibmmConfig_INCLUDE_DIR} ${Glibmm_INCLUDE_DIR} ${GlibConfig_INCLUDE_DIR} ${Glib_INCLUDE_DIR} ${SigC++Config_INCLUDE_DIR} ${SigC++_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} )

target_link_libraries (arcproxy-test ${Arc_LIBRARIES} ${Glibmm_LIBRARY} ${Glib_LIBRARIES} ${SigC++_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_SSL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES} )

