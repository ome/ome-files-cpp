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

#include <cassert>

#include <boost/format.hpp>
#include <boost/range/size.hpp>

#include <ome/files/FormatException.h>
#include <ome/files/FormatTools.h>
#include <ome/files/MetadataTools.h>
#include <ome/files/out/MinimalTIFFWriter.h>
#include <ome/files/tiff/Codec.h>
#include <ome/files/tiff/IFD.h>
#include <ome/files/tiff/TIFF.h>
#include <ome/files/tiff/Util.h>

using ome::files::detail::WriterProperties;
using ome::files::tiff::TIFF;
using ome::files::tiff::IFD;
using ome::files::tiff::enableBigTIFF;
using ome::xml::model::enums::PixelType;
using ome::xml::meta::MetadataRetrieve;

namespace ome
{
  namespace files
  {
    namespace out
    {

      namespace
      {

        WriterProperties
        tiff_properties()
        {
          WriterProperties p("MinimalTIFF",
                             "Baseline Tagged Image File Format");

          // Note that tf2, tf8 and btf are all extensions for
          // "bigTIFF" (2nd generation TIFF, TIFF with 8-byte offsets
          // and big TIFF respectively).
          p.suffixes = {"tif", "tiff", "tf2", "tf8", "btf"};

          const PixelType::value_map_type& pv = PixelType::values();
          for (PixelType::value_map_type::const_iterator i = pv.begin();
               i != pv.end();
               ++i)
            {
              const std::vector<std::string>& ptcodecs = tiff::getCodecNames(i->first);
              std::set<std::string> codecset(ptcodecs.begin(), ptcodecs.end());
              // Supported by default with no compression
              codecset.insert("default");
              p.compression_types.insert(codecset.begin(), codecset.end());
              p.pixel_compression_types.insert(WriterProperties::pixel_compression_type_map::value_type(i->first, codecset));
            }

          return p;
        }

        const WriterProperties props(tiff_properties());

      }

      MinimalTIFFWriter::MinimalTIFFWriter():
        ::ome::files::detail::FormatWriter(props),
        logger(ome::common::createLogger("MinimalTIFFWriter")),
        tiff(),
        ifd(),
        ifdIndex(0),
        seriesIFDRange(),
        bigTIFF(boost::none)
      {
      }

      MinimalTIFFWriter::MinimalTIFFWriter(const WriterProperties& writerProperties):
        ::ome::files::detail::FormatWriter(writerProperties),
        logger(ome::common::createLogger("MinimalTIFFWriter")),
        tiff(),
        ifd(),
        ifdIndex(0),
        seriesIFDRange(),
        bigTIFF(boost::none)
      {
      }

      MinimalTIFFWriter::~MinimalTIFFWriter()
      {
        try
          {
            close();
          }
        catch (...)
          {
          }
      }

      void
      MinimalTIFFWriter::setId(const boost::filesystem::path& id)
      {
        FormatWriter::setId(id);

        std::string flags("w");

        // Get expected size of pixel data.
        std::shared_ptr<const ::ome::xml::meta::MetadataRetrieve> mr(getMetadataRetrieve());
        storage_size_type pixelSize = significantPixelSize(*mr);

        if (enableBigTIFF(bigTIFF, pixelSize, *currentId, logger))
          flags += '8';


        tiff = TIFF::open(id, flags);
        ifd = tiff->getCurrentDirectory();
        setupIFD();

        // Create IFD mapping from metadata.
        MetadataRetrieve::index_type imageCount = metadataRetrieve->getImageCount();
        dimension_size_type currentIFD = 0U;
        for (MetadataRetrieve::index_type i = 0;
             i < imageCount;
             ++i)
          {
            dimension_size_type planeCount = getImageCount();

            tiff::IFDRange range;
            range.filename = *currentId;
            range.begin = currentIFD;
            range.end = currentIFD + planeCount;

            seriesIFDRange.push_back(range);
            currentIFD += planeCount;
          }
      }

      void
      MinimalTIFFWriter::close(bool fileOnly)
      {
        try
          {
            if (tiff)
              {
                // Flush last IFD if unwritten.
                nextIFD();
                tiff->close();
              }

            ifd.reset();
            tiff.reset();
            ifdIndex = 0;
            seriesIFDRange.clear();
            bigTIFF = boost::none;

            detail::FormatWriter::close(fileOnly);
          }
        catch (const std::exception&)
          {
            tiff.reset(); // Ensure we only flush the last IFD once.
            detail::FormatWriter::close(fileOnly);
            throw;
          }
      }

      void
      MinimalTIFFWriter::setSeries(dimension_size_type series) const
      {
        const dimension_size_type currentSeries = getSeries();
        detail::FormatWriter::setSeries(series);

        if (currentSeries != series)
          {
            nextIFD();
            setupIFD();
          }
      }

      void
      MinimalTIFFWriter::setPlane(dimension_size_type plane) const
      {
        const dimension_size_type currentPlane = getPlane();
        detail::FormatWriter::setPlane(plane);

        if (currentPlane != plane)
          {
            nextIFD();
            setupIFD();
          }
      }

      void
      MinimalTIFFWriter::nextIFD() const
      {
        tiff->writeCurrentDirectory();
        ifd = tiff->getCurrentDirectory();
        ++ifdIndex;
      }

      void
      MinimalTIFFWriter::setupIFD() const
      {
        // Default to single strips for now.
        ifd->setImageWidth(getSizeX());
        ifd->setImageHeight(getSizeY());

        // Default strip or tile size.  We base this upon a default
        // chunk size of 64KiB for greyscale images, which will
        // increase to 192KiB for 3 sample RGB images.  We use strips
        // up to a width of 2048 after which tiles are used.
        if(getSizeX() == 0)
          {
            throw FormatException("Can't set strip or tile size: SizeX is 0");
          }
        else if(getSizeX() < 2048)
          {
            // Default to strips, mainly for compatibility with
            // readers which don't support tiles.
            ifd->setTileType(tiff::STRIP);
            ifd->setTileWidth(getSizeX());
            uint32_t height = 65536U / getSizeX();
            if (height == 0)
              height = 1;
            ifd->setTileHeight(height);
          }
        else
          {
            // Default to tiles.
            ifd->setTileType(tiff::TILE);
            ifd->setTileWidth(256U);
            ifd->setTileHeight(256U);
          }

        std::array<dimension_size_type, 3> coords = getZCTCoords(getPlane());

        dimension_size_type channel = coords[1];

        ifd->setPixelType(getPixelType());
        ifd->setBitsPerSample(bitsPerPixel(getPixelType()));
        ifd->setSamplesPerPixel(getRGBChannelCount(channel));

        const boost::optional<bool> interleaved(getInterleaved());
        if (interleaved && *interleaved)
          ifd->setPlanarConfiguration(tiff::CONTIG);
        else
          ifd->setPlanarConfiguration(tiff::SEPARATE);

        // This isn't necessarily always true; we might want to use a
        // photometric interpretation other than RGB with three
        // subchannels.
        if (isRGB(channel) && getRGBChannelCount(channel) == 3)
          ifd->setPhotometricInterpretation(tiff::RGB);
        else
          ifd->setPhotometricInterpretation(tiff::MIN_IS_BLACK);

        const boost::optional<std::string> compression(getCompression());
        if(compression)
          ifd->setCompression(tiff::getCodecScheme(*compression));
      }

      void
      MinimalTIFFWriter::saveBytes(dimension_size_type plane,
                                   VariantPixelBuffer& buf,
                                   dimension_size_type x,
                                   dimension_size_type y,
                                   dimension_size_type w,
                                   dimension_size_type h)
      {
        assertId(currentId, true);

        setPlane(plane);

        dimension_size_type expectedIndex =
          tiff::ifdIndex(seriesIFDRange, getSeries(), plane);

        if (ifdIndex != expectedIndex)
          {
            boost::format fmt("IFD index mismatch: actual is %1% but %2% expected");
            fmt % ifdIndex % expectedIndex;
            throw FormatException(fmt.str());
          }

        ifd->writeImage(buf, x, y, w, h);
      }

      void
      MinimalTIFFWriter::setBigTIFF(boost::optional<bool> big)
      {
        bigTIFF = big;
      }

      boost::optional<bool>
      MinimalTIFFWriter::getBigTIFF() const
      {
        return bigTIFF;
      }

    }
  }
}
