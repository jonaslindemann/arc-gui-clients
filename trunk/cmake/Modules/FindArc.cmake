# - Try to find ImageMagick++
# Once done, this will define
#
#  Magick++_FOUND - system has Magick++
#  Magick++_INCLUDE_DIRS - the Magick++ include directories
#  Magick++_LIBRARIES - link these to use Magick++

include(LibFindMacros)


# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Arc_PKGCONF arcbase)

# Include dir
find_path(Arc_INCLUDE_DIR
  NAMES arc/UserConfig.h
  PATHS /usr
)

# Finally the library itself
find_library(Arc_CLIENT_LIB
  NAMES arcclient
  PATHS ${Arc_PKGCONF_LIBRARY_DIRS}
)

# Finally the library itself
find_library(Arc_COMMON_LIB
  NAMES arccommon
  PATHS ${Arc_PKGCONF_LIBRARY_DIRS}
)

# Finally the library itself
find_library(Arc_DATA2_LIB
  NAMES arcdata2
  PATHS ${Arc_PKGCONF_LIBRARY_DIRS}
)

# Finally the library itself
find_library(Arc_CREDENTIAL_LIB
  NAMES arccredential
  PATHS ${Arc_PKGCONF_LIBRARY_DIRS}
)

# Finally the library itself
find_library(Arc_CREDENTIALSTORE_LIB
  NAMES arccredentialstore
  PATHS ${Arc_PKGCONF_LIBRARY_DIRS}
)

find_library(Arc_LOADER_LIB
  NAMES arcloader
  PATHS ${Arc_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Arc_PROCESS_INCLUDES Arc_INCLUDE_DIR)
set(Arc_PROCESS_LIBS Arc_CLIENT_LIB Arc_COMMON_LIB Arc_DATA2_LIB Arc_CREDENTIAL_LIB Arc_CREDENTIALSTORE_LIB Arc_LOADER_LIB)
libfind_process(Arc)
