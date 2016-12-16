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

#include <map>

#include <ome/files/tiff/Codec.h>

#include <ome/compat/memory.h>

#include <tiffio.h>

using ome::xml::model::enums::PixelType;

namespace ome
{
  namespace files
  {
    namespace tiff
    {

      const std::vector<Codec>&
      getCodecs()
      {
        static std::vector<Codec> ret;

        if(ret.empty())
          {
            ome::compat::shared_ptr<TIFFCodec> codecs(TIFFGetConfiguredCODECs(), _TIFFfree);
            if (codecs)
              {
                for (const TIFFCodec *c = &*codecs; c->name != 0; ++c)
                  {
                    Codec nc;
                    nc.name = c->name;
                    nc.scheme = static_cast<Compression>(c->scheme);
                    ret.push_back(nc);
                  }
              }
          }

        return ret;
      }

      const std::vector<std::string>&
      getCodecNames()
      {
        static std::vector<std::string> ret;

        if(ret.empty())
          {
            const std::vector<Codec>& codecs = getCodecs();
            for (std::vector<Codec>::const_iterator i = codecs.begin();
                 i != codecs.end();
                 ++i)
              ret.push_back(i->name);
          }

        return ret;
      }

      const std::vector<std::string>&
      getCodecNames(PixelType pixeltype)
      {
        static std::map<PixelType, std::vector<std::string> > pixeltypes;

        std::map<PixelType, std::vector<std::string> >::const_iterator found = pixeltypes.find(pixeltype);
        if(found == pixeltypes.end())
          {
            std::vector<std::string> ptcodecs;
            const std::vector<Codec>& codecs = getCodecs();
            for (std::vector<Codec>::const_iterator i = codecs.begin();
                 i != codecs.end();
                 ++i)
              {
                switch(i->scheme)
                  {
                    // Don't expose directly since it's not a real
                    // codec and the API uses an optional here to
                    // signify no compression.
                  case COMPRESSION_NONE:
                    break;

                    // Bilevel codecs
                  case COMPRESSION_CCITTRLE:
                  case COMPRESSION_CCITT_T4:
                  case COMPRESSION_CCITT_T6:
                  case COMPRESSION_CCITTRLEW:
                  case COMPRESSION_PACKBITS:
                  case COMPRESSION_T85:
                  case COMPRESSION_T43:
                  case COMPRESSION_JBIG: // also for other pixel types
                                         // but there are better
                                         // choices for other types
                    if (pixeltype == PixelType::BIT)
                      ptcodecs.push_back(i->name);
                    break;

                    // Codecs which work with all pixel types.
                  case COMPRESSION_LZW:
                  case COMPRESSION_ADOBE_DEFLATE:
                  case COMPRESSION_DEFLATE:
                  case COMPRESSION_LZMA:
                  case COMPRESSION_JP2000:
                    ptcodecs.push_back(i->name);
                    break;

                    // JPEG compression of 8-bit data (12-bit not
                    // supported by default, and this interface does
                    // not cater for samples per pixel or bits per
                    // sample when querying)
                  case COMPRESSION_JPEG:
                    if (pixeltype == PixelType::UINT8)
                      ptcodecs.push_back(i->name);
                    break;

                    // Compatibility codecs for decompression only.
                  case COMPRESSION_OJPEG:
                    break;

                    // Codecs incompatible with all pixel types (ignore).
                  case COMPRESSION_NEXT: // 2-bit
                  case COMPRESSION_THUNDERSCAN: // 4-bit
                  case COMPRESSION_PIXARFILM: // Pixar-specific
                  case COMPRESSION_PIXARLOG: // Pixar-specific
                  case COMPRESSION_SGILOG: // SGI-specific
                  case COMPRESSION_SGILOG24: // SGI-specific
                  case COMPRESSION_DCS: // Kodac-specific 10/12-bit
                  case COMPRESSION_IT8CTPAD: // Prepress image data
                  case COMPRESSION_IT8LW: // Prepress image data
                  case COMPRESSION_IT8MP: // Prepress image data
                  case COMPRESSION_IT8BL: // Prepress image data
                    break;

                    // Allow by default so we support codecs we don't
                    // know about (but use with incompatible pixel
                    // types at own risk).
                  default:
                    ptcodecs.push_back(i->name);
                    break;
                  }
              }

            std::pair<std::map<PixelType, std::vector<std::string> >::iterator, bool> inserted = pixeltypes.insert(std::make_pair(pixeltype, ptcodecs));
            found = inserted.first;
          }

        return found->second;
      }

      Compression
      getCodecScheme(const std::string& name)
      {
        static std::map<std::string, Codec> cmap;

        if(cmap.empty())
          {
            const std::vector<Codec>& codecs = getCodecs();
            for (std::vector<Codec>::const_iterator i = codecs.begin();
                 i != codecs.end();
                 ++i)
              cmap.insert(std::make_pair(i->name, *i));
          }

        Compression ret = static_cast<Compression>(COMPRESSION_NONE);
        std::map<std::string, Codec>::const_iterator found = cmap.find(name);

        if (found != cmap.end())
          ret = found->second.scheme;

        return ret;
      }
    }
  }
}
