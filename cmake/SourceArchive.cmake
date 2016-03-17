if(SOURCE_ARCHIVE_DIR AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
  message(STATUS "Determining source archive file names")

  execute_process(COMMAND python ${PROJECT_SOURCE_DIR}/scripts/source-archive
                          --release=ome-files-cpp
                          "--target=${SOURCE_ARCHIVE_DIR}"
                          --list
                          --tag=HEAD
                  OUTPUT_VARIABLE srcfiles
                  RESULT_VARIABLE srcfiles_result
                  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
  if (srcfiles_result)
    message(FATAL_ERROR "Failed to determine source archive file names: ${srcfiles_result}")
  endif()

  string(REPLACE "\n" ";" srcfiles "${srcfiles}")
  if(WIN32)
    string(REPLACE "\\" "/" srcfiles "${srcfiles}")
  endif(WIN32)
  list(REMOVE_DUPLICATES srcfiles)

  if(SOURCE_ARCHIVE_DIR)
    add_custom_command(OUTPUT ${srcfiles}
                       COMMAND "${CMAKE_COMMAND}" -E make_directory
                               "${SOURCE_ARCHIVE_DIR}"
                       COMMAND python "${PROJECT_SOURCE_DIR}/scripts/source-archive"
                               --release=ome-files-cpp
                               "--target=${SOURCE_ARCHIVE_DIR}"
                               --tag=HEAD
                       DEPENDS "${PROJECT_SOURCE_DIR}/scripts/source-archive"
                       WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

    add_custom_target(source-archive DEPENDS ${srcfiles})
  endif()
endif()
