/*
 * #%L
 * OME-FILES C++ library for image IO.
 * Copyright © 2006 - 2015 Open Microscopy Environment:
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

#include <ome/files/PixelProperties.h>

#include <stdexcept>

#include <boost/preprocessor.hpp>

using ::ome::xml::model::enums::PixelType;

namespace ome
{
  namespace files
  {

    // No switch default to avoid -Wunreachable-code errors.
    // However, this then makes -Wswitch-default complain.  Disable
    // temporarily.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#define BYTE_PT_CASE(maR, maProperty, maType)                                           \
        case PixelType::maType:                                                         \
          maProperty = PixelProperties< PixelType::maType>::pixel_byte_size();          \
          break;

    pixel_size_type
    bytesPerPixel(::ome::xml::model::enums::PixelType pixeltype)
    {
      pixel_size_type size = 0;

      switch(pixeltype)
        {
          BOOST_PP_SEQ_FOR_EACH(BYTE_PT_CASE, size, OME_XML_MODEL_ENUMS_PIXELTYPE_VALUES);
        }

      return size;
    }

#undef BYTE_PT_CASE

#define BIT_PT_CASE(maR, maProperty, maType)                                  \
        case PixelType::maType:                                               \
          maProperty = PixelProperties< PixelType::maType>::pixel_bit_size(); \
          break;

    pixel_size_type
    bitsPerPixel(::ome::xml::model::enums::PixelType pixeltype)
    {
      pixel_size_type size = 0;

      switch(pixeltype)
        {
          BOOST_PP_SEQ_FOR_EACH(BIT_PT_CASE, size, OME_XML_MODEL_ENUMS_PIXELTYPE_VALUES);
        }

      return size;
    }

#undef BIT_PT_CASE

#define SIGBIT_PT_CASE(maR, maProperty, maType)                                                   \
        case PixelType::maType:                                                                   \
          maProperty = PixelProperties< PixelType::maType>::pixel_significant_bit_size();         \
          break;

    pixel_size_type
    significantBitsPerPixel(::ome::xml::model::enums::PixelType pixeltype)
    {
      pixel_size_type size = 0;

      switch(pixeltype)
        {
          BOOST_PP_SEQ_FOR_EACH(SIGBIT_PT_CASE, size, OME_XML_MODEL_ENUMS_PIXELTYPE_VALUES);
        }

      return size;
    }

#undef SIGBIT_PT_CASE

#define SIGN_PT_CASE(maR, maProperty, maType)                                 \
        case PixelType::maType:                                               \
          maProperty = PixelProperties< PixelType::maType>::is_signed;        \
          break;

    bool
    isSigned(::ome::xml::model::enums::PixelType pixeltype)
    {
      bool is_signed = false;

      switch(pixeltype)
        {
          BOOST_PP_SEQ_FOR_EACH(SIGN_PT_CASE, is_signed, OME_XML_MODEL_ENUMS_PIXELTYPE_VALUES);
        }

      return is_signed;
    }

#undef SIGN_PT_CASE

#define INTEGER_PT_CASE(maR, maProperty, maType)                            \
        case PixelType::maType:                                             \
          maProperty = PixelProperties< PixelType::maType>::is_integer;     \
          break;

    bool
    isInteger(::ome::xml::model::enums::PixelType pixeltype)
    {
      bool is_integer = false;

      switch(pixeltype)
        {
          BOOST_PP_SEQ_FOR_EACH(INTEGER_PT_CASE, is_integer, OME_XML_MODEL_ENUMS_PIXELTYPE_VALUES);
        }

      return is_integer;
    }

#undef INTEGER_PT_CASE

    bool
    isFloatingPoint(::ome::xml::model::enums::PixelType pixeltype)
    {
      return !isInteger(pixeltype);
    }

#define COMPLEX_PT_CASE(maR, maProperty, maType)                            \
        case PixelType::maType:                                             \
          maProperty = PixelProperties< PixelType::maType>::is_complex;     \
          break;

    bool
    isComplex(::ome::xml::model::enums::PixelType pixeltype)
    {
      bool is_complex = false;

      switch(pixeltype)
        {
          BOOST_PP_SEQ_FOR_EACH(COMPLEX_PT_CASE, is_complex, OME_XML_MODEL_ENUMS_PIXELTYPE_VALUES);
        }

      return is_complex;
    }

#undef COMPLEX_PT_CASE

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

    ::ome::xml::model::enums::PixelType
    pixelTypeFromBytes(pixel_size_type size,
                       bool            is_signed,
                       bool            is_integer,
                       bool            is_complex)
    {
      ::ome::xml::model::enums::PixelType type(::ome::xml::model::enums::PixelType::UINT8);

      if (!is_signed) // integer only
        {
          if (!is_integer || is_complex)
            throw std::logic_error("Unsigned pixel types can't be floating point or complex");

          if (size == bytesPerPixel(::ome::xml::model::enums::PixelType::UINT8))
            type = ::ome::xml::model::enums::PixelType::UINT8;
          else if (size == bytesPerPixel(::ome::xml::model::enums::PixelType::UINT16))
            type = ::ome::xml::model::enums::PixelType::UINT16;
          else if (size == bytesPerPixel(::ome::xml::model::enums::PixelType::UINT32))
            type = ::ome::xml::model::enums::PixelType::UINT32;
          else
            throw std::runtime_error("No suitable unsigned integer pixel type found");
        }
      else // is_signed
        {
          if (is_complex)
            {
              if (is_integer)
                throw std::logic_error("Complex pixel types must be floating point");

              if (size == bytesPerPixel(::ome::xml::model::enums::PixelType::COMPLEXFLOAT))
                type = ::ome::xml::model::enums::PixelType::COMPLEXFLOAT;
              else if (size == bytesPerPixel(::ome::xml::model::enums::PixelType::COMPLEXDOUBLE))
                type = ::ome::xml::model::enums::PixelType::COMPLEXDOUBLE;
              else
                throw std::runtime_error("No suitable complex pixel type found");
            }
          else if (!is_integer)
            {
              if (size == bytesPerPixel(::ome::xml::model::enums::PixelType::FLOAT))
                type = ::ome::xml::model::enums::PixelType::FLOAT;
              else if (size == bytesPerPixel(::ome::xml::model::enums::PixelType::DOUBLE))
                type = ::ome::xml::model::enums::PixelType::DOUBLE;
              else
                throw std::runtime_error("No suitable floating point pixel type found");
            }
          else // integer
            {
              if (size == bytesPerPixel(::ome::xml::model::enums::PixelType::INT8))
                type = ::ome::xml::model::enums::PixelType::INT8;
              else if (size == bytesPerPixel(::ome::xml::model::enums::PixelType::INT16))
                type = ::ome::xml::model::enums::PixelType::INT16;
              else if (size == bytesPerPixel(::ome::xml::model::enums::PixelType::INT32))
                type = ::ome::xml::model::enums::PixelType::INT32;
              else
                throw std::runtime_error("No suitable signed integer pixel type found");
            }
        }

      return type;
    }

    ::ome::xml::model::enums::PixelType
    pixelTypeFromBits(pixel_size_type size,
                      bool            is_signed,
                      bool            is_integer,
                      bool            is_complex)
    {
      if (size % 8)
        throw std::runtime_error("No suitable pixel type found");
      return pixelTypeFromBytes(size / 8, is_signed, is_integer, is_complex);
    }

  }
}
