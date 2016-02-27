# #%L
# OME Files C++ libraries (cmake build infrastructure)
# %%
# Copyright © 2006 - 2015 Open Microscopy Environment:
#   - Massachusetts Institute of Technology
#   - National Institutes of Health
#   - University of Dundee
#   - Board of Regents of the University of Wisconsin-Madison
#   - Glencoe Software, Inc.
# %%
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# The views and conclusions contained in the software and documentation are
# those of the authors and should not be interpreted as representing official
# policies, either expressed or implied, of any organization.
# #L%

cmake_minimum_required(VERSION 3.2.0)

# Default policy is from 3.2.0
cmake_policy(VERSION 3.2.0)

# For MSVC
enable_language(CXX)
# For feature tests with CMake 2.8.x
enable_language(C)

list(APPEND CMAKE_MODULE_PATH
     "${CMAKE_CURRENT_LIST_DIR}/cmake")

include("${CMAKE_CURRENT_LIST_DIR}/cmake/BioFormatsCommon.cmake")

project(ome-files)

include(cmake/Version.cmake)
ome_project_version("OME Files C++" "${CMAKE_CURRENT_SOURCE_DIR}")

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
  message(FATAL_ERROR "In-tree builds are not supported; please run cmake from a separate build directory.")
endif("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")

enable_language(CXX)
  set(BUILD_SHARED_LIBS_DEFAULT ON)
if(MSVC)
  set(BUILD_SHARED_LIBS_DEFAULT OFF)
endif()
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_DEFAULT} CACHE BOOL "Use shared libraries")

include(GNUInstallDirs)
include(CompilerChecks)
include(PlatformChecks)
include(BoostChecks)
include(RegexChecks)
include(ThreadChecks)
include(XercesChecks)
include(XalanChecks)
include(ImageLibraries)
include(QtGLChecks)
include(GTest)
include(Doxygen)
include(HeaderTest)

find_package(OME 5.2.0 REQUIRED Compat Common XML)

if(MSVC)
  # Debug library suffix.
  set(CMAKE_DEBUG_POSTFIX "d")
  # Require dynamic linking and disable autolinking.
  add_definitions(-DBOOST_ALL_DYN_LINK -DBOOST_ALL_NO_LIB)
  # To prevent a lot of spam warnings about standard POSIX functions
  # and unsafe use of the standard library.
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif()


if(NOT DEFINED OME_FILES_INSTALL_PKGDATADIR)
  set(OME_FILES_INSTALL_PKGDATADIR "${CMAKE_INSTALL_DATADIR}/ome/files"
      CACHE PATH "OME Files-specific datadir")
endif()

# Use standard path if using a prefix.
if(CMAKE_INSTALL_PREFIX)
  if (NOT DEFINED OME_FILES_INSTALL_PKGLIBEXECDIR)
    set(OME_FILES_INSTALL_PKGLIBEXECDIR "${CMAKE_INSTALL_LIBEXECDIR}/ome/files"
        CACHE PATH "OME Files-specific libexecdir")
  endif()
  if (NOT DEFINED OME_FILES_INSTALL_ICONDIR)
    set(OME_FILES_INSTALL_ICONDIR "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable"
        CACHE PATH "Icon directory")
  endif()
else()
  if (NOT DEFINED OME_FILES_INSTALL_PKGLIBEXECDIR)
    set(OME_FILES_INSTALL_PKGLIBEXECDIR "${CMAKE_INSTALL_LIBEXECDIR}"
        CACHE PATH "OME Files-specific libexecdir")
  endif()
  if (NOT DEFINED OME_FILES_INSTALL_ICONDIR)
    set(OME_FILES_INSTALL_ICONDIR "${CMAKE_INSTALL_DATADIR}/icons"
        CACHE PATH "Icon directory")
  endif()
endif()

foreach(dir
    PKGDATADIR
    PKGLIBEXECDIR
    ICONDIR
  )
  mark_as_advanced(OME_FILES_INSTALL_${dir})
endforeach()

add_subdirectory(docs/doxygen)
add_subdirectory(docs/sphinx)

set(OME_TOPLEVEL_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Add test, making sure it's run with a suitable environment.
# If updated, make sure to update the wrappers in cmake/.
function(ome_files_add_test)
  add_test(${ARGV})
endfunction()

add_subdirectory(lib)
add_subdirectory(libexec)
add_subdirectory(share/icons)
add_subdirectory(test)
add_subdirectory(examples)

set(LIBRARY_PREFIX OME)
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
configure_file(${PROJECT_SOURCE_DIR}/cmake/TemplateShellConfig.cmake.in
               ${PROJECT_BINARY_DIR}/config @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/cmake/TemplateInternalShellWrapper.cmake.in
               ${PROJECT_BINARY_DIR}/cmake/ome-files @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/cmake/TemplateShellWrapper.cmake.in
               ${PROJECT_BINARY_DIR}/bin/ome-files @ONLY)
file(COPY ${PROJECT_BINARY_DIR}/cmake/ome-files
     DESTINATION ${PROJECT_BINARY_DIR}
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
     GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/ome-files
        DESTINATION ${CMAKE_INSTALL_BINDIR})
if(WIN32)
  configure_file(${PROJECT_SOURCE_DIR}/cmake/TemplateCmdConfig.cmake.in
                 ${PROJECT_BINARY_DIR}/config.bat @ONLY)
  configure_file(${PROJECT_SOURCE_DIR}/cmake/TemplateInternalCmdWrapper.cmake.in
                 ${PROJECT_BINARY_DIR}/ome-files.bat @ONLY)
  configure_file(${PROJECT_SOURCE_DIR}/cmake/TemplateCmdWrapper.cmake.in
                 ${PROJECT_BINARY_DIR}/bin/ome-files.bat @ONLY)
  install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/ome-files.bat
          DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
return()
