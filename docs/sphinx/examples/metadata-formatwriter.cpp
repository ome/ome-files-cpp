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

#include <iostream>
#include <memory>

#include <boost/filesystem/path.hpp>

#include <ome/files/CoreMetadata.h>
#include <ome/files/MetadataTools.h>
#include <ome/files/VariantPixelBuffer.h>
#include <ome/files/out/OMETIFFWriter.h>
#include <ome/xml/meta/OMEXMLMetadata.h>

using boost::filesystem::path;
using std::make_shared;
using std::shared_ptr;
using std::static_pointer_cast;
using ome::files::dimension_size_type;
using ome::files::createID;
using ome::files::fillMetadata;
using ome::files::CoreMetadata;
using ome::files::DIM_SPATIAL_X;
using ome::files::DIM_SPATIAL_Y;
using ome::files::DIM_SAMPLE;
using ome::files::FormatWriter;
using ome::files::MetadataMap;
using ome::files::out::OMETIFFWriter;
using ome::files::PixelBuffer;
using ome::files::PixelBufferBase;
using ome::files::PixelProperties;
using ome::files::VariantPixelBuffer;
using ome::xml::model::enums::Binning;
using ome::xml::model::enums::Immersion;
using ome::xml::model::enums::PixelType;
using ome::xml::model::enums::DetectorType;
using ome::xml::model::enums::DimensionOrder;
using ome::xml::model::enums::UnitsLength;
using ome::xml::meta::MetadataRetrieve;
using ome::xml::meta::MetadataStore;
using ome::xml::meta::Metadata;
using ome::xml::meta::OMEXMLMetadata;

namespace
{

  shared_ptr<OMEXMLMetadata>
  createMetadata()
  {
    /* create-metadata-start */
    // OME-XML metadata store.
    auto meta = make_shared<OMEXMLMetadata>();

    // Create simple CoreMetadata and use this to set up the OME-XML
    // metadata.  This is purely for convenience in this example; a
    // real writer would typically set up the OME-XML metadata from an
    // existing MetadataRetrieve instance or by hand.
    std::vector<shared_ptr<CoreMetadata>> seriesList;
    shared_ptr<CoreMetadata> core(make_shared<CoreMetadata>());
    core->sizeX = 512U;
    core->sizeY = 512U;
    core->sizeC.clear(); // defaults to 1 channel with 1 sample; clear this
    core->sizeC.push_back(3U); // replace with single RGB channel
    core->pixelType = ome::xml::model::enums::PixelType::UINT16;
    core->interleaved = false;
    core->bitsPerPixel = 12U;
    core->dimensionOrder = DimensionOrder::XYZTC;
    seriesList.push_back(core);
    seriesList.push_back(core); // add two identical series

    fillMetadata(*meta, seriesList);
    /* create-metadata-end */

    return meta;
  }

  void
  addExtendedMetadata(shared_ptr<OMEXMLMetadata> store)
  {
    /* extended-metadata-start */
    // There is one image with one channel in this image.
    MetadataStore::index_type image_idx = 0;
    MetadataStore::index_type channel_idx = 0;
    MetadataStore::index_type annotation_idx = 0;

    // Create an Instrument.
    MetadataStore::index_type instrument_idx = 0;
    std::string instrument_id = createID("Instrument", instrument_idx);
    store->setInstrumentID(instrument_id, instrument_idx);

    // Create an Objective for this Instrument.
    MetadataStore::index_type objective_idx = 0;
    std::string objective_id = createID("Objective",
                                        instrument_idx, objective_idx);
    store->setObjectiveID(objective_id, instrument_idx, objective_idx);
    store->setObjectiveManufacturer("InterFocal", instrument_idx, objective_idx);
    store->setObjectiveNominalMagnification(40, instrument_idx, objective_idx);
    store->setObjectiveLensNA(0.4, instrument_idx, objective_idx);
    store->setObjectiveImmersion(Immersion::OIL, instrument_idx, objective_idx);
    store->setObjectiveWorkingDistance({0.34, UnitsLength::MILLIMETER},
                                       instrument_idx, objective_idx);

    // Create a Detector for this Instrument.
    MetadataStore::index_type detector_idx = 0;
    std::string detector_id = createID("Detector", instrument_idx, detector_idx);
    store->setDetectorID(detector_id, instrument_idx, detector_idx);
    store->setObjectiveManufacturer("MegaCapture", instrument_idx, detector_idx);
    store->setDetectorType(DetectorType::CCD, instrument_idx, detector_idx);

    // Create Settings for this Detector for the Channel on the Image.
    store->setDetectorSettingsID(detector_id, image_idx, channel_idx);
    store->setDetectorSettingsBinning(Binning::TWOBYTWO, image_idx, channel_idx);
    store->setDetectorSettingsGain(56.89, image_idx, channel_idx);
    /* extended-metadata-end */

    /* annotations-start */
    // Create a MapAnnotation.
    MetadataStore::index_type map_annotation_idx = 0;
    std::string annotation_id = createID("Annotation", annotation_idx);
    store->setMapAnnotationID(annotation_id, map_annotation_idx);
    store->setMapAnnotationNamespace
      ("https://microscopy.example.com/colour-balance", map_annotation_idx);
    ome::xml::model::primitives::OrderedMultimap map;
    map.push_back({"white-balance", "5,15,8"});
    map.push_back({"black-balance", "112,140,126"});
    store->setMapAnnotationValue(map, map_annotation_idx);

    // Link MapAnnotation to Detector.
    MetadataStore::index_type detector_ref_idx = 0;
    store->setDetectorAnnotationRef(annotation_id, instrument_idx, detector_idx,
                                    detector_ref_idx);

    // Create a LongAnnotation.
    ++annotation_idx;
    MetadataStore::index_type long_annotation_idx = 0;
    annotation_id = createID("Annotation", annotation_idx);
    store->setLongAnnotationID(annotation_id, long_annotation_idx);
    store->setLongAnnotationValue(239423, long_annotation_idx);
    store->setLongAnnotationNamespace
      ("https://microscopy.example.com/trigger-delay", long_annotation_idx);

    // Link LongAnnotation to Image.
    MetadataStore::index_type image_ref_idx = 0;
    store->setImageAnnotationRef(annotation_id, image_idx, image_ref_idx);

    // Create a second LongAnnotation.
    ++annotation_idx;
    ++long_annotation_idx;
    annotation_id = createID("Annotation", annotation_idx);
    store->setLongAnnotationID(annotation_id, long_annotation_idx);
    store->setLongAnnotationValue(934223, long_annotation_idx);
    store->setLongAnnotationNamespace
      ("https://microscopy.example.com/sample-number", long_annotation_idx);

    // Link second LongAnnotation to Image.
    ++image_ref_idx = 0;
    store->setImageAnnotationRef(annotation_id, image_idx, image_ref_idx);

    // Update all the annotation cross-references.
    store->resolveReferences();
    /* annotations-end */
  }

  void
  writePixelData(FormatWriter& writer,
                 std::ostream& stream)
  {
    /* pixel-example-start */
    // Total number of images (series)
    dimension_size_type ic = writer.getMetadataRetrieve()->getImageCount();
    stream << "Image count: " << ic << '\n';

    // Loop over images
    for (dimension_size_type i = 0 ; i < ic; ++i)
      {
        // Change the current series to this index
        writer.setSeries(i);

        // Total number of planes.
        dimension_size_type pc = 1U;
        pc *= writer.getMetadataRetrieve()->getPixelsSizeZ(i);
        pc *= writer.getMetadataRetrieve()->getPixelsSizeT(i);
        pc *= writer.getMetadataRetrieve()->getChannelCount(i);
        stream << "\tPlane count: " << pc << '\n';

        // Loop over planes (for this image index)
        for (dimension_size_type p = 0 ; p < pc; ++p)
          {
            // Pixel buffer; size 512 × 512 with 3 samples of type
            // uint16_t.  It uses the native endianness and has a
            // storage order of XYZTC without interleaving
            // (samples are planar).
            shared_ptr<PixelBuffer<PixelProperties<PixelType::UINT16>::std_type>>
              buffer(make_shared<PixelBuffer<PixelProperties<PixelType::UINT16>::std_type>>
                     (boost::extents[512][512][1][3],
                      PixelType::UINT16, ome::files::ENDIAN_NATIVE,
                      PixelBufferBase::make_storage_order(false)));

            // Fill each R, G or B sample with a different intensity
            // ramp in the 12-bit range.  In a real program, the pixel
            // data would typically be obtained from data acquisition
            // or another image.
            for (dimension_size_type x = 0; x < 512; ++x)
              for (dimension_size_type y = 0; y < 512; ++y)
                {
                  PixelBufferBase::indices_type idx;
                  std::fill(idx.begin(), idx.end(), 0);
                  idx[DIM_SPATIAL_X] = x;
                  idx[DIM_SPATIAL_Y] = y;

                  idx[DIM_SAMPLE] = 0;
                  buffer->at(idx) = (static_cast<float>(x) / 512.0f) * 4096.0f;
                  idx[DIM_SAMPLE] = 1;
                  buffer->at(idx) = (static_cast<float>(y) / 512.0f) * 4096.0f;
                  idx[DIM_SAMPLE] = 2;
                  buffer->at(idx) = (static_cast<float>(x+y) / 1024.0f) * 4096.0f;
                }

            VariantPixelBuffer vbuffer(buffer);
            stream << "PixelBuffer PixelType is " << buffer->pixelType() << '\n';
            stream << "VariantPixelBuffer PixelType is " << vbuffer.pixelType() << '\n';
            stream << std::flush;

            // Write the the entire pixel buffer to the plane.
            writer.saveBytes(p, vbuffer);

            stream << "Wrote " << buffer->num_elements() << ' '
                   << buffer->pixelType() << " pixels\n";
          }
      }
    /* pixel-example-end */
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
          // Add extended metadata.
          addExtendedMetadata(meta);

          // Create TIFF writer
          auto writer = make_shared<OMETIFFWriter>();

          // Set writer options before opening a file
          auto retrieve = static_pointer_cast<MetadataRetrieve>(meta);
          writer->setMetadataRetrieve(retrieve);
          writer->setInterleaved(false);
          writer->setTileSizeX(256);
          writer->setTileSizeY(256);

          // Open the file
          writer->setId(filename);

          // Write pixel data
          writePixelData(*writer, std::cout);

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
