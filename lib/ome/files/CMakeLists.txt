# #%L
# OME C++ libraries (cmake build infrastructure)
# %%
# Copyright © 2006 - 2015 Open Microscopy Environment:
#   - Massachusetts Institute of Technology
#   - National Institutes of Health
#   - University of Dundee
#   - Board of Regents of the University of Wisconsin-Madison
#   - Glencoe Software, Inc.
# Copyright © 2018 Quantitative Imaging Systems, LLC
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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/Version.h @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-internal.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config-internal.h @ONLY)

set(OME_FILES_SOURCES
    CoreMetadata.cpp
    FormatException.cpp
    FormatTools.cpp
    MetadataConfigurable.cpp
    MetadataOptions.cpp
    MetadataTools.cpp
    Modulo.cpp
    module.cpp
    PixelBuffer.cpp
    PixelProperties.cpp
    TileBuffer.cpp
    TileCache.cpp
    TileCoverage.cpp
    UnknownFormatException.cpp
    UnsupportedCompressionException.cpp
    VariantPixelBuffer.cpp
    Version.cpp
    XMLTools.cpp)

set(OME_FILES_HEADERS
    CoreMetadata.h
    CoreMetadataList.h
    FileInfo.h
    FormatException.h
    MetadataMap.h
    FormatHandler.h
    FormatReader.h
    FormatTools.h
    FormatWriter.h
    MetadataConfigurable.h
    MetadataList.h
    MetadataOptions.h
    MetadataTools.h
    Modulo.h
    module.h
    PixelBuffer.h
    PixelProperties.h
    PlaneRegion.h
    TileBuffer.h
    TileCache.h
    TileCoverage.h
    Types.h
    UnknownFormatException.h
    UnsupportedCompressionException.h
    VariantPixelBuffer.h
    XMLTools.h)

set(OME_FILES_GENERATED_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/Version.h)

set(OME_FILES_GENERATED_PRIVATE_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/config-internal.h)

set(OME_FILES_DETAIL_SOURCES
    detail/FormatReader.cpp
    detail/FormatWriter.cpp)

set(OME_FILES_DETAIL_HEADERS
    detail/FormatReader.h
    detail/FormatWriter.h
    detail/OMETIFF.h)

set(OME_FILES_IN_SOURCES
    in/MinimalTIFFReader.cpp
    in/OMETIFFReader.cpp
    in/TIFFReader.cpp)

set(OME_FILES_IN_HEADERS
    in/MinimalTIFFReader.h
    in/OMETIFFReader.h
    in/TIFFReader.h)

set(OME_FILES_OUT_SOURCES
    out/MinimalTIFFWriter.cpp
    out/OMETIFFWriter.cpp)

set(OME_FILES_OUT_HEADERS
    out/MinimalTIFFWriter.h
    out/OMETIFFWriter.h)

set(OME_FILES_TIFF_SOURCES
    tiff/Codec.cpp
    tiff/Exception.cpp
    tiff/Field.cpp
    tiff/IFD.cpp
    tiff/ImageJMetadata.cpp
    tiff/Sentry.cpp
    tiff/Tags.cpp
    tiff/TIFF.cpp
    tiff/TileInfo.cpp
    tiff/Util.cpp)

set(OME_FILES_TIFF_HEADERS
    tiff/config.h
    tiff/Codec.h
    tiff/Exception.h
    tiff/Field.h
    tiff/IFD.h
    tiff/ImageJMetadata.h
    tiff/Sentry.h
    tiff/Tags.h
    tiff/TIFF.h
    tiff/TileInfo.h
    tiff/Types.h
    tiff/Util.h)

set(OME_FILES_SOURCES
    ${OME_FILES_SOURCES}
    ${OME_FILES_DETAIL_SOURCES}
    ${OME_FILES_IN_SOURCES}
    ${OME_FILES_OUT_SOURCES}
    ${OME_FILES_TIFF_SOURCES})

set(OME_FILES_ALL_STATIC_HEADERS
    ${OME_FILES_HEADERS}
    ${OME_FILES_DETAIL_HEADERS}
    ${OME_FILES_IN_HEADERS}
    ${OME_FILES_OUT_HEADERS}
    ${OME_FILES_TIFF_HEADERS})

set(OME_FILES_ALL_GENERATED_HEADERS
    ${OME_FILES_GENERATED_HEADERS})

add_library(ome-files
            ${OME_FILES_SOURCES}
            ${OME_FILES_ALL_STATIC_HEADERS}
            ${OME_FILES_ALL_GENERATED_HEADERS}
            ${OME_FILES_GENERATED_PRIVATE_HEADERS})

target_include_directories(ome-files PUBLIC
                           $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
                           $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/lib>
                           $<BUILD_INTERFACE:${TIFF_INCLUDE_DIR}>)

target_link_libraries(ome-files
                      Threads::Threads
                      OME::Compat
                      OME::Common
                      OME::XML
                      Boost::boost
                      Boost::iostreams
                      Boost::filesystem
                      TIFF::TIFF)

if(WIN32)
  # Boost UUID (≥ 1.67) requires bcrypt on Windows
  target_link_libraries(ome-files bcrypt)
endif()

set_target_properties(ome-files PROPERTIES VERSION ${ome-files_VERSION})

add_library(OME::Files ALIAS ome-files)

if(WIN32)
  set(ome_files_config_dir "cmake")
else()
  set(ome_files_config_dir "${CMAKE_INSTALL_LIBDIR}/cmake/OMEFiles")
endif()

install(TARGETS ome-files
        EXPORT OMEFilesInternal
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT "runtime"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(EXPORT OMEFilesInternal
        DESTINATION "${ome_files_config_dir}"
        NAMESPACE "ome_files_"
        COMPONENT "development")

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/OMEFilesConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/OMEFilesConfig.cmake"
  INSTALL_DESTINATION "${ome_files_config_dir}")
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/OMEFilesConfigVersion.cmake
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OMEFilesConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/OMEFilesConfigVersion.cmake
        DESTINATION "${ome_files_config_dir}"
        COMPONENT "development")

set(ome_files_includedir "${CMAKE_INSTALL_INCLUDEDIR}/ome/files")

install(FILES ${OME_FILES_HEADERS} ${OME_FILES_GENERATED_HEADERS}
        DESTINATION ${ome_files_includedir}
        COMPONENT "development")
install(FILES ${OME_FILES_DETAIL_HEADERS}
        DESTINATION ${ome_files_includedir}/detail
        COMPONENT "development")
install(FILES ${OME_FILES_IN_HEADERS}
        DESTINATION ${ome_files_includedir}/in
        COMPONENT "development")
install(FILES ${OME_FILES_OUT_HEADERS}
        DESTINATION ${ome_files_includedir}/out
        COMPONENT "development")
install(FILES ${OME_FILES_TIFF_HEADERS}
        DESTINATION ${ome_files_includedir}/tiff
        COMPONENT "development")

# Dump header list for testing
header_include_list_write(OME_FILES_ALL_STATIC_HEADERS OME_FILES_ALL_GENERATED_HEADERS ome/files ${PROJECT_BINARY_DIR}/test/ome-files)
