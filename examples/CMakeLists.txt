# #%L
# OME C++ libraries (cmake build infrastructure)
# %%
# Copyright © 2015 Open Microscopy Environment:
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

include_directories(${OME_TOPLEVEL_INCLUDES}
                    ${XercesC_INCLUDE_DIRS})

set(exampledir "${PROJECT_SOURCE_DIR}/docs/sphinx/examples")

add_executable(model-io "${exampledir}/model-io.cpp")
target_link_libraries(model-io OME::XML ${CMAKE_THREAD_LIBS_INIT})

add_executable(metadata-io "${exampledir}/metadata-io.cpp")
target_link_libraries(metadata-io OME::XML OME::Files ${CMAKE_THREAD_LIBS_INIT})

add_executable(metadata-formatreader "${exampledir}/metadata-formatreader.cpp")
target_link_libraries(metadata-formatreader OME::XML OME::Files ${CMAKE_THREAD_LIBS_INIT})

add_executable(metadata-formatwriter "${exampledir}/metadata-formatwriter.cpp")
target_link_libraries(metadata-formatwriter OME::XML OME::Files ${CMAKE_THREAD_LIBS_INIT})

add_executable(metadata-formatwriter2 "${exampledir}/metadata-formatwriter2.cpp")
target_link_libraries(metadata-formatwriter2 OME::XML OME::Files ${CMAKE_THREAD_LIBS_INIT})

add_executable(pixeldata "${exampledir}/pixeldata.cpp")
target_link_libraries(pixeldata OME::Files ${CMAKE_THREAD_LIBS_INIT})

add_executable(units "${exampledir}/units.cpp")
target_link_libraries(units OME::Files ${CMAKE_THREAD_LIBS_INIT})

if(BUILD_TESTS)
  ome_files_add_test(examples/model-io model-io "${PROJECT_SOURCE_DIR}/test/ome-files/data/18x24y5z5t2c8b-text.ome")
  ome_files_add_test(examples/metadata-io metadata-io "${PROJECT_SOURCE_DIR}/test/ome-files/data/18x24y5z5t2c8b-text.ome")
  ome_files_add_test(examples/metadata-formatreader metadata-formatreader "${PROJECT_SOURCE_DIR}/test/ome-files/data/2010-06-18x24y5z1t2c8b-text.ome.tiff")
  ome_files_add_test(examples/metadata-formatwriter metadata-formatwriter "${CMAKE_CURRENT_BINARY_DIR}/test-write.ome.tiff")
  ome_files_add_test(examples/metadata-formatwriter2 metadata-formatwriter2 "${CMAKE_CURRENT_BINARY_DIR}/test-write2.ome.tiff")
  ome_files_add_test(examples/pixeldata pixeldata "${PROJECT_SOURCE_DIR}/test/ome-files/data/18x24y5z5t2c8b-text.ome")
  ome_files_add_test(examples/units units)
endif(BUILD_TESTS)
