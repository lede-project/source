cmake_minimum_required ( VERSION 2.8.5 FATAL_ERROR )

set ( CMAKE_POLICY_DEFAULT_CMP0022 OLD )
cmake_policy ( VERSION 2.8.5 )

project ( openobex C CXX)

#
# The project version
#
set ( VERSION_MAJOR 1 )
set ( VERSION_MINOR 7 )
set ( VERSION_PATCH 1 )
set ( VERSION "${VERSION_MAJOR}.${VERSION_MINOR}" )
if ( VERSION_PATCH GREATER 0 )
  set ( VERSION "${VERSION}.${VERSION_PATCH}" )
endif ( VERSION_PATCH GREATER 0 )

#
# The path for our own CMake modules
#
set ( CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/CMakeModules
)

#
# Define the default build type
#
set ( CMAKE_CONFIGURATION_TYPES "Release;Debug"
      CACHE STRING "" FORCE )
if ( NOT CMAKE_BUILD_TYPE )
  set ( CMAKE_BUILD_TYPE Release
        CACHE STRING "Build type" FORCE )
endif ( NOT CMAKE_BUILD_TYPE )

include ( MaintainerMode )
include ( GNUInstallDirs )

# This module currently expects C to be enabled in CMake-2.8.10.
include ( GenerateExportHeader )
add_compiler_export_flags ( C )

#
# define how to build libraries
#
option ( BUILD_SHARED_LIBS "Build shared libraries" ON )

#
# check common compiler flags
#
include ( CheckCCompilerFlag )
if ( CMAKE_COMPILER_IS_GNUCC )
  set ( LINKER_FLAG_NOUNDEFINED -Wl,--no-undefined )
  check_c_compiler_flag ( "${LINKER_FLAG_NOUNDEFINED}" COMPILER_SUPPORT_NOUNDEFINED )
  if ( NOT COMPILER_SUPPORT_NOUNDEFINED )
    set ( LINKER_FLAG_NOUNDEFINED )
  endif ( NOT COMPILER_SUPPORT_NOUNDEFINED )
endif ( CMAKE_COMPILER_IS_GNUCC )

if ( MSVC )
  # Some compiler options for MSVC to not print non-sense warnings.
  add_definitions ( -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE )
endif ( MSVC )

#
# which transports shall be included
#
find_package ( Bluetooth )
set ( OPENOBEX_BLUETOOTH_AVAILABLE ${Bluetooth_FOUND} )

find_package ( Irda )
set ( OPENOBEX_IRDA_AVAILABLE ${Irda_FOUND} )

find_package ( LibUSB )
set ( OPENOBEX_USB_AVAILABLE ${LibUSB_FOUND} )

foreach ( transport BLUETOOTH IRDA USB )
  if ( OPENOBEX_${transport}_AVAILABLE )
    set ( OPENOBEX_${transport} ON
          CACHE BOOL "Build with ${transport} support")
  else ( OPENOBEX_${transport}_AVAILABLE )
    set ( OPENOBEX_${transport} OFF
          CACHE BOOL "Build with ${transport} support")
  endif ( OPENOBEX_${transport}_AVAILABLE )
endforeach ( transport )

if ( OPENOBEX_USB )
  if ( LibUSB_VERSION_1.0 )
    add_definitions ( -DHAVE_USB1 )
  endif ( LibUSB_VERSION_1.0 )
  add_definitions ( -DHAVE_USB )
endif ( OPENOBEX_USB )

if ( OPENOBEX_IRDA )
  add_definitions ( -DHAVE_IRDA )
  if ( WIN32 )
    add_definitions ( -DHAVE_IRDA_WINDOWS )
  else ( WIN32 )
    string ( TOUPPER "HAVE_IRDA_${CMAKE_SYSTEM_NAME}" IRDA_SYSTEM_DEFINITION )
    add_definitions ( -D${IRDA_SYSTEM_DEFINITION} )
  endif ( WIN32 )
endif ( OPENOBEX_IRDA )

if ( OPENOBEX_BLUETOOTH )
  add_definitions ( -DHAVE_BLUETOOTH )
  if ( WIN32 )
    add_definitions ( -DHAVE_BLUETOOTH_WINDOWS )
  else ( WIN32 )
    string ( TOUPPER "HAVE_BLUETOOTH_${CMAKE_SYSTEM_NAME}" BLUETOOTH_SYSTEM_DEFINITION )
    add_definitions ( -D${BLUETOOTH_SYSTEM_DEFINITION} )
  endif ( WIN32 )
endif ( OPENOBEX_BLUETOOTH )

#
# create pkg-config files
# these get copied and installed in the library dirs
# TODO: those files should be moved to subdirs for each library
#
set ( prefix      "${CMAKE_INSTALL_PREFIX}" )
set ( exec_prefix "\${prefix}" )
set ( libdir      "\${prefix}/${CMAKE_INSTALL_LIBDIR}" )
set ( includedir  "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}" )
set ( top_srcdir   "${CMAKE_SOURCE_DIR}" )
set ( top_builddir "${CMAKE_BINARY_DIR}" )
if ( OPENOBEX_BLUETOOTH AND UNIT AND NOT WIN32 )
  foreach ( lib Bluetooth_LIBRARIES )
    set ( LIBS_PRIVATE "${LIBS_PRIVATE} ${lib}" )
  endforeach ( lib )
endif ( OPENOBEX_BLUETOOTH AND UNIT AND NOT WIN32 )
if ( OPENOBEX_USB AND UNIX AND NOT WIN32 )
  if ( PKGCONFIG_LIBUSB_FOUND )
    if ( LibUSB_VERSION_1.0 )
      set ( REQUIRES "${REQUIRES} libusb-1.0" )
    else ( LibUSB_VERSION_1.0 )
      set ( REQUIRES "${REQUIRES} libusb" )
    endif ( LibUSB_VERSION_1.0 )
  else ( PKGCONFIG_LIBUSB_FOUND )
    foreach ( lib LibUSB_LIBRARIES )
      set ( LIBS_PRIVATE "${LIBS_PRIVATE} ${lib}" )
    endforeach ( lib )
  endif ( PKGCONFIG_LIBUSB_FOUND )
endif ( OPENOBEX_USB AND UNIX AND NOT WIN32 )
configure_file (
  ${CMAKE_CURRENT_SOURCE_DIR}/openobex.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/openobex.pc
  @ONLY
)

if ( NOT PKGCONFIG_INSTALL_DIR )
  set ( PKGCONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        CACHE PATH "Where to install .pc files to" FORCE )
endif ( NOT PKGCONFIG_INSTALL_DIR )
mark_as_advanced ( PKGCONFIG_INSTALL_DIR )


#
# process include directory
#
set ( openobex_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_BINARY_DIR}/include"
)
include_directories ( "${openobex_INCLUDE_DIRS}" )
add_subdirectory ( include/openobex )
if ( MSVC )
  include_directories ( AFTER SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/include/msvc" )
endif ( MSVC )


#
# build the main library
#
add_subdirectory ( lib )
link_directories ( "${CMAKE_CURRENT_BINARY_DIR}/lib" )
if ( BUILD_SHARED_LIBS )
  add_definitions ( -DOPENOBEX_DLL )
endif ( BUILD_SHARED_LIBS )
option ( EXPORT_PACKAGE "Register build directory for find_package" OFF )
if ( EXPORT_PACKAGE )
  export ( PACKAGE OpenObex )
endif ( EXPORT_PACKAGE )

#
# build udev support 
#
add_subdirectory ( udev )

#
# build the applications
#
add_custom_target ( openobex-apps )
add_subdirectory ( apps )


#
# build the documentation
#
option ( BUILD_DOCUMENTATION "Build library and application documentation" ON)
if ( BUILD_DOCUMENTATION )
  add_subdirectory ( doc )
endif ( BUILD_DOCUMENTATION )


#
# The following adds CPack support
#
set ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenObex" )
set ( CPACK_PACKAGE_VENDOR "The OpenObex Development Team" )

set ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LIB" )
set ( CPACK_RESOURCE_FILE_README  "${CMAKE_CURRENT_SOURCE_DIR}/README" )

set ( CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}" )
set ( CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}" )
set ( CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}" )
set ( CPACK_PACKAGE_VERSION       "${VERSION}" )

if ( UNIX )
  set ( CPACK_GENERATOR "TGZ" )
  set ( CPACK_SOURCE_GENERATOR "TGZ" )

elseif ( WIN32 )
  #
  # For NSIS, install from http://nsis.sf.net.
  # For ZIP, install e.g. info-zip from http://www.info-zip.org.
  #
  set ( CPACK_GENERATOR "ZIP;NSIS" )
  set ( CPACK_SOURCE_GENERATOR "ZIP" )
endif ( UNIX )
set ( CPACK_SOURCE_IGNORE_FILES
  "/build/"
  "/\\\\.git/"
  "/\\\\.gitignore$"
  "~$"
)

# this must _follow_ the settings!
include ( CPack )