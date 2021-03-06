###############################################################################

# CMake build scripts for PKTOOLS
#
#
# History
# 2010/12/14 - Created by Peter Bunting for SPDLib
# 2012/02/07 - Edited by Peter Bunting for PKTOOLS
# 2014/11/22 - Edited by Peter Bunting for pktools
# 2015/12/10 - Edited by Pieter Kempeneers and Kris Vanhoof
# Change log
# Find package lib
# Boost: disable auto linking: add_definitions("-DBOOST_ALL_NO_LIB")
# add cmake env variable Boost_COMPILER -vc110
# FANN: create library CMake: add ADD_DEFINITIONS(-DFANN_DLL_EXPORTS)
# GDAL: disabled macro for Visual Studio 2015
#if defined(_MSC_VER) && (_MSC_VER < 1900)
#  define snprintf _snprintf
#endif
#set CMAKE_INSTALL_PREFIX to C:/OSGeo4W64;C:/Users/kempenep/Downloads/FANN-2.2.0-Source/FANN-2.2.0-Source/install;C:/Users/kempenep/Downloads/armadillo-6.200.4/install;C:\Users\kempenep\Downloads\nlopt-2.4.2-dll64
#
###############################################################################
cmake_minimum_required(VERSION 2.6.0)
#cmake_minimum_required(VERSION 2.8)
###############################################################################
# Set Project name and version
project (PKTOOLS)

set (PROJECT_BINARY_DIR bin)
set (PROJECT_LIBRARY_DIR lib)
set (PROJECT_INCLUDE_DIR include/pktools)
set (PROJECT_DOC_DIR doc)
set (PROJECT_SOURCE_DIR src)

# The version number.
set (PKTOOLS_VERSION_MAJOR 2)
set (PKTOOLS_VERSION_MINOR 6)
set (PKTOOLS_VERSION_PATCH 8)
set (PKTOOLS_VERSION "${PKTOOLS_VERSION_MAJOR}.${PKTOOLS_VERSION_MINOR}.${PKTOOLS_VERSION_PATCH}")
set (PKTOOLS_PACKAGE_VERSION "${PKTOOLS_VERSION_MAJOR}.${PKTOOLS_VERSION_MINOR}.${PKTOOLS_VERSION_PATCH}")
set (PKTOOLS_PACKAGE_STRING "PKTOOLS ${PKTOOLS_VERSION_MAJOR}.${PKTOOLS_VERSION_MINOR}.${PKTOOLS_VERSION_PATCH}")
set (PKTOOLS_PACKAGE_BUGREPORT "kempenep@gmail.com")
set (PKTOOLS_PACKAGE "pktools")
set (PKTOOLS_COPYRIGHT_YEAR 2016)

set (PKTOOLS_BASE_VERSION_MAJOR 1)
set (PKTOOLS_BASE_VERSION_MINOR 7)
set (PKTOOLS_BASE_VERSION_PATCH 0)
set (PKTOOLS_BASE_VERSION "${PKTOOLS_BASE_VERSION_MAJOR}.${PKTOOLS_BASE_VERSION_MINOR}.${PKTOOLS_BASE_VERSION_PATCH}")
set (PKTOOLS_BASE_SOVERSION 1)

set (PKTOOLS_ALGORITHMS_VERSION_MAJOR 1)
set (PKTOOLS_ALGORITHMS_VERSION_MINOR 7)
set (PKTOOLS_ALGORITHMS_VERSION_PATCH 0)
set (PKTOOLS_ALGORITHMS_VERSION "${PKTOOLS_ALGORITHMS_VERSION_MAJOR}.${PKTOOLS_ALGORITHMS_VERSION_MINOR}.${PKTOOLS_ALGORITHMS_VERSION_PATCH}")
set (PKTOOLS_ALGORITHMS_SOVERSION 1)

set (PKTOOLS_FILECLASSES_VERSION_MAJOR 1)
set (PKTOOLS_FILECLASSES_VERSION_MINOR 7)
set (PKTOOLS_FILECLASSES_VERSION_PATCH 0)
set (PKTOOLS_FILECLASSES_VERSION "${PKTOOLS_FILECLASSES_VERSION_MAJOR}.${PKTOOLS_FILECLASSES_VERSION_MINOR}.${PKTOOLS_FILECLASSES_VERSION_PATCH}")
set (PKTOOLS_FILECLASSES_SOVERSION 1)

set (PKTOOLS_IMAGECLASSES_VERSION_MAJOR 1)
set (PKTOOLS_IMAGECLASSES_VERSION_MINOR 7)
set (PKTOOLS_IMAGECLASSES_VERSION_PATCH 0)
set (PKTOOLS_IMAGECLASSES_VERSION "${PKTOOLS_IMAGECLASSES_VERSION_MAJOR}.${PKTOOLS_IMAGECLASSES_VERSION_MINOR}.${PKTOOLS_IMAGECLASSES_VERSION_PATCH}")
set (PKTOOLS_IMAGECLASSES_SOVERSION 1)

set (PKTOOLS_LASCLASSES_VERSION_MAJOR 1)
set (PKTOOLS_LASCLASSES_VERSION_MINOR 7)
set (PKTOOLS_LASCLASSES_VERSION_PATCH 0)
set (PKTOOLS_LASCLASSES_VERSION "${PKTOOLS_LASCLASSES_VERSION_MAJOR}.${PKTOOLS_LASCLASSES_VERSION_MINOR}.${PKTOOLS_LASCLASSES_VERSION_PATCH}")
set (PKTOOLS_LASCLASSES_SOVERSION 1)

# Name of C++ library
set(PKTOOLS_BASE_LIB_NAME base)
set(PKTOOLS_ALGORITHMS_LIB_NAME algorithms)
set(PKTOOLS_FILECLASSES_LIB_NAME fileClasses)
set(PKTOOLS_IMAGECLASSES_LIB_NAME imageClasses)
set(PKTOOLS_LASCLASSES_LIB_NAME lasClasses)

option (BUILD_SHARED_LIBS "Build with shared library" ON)
set(BUILD_WITH_LIBLAS FALSE CACHE BOOL "Choose if pktools is to be built with liblas")
set(BUILD_WITH_PYTHON TRUE CACHE BOOL "Choose if pktools is to be built with PYTHON support")
set(PKTOOLS_WITH_APPS TRUE CACHE BOOL "Choose if pktools applications should be built")

set(GSL_INCLUDE_DIR /usr/include CACHE PATH "Include PATH for GSL")
set(GSL_LIB_PATH /usr/lib CACHE PATH "Library PATH for GSL")

set(PKTOOLS_INCLUDE_DIRS ${PREFIX}/include/pktools)
# set(PKTOOLS_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")

###############################################################################

###############################################################################
# CMake settings
# cmake_minimum_required(VERSION 2.6.0)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "RELEASE")
ENDIF()

set(CMAKE_COLOR_MAKEFILE ON)

# Allow advanced users to generate Makefiles printing detailed commands
mark_as_advanced(CMAKE_VERBOSE_MAKEFILE)

# Path to additional CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
###############################################################################

###############################################################################
# Documentation
#file(READ "doc/index.txt" README )
file(WRITE "README.txt" "${README}")
###############################################################################

###############################################################################
# Platform and compiler specific settings

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has C++11 support.")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else(COMPILER_SUPPORTS_CXX11)
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

if(WIN32)
    if (MSVC)
        if (MSVC80 OR MSVC90 OR MSVC10)
            add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
            add_definitions(-D_CRT_SECURE_NO_WARNINGS)
            add_definitions(-D_CRT_NONSTDC_NO_WARNING)
            add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        endif()

        if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
            string(REGEX REPLACE "/W[0-4]" "/W4"
                   CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
        endif()

        # Generate dot-user file with user-specific settings for Visual Studio project
        set(MSVC_ENVIRONMENT_PATH "" CACHE STRING
            "Custom PATH for Environment property in Visual Studio project configuration")
        mark_as_advanced(MSVC_ENVIRONMENT_PATH)
        set(VCPROJ_USER_ENVIRONMENT_PATH_DEBUG "${ENVIRONMENT_PATH}")
        mark_as_advanced(VCPROJ_USER_ENVIRONMENT_PATH_DEBUG)

    endif()

  set(CMAKE_INCLUDE_PATH c:/osgeo4w/include;$ENV{CMAKE_INCLUDE_PATH})
    set(CMAKE_LIBRARY_PATH c:/osgeo4w/lib;$ENV{CMAKE_LIBRARY_PATH})
else()
  # Recommended C++ compilation flags
  # -Weffc++
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
  set(PKTOOLS_COMMON_CXX_FLAGS
    "-Wall -Wpointer-arith -Wcast-align -Wcast-qual -Wredundant-decls -Wno-long-long")

  if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC ${PKTOOLS_COMMON_CXX_FLAGS}")

  elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER}" MATCHES "clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PKTOOLS_COMMON_CXX_FLAGS}")
  endif()
endif(WIN32)

if (APPLE)
  set(SO_EXT dylib)
elseif(WIN32)
  set(SO_EXT dll)
else()
  set(SO_EXT so)
endif(APPLE)
###############################################################################

###############################################################################
# Check the required libraries are present

find_package(GDAL 1.10 REQUIRED)
if(GDAL_FOUND)
  include_directories(${GDAL_INCLUDE_DIRS})
endif()

find_package(GSL REQUIRED)
if(GSL_FOUND)
  include_directories(${GSL_INCLUDE_DIRS})
  add_definitions(-DGSL_DLL)
endif()

if (BUILD_WITH_LIBLAS)
  find_package(LIBLAS)
  if(LIBLAS_FOUND)
    include_directories(${LIBLAS_INCLUDE_DIRS})
  endif()
  set(Boost_DEBUG 0)
  find_package(Boost COMPONENTS filesystem system)
  if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_definitions("-DBOOST_ALL_NO_LIB")
  endif()

#	include_directories(${BOOST_INCLUDE_DIR})
#	if (MSVC)
#		set(BOOST_LIBRARIES -LIBPATH:${BOOST_LIB_PATH} libboost_filesystem-vc100-mt-1_56.lib libboost_system-vc100-mt-1_56.lib)
#	else()
#		set(BOOST_LIBRARIES -L${BOOST_LIB_PATH} -lboost_filesystem -lboost_system)
#	endif(MSVC)
endif(BUILD_WITH_LIBLAS)

find_package(FANN REQUIRED)
if(FANN_FOUND)
  include_directories(${FANN_INCLUDE_DIRS})
  add_definitions(-DFANN_DLL)
endif()

###############################################################################

###############################################################################
# Setup configure file
configure_file ( "cmake-config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/base/config.h" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/base")
add_definitions(-DHAVE_CONFIG_H)
###############################################################################

###############################################################################
# Build libraries
include_directories("${PROJECT_SOURCE_DIR}")
add_subdirectory ("${PROJECT_SOURCE_DIR}")
###############################################################################

###############################################################################
# Build executables

#configure_file ( "cmake-config.in" "${PROJECT_BINARY_DIR}/pktools-config" @ONLY )
#test
configure_file ( "cmake-config.in" "${CMAKE_CURRENT_BINARY_DIR}/pktools-config" @ONLY )
configure_file ( "cmake.pc.in" "pktools.pc"  @ONLY)
configure_file ( "pktools-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/pktools-config.cmake" @ONLY )


set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# set(PKLIBS ${GSL_LIBRARIES} ${PKTOOLS_BASE_LIB_NAME} ${PKTOOLS_IMAGECLASSES_LIB_NAME} ${PKTOOLS_FILECLASSES_LIB_NAME} ${PKTOOLS_ALGORITHMS_LIB_NAME} ${GDAL_LIBRARIES} ${GSL_LIBRARIES} ${Boost_LIBRARIES} ${GSL_LIBRARIES} )

set(PKLIBS ${GSL_LIBRARIES} ${PKTOOLS_BASE_LIB_NAME} ${PKTOOLS_IMAGECLASSES_LIB_NAME} ${PKTOOLS_FILECLASSES_LIB_NAME} ${PKTOOLS_ALGORITHMS_LIB_NAME} ${GDAL_LIBRARIES} ${GSL_LIBRARIES} ${Boost_LIBRARIES} ${GSL_LIBRARIES} ${FANN_LIBRARIES})

set(PKAPPS
  pkcrop
  pkcomposite
  pkinfo
  pkextractogr
  pkextractimg
  pkstatprofile
  pkfilter
  pktest
  pksvm
  pkann
  pksvmogr
  pkdiff
  pkvalidate
  pkreclass
  pksetmask
  pkgetmask
  pkdumpimg
  )

set(PKUTILS
  pkcreatect
  pksieve
  pkstat
  pkegcs
  pkfillnodata
  pkfilterdem
  #pkdsm2shadow
  pkpolygonize

  pkkalman

  pkdumpogr
  pkfssvm
  pkoptsvm
  pkstatascii
  pkstatogr
  pkfilterascii
  pkascii2img
  pkascii2ogr
  pkfsann
  pkregann
)

set(PKLASAPPS
  pklas2img
)

if (PKTOOLS_WITH_APPS)
  foreach(APP ${PKAPPS})
        add_executable(${APP} src/apps/${APP}_bin.cc)
        target_link_libraries(${APP} ${PKLIBS})
        set_target_properties(${APP} PROPERTIES FOLDER apps)
  endforeach()

  foreach(UTIL ${PKUTILS})
        add_executable(${UTIL} src/apps/${UTIL}.cc)
        target_link_libraries(${UTIL} ${PKLIBS})
        set_target_properties(${UTIL} PROPERTIES FOLDER apps)
  endforeach()

  if (BUILD_WITH_LIBLAS)
    set(PKLIBS ${PKLIBS} ${PKTOOLS_LASCLASSES_LIB_NAME})
    foreach(PKLASAPP ${PKLASAPPS})
    add_executable(${PKLASAPP} src/apps/${PKLASAPP}.cc)
    target_link_libraries (${PKLASAPP}
    ${PKLIBS} ${LIBLAS_LIBRARIES})
        set_target_properties(${PKLASAPP} PROPERTIES FOLDER apps)
    endforeach()
  endif()
  add_custom_target(apps DEPENDS ${PKAPPS} ${PKTUTILS} ${PKLASAPPS})
  set_target_properties(apps PROPERTIES FOLDER
  phony)
endif (PKTOOLS_WITH_APPS)

###############################################################################

###############################################################################
# Installation
#install (FILES "${PROJECT_BINARY_DIR}/pktools-config" DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
#install (FILES "${PROJECT_BINARY_DIR}/pktools-config" DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
#test
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/pktools-config" DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install (FILES "pktools.pc" DESTINATION lib/pkgconfig PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pktools-config.cmake" DESTINATION lib/cmake/pktools)

if (PKTOOLS_WITH_APPS)
  install (TARGETS ${PKAPPS} DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  install (TARGETS ${PKUTILS} DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  if (BUILD_WITH_LIBLAS)
    install (TARGETS ${PKLASAPPS} DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  endif(BUILD_WITH_LIBLAS)
endif(PKTOOLS_WITH_APPS)

###############################################################################

# Create Distribution
SET(CPACK_SOURCE_GENERATOR "TGZ;ZIP;TBZ2")
SET(CPACK_CMAKE_GENERATOR "Unix Makefiles")

set(CPACK_SOURCE_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${PKTOOLS_VERSION_MAJOR}.${PKTOOLS_VERSION_MINOR}.${PKTOOLS_VERSION_PATCH}")

# Set files to ignore
list(APPEND CPACK_SOURCE_IGNORE_FILES "_CPack_Packages")
list(APPEND CPACK_SOURCE_IGNORE_FILES ".gz")
list(APPEND CPACK_SOURCE_IGNORE_FILES ".bz2")
list(APPEND CPACK_SOURCE_IGNORE_FILES ".zip")
list(APPEND CPACK_SOURCE_IGNORE_FILES ".svn")
list(APPEND CPACK_SOURCE_IGNORE_FILES "README")
list(APPEND CPACK_SOURCE_IGNORE_FILES "HOWTORELEASE.txt")
list(APPEND CPACK_SOURCE_IGNORE_FILES "CMakeCache.txt")
list(APPEND CPACK_SOURCE_IGNORE_FILES "CPackConfig.cmake")
list(APPEND CPACK_SOURCE_IGNORE_FILES "schemas")

include(CPack)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
###############################################################################
