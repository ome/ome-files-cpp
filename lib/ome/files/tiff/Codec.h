/*
 * #%L
 * OME-FILES C++ library for image IO.
 * Copyright © 2006 - 2014 Open Microscopy Environment:
 *   - Massachusetts Institute of Technology
 *   - National Institutes of Health
 *   - University of Dundee
 *   - Board of Regents of the University of Wisconsin-Madison
 *   - Glencoe Software, Inc.
 * %%
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of any organization.
 * #L%
 */

#ifndef OME_FILES_TIFF_CODEC_H
#define OME_FILES_TIFF_CODEC_H

#include <string>
#include <vector>

#include <ome/files/tiff/Types.h>

#include <ome/xml/model/enums/PixelType.h>

namespace ome
{
  namespace files
  {
    namespace tiff
    {

      // Functions to query TIFF codec support.

      /// A TIFF codec.
      struct Codec
      {
        /// Codec name.
        std::string name;
        /// Codec number.
        Compression scheme;
      };

      /**
       * Get codecs registered with the TIFF library.
       *
       * @returns a list of available codecs (names and numbers).
       */
      const std::vector<Codec>&
      getCodecs();

      /**
       * Get codec names registered with the TIFF library.
       *
       * @returns a list of available codec names.
       */
      const std::vector<std::string>&
      getCodecNames();

      /**
       * Get codec names registered with the TIFF library available
       * for a given pixel type.
       *
       * @param pixetype the pixel type to compress.
       * @returns a list of available codec names.
       */
      const std::vector<std::string>&
      getCodecNames(ome::xml::model::enums::PixelType pixeltype);

      /**
       * Get the compression scheme enumeration for a codec name.
       *
       * @param name the codec name
       * @returns the compression scheme for the name, or
       * COMPRESSION_NONE if invalid.
       */
      Compression
      getCodecScheme(const std::string& name);
    }
  }
}

#endif // OME_FILES_TIFF_CODEC_H

/*
 * Local Variables:
 * mode:C++
 * End:
 */
