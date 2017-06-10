/*
* #%L
* OME-FILES C++ library for image IO.
* Copyright © 2015–2017 Open Microscopy Environment:
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

#include <iostream>
#include <memory>

#include <boost/filesystem/path.hpp>

#include <ome/files/CoreMetadata.h>
#include <ome/files/MetadataTools.h>
#include <ome/files/VariantPixelBuffer.h>
#include <ome/files/out/OMETIFFWriter.h>
#include <ome/xml/meta/OMEXMLMetadata.h>
#include <ome/xml/model/AnnotationRef.h>
#include <ome/xml/model/Channel.h>
#include <ome/xml/model/Detector.h>
#include <ome/xml/model/DetectorSettings.h>
#include <ome/xml/model/Image.h>
#include <ome/xml/model/Instrument.h>
#include <ome/xml/model/LongAnnotation.h>
#include <ome/xml/model/MapAnnotation.h>
#include <ome/xml/model/OME.h>
#include <ome/xml/model/Objective.h>

using boost::filesystem::path;
using std::make_shared;
using std::shared_ptr;
using std::static_pointer_cast;
using ome::files::createID;
using ome::files::dimension_size_type;
using ome::files::fillMetadata;
using ome::files::CoreMetadata;
using ome::files::DIM_SPATIAL_X;
using ome::files::DIM_SPATIAL_Y;
using ome::files::DIM_CHANNEL;
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
using ome::xml::model::primitives::Quantity;
using ome::xml::meta::MetadataRetrieve;
using ome::xml::meta::MetadataStore;
using ome::xml::meta::Metadata;
using ome::xml::meta::OMEXMLMetadata;

namespace
{

  shared_ptr<OMEXMLMetadata>
  createMetadata()
  {
    /* core-metadata-start */
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
    core->sizeC.clear(); // defaults to 1 channel with 1 subchannel; clear this
    core->sizeC.push_back(1);
    core->sizeC.push_back(1);
    core->sizeC.push_back(1);
    core->pixelType = PixelType::UINT16;
    core->interleaved = false;
    core->bitsPerPixel = 12U;
    core->dimensionOrder = DimensionOrder::XYZTC;
    seriesList.push_back(core);
    seriesList.push_back(core); // add two identical series

    fillMetadata(*meta, seriesList);
    /* core-metadata-end */

    return meta;
  }

  void
  addExtendedMetadata(shared_ptr<OMEXMLMetadata> store)
  {
    /* extended-metadata-start */
    // Get root OME object
    std::shared_ptr<ome::xml::meta::OMEXMLMetadataRoot>
      root(std::dynamic_pointer_cast<ome::xml::meta::OMEXMLMetadataRoot>
           (store->getRoot()));
    if (!root)
      return;

    MetadataStore::index_type annotation_idx = 0;

    // Create an Instrument.
    auto instrument = make_shared<ome::xml::model::Instrument>();
    instrument->setID(createID("Instrument", 0));
    root->addInstrument(instrument);

    // Create an Objective for this Instrument.
    auto objective = make_shared<ome::xml::model::Objective>();
    objective->setID(createID("Objective", 0));
    auto objective_manufacturer = std::make_shared<std::string>("InterFocal");
    objective->setManufacturer(objective_manufacturer);
    auto objective_nominal_mag = std::make_shared<double>(40.0);
    objective->setNominalMagnification(objective_nominal_mag);
    auto objective_na = std::make_shared<double>(0.4);
    objective->setLensNA(objective_na);
    auto objective_immersion = std::make_shared<Immersion>(Immersion::OIL);
    objective->setImmersion(objective_immersion);
    auto objective_wd = std::make_shared<Quantity<UnitsLength>>
      (0.34, UnitsLength::MILLIMETER);
    objective->setWorkingDistance(objective_wd);
    instrument->addObjective(objective);

    // Create a Detector for this Instrument.
    auto detector = make_shared<ome::xml::model::Detector>();
    std::string detector_id = createID("Detector", 0);
    detector->setID(detector_id);
    auto detector_manufacturer = std::make_shared<std::string>("MegaCapture");
    detector->setManufacturer(detector_manufacturer);
    auto detector_type = std::make_shared<DetectorType>(DetectorType::CCD);
    detector->setType(detector_type);
    instrument->addDetector(detector);

    // Get Image and Channel for future use.  Note for your own code,
    // you should check that the elements exist before accessing them;
    // here we know they are valid because we created them above.
    auto image = root->getImage(0);
    auto pixels = image->getPixels();
    auto channel0 = pixels->getChannel(0);
    auto channel1 = pixels->getChannel(1);
    auto channel2 = pixels->getChannel(2);

    // Create Settings for this Detector for each Channel on the Image.
    auto detector_settings0 = make_shared<ome::xml::model::DetectorSettings>();
    {
      detector_settings0->setID(detector_id);
      auto detector_binning = std::make_shared<Binning>(Binning::TWOBYTWO);
      detector_settings0->setBinning(detector_binning);
      auto detector_gain = std::make_shared<double>(83.81);
      detector_settings0->setGain(detector_gain);
      channel0->setDetectorSettings(detector_settings0);
    }

    auto detector_settings1 = make_shared<ome::xml::model::DetectorSettings>();
    {
      detector_settings1->setID(detector_id);
      auto detector_binning = std::make_shared<Binning>(Binning::TWOBYTWO);
      detector_settings1->setBinning(detector_binning);
      auto detector_gain = std::make_shared<double>(56.89);
      detector_settings1->setGain(detector_gain);
      channel1->setDetectorSettings(detector_settings1);
    }

    auto detector_settings2 = make_shared<ome::xml::model::DetectorSettings>();
    {
      detector_settings2->setID(detector_id);
      auto detector_binning = std::make_shared<Binning>(Binning::FOURBYFOUR);
      detector_settings2->setBinning(detector_binning);
      auto detector_gain = std::make_shared<double>(12.93);
      detector_settings2->setGain(detector_gain);
      channel2->setDetectorSettings(detector_settings2);
    }
    /* extended-metadata-end */

    /* annotations-start */
    // Add Structured Annotations.
    auto sa = std::make_shared<ome::xml::model::StructuredAnnotations>();
    root->setStructuredAnnotations(sa);

    // Create a MapAnnotation.
    auto map_ann0 = std::make_shared<ome::xml::model::MapAnnotation>();
    std::string annotation_id = createID("Annotation", annotation_idx);
    map_ann0->setID(annotation_id);
    auto map_ann0_ns = std::make_shared<std::string>
      ("https://microscopy.example.com/colour-balance");
    map_ann0->setNamespace(map_ann0_ns);
    map_ann0->setValue({{"white-balance", "5,15,8"},
          {"black-balance", "112,140,126"}});
    sa->addMapAnnotation(map_ann0);

    // Link MapAnnotation to Detector.
    detector->linkAnnotation(map_ann0);

    // Create a LongAnnotation.
    auto long_ann0 = std::make_shared<ome::xml::model::LongAnnotation>();
    ++annotation_idx;
    annotation_id = createID("Annotation", annotation_idx);
    long_ann0->setID(annotation_id);
    auto long_ann0_ns = std::make_shared<std::string>
      ("https://microscopy.example.com/trigger-delay");
    long_ann0->setNamespace(long_ann0_ns);
    long_ann0->setValue(239423);
    sa->addLongAnnotation(long_ann0);

    // Link LongAnnotation to Image.
    image->linkAnnotation(long_ann0);

    // Create a second LongAnnotation.
    auto long_ann1 = std::make_shared<ome::xml::model::LongAnnotation>();
    ++annotation_idx;
    annotation_id = createID("Annotation", annotation_idx);
    long_ann1->setID(annotation_id);
    auto long_ann1_ns = std::make_shared<std::string>
      ("https://microscopy.example.com/sample-number");
    long_ann1->setNamespace(long_ann1_ns);
    long_ann1->setValue(934223);

    // Link second LongAnnotation to Image.
    image->linkAnnotation(long_ann1);
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
            // Change the current plane to this index.
            writer.setPlane(p);

            // Pixel buffer; size 512 × 512 with 3 channels of type
            // uint16_t.  It uses the native endianness and has a
            // storage order of XYZTC without interleaving
            // (subchannels are planar).
            shared_ptr<PixelBuffer<PixelProperties<PixelType::UINT16>::std_type>>
              buffer(make_shared<PixelBuffer<PixelProperties<PixelType::UINT16>::std_type>>
                     (boost::extents[512][512][1][1][1][1][1][1][1],
                      PixelType::UINT16, ome::files::ENDIAN_NATIVE,
                      PixelBufferBase::make_storage_order(DimensionOrder::XYZTC, false)));

            // Fill each subchannel with a different intensity ramp in
            // the 12-bit range.  In a real program, the pixel data
            // would typically be obtained from data acquisition or
            // another image.
            for (dimension_size_type x = 0; x < 512; ++x)
              for (dimension_size_type y = 0; y < 512; ++y)
                {
                  PixelBufferBase::indices_type idx;
                  std::fill(idx.begin(), idx.end(), 0);
                  idx[DIM_SPATIAL_X] = x;
                  idx[DIM_SPATIAL_Y] = y;

                  idx[DIM_CHANNEL] = 0;

                  switch(p)
                    {
                    case 0:
                      buffer->at(idx) = (static_cast<float>(x) / 512.0f) * 4096.0f;
                      break;
                    case 1:
                      buffer->at(idx) = (static_cast<float>(y) / 512.0f) * 4096.0f;
                      break;
                    case 2:
                      buffer->at(idx) = (static_cast<float>(x+y) / 1024.0f) * 4096.0f;
                      break;
                    default:
                      break;
                    }
                }

            VariantPixelBuffer vbuffer(buffer);
            stream << "PixelBuffer PixelType is " << buffer->pixelType() << '\n';
            stream << "VariantPixelBuffer PixelType is " << vbuffer.pixelType() << '\n';
            stream << std::flush;

            // Write the the entire pixel buffer to the plane.
            writer.saveBytes(p, vbuffer);

            stream << "Wrote " << buffer->num_elements() << ' ' << buffer->pixelType() << " pixels\n";
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
          // Create metadata for the file to be written.
          auto meta = createMetadata();
          // Add extended metadata.
          addExtendedMetadata(meta);

          // Create TIFF writer
          auto writer = make_shared<OMETIFFWriter>();

          // Set writer options before opening a file
          auto retrieve = static_pointer_cast<MetadataRetrieve>(meta);
          writer->setMetadataRetrieve(retrieve);
          writer->setInterleaved(false);

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
