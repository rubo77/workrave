project(Workrave)

cmake_minimum_required(VERSION 2.8)

	
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
cmake_policy(SET CMP0009 NEW)

#
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build/cmake/modules")

# Require out of source build.
include(MacroEnsureOutOfSourceBuild)
macro_ensure_out_of_source_build(
 "${PROJECT_NAME} requires an out of source build. Please create a separate
 build directory and run 'cmake /path/to/${PROJECT_NAME} [options]' there."
 )

# Set Workrave version
set(VERSION_MAJOR   "1")
set(VERSION_MINOR   "11")
set(VERSION_RELEASE "1")
#set(VERSION_PATCH   "0")
if (VERSION_PATCH)
    set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE}-${VERSION_PATCH}" )
    set(RESOURCE_VERSION "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_RELEASE},${VERSION_PATCH}" )
else ()
    set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE}" )
    set(RESOURCE_VERSION "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_RELEASE},0" )
endif ()

set (PACKAGE_VERSION ${VERSION})

#
set(CMAKE_BUILD_TYPE_INIT "Debug")

# 
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#
#include(build/cmake/macros.cmake)

include(CheckLibraryExists)
include(CheckStructHasMember)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag) 
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckCSourceCompiles)
include(CMakeFindBinUtils)

#----------------------------------------------------------------------------------------------------

# TODO: GIT_VERSION

#----------------------------------------------------------------------------------------------------
# Define output location.

#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "" FORCE)
#set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE INTERNAL "" FORCE)

#----------------------------------------------------------------------------------------------------
# Macros

macro(pkgconfig_get_var OUT PKG VARNAME)
  execute_process(
    COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=${VARNAME} ${PKG}
    OUTPUT_VARIABLE _result
    RESULT_VARIABLE _null
  )

  if (_null)
  else()
    string(REGEX REPLACE "[\r\n]" " " _result "${_result}")
    string(REGEX REPLACE " +$" ""  _result "${_result}")
    separate_arguments(_result)
    set(${OUT} ${_result} CACHE INTERNAL "")
  endif()
endmacro()

#----------------------------------------------------------------------------------------------------
# Compiler

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

  CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11) 
  CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

  if(COMPILER_SUPPORTS_CXX11) 
    set(CXX11_FLAGS -std=c++11)
  elseif(COMPILER_SUPPORTS_CXX0X) 
    set(CXX11_FLAGS -std=c++01)
  elseif()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.") 
  endif()

  if (CXX11_FLAGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_FLAGS}")
    CHECK_CXX_COMPILER_FLAG("${CXX11_FLAGS} -stdlib=libc++" HAS_LIBCXX11)
  endif()

  CHECK_C_COMPILER_FLAG("-std=c99" COMPILER_SUPPORTS_C99)
  if(COMPILER_SUPPORTS_C99) 
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
  endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  if (NOT (MSVC11 OR MSVC12))
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.") 
  endif()
endif()


if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wall -g -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall -O2")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wall -g -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall -O2")
  if (NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-register")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

#----------------------------------------------------------------------------------------------------
# Testing

option(WITH_TESTS "Enable unit tests" OFF)

if (WITH_TESTS)
  #pkg_check_modules(
  #  GLIB REQUIRED
  #  glib-2.0>=2.36.0
  #  gio-2.0>=2.26.0)
  #
  #if (GLIB_FOUND)
  #  set (HAVE_DBUS_TEST_GIO ON)
  #endif()

  set (HAVE_TESTS ON)
endif()

option(WITH_COVERAGE "Enable coverage" OFF)

if (WITH_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -fno-inline -ftest-coverage -fprofile-arcs")
endif()

#----------------------------------------------------------------------------------------------------
# Toolkit selection

set(WITH_UI "Qt5" CACHE STRING "UI Toolkit to use")
set(WITH_UI_VALUES "Qt5" "Gtk+3" "Gtk+2")
set_property(CACHE WITH_UI PROPERTY STRINGS ${WITH_UI_VALUES})
list(FIND WITH_UI_VALUES ${WITH_UI} WITH_UI_INDEX)
if (${WITH_UI_INDEX} EQUAL -1)
  message(FATAL_ERROR "Option ${WITH_UI} not supported for WITH_UI, valid entries are ${WITH_UI_VALUES}")
endif()

#----------------------------------------------------------------------------------------------------
# Pkg-config

if (NOT ("${WITH_UI}" STREQUAL "Qt5"))
   find_package(PkgConfig REQUIRED)
endif()

#----------------------------------------------------------------------------------------------------
# GTK

if ("${WITH_UI}" STREQUAL "Gtk+3")
  message("Checking for: Gtk 3")

  pkg_check_modules(
    GTK REQUIRED
    gtkmm-3.0>=3.6.0
    gtk+-3.0>=3.6.0)
  
  set (HAVE_GSETTINGS ON)
  set (HAVE_GLIB ON)
  set (HAVE_GTK ON)
  set (HAVE_GTK3 ON)
  set (HAVE_DBUS ON)
  set (HAVE_DBUS_GIO ON)
  set (DBUS_BACKEND "gio")
  set (HAVE_APP_GTK ON)

  pkg_check_modules(
    GLIB REQUIRED
    glib-2.0>=2.36.0
    gio-2.0>=2.26.0)
 
  include_directories(${GLIB_INCLUDE_DIRS})

  find_package(GSettings)

endif()

if ("${WITH_UI}" STREQUAL "Gtk+2")
  message("Checking for: Gtk 2")

  pkg_check_modules(
    GTK REQUIRED
    gtkmm-2.4>=2.24.0
    glibmm-2.4>=2.10
    gtk+-2.0>=2.6.0)
  
  set (HAVE_GSETTINGS ON)
  set (HAVE_GLIB ON)
  set (HAVE_GTK ON)
  set (HAVE_DBUS ON)
  set (HAVE_DBUS_GIO ON)
  set (DBUS_BACKEND "gio")
  set (HAVE_APP_GTK ON)

  pkg_check_modules(
    GLIB REQUIRED
    glib-2.0>=2.28.0
    gio-2.0>=2.26.0)
 
  include_directories(${GLIB_INCLUDE_DIRS})

  find_package(GSettings)

endif()

#----------------------------------------------------------------------------------------------------
# Gnome

option(WITH_GCONF "Enable GConf support" OFF)

if (WITH_GCONF)
  message("Checking for: Gnome GConf")

  pkg_check_modules(GCONF QUIET gconf-2.0)
  if (GCONF_FOUND)
    set (HAVE_GCONF ON)
  else()
    set (WITH_GCONF OFF)
  endif()
endif()

option(WITH_GNOME_CLASSIC_PANEL "Enable Gnome 3 Classic pannel applet support" OFF)

if (WITH_GNOME_CLASSIC_PANEL)
  message("Checking for: Gnome 3 classic panel applets")

  pkg_check_modules(PANELAPPLET4 QUIET libpanelapplet-4.0 gio-2.0>=2.26.0)
  if (PANELAPPLET4_FOUND)
    set (HAVE_PANELAPPLET4 ON)
    set (HAVE_GNOME3 ON)
  else()
    set (WITH_GNOME_CLASSIC_PANEL OFF)
  endif()
endif()

#----------------------------------------------------------------------------------------------------
# Qt5

if ("${WITH_UI}" STREQUAL "Qt5")
  message("Checking for: Qt5")

  find_package(Qt5Widgets REQUIRED)
  find_package(Qt5Core REQUIRED)
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5DBus QUIET)

  if (Qt5DBus_FOUND)
    set (HAVE_DBUS ON)
    set (HAVE_DBUS_QT5 ON)
    set (DBUS_BACKEND "qt5")
    include_directories(${Qt5DBus_INCLUDE_DIRS})
  endif()
  
  include_directories(${Qt5Core_INCLUDE_DIRS})
  include_directories(${Qt5Gui_INCLUDE_DIRS})
  include_directories(${Qt5Widgets_INCLUDE_DIRS})

  # Can be removed with Qt 5.1
  include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})
  
  get_target_property(QtCore_location_Release Qt5::Core LOCATION_Release)
  get_filename_component(Qt_BIN_DIR "${QtCore_location_Release}" PATH)

  message ( "loc : ${Qt_BIN_DIR}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
  set (HAVE_QT5 ON)
  set (HAVE_APP_QT5 ON)
endif()


#----------------------------------------------------------------------------------------------------
# DBus

if (HAVE_DBUS)
  message("Checking for: DBus")
  
  find_package(PythonInterp)
  if (NOT PYTHONINTERP_FOUND)
    set (HAVE_DBUS FALSE)
  else()
    include(DBus)
  endif()
endif()

if (HAVE_DBUS_FREEDESKTOP)
  include_directories(
    ${DBUS_INCLUDE_DIR}
    ${DBUS_ARCH_INCLUDE_DIR}
    )
endif()

# TODO: check python Cheetah

#----------------------------------------------------------------------------------------------------
# GStreamer

if (UNIX AND NOT APPLE)
  option(WITH_GSTREAMER "Enable GStreamer audio" ON)
  
  if (WITH_GSTREAMER)
    message("Checking for: GStreamer")
  
    pkg_check_modules(GSTREAMER QUIET gstreamer-1.0)
    if (GSTREAMER_FOUND)
      set (HAVE_GSTREAMER ON)
    else()
      pkg_check_modules(GSTREAMER QUIET gstreamer-0.10)
      if (GSTREAMER_FOUND)
        set (HAVE_GSTREAMER ON)
      else()
        set (WITH_GSTREAMER OFF)
      endif()
    endif()
  endif()
endif()

#----------------------------------------------------------------------------------------------------
# Pulseaudio

if (UNIX AND NOT APPLE)
  option(WITH_PULSE "Enable Pulseaudio Mixer support" ON)
  
  if (WITH_PULSE)
    message("Checking for: Pulseaudio")
  
    pkg_check_modules(PULSE glib-2.0 libpulse libpulse-mainloop-glib)
    
    if (PULSE_FOUND)
      set (HAVE_PULSE ON)
    else()
      set (WITH_PULSE OFF)
    endif()
  endif()
endif()

#----------------------------------------------------------------------------------------------------
# Indicator

if (UNIX AND NOT APPLE)
  option(WITH_INDICATOR "Enable Unity indicator support" ON)
  option(LOCALINSTALL "Install file locally instead of to system location" OFF)

  if (WITH_INDICATOR)
    message("Checking for: Unity Indicators")

    find_package(GObjectIntrospection 0.9.12)
    set(HAVE_INTROSPECTION ${INTROSPECTION_FOUND})

    if (HAVE_INTROSPECTION)
      pkg_check_modules(INDICATOR QUIET indicator3-0.4 dbusmenu-glib-0.4 dbusmenu-gtk3-0.4)
    endif()
      
    if (INDICATOR_FOUND)
      set(HAVE_INDICATOR ON)
    else()
      set(WITH_INDICATOR OFF)
    endif()
  endif()

  if (HAVE_INDICATOR)
    if (LOCALINSTALL)
      set(INDICATORDIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/indicators3/7")
      set(INDICATORICONSDIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/libindicate/icons")
    else()
      pkgconfig_get_var(INDICATORDIR indicator3-0.4 indicatordir)
      pkgconfig_get_var(INDICATORICONSDIR indicator3-0.4 iconsdir)

      #string(REGEX REPLACE "/$" "" INDICATORDIR "${INDICATORDIR}")
      #set(INDICATORDIR ${INDICATORDIR} CACHE INTERNAL "")
    endif()
  endif()
endif()

#----------------------------------------------------------------------------------------------------
# Boost

if(MSVC)
  if (NOT DEFINED BOOST_ALL_DYN_LINK)
    set(BOOST_ALL_DYN_LINK TRUE)
  endif()

  set(BOOST_ALL_DYN_LINK "${BOOST_ALL_DYN_LINK}" CACHE BOOL "boost enable dynamic linking")
  if (BOOST_ALL_DYN_LINK)
    add_definitions(-DBOOST_ALL_DYN_LINK)
  endif()
endif(MSVC)

if (HAVE_TESTS)
  find_package(Boost 1.52.0 REQUIRED COMPONENTS system thread filesystem chrono test_exec_monitor)
else()
  find_package(Boost 1.52.0 REQUIRED COMPONENTS system thread filesystem chrono)
endif()

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

#----------------------------------------------------------------------------------------------------
# Translations

set(ALL_LINGUAS "")
file(STRINGS "${CMAKE_SOURCE_DIR}/po/LINGUAS" pofile)
foreach(line ${pofile})
  if("${line}" MATCHES "^[a-zA-Z]")
    set(ALL_LINGUAS "${ALL_LINGUAS}${line} ")
  endif()
endforeach()
string(STRIP ${ALL_LINGUAS} ALL_LINGUAS)

find_package(Gettext)
find_package(Libintl)

find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)

if (NOT GETTEXT_FOUND)
  message("=== GetText not found - disabling i18n")
else()
  set(ENABLE_NLS ON)
endif (NOT GETTEXT_FOUND)

if (APPLE) # XXX: workaround intl for Mac OSX
    set(GETTEXT_LIBRARIES ${LIBINTL_LIBRARIES})
endif(APPLE)

set (GETTEXT_PACKAGE "workrave")

# TODO: generate languages.iss for InnoSetup

include(Intltool)

#----------------------------------------------------------------------------------------------------
# Tracing

option(WITH_TRACING "Enable debug tracing" OFF)

if (WITH_TRACING)
  set(TRACING ON)
  #SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DTRACING")
endif()


#----------------------------------------------------------------------------------------------------
# platform - win32

if (WIN32)
  option(DEVELOPMENT_MODE "Development" OFF)
  if(DEVELOPMENT_MODE)
    set(DATA_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../)
  endif()
  
  if (NOT MSVC)
     option(PREBUILT_PATH "Location of prebuilt binaries" OFF)

     if (NOT PREBUILT_PATH)
        message(FATAL_ERROR "Use -DPREBUILT_PATH=location to specify location if prebuilt win32")
     endif()
  endif()

  set (PLATFORM_OS_WIN32 ON)
  if (MSVC)
    set(PLATFORM_OS_WIN32_NATIVE 1)
  endif()
  
  set (HAVE_GSETTINGS OFF)
  set(HAVE_LANGUAGE_SELECTION TRUE)

  if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
  
    set(MSVC_WARNINGS "/W3 /wd4127 /wd4250 /wd4413 /wd4800")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   /FIconfig.h ${MSVC_WARNINGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /FIconfig.h ${MSVC_WARNINGS}")

    set (CMAKE_DEBUG_POSTFIX "d")

    add_definitions( 
      -DWIN32
      -D_WINDOWS
      -D_WIN32_WINNT=0x0600
      ) 
  endif()

  check_include_files(dsound.h HAVE_DSOUND)

  check_c_source_compiles(
    "#include <mmdeviceapi.h>
     #include <endpointvolume.h>

     static REFIID foo = __uuidof(IAudioEndpointVolume);
     IMMDeviceEnumerator *device_enum = NULL;"
    HAVE_MMDEVICEAPI_H
    )

  check_c_source_compiles(
    "#include <shlobj.h>
     IShellDispatch* pShellDispatch = NULL;
     int main(){return 0;}"
    HAVE_ISHELLDISPATCH
    )

  check_struct_has_member("MOUSEHOOKSTRUCT" hwnd "windows.h" HAVE_STRUCT_MOUSEHOOKSTRUCT)  
  check_struct_has_member("MOUSEHOOKSTRUCTEX" mouseData "windows.h" HAVE_STRUCT_MOUSEHOOKSTRUCTEX)  
  
endif()

#----------------------------------------------------------------------------------------------------
# platform - apple

if (APPLE)
  set (PLATFORM_OS_OSX ON)

  if (HAS_LIBCXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()

  find_library(COCOA_LIBRARY Cocoa REQUIRED)
  find_library(IOKIT_LIBRARY IOKit REQUIRED)
  find_library(QTKIT_LIBRARY QTKit REQUIRED)
  include_directories(/usr/local/include)
  include_directories(${CMAKE_SYSTEM_INCLUDE_PATH})
endif()

#----------------------------------------------------------------------------------------------------
# platform - unix

if (UNIX AND NOT APPLE)
  set (PLATFORM_OS_UNIX ON)

  # TODO: make option.
  set (HAVE_MONITORS "screensaver,record,x11events")

  find_package(X11)
  
  check_library_exists(Xtst XRecordEnableContext "" HAVE_XRECORD)

  check_library_exists(Xext XScreenSaverRegister "" SCREENSAVER_IN_XEXT)
  if(SCREENSAVER_IN_XEXT)
    set(XSS_LIB "Xext")			
  else()
    check_library_exists(Xss XScreenSaverRegister "" SCREENSAVER_IN_XSS)
    if(SCREENSAVER_IN_XSS)
      set(XSS_LIB "Xss")
    endif()
  endif()
  
  if (XSS_LIB)
    check_include_files(X11/extensions/scrnsaver.h HAVE_SCREENSAVER)
  endif()
endif()  

#----------------------------------------------------------------------------------------------------
#  misc checks

check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/select.h HAVE_SYS_SELECT_H)
check_include_files(stdlib.h stddef.h STDC_HEADERS)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(strings.h HAVE_STRINGS_H)

check_function_exists(nanosleep HAVE_NANOSLEEP)
check_function_exists(realpath HAVE_REALPATH)
check_function_exists(setlocale HAVE_SETLOCALE)

#----------------------------------------------------------------------------------------------------
# Locations

if (UNIX AND NOT APPLE)
  set (BINDIR bin)
  set (DATADIR share                                        CACHE PATH "datadir")
  set (LIBDIR lib                                           CACHE PATH "libdir")
  set (PKGDATADIR share/workrave                            CACHE PATH "pkgdatadir")
  set (SOUNDSDIR "${DATADIR}/sounds/workrave")
  set (EXERCISESDIR "${PKGDATADIR}/exercises")
  set (IMAGESDIR "${PKGDATADIR}/images")
  set (LOCALEDIR "${DATADIR}/locale")
elseif (APPLE)
  set (BUNDLEDIR Workrave.app/Contents/)
  set (BINDIR {BUNDLEDIR}/MacOS/)
  set (DATADIR ${BUNDLEDIR}/Resources                       CACHE PATH "datadir")
  set (LIBDIR ${DATADIR}                                    CACHE PATH "libadir")
  set (PKGDATADIR ${DATADIR}                                CACHE PATH "pkgdatadir")
  set (SOUNDSDIR "${DATADIR}/sounds")
  set (EXERCISESDIR "${DATADIR}/exercises")
  set (IMAGESDIR "${DATADIR}/images")
  set (LOCALEDIR "${DATADIR}/locale")
elseif (WIN32)
  set (BINDIR lib)
  set (DATADIR share                                        CACHE PATH "datadir")
  set (LIBDIR lib                                           CACHE PATH "libdir")
  set (PKGDATADIR share/workrave                            CACHE PATH "pkgdatadir")
  set (SOUNDSDIR "${DATADIR}/sounds")
  set (EXERCISESDIR "${DATADIR}/exercises")
  set (IMAGESDIR "${DATADIR}/images")
  set (LOCALEDIR "${LIBDIR}/locale")
endif()

add_definitions(
  -DWORKRAVE_PKGDATADIR="${CMAKE_INSTALL_PREFIX}/${PKGDATADIR}"
  -DWORKRAVE_DATADIR="${CMAKE_INSTALL_PREFIX}/${DATADIR}"
)

#----------------------------------------------------------------------------------------------------
# Autoconf compatibility

# compiler definitions 
add_definitions(-DHAVE_CONFIG_H=1)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Generate config.h
configure_file(${CMAKE_SOURCE_DIR}/build/cmake/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

#----------------------------------------------------------------------------------------------------
# Make dist.

set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${PACKAGE_VERSION})
add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

#----------------------------------------------------------------------------------------------------
# 

add_subdirectory(po)
add_subdirectory(libs)
add_subdirectory(backend)
add_subdirectory(frontend)

#----------------------------------------------------------------------------------------------------
# Install

if (UNIX AND NOT APPLE)
  install(FILES README NEWS AUTHORS COPYING DESTINATION ${DATADIR}/doc/workrave)
elseif (APPLE)
  install(FILES README NEWS AUTHORS COPYING DESTINATION ${DATADIR}/doc)
elseif (WIN32)
  configure_file(README  README.txt  NEWLINE_STYLE WIN32)
  configure_file(NEWS    NEWS.txt    NEWLINE_STYLE WIN32)
  configure_file(AUTHORS AUTHORS.txt NEWLINE_STYLE WIN32)
  configure_file(COPYING COPYING.txt NEWLINE_STYLE WIN32)

  install(
    FILES ${CMAKE_BINARY_DIR}/README.txt ${CMAKE_BINARY_DIR}/NEWS.txt ${CMAKE_BINARY_DIR}/AUTHORS.txt ${CMAKE_BINARY_DIR}/COPYING.txt 
    DESTINATION ${DATADIR}/doc/workrave)
endif()

if (MSVC)
   set(APPLET_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/frontend/applets/win32/src/Output")
   set(HOOK_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/hooks/Output")
elseif(WIN32)
   string(REPLACE "/" "\\" APPLET_BINARY_DIR ${PREBUILT_PATH})
   string(REPLACE "/" "\\" HOOK_BINARY_DIR ${PREBUILT_PATH})
endif()

#----------------------------------------------------------------------------------------------------
# App Bundle

set(CPACK_PACKAGE_NAME                  "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR                "Rob Caelers, Raymond Penners, Ray Satiro")
set(CPACK_PACKAGE_EXECUTABLES           "${PROJECT_NAME};${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION                "${VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR         ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR         ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH         ${VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY   "Repetitive Strain Injury prevention tool")
set(CPACK_PACKAGE_FILE_NAME             "${PROJECT_NAME} v${VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY     "${PROJECT_NAME}")
set(CPACK_PACKAGE_ICON                  "${CMAKE_SOURCE_DIR}/frontend/data/images/osx/workrave.icns")
set(CPACK_RESOURCE_FILE_LICENSE         "${CMAKE_SOURCE_DIR}/COPYING")

if (APPLE)
  set(CPACK_GENERATOR "DragNDrop")

  configure_file(${CMAKE_SOURCE_DIR}/frontend/apps/qt5/dist/osx/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist)

  set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
  set(CPACK_DMG_DS_STORE "${CMAKE_SOURCE_DIR}/frontend/apps/qt5/dist/osx/DS_Store")
  set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/frontend/apps/qt5/dist/osx/dmg_background.png")
  set(CPACK_DMG_FORMAT "UDBZ")

  set(CPACK_OSX_PACKAGE_VERSION "10.6")
endif()

set(CPACK_BINARY_DRAGNDROP ON)
include(CPack)

if (APPLE)
  add_subdirectory(frontend/apps/qt5/dist/osx)
elseif(WIN32)
  if ("${WITH_UI}" STREQUAL "Qt5")
    add_subdirectory(frontend/apps/qt5/dist/win32)
  elseif ("${WITH_UI}" STREQUAL "Gtk+2")
    add_subdirectory(frontend/apps/gtkmm/dist/win32)
  elseif ("${WITH_UI}" STREQUAL "Gtk+3")
    add_subdirectory(frontend/apps/gtkmm/dist/win32)
  endif()
endif()