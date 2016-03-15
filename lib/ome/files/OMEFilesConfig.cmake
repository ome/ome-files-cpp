include(CMakeFindDependencyMacro)
find_dependency(OMECommon REQUIRED COMPONENTS Common XML)
find_dependency(Boost 1.46 REQUIRED COMPONENTS boost iostreams filesystem)
find_dependency(TIFF REQUIRED)

include(${CMAKE_CURRENT_LIST_DIR}/OMEFilesInternal.cmake)

add_library(OME::Files INTERFACE IMPORTED)
set_target_properties(OME::Files PROPERTIES INTERFACE_LINK_LIBRARIES ome-files)
