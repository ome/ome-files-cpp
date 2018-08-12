/*
 * #%L
 * OME-FILES C++ library for image IO.
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

#include <complex>
#include <iostream>
#include <memory>

#include <boost/filesystem/path.hpp>

#include <ome/files/CoreMetadata.h>
#include <ome/files/MetadataTools.h>
#include <ome/files/out/OMETIFFWriter.h>
#include <ome/xml/meta/OMEXMLMetadata.h>

#include "fractal.h"

using boost::filesystem::path;
using std::make_shared;
using std::shared_ptr;
using std::static_pointer_cast;
using ome::files::dimension_size_type;
using ome::files::addResolutions;
using ome::files::fillMetadata;
using ome::files::CoreMetadata;
using ome::files::FormatWriter;
using ome::files::out::OMETIFFWriter;
using ome::xml::model::enums::PixelType;
using ome::xml::model::enums::DimensionOrder;
using ome::xml::meta::MetadataRetrieve;
using ome::xml::meta::OMEXMLMetadata;

namespace
{

  shared_ptr<OMEXMLMetadata>
  createMetadata()
  {
    /* create-metadata-start */
    // OME-XML metadata store.
    auto meta = make_shared<OMEXMLMetadata>();

    // Full image size is 2ᵒʳᵈᵉʳ.
    constexpr dimension_size_type order = 12U;
    static_assert(order > 7U, "Image size too small to generate sub-resolutions");

    // Create simple CoreMetadata and use this to set up the OME-XML
    // metadata.  This is purely for convenience in this example; a
    // real writer would typically set up the OME-XML metadata from an
    // existing MetadataRetrieve instance or by hand.
    std::vector<shared_ptr<CoreMetadata>> seriesList;
    shared_ptr<CoreMetadata> core(make_shared<CoreMetadata>());
    core->sizeX = std::pow(2UL, order);
    core->sizeY = std::pow(2UL, order);
    core->sizeC.clear(); // Defaults to 1 channel with 1 sample; clear this.
    core->sizeC.push_back(3U); // Replace with single RGB channel (3 samples).
    core->pixelType = ome::xml::model::enums::PixelType::UINT8;
    core->interleaved = true;
    core->bitsPerPixel = 8U;
    core->dimensionOrder = DimensionOrder::XYZTC;
    seriesList.push_back(core);
    seriesList.push_back(core); // Add two identical series.

    fillMetadata(*meta, seriesList);

    // Add sub-resolution levels as power of two reductions.
    std::vector<std::array<dimension_size_type, 3>> levels;
    for (dimension_size_type i = order - 1; i > 7U; --i)
      levels.push_back({static_cast<dimension_size_type>(std::pow(2UL, i)), // X
                        static_cast<dimension_size_type>(std::pow(2UL, i)), // Y
                        1UL});                                              // Z (placeholder)
    for (dimension_size_type s = 0; s < seriesList.size(); ++s)
        ome::files::addResolutions(*meta, s, levels);
    /* create-metadata-end */

    return meta;
  }

}

int
main(int argc, char *argv[])
{
  // This is the default, but needs setting manually on Windows.
  ome::common::setLogLevel(ome::logging::trivial::warning);

  try
    {
      if (argc > 1)
        {
          // Portable path
          path filename(argv[1]);

          /* writer-example-start */
          // Create minimal metadata for the file to be written.
          auto meta = createMetadata();

          // Create TIFF writer
          auto writer = make_shared<OMETIFFWriter>();

          // Set writer options before opening a file
          auto retrieve = static_pointer_cast<MetadataRetrieve>(meta);
          writer->setMetadataRetrieve(retrieve);
          /* writer-options-start */
          writer->setInterleaved(true);
          writer->setTileSizeX(256);
          writer->setTileSizeY(256);
          writer->setCompression("Deflate");
          /* writer-options-end */

          // Open the file
          writer->setId(filename);

          // Write pixel data for each series and resolution.

          /* pixel-data-start */
          // Total number of images (series)
          dimension_size_type ic = writer->getSeriesCount();

          // Loop over images
          for (dimension_size_type i = 0 ; i < ic; ++i)
            {
              // Change the current series to this index
              writer->setSeries(i);

              // Total number of resolutions for this series
              dimension_size_type rc = writer->getResolutionCount();

              // Loop over resolutions
              for (dimension_size_type r = 0 ; r < rc; ++r)
                {
                  // Change the current resolution to this index
                  writer->setResolution(r);

                  std::cout << "Writing series " << i
                            << " resolution " << r << '\n';

                  // Write a fractal tile-by-tile for this resolution.
                  FractalType ft = (i % 2) ? FractalType::JULIA : FractalType::MANDELBROT;
                  write_fractal(*writer, ft, std::cout);
                }
            }
          /* pixel-data-end */

          // Explicitly close writer
          writer->close();
          /* writer-example-end */
        }
      else
        {
          std::cerr << "Usage: " << argv[0] << " ome-xml.ome.tiff\n";
          std::exit(1);
        }
    }
  catch (const std::exception& e)
    {
      std::cerr << "Caught exception: " << e.what() << '\n';
      std::exit(1);
    }
  catch (...)
    {
      std::cerr << "Caught unknown exception\n";
      std::exit(1);
    }
}
