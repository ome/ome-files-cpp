/*
 * #%L
 * OME-FILES C++ library for image IO.
 * %%
 * Copyright © 2015–2017 Open Microscopy Environment:
 *   - Massachusetts Institute of Technology
 *   - National Institutes of Health
 *   - University of Dundee
 *   - Board of Regents of the University of Wisconsin-Madison
 *   - Glencoe Software, Inc.
 * Copyright © 2018 Quantitative Imaging Systems, LLC
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

#include <stdexcept>
#include <vector>

#include <ome/files/CoreMetadata.h>
#include <ome/files/MetadataTools.h>
#include <ome/files/VariantPixelBuffer.h>
#include <ome/files/in/OMETIFFReader.h>
#include <ome/files/out/OMETIFFWriter.h>
#include <ome/files/tiff/Field.h>
#include <ome/files/tiff/IFD.h>
#include <ome/files/tiff/Tags.h>
#include <ome/files/tiff/TIFF.h>
#include <ome/files/tiff/Util.h>

#include <ome/xml/meta/OMEXMLMetadata.h>

#include <ome/test/test.h>

using ome::files::DIM_SPATIAL_X;
using ome::files::DIM_SPATIAL_Y;
using ome::files::DIM_SAMPLE;
using ome::files::dimension_size_type;
using ome::files::CoreMetadata;
using ome::files::PixelBufferBase;
using ome::files::PixelBuffer;
using ome::files::PixelProperties;
using ome::files::VariantPixelBuffer;
using ome::files::in::OMETIFFReader;
using ome::files::out::OMETIFFWriter;

using ome::xml::model::enums::DimensionOrder;
using ome::xml::model::enums::PixelType;

using namespace boost::filesystem;

namespace
{

  std::shared_ptr<ome::xml::meta::OMEXMLMetadata>
  createMetadata()
  {
    auto meta = std::make_shared<ome::xml::meta::OMEXMLMetadata>();

    std::vector<std::shared_ptr<CoreMetadata>> seriesList;
    std::shared_ptr<CoreMetadata> core(std::make_shared<CoreMetadata>());
    core->sizeX = 2048U;
    core->sizeY = 1024U;
    core->sizeC.clear();
    core->sizeC.push_back(3U);
    core->pixelType = PixelType::UINT16;
    core->interleaved = true;
    core->bitsPerPixel = 16U;
    core->dimensionOrder = DimensionOrder::XYZTC;
    seriesList.push_back(core);
    seriesList.push_back(core);

    fillMetadata(*meta, seriesList);

    return meta;
  }

  std::shared_ptr<VariantPixelBuffer>
  makeBuffer(dimension_size_type xsize,
             dimension_size_type ysize)
  {
    std::shared_ptr<PixelBuffer<PixelProperties<PixelType::UINT16>::std_type>>
      buffer(std::make_shared<PixelBuffer<PixelProperties<PixelType::UINT16>::std_type>>
             (boost::extents[xsize][ysize][1][3],
              PixelType::UINT16, ome::files::ENDIAN_NATIVE,
              PixelBufferBase::make_storage_order(true)));

    for (dimension_size_type x = 0; x < xsize; ++x)
      {
        for (dimension_size_type y = 0; y < ysize; ++y)
          {
            PixelBufferBase::indices_type idx;
            std::fill(idx.begin(), idx.end(), 0);
            idx[DIM_SPATIAL_X] = x;
            idx[DIM_SPATIAL_Y] = y;

            idx[DIM_SAMPLE] = 0;
            buffer->at(idx) = (static_cast<float>(x) / static_cast<float>(xsize)) * 4096.0f;
            idx[DIM_SAMPLE] = 1;
            buffer->at(idx) = (static_cast<float>(y) / static_cast<float>(ysize)) * 4096.0f;
            idx[DIM_SAMPLE] = 2;
            buffer->at(idx) = (static_cast<float>(x+y) / static_cast<float>(xsize + ysize)) * 4096.0f;
          }
      }

    return std::make_shared<VariantPixelBuffer>(buffer);
  }

}


TEST(OMETIFFWriter, SubResolutions)
{
  auto meta = createMetadata();

  ome::files::MetadataList<ome::files::Resolution> resolutions =
    {{
        {
          {1024U, 512U, 1U},
          {512U, 256U, 1U},
          {256U, 128U, 1U},
          {128U, 64U, 1U},
          {64U, 32U, 1U}
        },
        {
          {1024U, 512U, 1U},
          {512U, 256U, 1U}
        }
      }};
  ome::files::addResolutions(*meta, resolutions);

  ome::files::MetadataList<std::shared_ptr<VariantPixelBuffer>> pixels;
  auto set_resolutions = ome::files::getResolutions(*meta);
  ASSERT_EQ(resolutions, set_resolutions);

  path filename(PROJECT_BINARY_DIR "/test/ome-files/data/subresolution.ome.tiff");

  {
    auto writer = std::make_shared<OMETIFFWriter>();

    auto retrieve = std::static_pointer_cast<ome::xml::meta::MetadataRetrieve>(meta);
    writer->setMetadataRetrieve(retrieve);
    writer->setInterleaved(true);
    writer->setTileSizeX(256);
    writer->setTileSizeY(256);


    // Open the file
    writer->setId(filename);

    // Write pixel data for each series and resolution
    dimension_size_type ic = writer->getSeriesCount();
    pixels.resize(ic);
    for (dimension_size_type i = 0 ; i < ic; ++i)
      {
        writer->setSeries(i);

        dimension_size_type rc = writer->getResolutionCount();
        pixels[i].resize(rc);
        for (dimension_size_type r = 0 ; r < rc; ++r)
          {
            writer->setResolution(r);

            std::cout << "Writing series " << i+1 << '/' << ic
                      << " resolution " << r+1 << '/' << rc
                      << " (" << writer->getSizeX()
                      << ',' << writer->getSizeY()
                      << ',' << writer->getSizeZ()
                      << ')' << std::endl;

            if (r)
              {
                ASSERT_EQ(resolutions[i][r-1][0], writer->getSizeX());
                ASSERT_EQ(resolutions[i][r-1][1], writer->getSizeY());
                ASSERT_EQ(resolutions[i][r-1][2], writer->getSizeZ());
              }
            else
              {
                ASSERT_EQ(static_cast<dimension_size_type>(meta->getPixelsSizeX(i)), writer->getSizeX());
                ASSERT_EQ(static_cast<dimension_size_type>(meta->getPixelsSizeY(i)), writer->getSizeY());
                ASSERT_EQ(static_cast<dimension_size_type>(meta->getPixelsSizeZ(i)), writer->getSizeZ());
              }

            pixels[i][r] = makeBuffer(writer->getSizeX(), writer->getSizeY());
            writer->saveBytes(0U, *(pixels[i][r]));
          }
      }
    writer->close();
  }

  // Now verify metadata and pixel data.

  {
    auto reader = std::make_shared<OMETIFFReader>();
    reader->setId(filename);

    dimension_size_type ic = reader->getSeriesCount();
    ASSERT_EQ(meta->getImageCount(), ic);
    for (dimension_size_type i = 0 ; i < ic; ++i)
      {
        reader->setSeries(i);
        dimension_size_type rc = reader->getResolutionCount();
        ASSERT_EQ(resolutions.at(i).size() + 1, rc);
        for (dimension_size_type r = 0 ; r < rc; ++r)
          {
            reader->setResolution(r);

            std::cout << "Reading and checking series " << i+1 << '/' << ic
                      << " resolution " << r+1 << '/' << rc
                      << std::endl;

              if (r)
              {
                ASSERT_EQ(resolutions[i][r-1][0], reader->getSizeX());
                ASSERT_EQ(resolutions[i][r-1][1], reader->getSizeY());
                ASSERT_EQ(resolutions[i][r-1][2], reader->getSizeZ());
              }
            else
              {
                ASSERT_EQ(static_cast<dimension_size_type>(meta->getPixelsSizeX(i)), reader->getSizeX());
                ASSERT_EQ(static_cast<dimension_size_type>(meta->getPixelsSizeY(i)), reader->getSizeY());
                ASSERT_EQ(static_cast<dimension_size_type>(meta->getPixelsSizeZ(i)), reader->getSizeZ());
              }
            VariantPixelBuffer vbuffer;
            reader->openBytes(0U, vbuffer);
            ASSERT_EQ(*(pixels[i][r]), vbuffer);
          }
      }
    reader->close();
  }
}
