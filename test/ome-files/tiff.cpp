/*
 * #%L
 * OME-FILES C++ library for image IO.
 * %%
 * Copyright © 2014 - 2015 Open Microscopy Environment:
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

#include <cstdio>
#include <stdexcept>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/type_traits.hpp>

#include <ome/files/PixelProperties.h>
#include <ome/files/tiff/config.h>
#include <ome/files/tiff/Codec.h>
#include <ome/files/tiff/TileInfo.h>
#include <ome/files/tiff/TIFF.h>
#include <ome/files/tiff/IFD.h>
#include <ome/files/tiff/Field.h>
#include <ome/files/tiff/Exception.h>

#include <ome/compat/regex.h>

#include <ome/test/config.h>
#include <ome/test/test.h>

#include <png.h>

#include "pixel.h"
#include "tiffsamples.h"

using ome::files::tiff::directory_index_type;
using ome::files::tiff::TileInfo;
using ome::files::tiff::TIFF;
using ome::files::tiff::IFD;
using ome::files::tiff::Codec;
using ome::files::dimension_size_type;
using ome::files::significantBitsPerPixel;
using ome::files::VariantPixelBuffer;
using ome::files::PixelBuffer;
using ome::files::PixelProperties;
using ome::files::PlaneRegion;
typedef ome::xml::model::enums::PixelType PT;

using namespace boost::filesystem;

namespace
{

  struct DumpPixelBufferVisitor : public boost::static_visitor<>
  {
    typedef ::ome::files::PixelProperties< ::ome::xml::model::enums::PixelType::BIT>::std_type bit_type;

    std::ostream& stream;

    DumpPixelBufferVisitor(std::ostream& stream):
      stream(stream)
    {}

    template <typename T>
    typename boost::enable_if_c<
      boost::is_integral<T>::value, float
      >::type
    dump(const ome::compat::shared_ptr< ::ome::files::PixelBuffer<T> >& buf,
         const typename ::ome::files::PixelBuffer<T>::indices_type& idx) const
    {
      float v = static_cast<float>(buf->at(idx));
      float max = static_cast<float>(std::numeric_limits<T>::max());

      return v / max;
    }

    template <typename T>
    typename boost::enable_if_c<
      boost::is_floating_point<T>::value, float
      >::type
    dump(const ome::compat::shared_ptr< ::ome::files::PixelBuffer<T> >& buf,
         const typename ::ome::files::PixelBuffer<T>::indices_type& idx) const
    {
      // Assume float is already normalised.
      return static_cast<float>(buf->at(idx));
    }

    template <typename T>
    typename boost::enable_if_c<
      boost::is_complex<T>::value, float
      >::type
    dump(const ome::compat::shared_ptr< ::ome::files::PixelBuffer<T> >& buf,
         const typename ::ome::files::PixelBuffer<T>::indices_type& idx) const
    {
      // Assume float is already normalised.
      return static_cast<float>(buf->at(idx).real());
    }

    // Split the pixel range into two, the lower half being set to false
    // and the upper half being set to true for the destination boolean
    // pixel type.
    float
    dump(const ome::compat::shared_ptr< ::ome::files::PixelBuffer<bit_type> >& buf,
         const ::ome::files::PixelBuffer<bit_type>::indices_type& idx)
    {
      return buf->at(idx) ? 1.0f : 0.0f;
    }

    template<typename T>
    void
    operator()(const T& buf) const
    {
      const VariantPixelBuffer::size_type *shape = buf->shape();
      VariantPixelBuffer::size_type w = shape[ome::files::DIM_SPATIAL_X];
      VariantPixelBuffer::size_type h = shape[ome::files::DIM_SPATIAL_X];
      VariantPixelBuffer::size_type s = shape[ome::files::DIM_SUBCHANNEL];

      typename T::element_type::indices_type idx;
      idx[ome::files::DIM_SPATIAL_X] = 0;
      idx[ome::files::DIM_SPATIAL_Y] = 0;
      idx[ome::files::DIM_SUBCHANNEL] = 0;
      idx[ome::files::DIM_SPATIAL_Z] = idx[ome::files::DIM_TEMPORAL_T] =
        idx[ome::files::DIM_CHANNEL] = idx[ome::files::DIM_MODULO_Z] =
        idx[ome::files::DIM_MODULO_T] = idx[ome::files::DIM_MODULO_C] = 0;

      const char * const shades[] = {" ", "░", "▒", "▓", "█"};

      for (VariantPixelBuffer::size_type y = 0; y < h; ++y)
        {
          std::vector<std::string> line(s);
          for (VariantPixelBuffer::size_type x = 0; x < w; ++x)
            {
              for (VariantPixelBuffer::size_type c = 0; c < s; ++c)
                {
                  idx[ome::files::DIM_SPATIAL_X] = x;
                  idx[ome::files::DIM_SPATIAL_Y] = y;
                  idx[ome::files::DIM_SUBCHANNEL] = c;

                  uint16_t shadeidx = static_cast<uint16_t>(std::floor(dump(buf, idx) * 5.0f));
                  if (shadeidx > 4)
                    shadeidx = 4;
                  line[c] += shades[shadeidx];
                }
            }
          for (std::vector<std::string>::const_iterator i = line.begin();
               i != line.end();
               ++i)
            {
              stream << *i;
              if (i + 1 != line.end())
                stream << "  ";
            }
          stream << '\n';
        }
    }
  };

  void
  dump_image_representation(const VariantPixelBuffer& buf,
                            std::ostream&             stream)
  {
    DumpPixelBufferVisitor v(stream);
    boost::apply_visitor(v, buf.vbuffer());
  }

}

std::vector<TileTestParameters> tile_params(find_tile_tests());

class TIFFTest : public ::testing::Test
{
public:
  boost::filesystem::path tiff_path;

  TIFFTest():
    ::testing::Test(),
    tiff_path(PROJECT_SOURCE_DIR "/test/ome-files/data/2010-06-18x24y5z1t2c8b-text.ome.tiff")
  {
  }

};

TEST_F(TIFFTest, Construct)
{
  ASSERT_NO_THROW(TIFF::open(tiff_path, "r"));
}

TEST_F(TIFFTest, ConstructFailMode)
{
  ASSERT_THROW(TIFF::open(tiff_path, "XK"),
               ome::files::tiff::Exception);
}

TEST_F(TIFFTest, ConstructFailFile)
{
  ASSERT_THROW(TIFF::open(PROJECT_SOURCE_DIR "/CMakeLists.txt", "r"), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, IFDsByIndex)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t =TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  for (directory_index_type i = 0; i < 10; ++i)
    {
      ome::compat::shared_ptr<IFD> ifd;
      ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(i));
    }

  ASSERT_THROW(t->getDirectoryByIndex(40), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, IFDsByOffset)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t =TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  for (directory_index_type i = 0; i < 10; ++i)
    {
      uint64_t offset = t->getDirectoryByIndex(i)->getOffset();
      ome::compat::shared_ptr<IFD> ifd;
      ASSERT_NO_THROW(ifd = t->getDirectoryByOffset(offset));
      ASSERT_EQ(ifd->getOffset(), offset);
    }

  ASSERT_THROW(t->getDirectoryByOffset(0), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, IFDSimpleIter)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd = t->getDirectoryByIndex(0);

  while(ifd)
    {
      ifd = ifd->next();
    }
}

TEST_F(TIFFTest, TIFFIter)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  for (TIFF::iterator pos = t->begin(); pos != t->end(); ++pos)
    {
    }
}

TEST_F(TIFFTest, TIFFConstIter)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  for (TIFF::const_iterator pos = t->begin(); pos != t->end(); ++pos)
    {
    }
}

TEST_F(TIFFTest, TIFFConstIter2)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  for (TIFF::const_iterator pos = t->begin(); pos != t->end(); ++pos)
    {
    }
}

TEST_F(TIFFTest, RawField)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  char *text;
  ifd->getRawField(270, &text);
}

TEST_F(TIFFTest, RawField0)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  char *text;
  ASSERT_THROW(ifd->getRawField(0, &text), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapString)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  std::string text;
  ASSERT_THROW(ifd->getField(ome::files::tiff::ARTIST).get(text), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::COPYRIGHT).get(text), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::DATETIME).get(text), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::DOCUMENTNAME).get(text), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::HOSTCOMPUTER).get(text), ome::files::tiff::Exception);
  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::IMAGEDESCRIPTION).get(text));
  ASSERT_THROW(ifd->getField(ome::files::tiff::MAKE).get(text), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::MODEL).get(text), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::PAGENAME).get(text), ome::files::tiff::Exception);
  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::SOFTWARE).get(text));
  ASSERT_THROW(ifd->getField(ome::files::tiff::TARGETPRINTER).get(text), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapStringArray)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  std::vector<std::string> text;
  ASSERT_THROW(ifd->getField(ome::files::tiff::INKNAMES).get(text), ome::files::tiff::Exception);
  for (std::vector<std::string>::const_iterator i = text.begin(); i != text.end(); ++i)
    {
    }
}

TEST_F(TIFFTest, FieldWrapUInt16)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  uint16_t value;

  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::BITSPERSAMPLE).get(value));
  ASSERT_EQ(8, value);
  ASSERT_THROW(ifd->getField(ome::files::tiff::CLEANFAXDATA).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::DATATYPE).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::INDEXED).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::INKSET).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::MATTEING).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::MAXSAMPLEVALUE).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::MINSAMPLEVALUE).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::RESOLUTIONUNIT).get(value), ome::files::tiff::Exception);
  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::SAMPLESPERPIXEL).get(value));
  ASSERT_EQ(1, value);
}

TEST_F(TIFFTest, FieldWrapCompression)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::files::tiff::Compression value;

  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::COMPRESSION).get(value));
  ASSERT_EQ(ome::files::tiff::COMPRESSION_NONE, value);
}

TEST_F(TIFFTest, FieldWrapFillOrder)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::files::tiff::FillOrder value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::FILLORDER).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapOrientation)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::files::tiff::Orientation value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::ORIENTATION).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapPlanarConfiguration)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::files::tiff::PlanarConfiguration value;

  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::PLANARCONFIG).get(value));
  ASSERT_EQ(ome::files::tiff::SEPARATE, value);
}

TEST_F(TIFFTest, FieldWrapPhotometricInterpretation)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::files::tiff::PhotometricInterpretation value;

  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::PHOTOMETRIC).get(value));
  ASSERT_EQ(ome::files::tiff::MIN_IS_BLACK, value);
}

TEST_F(TIFFTest, FieldWrapPredictor)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::files::tiff::Predictor value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::PREDICTOR).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapSampleFormat)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::files::tiff::SampleFormat value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::SAMPLEFORMAT).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapThreshholding)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::files::tiff::Threshholding value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::THRESHHOLDING).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapYCbCrPosition)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::files::tiff::YCbCrPosition value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::YCBCRPOSITIONING).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapUInt16Pair)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::compat::array<uint16_t, 2> value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::DOTRANGE).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::HALFTONEHINTS).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::PAGENUMBER).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::YCBCRSUBSAMPLING).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapFloat)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  float value = -1.0f;

  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::XRESOLUTION).get(value));
  ASSERT_FLOAT_EQ(1.0f, value);
  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::YRESOLUTION).get(value));
  ASSERT_FLOAT_EQ(1.0f, value);
  ASSERT_THROW(ifd->getField(ome::files::tiff::XPOSITION).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::YPOSITION).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapFloat2)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::compat::array<float, 2> value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::WHITEPOINT).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapFloat3)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::compat::array<float, 3> value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::YCBCRCOEFFICIENTS).get(value), ome::files::tiff::Exception);
}


TEST_F(TIFFTest, FieldWrapFloat6)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::compat::array<float, 6> value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::PRIMARYCHROMATICITIES).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::REFERENCEBLACKWHITE).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapUInt16ExtraSamplesArray)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  std::vector<ome::files::tiff::ExtraSamples> value;
  ASSERT_THROW(ifd->getField(ome::files::tiff::EXTRASAMPLES).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapUInt16Array3)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::compat::array<std::vector<uint16_t>, 3> value;
  ASSERT_THROW(ifd->getField(ome::files::tiff::COLORMAP).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::TRANSFERFUNCTION).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapUInt32)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  uint32_t value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::BADFAXLINES).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::CONSECUTIVEBADFAXLINES).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::GROUP3OPTIONS).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::GROUP4OPTIONS).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::IMAGEDEPTH).get(value), ome::files::tiff::Exception);
  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::IMAGELENGTH).get(value));
  ASSERT_EQ(24U, value);
  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::IMAGEWIDTH).get(value));
  ASSERT_EQ(18U, value);
  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::ROWSPERSTRIP).get(value));
  ASSERT_EQ(1U, value);
  ASSERT_THROW(ifd->getField(ome::files::tiff::SUBFILETYPE).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::TILEDEPTH).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::TILELENGTH).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::TILEWIDTH).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapUInt32Array)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  std::vector<uint32_t> value;
  ASSERT_THROW(ifd->getField(ome::files::tiff::IMAGEJ_META_DATA_BYTE_COUNTS).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::RICHTIFFIPTC).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapUInt64Array)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  std::vector<uint64_t> value;
  ASSERT_THROW(ifd->getField(ome::files::tiff::SUBIFD).get(value), ome::files::tiff::Exception);
  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::STRIPBYTECOUNTS).get(value));
  ASSERT_NO_THROW(ifd->getField(ome::files::tiff::STRIPOFFSETS).get(value));
  ASSERT_THROW(ifd->getField(ome::files::tiff::TILEBYTECOUNTS).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::TILEOFFSETS).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, FieldWrapByteArray)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  std::vector<uint8_t> value;

  ASSERT_THROW(ifd->getField(ome::files::tiff::ICCPROFILE).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::JPEGTABLES).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::PHOTOSHOP).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::XMLPACKET).get(value), ome::files::tiff::Exception);
  ASSERT_THROW(ifd->getField(ome::files::tiff::IMAGEJ_META_DATA).get(value), ome::files::tiff::Exception);
}

TEST_F(TIFFTest, ValueProxy)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  std::string text;
  ome::files::tiff::ValueProxy<std::string> d(text);
  d = ifd->getField(ome::files::tiff::IMAGEDESCRIPTION);
}

TEST_F(TIFFTest, Value)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ome::files::tiff::Value<std::string> text;
  text = ifd->getField(ome::files::tiff::IMAGEDESCRIPTION);
}

TEST_F(TIFFTest, FieldName)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  std::string name;
  name = ifd->getField(ome::files::tiff::IMAGEDESCRIPTION).name();

#if defined(TIFF_HAVE_FIELD) || defined(TIFF_HAVE_FIELDINFO)
  ASSERT_EQ(std::string("ImageDescription"), name);
#else
  ASSERT_EQ(std::string("Unknown"), name);
#endif
}

TEST_F(TIFFTest, FieldCount)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  std::string name;
  bool count = ifd->getField(ome::files::tiff::IMAGEDESCRIPTION).passCount();

  ASSERT_FALSE(count);
}

TEST_F(TIFFTest, PixelType)
{
  ome::compat::shared_ptr<TIFF> t;
  ASSERT_NO_THROW(t = TIFF::open(tiff_path, "r"));
  ASSERT_TRUE(static_cast<bool>(t));

  ome::compat::shared_ptr<IFD> ifd;
  ASSERT_NO_THROW(ifd = t->getDirectoryByIndex(0));
  ASSERT_TRUE(static_cast<bool>(ifd));

  ASSERT_EQ(PT::UINT8, ifd->getPixelType());
  ASSERT_EQ(8U, ifd->getBitsPerSample());
}

TEST(TIFFCodec, ListCodecs)
{
  // Note this list depends upon the codecs provided by libtiff, which
  // can vary, so we don't attempt to validate specific codecs are
  // present here.

  std::vector<Codec> codecs = ome::files::tiff::getConfiguredCodecs();
  for (std::vector<Codec>::const_iterator c = codecs.begin();
       c != codecs.end();
       ++c)
    {
      // std::cout << "C: " << c->name << " = " << c->scheme << '\n';
    }
}

class TIFFTileTest : public ::testing::TestWithParam<TileTestParameters>
{
public:
  ome::compat::shared_ptr<TIFF> tiff;
  ome::compat::shared_ptr<IFD> ifd;
  uint32_t iwidth;
  uint32_t iheight;
  ome::files::tiff::PlanarConfiguration planarconfig;
  uint16_t samples;
  static VariantPixelBuffer pngdata_chunky;
  static VariantPixelBuffer pngdata_planar;
  typedef std::map<std::pair<PT,ome::files::tiff::PlanarConfiguration>,VariantPixelBuffer> pngdata_map_type;
  static pngdata_map_type pngdata_map;
  static bool pngdata_init;
  static uint32_t pwidth;
  static uint32_t pheight;

  static void
  readPNGData()
  {
    // Sample image to check validity of TIFF reading.
    const char * const pngfile = PROJECT_SOURCE_DIR "/test/ome-files/data/data-layout.png";

    std::FILE *png = std::fopen(pngfile, "rb");
    ASSERT_TRUE(png);
    uint8_t header[8];
    std::fread(header, 1, 8, png);
    ASSERT_FALSE((png_sig_cmp(header, 0, 8)));

    png_structp pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    ASSERT_TRUE(pngptr != 0);

    png_infop infoptr = png_create_info_struct(pngptr);
    if (!infoptr)
      png_destroy_read_struct(&pngptr, 0, 0);
    ASSERT_TRUE(infoptr != 0);

    png_infop endinfoptr = png_create_info_struct(pngptr);
    if (!endinfoptr)
      png_destroy_read_struct(&pngptr, 0, 0);
    ASSERT_TRUE(endinfoptr != 0);

    int result = setjmp(png_jmpbuf(pngptr));
    ASSERT_FALSE((result));

    png_init_io(pngptr, png);
    png_set_sig_bytes(pngptr, 8);

    png_read_info(pngptr, infoptr);

    pwidth = png_get_image_width(pngptr, infoptr);
    pheight = png_get_image_height(pngptr, infoptr);
    png_byte color_type = png_get_color_type(pngptr, infoptr);
    ASSERT_EQ(PNG_COLOR_TYPE_RGB, color_type);
    png_byte bit_depth = png_get_bit_depth(pngptr, infoptr);
    ASSERT_EQ(8U, bit_depth);

    png_set_interlace_handling(pngptr);
    png_read_update_info(pngptr, infoptr);

    ome::compat::array<VariantPixelBuffer::size_type, 9> shape;
    shape[::ome::files::DIM_SPATIAL_X] = pwidth;
    shape[::ome::files::DIM_SPATIAL_Y] = pheight;
    shape[::ome::files::DIM_SUBCHANNEL] = 3U;
    shape[::ome::files::DIM_SPATIAL_Z] = shape[::ome::files::DIM_TEMPORAL_T] = shape[::ome::files::DIM_CHANNEL] =
      shape[::ome::files::DIM_MODULO_Z] = shape[::ome::files::DIM_MODULO_T] = shape[::ome::files::DIM_MODULO_C] = 1;

    ::ome::files::PixelBufferBase::storage_order_type order_chunky(::ome::files::PixelBufferBase::default_storage_order());
    ::ome::files::PixelBufferBase::storage_order_type order_planar(::ome::files::PixelBufferBase::make_storage_order(::ome::xml::model::enums::DimensionOrder::XYZTC, false));

    pngdata_chunky.setBuffer(shape, PT::UINT8, order_chunky);
    pngdata_planar.setBuffer(shape, PT::UINT8, order_planar);

    ome::compat::shared_ptr<PixelBuffer<PixelProperties<PT::UINT8>::std_type> >& uint8_pngdata_chunky(boost::get<ome::compat::shared_ptr<PixelBuffer<PixelProperties<PT::UINT8>::std_type> > >(pngdata_chunky.vbuffer()));
    std::vector<png_bytep> row_pointers(pheight);
    for (dimension_size_type y = 0; y < pheight; ++y)
      {
        VariantPixelBuffer::indices_type coord;
        coord[ome::files::DIM_SPATIAL_X] = 0;
        coord[ome::files::DIM_SPATIAL_Y] = y;
        coord[ome::files::DIM_SUBCHANNEL] = 0;
        coord[ome::files::DIM_SPATIAL_Z] = coord[ome::files::DIM_TEMPORAL_T] =
          coord[ome::files::DIM_CHANNEL] = coord[ome::files::DIM_MODULO_Z] =
          coord[ome::files::DIM_MODULO_T] = coord[ome::files::DIM_MODULO_C] = 0;

        row_pointers[y] = reinterpret_cast<png_bytep>(&(uint8_pngdata_chunky->at(coord)));
      }

    png_read_image(pngptr, &row_pointers[0]);

    png_read_end(pngptr, infoptr);

    png_destroy_read_struct(&pngptr, &infoptr, &endinfoptr);

    std::fclose(png);
    png = 0;

    pngdata_planar = pngdata_chunky;
    ASSERT_TRUE(pngdata_chunky == pngdata_planar);

    pngdata_init = true;
  }

  static const VariantPixelBuffer&
  getPNGDataChunky()
  {
    if (!pngdata_init)
      readPNGData();
    return pngdata_chunky;
  }

  static const VariantPixelBuffer&
  getPNGDataPlanar()
  {
    if (!pngdata_init)
      readPNGData();
    return pngdata_planar;
  }

  static const VariantPixelBuffer&
  getPNGData(PT pixeltype,
             ome::files::tiff::PlanarConfiguration planarconfig)
  {
    pngdata_map_type::const_iterator found = pngdata_map.find(std::make_pair(pixeltype, planarconfig));
    if (found == pngdata_map.end())
      {
        const VariantPixelBuffer& src(planarconfig == ome::files::tiff::CONTIG ? getPNGDataChunky() : getPNGDataPlanar());

        const VariantPixelBuffer::size_type *shape = src.shape();

        VariantPixelBuffer dest(boost::extents[shape[0]][shape[1]][shape[2]][shape[3]][shape[4]][shape[5]][shape[6]][shape[7]][shape[8]], pixeltype, src.storage_order());

        PixelTypeConversionVisitor<PT::UINT8> v(src, dest);
        boost::apply_visitor(v, dest.vbuffer());

        pngdata_map_type::key_type key(pixeltype, planarconfig);
        std::pair<pngdata_map_type::iterator,bool> ins
          (pngdata_map.insert(pngdata_map_type::value_type(key, dest)));

        if (!ins.second)
          throw std::runtime_error("Failed to cache test pixel data");

        found = ins.first;
      }

    return found->second;
  }

  virtual void SetUp()
  {
    const TileTestParameters& params = GetParam();

    ASSERT_NO_THROW(tiff = TIFF::open(params.file, "r"));
    ASSERT_TRUE(static_cast<bool>(tiff));
    ASSERT_NO_THROW(ifd = tiff->getDirectoryByIndex(0));
    ASSERT_TRUE(static_cast<bool>(ifd));

    ASSERT_NO_THROW(ifd->getField(ome::files::tiff::IMAGEWIDTH).get(iwidth));
    ASSERT_NO_THROW(ifd->getField(ome::files::tiff::IMAGELENGTH).get(iheight));
    ASSERT_NO_THROW(ifd->getField(ome::files::tiff::PLANARCONFIG).get(planarconfig));
    ASSERT_NO_THROW(ifd->getField(ome::files::tiff::SAMPLESPERPIXEL).get(samples));
  }

};

VariantPixelBuffer TIFFTileTest::pngdata_chunky;
VariantPixelBuffer TIFFTileTest::pngdata_planar;
TIFFTileTest::pngdata_map_type TIFFTileTest::pngdata_map;
bool TIFFTileTest::pngdata_init = false;
uint32_t TIFFTileTest::pwidth = 0;
uint32_t TIFFTileTest::pheight = 0;

// Check basic tile metadata
TEST_P(TIFFTileTest, TileInfo)
{
  const TileTestParameters& params = GetParam();
  TileInfo info = ifd->getTileInfo();

  EXPECT_EQ(params.tilewidth, info.tileWidth());
  EXPECT_EQ(params.tilelength, info.tileHeight());
  EXPECT_NE(0U, info.bufferSize());

  dimension_size_type ecol = iwidth / params.tilewidth;
  if (iwidth % params.tilewidth)
    ++ecol;
  dimension_size_type erow = iheight / params.tilelength;
  if (iheight % params.tilelength)
    ++erow;
  EXPECT_EQ(erow, info.tileRowCount());
  EXPECT_EQ(ecol, info.tileColumnCount());

  if (params.imageplanar)
    ASSERT_EQ(ome::files::tiff::SEPARATE, planarconfig);
  else
    ASSERT_EQ(ome::files::tiff::CONTIG, planarconfig);

  if (params.tile)
    ASSERT_EQ(ome::files::tiff::TILE, info.tileType());
}

// Check that the first tile matches the expected tile size
TEST_P(TIFFTileTest, TilePlaneRegion0)
{
  const TileTestParameters& params = GetParam();
  TileInfo info = ifd->getTileInfo();

  PlaneRegion full(0, 0, iwidth, iheight);

  PlaneRegion region0 = info.tileRegion(0, full);
  EXPECT_EQ(0U, region0.x);
  EXPECT_EQ(0U, region0.y);
  EXPECT_EQ(params.tilewidth, region0.w);
  EXPECT_EQ(params.tilelength, region0.h);
}

// Check tiling of whole image including edge overlaps being correctly
// computed and all tiles being accounted for.
TEST_P(TIFFTileTest, PlaneArea1)
{
  const TileTestParameters& params = GetParam();
  TileInfo info = ifd->getTileInfo();

  PlaneRegion full(0, 0, iwidth, iheight);
  std::vector<dimension_size_type> tiles = info.tileCoverage(full);
  EXPECT_EQ(info.tileCount(), tiles.size());

  dimension_size_type area = 0;
  std::vector<PlaneRegion> regions;
  for (std::vector<dimension_size_type>::const_iterator i = tiles.begin();
       i != tiles.end();
       ++i)
    {
      PlaneRegion r = info.tileRegion(*i, full);
      regions.push_back(r);
      area += (r.w * r.h);
    }
  if (params.imageplanar)
    {
      EXPECT_EQ(0U, area % samples);
      area /= samples;
    }

  EXPECT_EQ((full.w * full.h), area);

  // Check there are no overlaps.
  for (std::vector<PlaneRegion>::size_type i = 0; i < regions.size(); ++i)
    for (std::vector<PlaneRegion>::size_type j = 0; j < regions.size(); ++j)
      {
        if (i == j)
          continue;

        // Overlaps expected between different subchannels
        if (info.tileSample(tiles.at(i)) != info.tileSample(tiles.at(j)))
          continue;

        PlaneRegion overlap = regions.at(i) & regions.at(j);

        EXPECT_EQ(0U, overlap.w * overlap.h);
      }
}

// Check tiling of multiple-of-16 subrange including edge overlaps
// being correctly computed and all tiles being accounted for.
TEST_P(TIFFTileTest, PlaneArea2)
{
  const TileTestParameters& params = GetParam();
  TileInfo info = ifd->getTileInfo();

  PlaneRegion partial(16U, 16U, iwidth - 32U, iheight - 32U);
  std::vector<dimension_size_type> tiles = info.tileCoverage(partial);

  dimension_size_type area = 0;
  std::vector<PlaneRegion> regions;
  for (std::vector<dimension_size_type>::const_iterator i = tiles.begin();
       i != tiles.end();
       ++i)
    {
      PlaneRegion r = info.tileRegion(*i, partial);
      regions.push_back(r);
      area += (r.w * r.h);
    }
  if (params.imageplanar)
    {
      EXPECT_EQ(0U, area % samples);
      area /= samples;
    }

  EXPECT_EQ((partial.w * partial.h), area);

  // Check there are no overlaps.
  for (std::vector<PlaneRegion>::size_type i = 0; i < regions.size(); ++i)
    for (std::vector<PlaneRegion>::size_type j = 0; j < regions.size(); ++j)
      {
        if (i == j)
          continue;

        if (info.tileSample(tiles.at(i)) != info.tileSample(tiles.at(j)))
          continue;

        PlaneRegion overlap = regions.at(i) & regions.at(j);

        EXPECT_EQ(0U, overlap.w * overlap.h);
      }
}

// Check tiling of non-multiple-of-16 subrange including edge overlaps
// being correctly computed and all tiles being accounted for.
TEST_P(TIFFTileTest, PlaneArea3)
{
  const TileTestParameters& params = GetParam();
  TileInfo info = ifd->getTileInfo();

  PlaneRegion partial(19U, 31U, iwidth - 39U, iheight - 40U);
  std::vector<dimension_size_type> tiles = info.tileCoverage(partial);

  dimension_size_type area = 0;
  std::vector<PlaneRegion> regions;
  for (std::vector<dimension_size_type>::const_iterator i = tiles.begin();
       i != tiles.end();
       ++i)
    {
      PlaneRegion r = info.tileRegion(*i, partial);
      regions.push_back(r);
      area += (r.w * r.h);
    }
  if (params.imageplanar)
    {
      EXPECT_EQ(0U, area % samples);
      area /= samples;
    }

  EXPECT_EQ((partial.w * partial.h), area);

  // Check there are no overlaps.
  for (std::vector<PlaneRegion>::size_type i = 0; i < regions.size(); ++i)
    for (std::vector<PlaneRegion>::size_type j = 0; j < regions.size(); ++j)
      {
        if (i == j)
          continue;

        if (info.tileSample(tiles.at(i)) != info.tileSample(tiles.at(j)))
          continue;

        PlaneRegion overlap = regions.at(i) & regions.at(j);

        EXPECT_EQ(0U, overlap.w * overlap.h);
      }
}

namespace
{
  void
  read_test(const std::string& file,
            const VariantPixelBuffer& reference)
  {
    ome::compat::shared_ptr<TIFF> tiff;
    ASSERT_NO_THROW(tiff = TIFF::open(file, "r"));
    ASSERT_TRUE(static_cast<bool>(tiff));
    ome::compat::shared_ptr<IFD> ifd;
    ASSERT_NO_THROW(ifd = tiff->getDirectoryByIndex(0));
    ASSERT_TRUE(static_cast<bool>(ifd));

    EXPECT_EQ(64U, ifd->getImageWidth());
    EXPECT_EQ(64U, ifd->getImageHeight());

    VariantPixelBuffer vb;
    ifd->readImage(vb);

    if(reference != vb)
      {
        std::cout << "Observed\n";
        dump_image_representation(vb, std::cout);
        std::cout << "Expected\n";
        dump_image_representation(reference, std::cout);
      }
    ASSERT_TRUE(reference == vb);
  }
}

TEST_P(TIFFTileTest, PlaneRead)
{
  const TileTestParameters& params = GetParam();

  read_test(params.file, getPNGDataChunky());
}

TEST_P(TIFFTileTest, PlaneReadAlignedTileOrdered)
{
  TileInfo info = ifd->getTileInfo();

  PlaneRegion full(0, 0, ifd->getImageWidth(), ifd->getImageHeight());
  std::vector<dimension_size_type> tiles = info.tileCoverage(full);

  VariantPixelBuffer vb;

  for (std::vector<dimension_size_type>::const_iterator i = tiles.begin();
       i != tiles.end();
       ++i)
    {
      PlaneRegion r = info.tileRegion(*i, full);

      ifd->readImage(vb, r.x, r.y, r.w, r.h);

      /// @todo Verify buffer contents once pixelbuffer subsetting is
      /// available.
    }
}

TEST_P(TIFFTileTest, PlaneReadAlignedTileRandom)
{
  TileInfo info = ifd->getTileInfo();

  PlaneRegion full(0, 0, ifd->getImageWidth(), ifd->getImageHeight());
  std::vector<dimension_size_type> tiles = info.tileCoverage(full);

  VariantPixelBuffer vb;

  std::random_shuffle(tiles.begin(), tiles.end());
  for (std::vector<dimension_size_type>::const_iterator i = tiles.begin();
       i != tiles.end();
       ++i)
    {
      PlaneRegion r = info.tileRegion(*i, full);

      ifd->readImage(vb, r.x, r.y, r.w, r.h);

      /// @todo Verify buffer contents once pixelbuffer subsetting is
      /// available.
    }
}

TEST_P(TIFFTileTest, PlaneReadUnalignedTileOrdered)
{
  TileInfo info = ifd->getTileInfo();

  PlaneRegion full(0, 0, ifd->getImageWidth(), ifd->getImageHeight());

  std::vector<PlaneRegion> tiles;
  for (dimension_size_type x = 0; x < full.w; x+= 5)
    for (dimension_size_type y = 0; y < full.h; y+= 7)
      {
        PlaneRegion r = PlaneRegion(x, y, 5, 7) & full;
        tiles.push_back(r);
      }

  VariantPixelBuffer vb;

  for (std::vector<PlaneRegion>::const_iterator i = tiles.begin();
       i != tiles.end();
       ++i)
    {
      const PlaneRegion& r = *i;

      ifd->readImage(vb, r.x, r.y, r.w, r.h);

      /// @todo Verify buffer contents once pixelbuffer subsetting is
      /// available.
    }
}

TEST_P(TIFFTileTest, PlaneReadUnalignedTileRandom)
{
  TileInfo info = ifd->getTileInfo();

  PlaneRegion full(0, 0, ifd->getImageWidth(), ifd->getImageHeight());

  std::vector<PlaneRegion> tiles;
  for (dimension_size_type x = 0; x < full.w; x+= 5)
    for (dimension_size_type y = 0; y < full.h; y+= 7)
      {
        PlaneRegion r = PlaneRegion(x, y, 5, 7) & full;
        tiles.push_back(r);
      }

  VariantPixelBuffer vb;

  std::random_shuffle(tiles.begin(), tiles.end());
  for (std::vector<PlaneRegion>::const_iterator i = tiles.begin();
       i != tiles.end();
       ++i)
    {
      const PlaneRegion& r = *i;

      ifd->readImage(vb, r.x, r.y, r.w, r.h);

      /// @todo Verify buffer contents once pixelbuffer subsetting is
      /// available.
    }
}

class PixelTestParameters
{
public:
  PT pixeltype;
  ome::files::tiff::TileType                  tiletype;
  ome::files::tiff::PlanarConfiguration       planarconfig;
  ome::files::tiff::PhotometricInterpretation photometricinterp;
  dimension_size_type                              tilewidth;
  dimension_size_type                              tileheight;
  bool                                             optimal;
  bool                                             ordered;
  std::string                                      filename;

  PixelTestParameters(PT pixeltype,
                      ome::files::tiff::TileType tiletype,
                      ome::files::tiff::PlanarConfiguration planarconfig,
                      ome::files::tiff::PhotometricInterpretation photometricinterp,
                      dimension_size_type tilewidth,
                      dimension_size_type tileheight,
                      bool optimal,
                      bool ordered):
    pixeltype(pixeltype),
    tiletype(tiletype),
    planarconfig(planarconfig),
    photometricinterp(photometricinterp),
    tilewidth(tilewidth),
    tileheight(tileheight),
    optimal(optimal),
    ordered(ordered)
  {
    std::ostringstream f;
    f << "data-layout-" << pixeltype << '-'
      << (planarconfig == ome::files::tiff::CONTIG ? "chunky" : "planar") << '-'
      << "pi" << photometricinterp << '-'
      << (tiletype == ome::files::tiff::TILE ? "tile" : "strip") << '-';
    if (tiletype == ome::files::tiff::TILE)
      f << tilewidth << 'x' << tileheight;
    else
      f << tileheight;
    f << '-' << (ordered ? "ordered" : "random")
      << '-' << (optimal ? "optimal" : "suboptimal")
      << ".tiff";

    path dir(PROJECT_BINARY_DIR "/test/ome-files/data");
    if (!exists(dir) && !is_directory(dir) && !create_directories(dir))
      throw std::runtime_error("Image directory unavailable and could not be created");
    dir /= f.str();
    filename = dir.string();
  }
};

template<class charT, class traits>
inline std::basic_ostream<charT,traits>&
operator<< (std::basic_ostream<charT,traits>& os,
            const PixelTestParameters& params)
{
  return os << params.filename;
}

class PixelTest : public ::testing::TestWithParam<PixelTestParameters>
{
  void
  TearDown()
  {
    // Delete file (if any)
    const PixelTestParameters& params = GetParam();
    if (boost::filesystem::exists(params.filename))
      boost::filesystem::remove(params.filename);
  }
};

TEST_P(PixelTest, WriteTIFF)
{
  const PixelTestParameters& params = GetParam();
  const VariantPixelBuffer& pixels(TIFFTileTest::getPNGData(params.pixeltype, params.planarconfig));
  const VariantPixelBuffer::size_type *shape = pixels.shape();

  dimension_size_type exp_size = (params.tilewidth * params.tileheight *
                                  ::ome::files::bytesPerPixel(params.pixeltype) *
                                  (params.planarconfig == ::ome::files::tiff::CONTIG ? shape[ome::files::DIM_SUBCHANNEL] : 1));

  // Write TIFF
  {
    ome::compat::shared_ptr<TIFF> wtiff;
    ASSERT_NO_THROW(wtiff = TIFF::open(params.filename, "w"));
    ASSERT_TRUE(static_cast<bool>(wtiff));
    ome::compat::shared_ptr<IFD> wifd;
    ASSERT_NO_THROW(wifd = wtiff->getCurrentDirectory());
    ASSERT_TRUE(static_cast<bool>(wifd));

    // Set IFD tags
    ASSERT_NO_THROW(wifd->setImageWidth(shape[ome::files::DIM_SPATIAL_X]));
    ASSERT_NO_THROW(wifd->setImageHeight(shape[ome::files::DIM_SPATIAL_Y]));
    ASSERT_NO_THROW(wifd->setTileType(params.tiletype));
    ASSERT_NO_THROW(wifd->setTileWidth(params.tilewidth));
    ASSERT_NO_THROW(wifd->setTileHeight(params.tileheight));
    ASSERT_NO_THROW(wifd->setPixelType(params.pixeltype));
    ASSERT_NO_THROW(wifd->setBitsPerSample(significantBitsPerPixel(params.pixeltype)));
    ASSERT_NO_THROW(wifd->setSamplesPerPixel(shape[ome::files::DIM_SUBCHANNEL]));
    ASSERT_NO_THROW(wifd->setPlanarConfiguration(params.planarconfig));
    ASSERT_NO_THROW(wifd->setPhotometricInterpretation(params.photometricinterp));

    // Verify IFD tags
    EXPECT_EQ(shape[ome::files::DIM_SPATIAL_X], wifd->getImageWidth());
    EXPECT_EQ(shape[ome::files::DIM_SPATIAL_Y], wifd->getImageHeight());
    EXPECT_EQ(params.tiletype, wifd->getTileType());
    EXPECT_EQ(params.tilewidth, wifd->getTileWidth());
    EXPECT_EQ(params.tileheight, wifd->getTileHeight());
    EXPECT_EQ(params.pixeltype, wifd->getPixelType());
    EXPECT_EQ(significantBitsPerPixel(params.pixeltype), wifd->getBitsPerSample());
    EXPECT_EQ(shape[ome::files::DIM_SUBCHANNEL], wifd->getSamplesPerPixel());
    EXPECT_EQ(params.planarconfig, wifd->getPlanarConfiguration());

    // Make sure our expectations about buffer size are correct
    if (params.pixeltype == PT::BIT)
      {
        dimension_size_type size;
        size = exp_size / 8;
        if (exp_size % 8)
          ++size;
        exp_size = size;
      }
    ASSERT_EQ(exp_size,
              wifd->getTileInfo().bufferSize());

    PlaneRegion full(0, 0, wifd->getImageWidth(), wifd->getImageHeight());

    dimension_size_type wtilewidth = params.tilewidth;
    dimension_size_type wtileheight = params.tileheight;
    if (!params.optimal)
      {
        wtilewidth = 5;
        wtileheight = 7;
      }

    std::vector<PlaneRegion> tiles;
    for (dimension_size_type x = 0; x < full.w; x+= wtilewidth)
      for (dimension_size_type y = 0; y < full.h; y+= wtileheight)
        {
          PlaneRegion r = PlaneRegion(x, y, wtilewidth, wtileheight) & full;
          tiles.push_back(r);
        }

    if (!params.ordered)
      std::random_shuffle(tiles.begin(), tiles.end());

    for (std::vector<PlaneRegion>::const_iterator i = tiles.begin();
         i != tiles.end();
         ++i)
      {
        const PlaneRegion& r = *i;

        ome::compat::array<VariantPixelBuffer::size_type, 9> shape;
        shape[::ome::files::DIM_SPATIAL_X] = r.w;
        shape[::ome::files::DIM_SPATIAL_Y] = r.h;
        shape[::ome::files::DIM_SUBCHANNEL] = 3U;
        shape[::ome::files::DIM_SPATIAL_Z] = shape[::ome::files::DIM_TEMPORAL_T] = shape[::ome::files::DIM_CHANNEL] =
          shape[::ome::files::DIM_MODULO_Z] = shape[::ome::files::DIM_MODULO_T] = shape[::ome::files::DIM_MODULO_C] = 1;

        ::ome::files::PixelBufferBase::storage_order_type order
            (::ome::files::PixelBufferBase::make_storage_order(::ome::xml::model::enums::DimensionOrder::XYZTC,
                                                                    params.planarconfig == ::ome::files::tiff::CONTIG));

        VariantPixelBuffer vb;
        vb.setBuffer(shape, params.pixeltype, order);

        // Temporary subrange to write into tile
        PixelSubrangeVisitor sv(r.x, r.y);
        boost::apply_visitor(sv, pixels.vbuffer(), vb.vbuffer());

        wifd->writeImage(vb, r.x, r.y, r.w, r.h);
      }

    wtiff->writeCurrentDirectory();
    wtiff->close();
  }

  // Read and validate TIFF
  {
    // Note "c" to disable automatic strip chopping so we can verify
    // the exact tag content of ROWSPERSTRIP.
    ome::compat::shared_ptr<TIFF> tiff;
    ASSERT_NO_THROW(tiff = TIFF::open(params.filename, "rc"));
    ASSERT_TRUE(static_cast<bool>(tiff));
    ome::compat::shared_ptr<IFD> ifd;
    ASSERT_NO_THROW(ifd = tiff->getDirectoryByIndex(0));
    ASSERT_TRUE(static_cast<bool>(ifd));

    EXPECT_EQ(shape[ome::files::DIM_SPATIAL_X], ifd->getImageWidth());
    EXPECT_EQ(shape[ome::files::DIM_SPATIAL_Y], ifd->getImageHeight());
    EXPECT_EQ(params.tiletype, ifd->getTileType());
    EXPECT_EQ(params.tilewidth, ifd->getTileWidth());
    EXPECT_EQ(params.tileheight, ifd->getTileHeight());
    EXPECT_EQ(params.pixeltype, ifd->getPixelType());
    EXPECT_EQ(significantBitsPerPixel(params.pixeltype), ifd->getBitsPerSample());
    EXPECT_EQ(shape[ome::files::DIM_SUBCHANNEL], ifd->getSamplesPerPixel());
    EXPECT_EQ(params.planarconfig, ifd->getPlanarConfiguration());
    EXPECT_EQ(params.photometricinterp, ifd->getPhotometricInterpretation());

    VariantPixelBuffer vb;
    ifd->readImage(vb);

    if(pixels != vb)
      {
        std::cout << "Observed\n";
        dump_image_representation(vb, std::cout);
        std::cout << "Expected\n";
        dump_image_representation(pixels, std::cout);
      }
    EXPECT_TRUE(pixels == vb);
  }

}

namespace
{

  PT
  ptkey(const std::pair<PT::enum_value, std::string>& mapval)
  {
    return PT(mapval.first);
  }

  std::vector<PixelTestParameters>
  pixel_tests()
  {
    std::vector<PixelTestParameters> ret;

    std::vector<dimension_size_type> tilesizes;
#ifdef EXTENDED_TESTS
    tilesizes.push_back(16);
#endif // EXTENDED_TESTS
    tilesizes.push_back(32);
    tilesizes.push_back(48);
#ifdef EXTENDED_TESTS
    tilesizes.push_back(64);
#endif // EXTENDED_TESTS

    std::vector<dimension_size_type> stripsizes;
    stripsizes.push_back(1);
    stripsizes.push_back(2);
#ifdef EXTENDED_TESTS
    stripsizes.push_back(5);
    stripsizes.push_back(14);
    stripsizes.push_back(32);
#endif // EXTENDED_TESTS
    stripsizes.push_back(60);
    stripsizes.push_back(64);

    std::vector<ome::files::tiff::PlanarConfiguration> planarconfigs;
    planarconfigs.push_back(ome::files::tiff::CONTIG);
    planarconfigs.push_back(ome::files::tiff::SEPARATE);

    std::vector<ome::files::tiff::PhotometricInterpretation> photometricinterps;
    photometricinterps.push_back(ome::files::tiff::MIN_IS_BLACK);
    photometricinterps.push_back(ome::files::tiff::RGB);

    std::vector<bool> optimal;
    optimal.push_back(true);
#ifdef EXTENDED_TESTS
    optimal.push_back(false);
#endif // EXTENDED_TESTS

    std::vector<bool> ordered;
    ordered.push_back(true);
#ifdef EXTENDED_TESTS
    ordered.push_back(false);
#endif // EXTENDED_TESTS

    const PT::value_map_type& pixeltypemap = PT::values();
    std::vector<PT> pixeltypes;
    std::transform(pixeltypemap.begin(), pixeltypemap.end(), std::back_inserter(pixeltypes), ptkey);

    for (std::vector<PT>::const_iterator pt = pixeltypes.begin(); pt != pixeltypes.end(); ++pt)
      for (std::vector<ome::files::tiff::PlanarConfiguration>::const_iterator pc = planarconfigs.begin(); pc != planarconfigs.end(); ++pc)
        for (std::vector<ome::files::tiff::PhotometricInterpretation>::const_iterator pi = photometricinterps.begin(); pi != photometricinterps.end(); ++pi)
          {
            for(std::vector<dimension_size_type>::const_iterator wid = tilesizes.begin(); wid != tilesizes.end(); ++wid)
              for(std::vector<dimension_size_type>::const_iterator ht = tilesizes.begin(); ht != tilesizes.end(); ++ht)
                {
                  for(std::vector<bool>::const_iterator opt = optimal.begin(); opt != optimal.end(); ++opt)
                    for(std::vector<bool>::const_iterator ord = ordered.begin(); ord != ordered.end(); ++ord)
                    ret.push_back(PixelTestParameters(*pt, ome::files::tiff::TILE, *pc, *pi, *wid, *ht, *opt, *ord));
                }
            for(std::vector<dimension_size_type>::const_iterator rows = stripsizes.begin(); rows != stripsizes.end(); ++rows)
              {
                for(std::vector<bool>::const_iterator opt = optimal.begin(); opt != optimal.end(); ++opt)
                  for(std::vector<bool>::const_iterator ord = ordered.begin(); ord != ordered.end(); ++ord)
                    ret.push_back(PixelTestParameters(*pt, ome::files::tiff::STRIP, *pc, *pi, 64, *rows, *opt, *ord));
              }
          }

    return ret;
  }

}

std::vector<PixelTestParameters> pixel_params(pixel_tests());



// Disable missing-prototypes warning for INSTANTIATE_TEST_CASE_P;
// this is solely to work around a missing prototype in gtest.
#ifdef __GNUC__
#  if defined __clang__ || defined __APPLE__
#    pragma GCC diagnostic ignored "-Wmissing-prototypes"
#  endif
#  pragma GCC diagnostic ignored "-Wmissing-declarations"
#endif

INSTANTIATE_TEST_CASE_P(TileVariants, TIFFTileTest, ::testing::ValuesIn(tile_params));
INSTANTIATE_TEST_CASE_P(PixelVariants, PixelTest, ::testing::ValuesIn(pixel_params));
