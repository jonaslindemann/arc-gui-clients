# - Try to find Glib-2.0 (with gobject)
# Once done, this will define
#
#  Glib_FOUND - system has Glib
#  Glib_INCLUDE_DIRS - the Glib include directories
#  Glib_LIBRARIES - link these to use Glib

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(NSS_PKGCONF nss)
message("RESULT: ${NSS_PKGCONF_VERSION}")

# Main include dir
#find_path(NSS_INCLUDE_DIR
#  NAMES nss.h
#  PATHS ${NSS_PKGCONF_INCLUDE_DIRS}
#)

find_path(NSS_INCLUDE_DIR
  NAME pk11pub.h
  PATHS ${NSS_PKGCONF_INCLUDE_DIRS}
)

message("found NSS at ${NSS_INCLUDE_DIR}")
message("pkg-config : ${NSS_PKGCONF_INCLUDE_DIRS}")

# Finally the library itself
find_library(NSS_LIBRARY
  NAMES nss3
  PATHS ${NSS_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(NSS_PROCESS_INCLUDES NSS_INCLUDE_DIR)
set(NSS_PROCESS_LIBS NSS_LIBRARY)
libfind_process(NSS)

