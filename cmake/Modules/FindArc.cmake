# - Try to find ImageMagick++
# Once done, this will define
#
#  Magick++_FOUND - system has Magick++
#  Magick++_INCLUDE_DIRS - the Magick++ include directories
#  Magick++_LIBRARIES - link these to use Magick++

include(LibFindMacros)


# Use pkg-config to get hints about paths
#libfind_pkg_check_modules(Arc_PKGCONF arcbase)

# Include dir
  
find_path(Arc_INCLUDE_DIR NAMES arc/UserConfig.h PATHS $ENV{ARC_LOCATION}/include NO_DEFAULT_PATH)

set(Arc_INSTALL_TYPE "Unknown")

if (Arc_INCLUDE_DIR STREQUAL "Arc_INCLUDE_DIR-NOTFOUND")
    message("-- ARC not found at ARC_LOCATION. Checking for system wide ARC.")
    find_path(Arc_INCLUDE_DIR NAMES arc/UserConfig.h PATHS ${Arc_PKGCONF_INCLUDE_DIRS})
    if (Arc_INCLUDE_DIR STREQUAL "Arc_INCLUDE_DIR-NOTFOUND")
        message("-- System wide ARC not found.")
    else()
        set(Arc_INSTALL_TYPE "System")
    endif()
else()
    message("-- Using local installed ARC version.")
    set(Arc_INSTALL_TYPE "Local")
endif()

if (Arc_INSTALL_TYPE STREQUAL "Local")

    # ARC v2.0 libs

    find_library(Arc_CLIENT_LIB NAMES arcclient PATHS $ENV{ARC_LOCATION}/lib NO_DEFAULT_PATH)
    find_library(Arc_CLIENT_LIB NAMES arcclient PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_COMMON_LIB NAMES arccommon PATHS $ENV{ARC_LOCATION}/lib  NO_DEFAULT_PATH)
    find_library(Arc_COMMON_LIB NAMES arccommon PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_DATA2_LIB NAMES arcdata2 PATHS $ENV{ARC_LOCATION}/lib NO_DEFAULT_PATH)
    find_library(Arc_DATA2_LIB NAMES arcdata2 PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_CREDENTIAL_LIB NAMES arccredential PATHS $ENV{ARC_LOCATION}/lib NO_DEFAULT_PATH)
    find_library(Arc_CREDENTIAL_LIB NAMES arccredential PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_CREDENTIALSTORE_LIB NAMES arccredentialstore PATHS $ENV{ARC_LOCATION}/lib NO_DEFAULT_PATH)
    find_library(Arc_CREDENTIALSTORE_LIB NAMES arccredentialstore PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_CRYPTO_LIB NAMES arccrypto PATHS $ENV{ARC_LOCATION}/lib NO_DEFAULT_PATH)
    find_library(Arc_CRYPTO_LIB NAMES arccrypto PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_MESSAGE_LIB NAMES arcmessage PATHS $ENV{ARC_LOCATION}/lib NO_DEFAULT_PATH)
    find_library(Arc_MESSAGE_LIB NAMES arcmessage PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_LOADER_LIB NAMES arcloader PATHS $ENV{ARC_LOCATION}/lib NO_DEFAULT_PATH)
    find_library(Arc_LOADER_LIB NAMES arcloader PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)

    # ARC v3.0 libs

    find_library(Arc_COMMUNICATION_LIB NAMES arccommunication PATHS $ENV{ARC_LOCATION}/lib NO_DEFAULT_PATH)
    find_library(Arc_COMMUNICATION_LIB NAMES arccommunication PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_COMPUTE_LIB NAMES arccompute PATHS $ENV{ARC_LOCATION}/lib NO_DEFAULT_PATH)
    find_library(Arc_COMPUTE_LIB NAMES arccompute PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_DATA_LIB NAMES arcdata PATHS $ENV{ARC_LOCATION}/lib NO_DEFAULT_PATH)
    find_library(Arc_DATA_LIB NAMES arcdata PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)

    find_library(Arc_SECURITY_LIB NAMES arcsecurity PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_SECURITY_LIB NAMES arcsecurity PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_XMLSEC_LIB NAMES arcxmlsec PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_XMLSEC_LIB NAMES arcxmlsec PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_DATASTAGING_LIB NAMES arcdatastaging PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_DATASTAGING_LIB NAMES arcdatastaging PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_WSADDRESSING_LIB NAMES arcwsaddressing PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_WSADDRESSING_LIB NAMES arcwsaddressing PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_COMPUTE_LIB NAMES arccompute PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_COMPUTE_LIB NAMES arccompute PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_INFOSYS_LIB NAMES arcinfosys PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_INFOSYS_LIB NAMES arcinfosys PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_OTOKENS_LIB NAMES arcotokens PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_OTOKENS_LIB NAMES arcotokens PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_WSSECURITY_LIB NAMES arcwssecurity PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)
    find_library(Arc_WSSECURITY_LIB NAMES arcwssecurity PATHS $ENV{ARC_LOCATION}/lib/arc NO_DEFAULT_PATH)

    if ( (Arc_CLIENT_LIB STREQUAL "Arc_CLIENT_LIB-NOTFOUND") AND NOT(Arc_COMMUNICATION_LIB STREQUAL "Arc_COMMUNICATION_LIB-NOTFOUND") )
        message("-- ARC Version 3 found.")
        set(Arc_VERSION "3")
        set(Arc_CLIENT_LIB "-")
        set(Arc_DATA2_LIB "-")
    else()
        message("-- ARC Version 2 found.")
        set(Arc_VERSION "2")
        set(Arc_COMMUNICATION_LIB "-")
        set(Arc_COMPUTE_LIB "-")
        set(Arc_DATA_LIB "-")
    endif()

else()

    # ARC v2.0 libs

    find_library(Arc_CREDENTIALSTORE_LIB NAMES arccredentialstore PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_MESSAGE_LIB NAMES arcmessage PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_LOADER_LIB NAMES arcloader PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_CLIENT_LIB NAMES arcclient PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_COMMON_LIB NAMES arccommon PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_DATA2_LIB NAMES arcdata2 PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_CREDENTIAL_LIB NAMES arccredential PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_CRYPTO_LIB NAMES arccrypto PATHS ${Arc_PKGCONF_LIBRARY_DIRS})

    # ARC v3.0 libs

    find_library(Arc_COMMUNICATION_LIB NAMES arccommunication PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_COMPUTE_LIB NAMES arccompute PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_DATA_LIB NAMES arcdata PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    
    find_library(Arc_SECURITY_LIB NAMES arcsecurity PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_XMLSEC_LIB NAMES arcxmlsec PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_DATASTAGING_LIB NAMES arcdatastaging PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_WSADDRESSING_LIB NAMES arcwsaddressing PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_COMPUTE_LIB NAMES arccompute PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_INFOSYS_LIB NAMES arcinfosys PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_OTOKENS_LIB NAMES arcotokens PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
    find_library(Arc_WSSECURITY_LIB NAMES arcwssecurity PATHS ${Arc_PKGCONF_LIBRARY_DIRS})
								
    if ( (Arc_CLIENT_LIB STREQUAL "Arc_CLIENT_LIB-NOTFOUND") AND NOT(Arc_COMMUNICATION_LIB STREQUAL "Arc_COMMUNICATION_LIB-NOTFOUND") )
        message("-- ARC Version 3 found.")
        set(Arc_VERSION "3")
        set(Arc_CLIENT_LIB "-")
        set(Arc_DATA2_LIB "-")
    else()
        message("-- ARC Version 2 found.")
        set(Arc_VERSION "2")
        set(Arc_COMMUNICATION_LIB "-")
        set(Arc_COMPUTE_LIB "-")
        set(Arc_DATA_LIB "-")
    endif()

endif()

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Arc_PROCESS_INCLUDES Arc_INCLUDE_DIR)

if (Arc_VERSION STREQUAL "2")
    set(Arc_PROCESS_LIBS Arc_CLIENT_LIB Arc_COMMON_LIB Arc_DATA2_LIB Arc_CREDENTIAL_LIB Arc_CREDENTIALSTORE_LIB Arc_LOADER_LIB Arc_CRYPTO_LIB Arc_MESSAGE_LIB)
else()
    set(Arc_PROCESS_LIBS Arc_COMMUNICATION_LIB Arc_COMPUTE_LIB Arc_COMMON_LIB Arc_DATA_LIB Arc_CREDENTIAL_LIB Arc_CREDENTIALSTORE_LIB Arc_LOADER_LIB Arc_CRYPTO_LIB Arc_MESSAGE_LIB)
endif()    

libfind_process(Arc)

message("-- ARC libraries = " ${Arc_LIBRARIES})
message("-- ARC includes  = " ${Arc_INCLUDE_DIRS})


