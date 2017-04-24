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

find_program(GRAPHICSMAGICK_EXECUTABLE gm)

if(GRAPHICSMAGICK_EXECUTABLE)
  foreach(image data-layout)
    foreach(planarconfig chunky planar)
      set(interlace none)
      if(planarconfig STREQUAL planar)
        set(interlace plane)
      endif()
      foreach(imagewidth 32 43 64)
        foreach(imageheight 32 37 64)
          # Generate different reference PNG image sizes
          set(genpng "${CMAKE_CURRENT_BINARY_DIR}/${image}-${imagewidth}x${imageheight}.png")
          add_custom_command(OUTPUT "${genpng}"
                             DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${image}.png"
                             COMMAND "${GRAPHICSMAGICK_EXECUTABLE}" convert
                                     "${CMAKE_CURRENT_SOURCE_DIR}/${image}.png"
                                     -resize "${imagewidth}x${imageheight}!"
                                     "${genpng}")
          list(APPEND images "${genpng}")

          # Generate TIFF without explicit tiling information
          set(gentiff "${CMAKE_CURRENT_BINARY_DIR}/${image}-${imagewidth}x${imageheight}-${planarconfig}.tiff")
          add_custom_command(OUTPUT "${gentiff}"
                             DEPENDS "${genpng}"
                             COMMAND "${GRAPHICSMAGICK_EXECUTABLE}" convert
                             "${genpng}"
                             -interlace "${interlace}"
                             "${gentiff}")
          list(APPEND images "${gentiff}")

          # Generate TIFF tile variants for each image size
          foreach(tilewidth 16 32 48 64)
            foreach(tileheight 16 32 48 64)
              set(gentiff "${CMAKE_CURRENT_BINARY_DIR}/${image}-${imagewidth}x${imageheight}-${planarconfig}-tiles-${tilewidth}x${tileheight}.tiff")
              add_custom_command(OUTPUT "${gentiff}"
                                 DEPENDS "${genpng}"
                                 COMMAND "${GRAPHICSMAGICK_EXECUTABLE}" convert
                                         "${genpng}"
                                         -interlace "${interlace}"
                                         -define "tiff:tile-geometry=${tilewidth}x${tileheight}"
                                         "${gentiff}")
              list(APPEND images "${gentiff}")
            endforeach()
          endforeach()

          # Generate TIFF strip variants for each image size
          foreach(striprows 01 02 05 14 32 60 64)
            set(gentiff "${CMAKE_CURRENT_BINARY_DIR}/${image}-${imagewidth}x${imageheight}-${planarconfig}-strips-${striprows}.tiff")
            add_custom_command(OUTPUT "${gentiff}"
                               DEPENDS "${genpng}"
                               COMMAND "${GRAPHICSMAGICK_EXECUTABLE}" convert
                               "${genpng}"
                                         -interlace "${interlace}"
                                         -define "tiff:rows-per-strip=${striprows}"
                                         "${gentiff}")
            list(APPEND images "${gentiff}")
          endforeach()
        endforeach()
      endforeach()
    endforeach()
  endforeach()
else()
  foreach(image data-layout)
    set(genpng "${CMAKE_CURRENT_BINARY_DIR}/${image}-64x64.png")
    add_custom_command(OUTPUT "${genpng}"
                       DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${image}.png"
                       COMMAND "${CMAKE_COMMAND}" -E copy
                               "${CMAKE_CURRENT_SOURCE_DIR}/${image}.png"
                               "${genpng}")
    list(APPEND images "${genpng}")
  endforeach()
endif()
add_custom_target(gentestimages DEPENDS ${images})
