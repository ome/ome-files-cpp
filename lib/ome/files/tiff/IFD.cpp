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

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cassert>

#include <boost/format.hpp>

#include <ome/files/PlaneRegion.h>
#include <ome/files/TileBuffer.h>
#include <ome/files/TileCache.h>
#include <ome/files/tiff/IFD.h>
#include <ome/files/tiff/Tags.h>
#include <ome/files/tiff/Field.h>
#include <ome/files/tiff/TIFF.h>
#include <ome/files/tiff/Sentry.h>
#include <ome/files/tiff/Exception.h>

#include <ome/common/string.h>

#include <tiffio.h>

using ome::xml::model::enums::PixelType;
using index_type = ome::files::VariantPixelBuffer::indices_type::value_type;

namespace
{

  using namespace ::ome::files::tiff;
  using ::ome::files::dimension_size_type;
  using ::ome::files::PixelBuffer;
  using ::ome::files::PixelProperties;
  using ::ome::files::PlaneRegion;
  using ::ome::files::TileBuffer;
  using ::ome::files::TileCache;
  using ::ome::files::TileCoverage;

  // VariantPixelBuffer tile transfer
  // ────────────────────────────────
  //
  // ReadVisitor: Transfer a set of tiles to a destination pixel buffer.
  // WriteVisitor: Transfer source pixel buffer data to a set of tiles.
  //
  // ┏━━━━━━┯━━━━━━┯━━━━━━┯━━━┓
  // ┃      │      │      │░░░┃
  // ┃      │      │      │░░░┃
  // ┃      │      │      │░░░┃
  // ┃   ╔══╪══════╪════╗ │░░░┃
  // ┃   ║  │      │    ║ │░░░┃
  // ┃   ║  │      │    ║ │░░░┃
  // ┠───╫──┼──────┼────╫─┼───┨
  // ┃   ║  │      │╔══╗║ │░░░┃
  // ┃   ║  │      │║▓▓║║ │░░░┃
  // ┃   ║  │      │║▓▓║║ │░░░┃
  // ┃   ║  │      │╚══╝║ │░░░┃
  // ┃   ║  │      │    ║ │░░░┃
  // ┃   ║  │      │    ║ │░░░┃
  // ┠───╫──┼──────┼────╫─┼───┨
  // ┃   ║  │▒▒▒▒▒▒│    ║ │░░░┃
  // ┃   ║  │▒▒▒▒▒▒│    ║ │░░░┃
  // ┃   ║  │▒▒▒▒▒▒│    ║ │░░░┃
  // ┃   ╚══╪══════╪════╝ │░░░┃
  // ┃      │      │      │░░░┃
  // ┃      │      │      │░░░┃
  // ┠──────┼──────┼──────┼───┨
  // ┃░░░░░░│░░░░░░│░░░░░░│░░░┃
  // ┃░░░░░░│░░░░░░│░░░░░░│░░░┃
  // ┗━━━━━━┷━━━━━━┷━━━━━━┷━━━┛
  //
  // ━━━━ Image region
  // ──── TIFF tile and TileBuffer region
  // ════ VariantPixelBuffer region
  //
  // ░░░░ Incomplete tiles which overlap the image region
  // ▒▒▒▒ Intersection (clip region) of pixel buffer with tile buffer
  // ▓▓▓▓ Unaligned clip region (of a smaller size than the tile
  //      dimensions)
  //
  // Both visitors iterate over the tiles partially or fully covered
  // by the pixel buffer, and use the optimal strategy to copy data
  // between the pixel buffer and tile buffer.  This will typically be
  // std::copy (usually memmove(3) internally) of whole tiles or tile
  // chunks where the tile widths are compatible, or individual
  // scanlines where they are not compatible.

  struct ReadVisitor
  {
    const IFD&                              ifd;
    const TileInfo&                         tileinfo;
    const PlaneRegion&                      region;
    const std::vector<dimension_size_type>& tiles;
    TileBuffer                              tilebuf;

    ReadVisitor(const IFD&                              ifd,
                const TileInfo&                         tileinfo,
                const PlaneRegion&                      region,
                const std::vector<dimension_size_type>& tiles):
      ifd(ifd),
      tileinfo(tileinfo),
      region(region),
      tiles(tiles),
      tilebuf(tileinfo.bufferSize())
    {}

    ~ReadVisitor()
    {
    }

    template<typename T>
    void
    transfer(std::shared_ptr<T>&       buffer,
             typename T::indices_type& destidx,
             const TileBuffer&         tilebuf,
             PlaneRegion&              rfull,
             PlaneRegion&              rclip,
             uint16_t                  copysamples)
    {
      if (rclip.w == rfull.w &&
          rclip.x == region.x &&
          rclip.w == region.w)
        {
          // Transfer contiguous block since the tile spans the
          // whole region width for both source and destination
          // buffers.

          destidx[ome::files::DIM_SPATIAL_X] = rclip.x - region.x;
          destidx[ome::files::DIM_SPATIAL_Y] = rclip.y - region.y;

          typename T::value_type *dest = &buffer->at(destidx);
          const typename T::value_type *src = reinterpret_cast<const typename T::value_type *>(tilebuf.data());
          std::copy(src,
                    src + (rclip.w * rclip.h * copysamples),
                    dest);
        }
      else
        {
          // Transfer discontiguous block.

          dimension_size_type xoffset = (rclip.x - rfull.x) * copysamples;

          for (dimension_size_type row = rclip.y;
               row != rclip.y + rclip.h;
               ++row)
            {
              dimension_size_type yoffset = (row - rfull.y) * (rfull.w * copysamples);

              destidx[ome::files::DIM_SPATIAL_X] = rclip.x - region.x;
              destidx[ome::files::DIM_SPATIAL_Y] = row - region.y;

              typename T::value_type *dest = &buffer->at(destidx);
              const typename T::value_type *src = reinterpret_cast<const typename T::value_type *>(tilebuf.data());
              std::copy(src + yoffset + xoffset,
                        src + yoffset + xoffset + (rclip.w * copysamples),
                        dest);
            }
        }
    }

    // Special case for BIT
    void
    transfer(std::shared_ptr<PixelBuffer<PixelProperties<PixelType::BIT>::std_type>>& buffer,
             PixelBuffer<PixelProperties<PixelType::BIT>::std_type>::indices_type&    destidx,
             const TileBuffer&                                                        tilebuf,
             PlaneRegion&                                                             rfull,
             PlaneRegion&                                                             rclip,
             uint16_t                                                                 copysamples)
    {
      // Unpack bits from buffer.

      typedef PixelBuffer<PixelProperties<PixelType::BIT>::std_type> T;

      dimension_size_type xoffset = (rclip.x - rfull.x) * copysamples;

      for (dimension_size_type row = rclip.y;
           row != rclip.y + rclip.h;
           ++row)
        {
          const dimension_size_type full_row_width = rfull.w * copysamples;
          dimension_size_type yoffset = (row - rfull.y) * full_row_width;

          destidx[ome::files::DIM_SPATIAL_X] = rclip.x - region.x;
          destidx[ome::files::DIM_SPATIAL_Y] = row - region.y;

          T::value_type *dest = &buffer->at(destidx);
          const uint8_t *src = reinterpret_cast<const uint8_t *>(tilebuf.data());

          for (dimension_size_type sampleoffset = 0U;
               sampleoffset < (rclip.w * copysamples);
               ++sampleoffset)
            {
              dimension_size_type src_bit = yoffset + xoffset + sampleoffset;
              const uint8_t *src_byte = src + (src_bit / 8U);
              const uint8_t bit_offset = 7U - (src_bit % 8U);
              const uint8_t mask = static_cast<uint8_t>(1U << bit_offset);
              assert(src_byte >= src && src_byte < src + tilebuf.size());
              *(dest+sampleoffset) = static_cast<T::value_type>(*src_byte & mask);
            }
        }
    }

    template<typename T>
    void
    transferRGBA(std::shared_ptr<T>&       /* buffer */,
                 typename T::indices_type& /* destidx */,
                 const TileBuffer&         /* tilebuf */,
                 PlaneRegion&              /* rfull */,
                 PlaneRegion&              /* rclip */,
                 uint16_t                  /* copysamples */)
    {
      boost::format fmt("Unsupported TIFF RGBA pixel type %1%");
      fmt % ifd.getPixelType();
      throw Exception(fmt.str());
    }

    // Special case for UINT8 RGBA
    void
    transferRGBA(std::shared_ptr<PixelBuffer<PixelProperties<PixelType::UINT8>::std_type>>& buffer,
                 PixelBuffer<PixelProperties<PixelType::UINT8>::std_type>::indices_type&    destidx,
                 const TileBuffer&                                                          tilebuf,
                 PlaneRegion&                                                               rfull,
                 PlaneRegion&                                                               rclip,
                 uint16_t                                                                   copysamples)
    {
      // Transfer discontiguous block (typically dropping alpha).

      typedef PixelBuffer<PixelProperties<PixelType::UINT8>::std_type> T;

      const T::value_type *src = reinterpret_cast<const T::value_type *>(tilebuf.data());

      for (dimension_size_type row = rclip.y;
           row < rclip.y + rclip.h;
           ++row)
        {
          // Indexed from bottom-left, so invert y.
          dimension_size_type yoffset = (rclip.y + rclip.h - row - 1) * (rfull.w * 4);
          destidx[ome::files::DIM_SPATIAL_Y] = row - region.y;

          for (dimension_size_type col = rclip.x;
               col < rclip.x + rclip.w;
               ++col)
            {
              dimension_size_type xoffset = (rclip.x - rfull.x + col) * 4;
              destidx[ome::files::DIM_SPATIAL_X] = col - region.x;

              for (uint16_t sample = 0; sample < copysamples; ++sample)
                {
                  destidx[ome::files::DIM_SAMPLE] = sample;
                  buffer->array()(destidx) = src[xoffset + yoffset + sample];
                }
            }
        }
    }

    template<typename T>
    dimension_size_type
    expected_read(const std::shared_ptr<T>& /* buffer */,
                  const PlaneRegion&        rclip,
                  uint16_t                  copysamples) const
    {
      return rclip.w * rclip.h * copysamples * sizeof(typename T::value_type);
    }

    // Special case for BIT
    dimension_size_type
    expected_read(const std::shared_ptr<PixelBuffer<PixelProperties<PixelType::BIT>::std_type>>& /* buffer */,
                  const PlaneRegion&                                                             rclip,
                  uint16_t                                                                       copysamples) const
    {
      dimension_size_type expectedread = rclip.w;

      if (expectedread % 8)
        ++expectedread;
      expectedread *= rclip.h * copysamples;
      expectedread /= 8;

      return expectedread;
    }

    template<typename T>
    void
    operator()(std::shared_ptr<T>& buffer)
    {
      std::shared_ptr<::ome::files::tiff::TIFF>& tiff(ifd.getTIFF());
      ::TIFF *tiffraw = reinterpret_cast<::TIFF *>(tiff->getWrapped());
      TileType type = tileinfo.tileType();

      uint16_t samples = ifd.getSamplesPerPixel();
      PlanarConfiguration planarconfig = ifd.getPlanarConfiguration();

      Sentry sentry;

      for(const auto i : tiles)
        {
          tstrile_t tile = static_cast<tstrile_t>(i);
          PlaneRegion rfull = tileinfo.tileRegion(tile);
          PlaneRegion rclip = tileinfo.tileRegion(tile, region);
          dimension_size_type sample = tileinfo.tileSample(tile);

          uint16_t copysamples = samples;
          dimension_size_type dest_sample = 0;
          if (planarconfig == SEPARATE)
            {
              copysamples = 1;
              dest_sample = sample;
            }

          if (type == TILE)
            {
              tmsize_t bytesread = TIFFReadEncodedTile(tiffraw, tile, tilebuf.data(), static_cast<tsize_t>(tilebuf.size()));
              if (bytesread < 0)
                sentry.error("Failed to read encoded tile");
              else if (static_cast<dimension_size_type>(bytesread) != tilebuf.size())
                sentry.error("Failed to read encoded tile fully");
            }
          else
            {
              tmsize_t bytesread = TIFFReadEncodedStrip(tiffraw, tile, tilebuf.data(), static_cast<tsize_t>(tilebuf.size()));
              dimension_size_type expectedread = expected_read(buffer, rclip, copysamples);
              if (bytesread < 0)
                sentry.error("Failed to read encoded strip");
              else if (static_cast<dimension_size_type>(bytesread) < expectedread)
                sentry.error("Failed to read encoded strip fully");
            }

          typename T::indices_type destidx = {0, 0, 0, static_cast<index_type>(dest_sample)};

          transfer(buffer, destidx, tilebuf, rfull, rclip, copysamples);
        }
    }
  };

  struct WriteVisitor
  {
    IFD&                                    ifd;
    std::vector<TileCoverage>&              tilecoverage;
    TileCache&                              tilecache;
    const TileInfo&                         tileinfo;
    const PlaneRegion&                      region;
    const std::vector<dimension_size_type>& tiles;

    WriteVisitor(IFD&                                    ifd,
                 std::vector<TileCoverage>&              tilecoverage,
                 TileCache&                              tilecache,
                 const TileInfo&                         tileinfo,
                 const PlaneRegion&                      region,
                 const std::vector<dimension_size_type>& tiles):
      ifd(ifd),
      tilecoverage(tilecoverage),
      tilecache(tilecache),
      tileinfo(tileinfo),
      region(region),
      tiles(tiles)
    {}

    // Flush covered tile.
    void
    flush(tstrile_t tile)
    {
      std::shared_ptr<::ome::files::tiff::TIFF>& tiff(ifd.getTIFF());
      ::TIFF *tiffraw = reinterpret_cast<::TIFF *>(tiff->getWrapped());
      TileType type = tileinfo.tileType();
      PlaneRegion rimage(0, 0, ifd.getImageWidth(), ifd.getImageHeight());

      Sentry sentry;

      dimension_size_type tile_sample = tileinfo.tileSample(tile);

      PlaneRegion validarea = tileinfo.tileRegion(tile) & rimage;
      if (!validarea.area())
        return;

      if (!tilecoverage.at(tile_sample).covered(validarea))
        return;

      assert(tilecache.find(tile));
      TileBuffer& tilebuf = *tilecache.find(tile);
      if (type == TILE)
        {
          tsize_t byteswritten = TIFFWriteEncodedTile(tiffraw, tile, tilebuf.data(), static_cast<tsize_t>(tilebuf.size()));
          if (byteswritten < 0)
            sentry.error("Failed to write encoded tile");
          else if (static_cast<dimension_size_type>(byteswritten) != tilebuf.size())
            sentry.error("Failed to write encoded tile fully");
        }
      else
        {
          tsize_t byteswritten = TIFFWriteEncodedStrip(tiffraw, tile, tilebuf.data(), static_cast<tsize_t>(tilebuf.size()));
          if (byteswritten < 0)
            sentry.error("Failed to write encoded strip");
          else if (static_cast<dimension_size_type>(byteswritten) != tilebuf.size())
            sentry.error("Failed to write encoded strip fully");
        }
      tilecache.erase(tile);
    }

    template<typename T>
    void
    transfer(const std::shared_ptr<T>& buffer,
             typename T::indices_type& srcidx,
             TileBuffer&               tilebuf,
             PlaneRegion&              rfull,
             PlaneRegion&              rclip,
             uint16_t                  copysamples)
    {
      if (rclip.w == rfull.w &&
          rclip.x == region.x &&
          rclip.w == region.w)
        {
          // Transfer contiguous block since the tile spans the
          // whole region width for both source and destination
          // buffers.

          srcidx[ome::files::DIM_SPATIAL_X] = rclip.x - region.x;
          srcidx[ome::files::DIM_SPATIAL_Y] = rclip.y - region.y;

          typename T::value_type *dest = reinterpret_cast<typename T::value_type *>(tilebuf.data());
          const typename T::value_type *src = &buffer->at(srcidx);

          assert(dest + (rclip.w * rclip.h * copysamples) <= dest + tilebuf.size());
          std::copy(src,
                    src + (rclip.w * rclip.h * copysamples),
                    dest);
        }
      else
        {
          // Transfer discontiguous block.

          dimension_size_type xoffset = (rclip.x - rfull.x) * copysamples;

          for (dimension_size_type row = rclip.y;
               row < rclip.y + rclip.h;
               ++row)
            {
              dimension_size_type yoffset = (row - rfull.y) * (rfull.w * copysamples);

              srcidx[ome::files::DIM_SPATIAL_X] = rclip.x - region.x;
              srcidx[ome::files::DIM_SPATIAL_Y] = row - region.y;

              typename T::value_type *dest = reinterpret_cast<typename T::value_type *>(tilebuf.data());
              const typename T::value_type *src = &buffer->at(srcidx);

              assert(dest + yoffset + xoffset + (rclip.w * copysamples) <= dest + tilebuf.size());
              std::copy(src,
                        src + (rclip.w * copysamples),
                        dest + yoffset + xoffset);
            }
        }
    }

    // Special case for BIT
    void
    transfer(const std::shared_ptr<PixelBuffer<PixelProperties<PixelType::BIT>::std_type>>& buffer,
             PixelBuffer<PixelProperties<PixelType::BIT>::std_type>::indices_type&          srcidx,
             TileBuffer&                                                                    tilebuf,
             PlaneRegion&                                                                   rfull,
             PlaneRegion&                                                                   rclip,
             uint16_t                                                                       copysamples)
    {
      // Pack bits into buffer.

      typedef PixelBuffer<PixelProperties<PixelType::BIT>::std_type> T;

      dimension_size_type xoffset = (rclip.x - rfull.x) * copysamples;

      for (dimension_size_type row = rclip.y;
           row != rclip.y + rclip.h;
           ++row)
        {
          const dimension_size_type full_row_width = rfull.w * copysamples;
          dimension_size_type yoffset = (row - rfull.y) * full_row_width;

          srcidx[ome::files::DIM_SPATIAL_X] = rclip.x - region.x;
          srcidx[ome::files::DIM_SPATIAL_Y] = row - region.y;

          uint8_t *dest = reinterpret_cast<uint8_t *>(tilebuf.data());
          const T::value_type *src = &buffer->at(srcidx);

          for (dimension_size_type sampleoffset = 0;
               sampleoffset < (rclip.w * copysamples);
               ++sampleoffset)
            {
              const T::value_type *srcsample = src + sampleoffset;
              dimension_size_type dest_bit = yoffset + xoffset + sampleoffset;
              uint8_t *dest_byte = dest + (dest_bit / 8);
              const uint8_t bit_offset = 7 - (dest_bit % 8);

              assert(dest_byte >= dest && dest_byte < dest + tilebuf.size());
              // Don't clear the bit since the tile will only be written once.
              *dest_byte |= static_cast<uint8_t>(*srcsample << bit_offset);
            }
        }
    }

    template<typename T>
    void
    operator()(const std::shared_ptr<T>& buffer)
    {
      uint16_t samples = ifd.getSamplesPerPixel();
      PlanarConfiguration planarconfig = ifd.getPlanarConfiguration();

      if (tilecoverage.size() != (planarconfig == CONTIG ? 1 : samples))
        tilecoverage.resize(planarconfig == CONTIG ? 1 : samples);

      for(const auto i : tiles)
        {
          tstrile_t tile = static_cast<tstrile_t>(i);
          PlaneRegion rfull = tileinfo.tileRegion(tile);
          PlaneRegion rclip = tileinfo.tileRegion(tile, region);
          dimension_size_type sample = tileinfo.tileSample(tile);

          uint16_t copysamples = samples;
          dimension_size_type dest_sample = 0;
          if (planarconfig == SEPARATE)
            {
              copysamples = 1;
              dest_sample = sample;
            }

          if (!tilecache.find(tile))
            {
              tilecache.insert(tile, std::make_shared<TileBuffer>(tileinfo.bufferSize()));
            }
          assert(tilecache.find(tile));
          TileBuffer& tilebuf = *tilecache.find(tile);

          typename T::indices_type srcidx = {0, 0, 0, static_cast<index_type>(dest_sample)};

          transfer(buffer, srcidx, tilebuf, rfull, rclip, copysamples);
          tilecoverage.at(dest_sample).insert(rclip);

          // Flush tile if covered.
          flush(tile);
        }

    }
  };

}

namespace ome
{
  namespace files
  {
    namespace tiff
    {

      namespace
      {

        class IFDConcrete : public IFD
        {
        public:
          IFDConcrete(std::shared_ptr<TIFF>& tiff,
                      offset_type            offset):
            IFD(tiff, offset)
          {
          }

          IFDConcrete(std::shared_ptr<TIFF>& tiff):
            IFD(tiff, 0)
          {
          }

          virtual
          ~IFDConcrete()
          {
          }
        };

      }

      /**
       * Internal implementation details of OffsetIFD.
       */
      class IFD::Impl
      {
      public:
        /// Reference to the parent TIFF.
        std::shared_ptr<TIFF> tiff;
        /// Offset of this IFD.
        offset_type offset;
        /// Tile coverage cache (used when writing).
        std::shared_ptr<std::vector<TileCoverage>> coverage;
        /// Tile cache (used when writing).
        std::shared_ptr<TileCache> tilecache;
        /// Tile type.
        boost::optional<TileType> tiletype;
        /// Image width.
        boost::optional<uint32_t> imagewidth;
        /// Image height.
        boost::optional<uint32_t> imageheight;
        /// Tile width.
        boost::optional<uint32_t> tilewidth;
        /// Tile height.
        boost::optional<uint32_t> tileheight;
        /// Pixel type.
        boost::optional<PixelType> pixeltype;
        /// Bits per sample.
        boost::optional<uint16_t> bits;
        /// Samples per pixel.
        boost::optional<uint16_t> samples;
        /// Planar configuration.
        boost::optional<PlanarConfiguration> planarconfig;
        /// Photometric interpretation.
        boost::optional<PhotometricInterpretation> photometric;
        /// Compression scheme.
        boost::optional<Compression> compression;
        /// Compression scheme.
        boost::optional<std::vector<uint64_t>> subifds;

        /**
         * Constructor.
         *
         * @param tiff the parent TIFF.
         * @param offset the IFD offset.
         */
        Impl(std::shared_ptr<TIFF>& tiff,
             offset_type            offset):
          tiff(tiff),
          offset(offset),
          coverage(std::make_shared<std::vector<TileCoverage>>()),
          tilecache(std::make_shared<TileCache>()),
          imagewidth(),
          imageheight(),
          tilewidth(),
          tileheight(),
          pixeltype(),
          samples(),
          planarconfig()
        {
        }

        /**
         * Copy constructor.
         *
         * @param copy the object to copy.
         */
        Impl(const Impl& copy):
          tiff(copy.tiff),
          offset(copy.offset),
          coverage(copy.coverage),
          tilecache(copy.tilecache),
          imagewidth(copy.imagewidth),
          imageheight(copy.imageheight),
          tilewidth(copy.tilewidth),
          tileheight(copy.tileheight),
          pixeltype(copy.pixeltype),
          samples(copy.samples),
          planarconfig(copy.planarconfig),
          subifds(copy.subifds)
        {
        }

        /// Destructor.
        ~Impl()
        {
        }

        /**
         * Copy assignment operator.
         *
         * @param rhs the object to assign.
         * @returns the modified object.
         */
        Impl&
        operator= (const Impl& rhs)
        {
          tiff = rhs.tiff;
          offset = rhs.offset;
          coverage = rhs.coverage;
          tilecache = rhs.tilecache;
          imagewidth = rhs.imagewidth;
          imageheight = rhs.imageheight;
          tilewidth = rhs.tilewidth;
          tileheight = rhs.tileheight;
          pixeltype = rhs.pixeltype;
          samples = rhs.samples;
          planarconfig = rhs.planarconfig;
          subifds = rhs.subifds;

          return *this;
        }
      };

      IFD::IFD(std::shared_ptr<TIFF>& tiff,
               offset_type            offset):
        enable_shared_from_this(),
        impl(std::make_unique<Impl>(tiff, offset))
      {
      }

      IFD::IFD(std::shared_ptr<TIFF>& tiff):
        enable_shared_from_this(),
        impl(std::make_unique<Impl>(tiff, 0))
      {
      }

      IFD::IFD(const IFD& copy):
        enable_shared_from_this(),
        impl(std::make_unique<Impl>(*(copy.impl)))
      {
      }

      IFD::~IFD()
      {
      }

      IFD&
      IFD::operator= (const IFD& rhs)
      {
        *impl = *(rhs.impl);
        return *this;
      }

      std::shared_ptr<IFD>
      IFD::openIndex(std::shared_ptr<TIFF>& tiff,
                     directory_index_type   index)
      {
        ::TIFF *tiffraw = reinterpret_cast<::TIFF *>(tiff->getWrapped());

        Sentry sentry;

        if (!TIFFSetDirectory(tiffraw, index))
          sentry.error();

        return openOffset(tiff, static_cast<uint64_t>(TIFFCurrentDirOffset(tiffraw)));
      }

      std::shared_ptr<IFD>
      IFD::openOffset(std::shared_ptr<TIFF>& tiff,
                      offset_type            offset)
      {
        return std::make_shared<IFDConcrete>(tiff, offset);
      }

      std::shared_ptr<IFD>
      IFD::current(std::shared_ptr<TIFF>& tiff)
      {
        return std::make_shared<IFDConcrete>(tiff);
      }

      void
      IFD::makeCurrent() const
      {
        std::shared_ptr<TIFF>& tiff = getTIFF();
        ::TIFF *tiffraw = reinterpret_cast<::TIFF *>(tiff->getWrapped());

        Sentry sentry;

        if (static_cast<offset_type>(TIFFCurrentDirOffset(tiffraw)) != impl->offset)
          {
            if (!TIFFSetSubDirectory(tiffraw, impl->offset))
              sentry.error();
          }
      }

      std::shared_ptr<TIFF>&
      IFD::getTIFF() const
      {
        return impl->tiff;
      }

      offset_type
      IFD::getOffset() const
      {
        return impl->offset;
      }

      void
      IFD::getRawField(tag_type tag,
                       ...) const
      {
        std::shared_ptr<TIFF>& tiff = getTIFF();
        ::TIFF *tiffraw = reinterpret_cast<::TIFF *>(tiff->getWrapped());

        Sentry sentry;

        makeCurrent();

        va_list ap;
        va_start(ap, tag);

        if (!tag)
          {
            boost::format fmt("Error getting field: Tag %1% is not valid");
            fmt % tag;
            throw Exception(fmt.str());
          }

        if (!TIFFVGetField(tiffraw, tag, ap))
          {
            boost::format fmt("Error getting field: Tag %1% was not found");
            fmt % tag;
            sentry.error();
          }
      }

      void
      IFD::getRawFieldDefaulted(tag_type tag,
                                ...) const
      {
        std::shared_ptr<TIFF>& tiff = getTIFF();
        ::TIFF *tiffraw = reinterpret_cast<::TIFF *>(tiff->getWrapped());

        Sentry sentry;

        makeCurrent();

        va_list ap;
        va_start(ap, tag);

        if (!tag)
          {
            boost::format fmt("Error getting field: Tag %1% is not valid");
            fmt % tag;
            throw Exception(fmt.str());
          }

        if (!TIFFVGetFieldDefaulted(tiffraw, tag, ap))
          {
            boost::format fmt("Error getting field: Tag %1% was not found");
            fmt % tag;
            sentry.error();
          }
      }

      void
      IFD::setRawField(tag_type tag,
                       ...)
      {
        std::shared_ptr<TIFF>& tiff = getTIFF();
        ::TIFF *tiffraw = reinterpret_cast<::TIFF *>(tiff->getWrapped());

        Sentry sentry;

        makeCurrent();

        va_list ap;
        va_start(ap, tag);

        if (!tag)
          {
            boost::format fmt("Error getting field: Tag %1% is not valid");
            fmt % tag;
            throw Exception(fmt.str());
          }

        if (!TIFFVSetField(tiffraw, tag, ap))
          sentry.error();
      }

      TileType
      IFD::getTileType() const
      {
        if (!impl->tiletype)
          {
            uint32_t w, h;
            try
              {
                getField(TILEWIDTH).get(w);
                getField(TILELENGTH).get(h);
                impl->tiletype = TILE;
              }
            catch (const Exception&)
              {
                getField(ROWSPERSTRIP).get(h);
                impl->tiletype = STRIP;
              }
          }

        return impl->tiletype.get();
      }

      void
      IFD::setTileType(TileType type)
      {
        impl->tiletype = type;
      }

      TileInfo
      IFD::getTileInfo()
      {
        return TileInfo(this->shared_from_this());
      }

      const TileInfo
      IFD::getTileInfo() const
      {
        return TileInfo(const_cast<IFD *>(this)->shared_from_this());
      }

      std::vector<TileCoverage>&
      IFD::getTileCoverage()
      {
        return *(impl->coverage);
      }

      const std::vector<TileCoverage>&
      IFD::getTileCoverage() const
      {
        return *(impl->coverage);
      }

      uint32_t
      IFD::getImageWidth() const
      {
        if (!impl->imagewidth)
          {
            uint32_t width;
            getField(IMAGEWIDTH).get(width);
            impl->imagewidth = width;
          }
        return impl->imagewidth.get();
      }

      void
      IFD::setImageWidth(uint32_t width)
      {
        getField(IMAGEWIDTH).set(width);
        impl->imagewidth = width;
      }

      uint32_t
      IFD::getImageHeight() const
      {
        if (!impl->imageheight)
          {
            uint32_t height;
            getField(IMAGELENGTH).get(height);
            impl->imageheight = height;
          }
        return impl->imageheight.get();
      }

      void
      IFD::setImageHeight(uint32_t height)
      {
        getField(IMAGELENGTH).set(height);
        impl->imageheight = height;
      }

      uint32_t
      IFD::getTileWidth() const
      {
        if (!impl->tilewidth)
          {
            if (getTileType() == TILE)
              {
                uint32_t width;
                getField(TILEWIDTH).get(width);
                impl->tilewidth = width;
              }
            else // strip
              {
                impl->tilewidth = getImageWidth();
              }
          }
        return impl->tilewidth.get();
      }

      void
      IFD::setTileWidth(uint32_t width)
      {
        if (getTileType() == TILE)
          {
            getField(TILEWIDTH).set(width);
            impl->tilewidth = width;
          }
        else
          {
            // Do nothing for strips.
          }
      }

      uint32_t
      IFD::getTileHeight() const
      {
        if (!impl->tileheight)
          {
            if (getTileType() == TILE)
              {
                uint32_t height;
                getField(TILELENGTH).get(height);
                impl->tileheight = height;
              }
            else
              {
                uint32_t rows;
                getField(ROWSPERSTRIP).get(rows);
                impl->tileheight = rows;
              }
          }
        return impl->tileheight.get();
      }

      void
      IFD::setTileHeight(uint32_t height)
      {
        if (getTileType() == TILE)
          {
            getField(TILELENGTH).set(height);
          }
        else // strip
          {
            getField(ROWSPERSTRIP).set(height);
          }
        impl->tileheight = height;
      }

      ::ome::xml::model::enums::PixelType
      IFD::getPixelType() const
      {
        PixelType pt = PixelType::UINT8;

        if (impl->pixeltype)
          {
            pt = impl->pixeltype.get();
          }
        else
          {
            SampleFormat sampleformat;
            try
              {
                getField(SAMPLEFORMAT).get(sampleformat);
              }
            catch(const Exception&)
              {
                // Default to unsigned integer.
                sampleformat = UNSIGNED_INT;
              }

            uint16_t bits = getBitsPerSample();

            switch(sampleformat)
              {
              case UNSIGNED_INT:
                {
                  if (bits == 1)
                    pt = PixelType::BIT;
                  else if (bits == 8)
                    pt = PixelType::UINT8;
                  else if (bits == 16)
                    pt = PixelType::UINT16;
                  else if (bits == 32)
                    pt = PixelType::UINT32;
                  else
                    {
                      boost::format fmt("Bit depth %1% unsupported for unsigned integer pixel type");
                      fmt % bits;
                      throw Exception(fmt.str());
                    }
                }
                break;
              case SIGNED_INT:
                {
                  if (bits == 8)
                    pt = PixelType::INT8;
                  else if (bits == 16)
                    pt = PixelType::INT16;
                  else if (bits == 32)
                pt = PixelType::INT32;
                  else
                    {
                      boost::format fmt("Bit depth %1% unsupported for signed integer pixel type");
                      fmt % bits;
                      throw Exception(fmt.str());
                    }
                }
                break;
              case FLOAT:
                {
                  if (bits == 32)
                    pt = PixelType::FLOAT;
                  else if (bits == 64)
                    pt = PixelType::DOUBLE;
                  else
                    {
                      boost::format fmt("Bit depth %1% unsupported for floating point pixel type");
                      fmt % bits;
                      throw Exception(fmt.str());
                    }
                }
                break;
              case COMPLEX_FLOAT:
                {
                  if (bits == 64)
                    pt = PixelType::COMPLEXFLOAT;
                  else if (bits == 128)
                    pt = PixelType::COMPLEXDOUBLE;
                  else
                    {
                      boost::format fmt("Bit depth %1% unsupported for complex floating point pixel type");
                      fmt % bits;
                      throw Exception(fmt.str());
                    }
                }
                break;
              default:
                {
                  boost::format fmt("TIFF SampleFormat %1% unsupported by OME data model PixelType");
                  fmt % sampleformat;
                  throw Exception(fmt.str());
                }
                break;
              }
          }
        return pt;
      }

      void
      IFD::setPixelType(::ome::xml::model::enums::PixelType type)
      {
        SampleFormat fmt = UNSIGNED_INT;

        switch(type)
          {
          case PixelType::BIT:
          case PixelType::UINT8:
          case PixelType::UINT16:
          case PixelType::UINT32:
            fmt = UNSIGNED_INT;
            break;

          case PixelType::INT8:
          case PixelType::INT16:
          case PixelType::INT32:
            fmt = SIGNED_INT;
            break;

          case PixelType::FLOAT:
          case PixelType::DOUBLE:
            fmt = FLOAT;
            break;

          case PixelType::COMPLEXFLOAT:
          case PixelType::COMPLEXDOUBLE:
            fmt = COMPLEX_FLOAT;
            break;

          default:
            {
              boost::format fmt("Unsupported OME data model PixelType %1%");
              fmt % type;
              throw Exception(fmt.str());
            }
            break;
          }

        getField(SAMPLEFORMAT).set(fmt);
        impl->pixeltype = type;
      }

      uint16_t
      IFD::getBitsPerSample() const
      {
        if (!impl->bits)
          {
            uint16_t bits;
            getField(BITSPERSAMPLE).get(bits);
            impl->bits = bits;
          }
        return impl->bits.get();
      }

      void
      IFD::setBitsPerSample(uint16_t bits)
      {
        uint16 max_bits = significantBitsPerPixel(getPixelType());
        if (bits > max_bits)
          bits = max_bits;

        getField(BITSPERSAMPLE).set(bits);
        impl->bits = bits;
      }

      uint16_t
      IFD::getSamplesPerPixel() const
      {
        if (!impl->samples)
          {
            uint16_t samples;
            getField(SAMPLESPERPIXEL).get(samples);
            impl->samples = samples;
          }
        return impl->samples.get();
      }

      void
      IFD::setSamplesPerPixel(uint16_t samples)
      {
        getField(SAMPLESPERPIXEL).set(samples);
        impl->samples = samples;
      }

      PlanarConfiguration
      IFD::getPlanarConfiguration() const
      {
        if (!impl->planarconfig)
          {
            PlanarConfiguration config;
            getField(PLANARCONFIG).get(config);
            impl->planarconfig = config;
          }
        return impl->planarconfig.get();
      }

      void
      IFD::setPlanarConfiguration(PlanarConfiguration planarconfig)
      {
        getField(PLANARCONFIG).set(planarconfig);
        impl->planarconfig = planarconfig;
      }

      PhotometricInterpretation
      IFD::getPhotometricInterpretation() const
      {
        if (!impl->photometric)
          {
            PhotometricInterpretation photometric;
            getField(PHOTOMETRIC).get(photometric);
            impl->photometric = photometric;
          }
        return impl->photometric.get();
      }

      void
      IFD::setPhotometricInterpretation(PhotometricInterpretation photometric)
      {
        getField(PHOTOMETRIC).set(photometric);
        impl->photometric = photometric;
      }

      Compression
      IFD::getCompression() const
      {
        if (!impl->compression)
          {
            Compression compression;
            getField(COMPRESSION).get(compression);
            impl->compression = compression;
          }
        return impl->compression.get();
      }

      void
      IFD::setCompression(Compression compression)
      {
        getField(COMPRESSION).set(compression);
        impl->compression = compression;
      }

      uint16_t
      IFD::getSubIFDCount() const
      {
        return getSubIFDOffsets().size();
      }

      std::vector<uint64_t>
      IFD::getSubIFDOffsets() const
      {
        if (!impl->subifds)
          {
            std::vector<uint64_t> subifds;
            getField(SUBIFD).get(subifds);
            impl->subifds = subifds;
          }
        return impl->subifds.get();
      }

      void
      IFD::setSubIFDCount(uint16_t size)
      {
        std::vector<uint64_t> subifds(size, 0U);
        setSubIFDOffsets(subifds);
      }


      void
      IFD::setSubIFDOffsets(const std::vector<uint64_t>& subifds)
      {
        getField(SUBIFD).set(subifds);
        impl->subifds = subifds;
      }

      void
      IFD::readImage(VariantPixelBuffer& buf) const
      {
        readImage(buf, 0, 0, getImageWidth(), getImageHeight());
      }

      void
      IFD::readImage(VariantPixelBuffer& buf,
                     dimension_size_type sample) const
      {
        readImage(buf, 0, 0, getImageWidth(), getImageHeight(), sample);
      }

      void
      IFD::readImage(VariantPixelBuffer& dest,
                     dimension_size_type x,
                     dimension_size_type y,
                     dimension_size_type w,
                     dimension_size_type h) const
      {
        PixelType type = getPixelType();
        PlanarConfiguration planarconfig = getPlanarConfiguration();
        uint16_t sample = getSamplesPerPixel();

        std::array<VariantPixelBuffer::size_type, PixelBufferBase::dimensions> shape, dest_shape;
        shape = {w, h, 1, sample};

        const VariantPixelBuffer::size_type *dest_shape_ptr(dest.shape());
        std::copy(dest_shape_ptr, dest_shape_ptr + PixelBufferBase::dimensions,
                  dest_shape.begin());

        PixelBufferBase::storage_order_type order(PixelBufferBase::make_storage_order(planarconfig == SEPARATE ? false : true));

        if (type != dest.pixelType() ||
            shape != dest_shape ||
            !(order == dest.storage_order()))
          dest.setBuffer(shape, type, order);

        TileInfo info = getTileInfo();

        PlaneRegion region(x, y, w, h);
        std::vector<dimension_size_type> tiles(info.tileCoverage(region));

        ReadVisitor v(*this, info, region, tiles);
        ome::compat::visit(v, dest.vbuffer());
      }

      void
      IFD::readImage(VariantPixelBuffer& dest,
                     dimension_size_type x,
                     dimension_size_type y,
                     dimension_size_type w,
                     dimension_size_type h,
                     dimension_size_type sample) const
      {
        // Copy the desired sample into the destination buffer.
        VariantPixelBuffer tmp;
        readImage(tmp, x, y, w, h);

        detail::CopySampleVisitor v(dest, sample);
        ome::compat::visit(v, tmp.vbuffer());
      }

      void
      IFD::readLookupTable(VariantPixelBuffer& buf) const
      {
        std::array<std::vector<uint16_t>, 3> cmap;
        getField(tiff::COLORMAP).get(cmap);

        std::array<VariantPixelBuffer::size_type, PixelBufferBase::dimensions>
          shape = {cmap.at(0).size(), 1, 1, cmap.size()};

        ::ome::files::PixelBufferBase::storage_order_type order_planar(::ome::files::PixelBufferBase::make_storage_order(false));

        buf.setBuffer(shape, PixelType::UINT16, order_planar);

        std::shared_ptr<PixelBuffer<PixelProperties<PixelType::UINT16>::std_type>> uint16_buffer
          (ome::compat::get<std::shared_ptr<PixelBuffer<PixelProperties<PixelType::UINT16>::std_type>>>(buf.vbuffer()));
        assert(uint16_buffer);

        for (VariantPixelBuffer::size_type s = 0U; s < shape[DIM_SAMPLE]; ++s)
          {
            const std::vector<uint16_t>& channel(cmap.at(s));

            VariantPixelBuffer::indices_type coord = {0U, 0U, 0U,
                                                      static_cast<index_type>(s)};

            std::copy(channel.begin(), channel.end(),
                      &uint16_buffer->at(coord));
          }

      }

      void
      IFD::writeImage(const VariantPixelBuffer& buf,
                      dimension_size_type       sample)
      {
        writeImage(buf, 0, 0, getImageWidth(), getImageHeight(), sample);
      }

      void
      IFD::writeImage(const VariantPixelBuffer& buf)
      {
        writeImage(buf, 0, 0, getImageWidth(), getImageHeight());
      }

      void
      IFD::writeImage(const VariantPixelBuffer& source,
                      dimension_size_type       x,
                      dimension_size_type       y,
                      dimension_size_type       w,
                      dimension_size_type       h)
      {
        PixelType type = getPixelType();
        PlanarConfiguration planarconfig = getPlanarConfiguration();
        uint16_t sample = getSamplesPerPixel();

        std::array<VariantPixelBuffer::size_type, PixelBufferBase::dimensions> shape, source_shape;
        shape = {w, h, 1, sample};

        const VariantPixelBuffer::size_type *source_shape_ptr(source.shape());
        std::copy(source_shape_ptr, source_shape_ptr + PixelBufferBase::dimensions,
                  source_shape.begin());

        PixelBufferBase::storage_order_type order(PixelBufferBase::make_storage_order(planarconfig == SEPARATE ? false : true));
        PixelBufferBase::storage_order_type source_order(source.storage_order());

        if (type != source.pixelType())
          {
            boost::format fmt("VariantPixelBuffer %1% pixel type is incompatible with TIFF %2% sample format and bit depth");
            fmt % source.pixelType() % type;
            throw Exception(fmt.str());
          }

        if (shape != source_shape)
          {
            if (shape[DIM_SPATIAL_X] != source_shape[DIM_SPATIAL_X] ||
                shape[DIM_SPATIAL_Y] != source_shape[DIM_SPATIAL_Y] ||
                shape[DIM_SAMPLE] != source_shape[DIM_SAMPLE])
              {
                boost::format fmt("VariantPixelBuffer dimensions (%1%×%2%, %3% samples) incompatible with TIFF image size (%4%×%5%, %6% samples)");
                fmt % source_shape[DIM_SPATIAL_X] % source_shape[DIM_SPATIAL_Y] % source_shape[DIM_SAMPLE];
                fmt % shape[DIM_SPATIAL_X] % shape[DIM_SPATIAL_Y] % shape[DIM_SAMPLE];
                throw Exception(fmt.str());
              }
            else
              {
                boost::format fmt("VariantPixelBuffer dimensions (%1%×%2%×%3%, %4% samples) incompatible with TIFF image size (%5%×%6%, %7% samples)");
                fmt % source_shape[DIM_SPATIAL_X] % source_shape[DIM_SPATIAL_Y] % source_shape[DIM_SPATIAL_Z];
                fmt % source_shape[DIM_SAMPLE];
                fmt % shape[DIM_SPATIAL_X] % shape[DIM_SPATIAL_Y] % shape[DIM_SAMPLE];
                throw Exception(fmt.str());
              }
          }

        if (!(order == source_order))
          {
            boost::format fmt("VariantPixelBuffer storage order (%1%%2%%3%%4%) incompatible with %5% TIFF planar configuration (%6%%7%%8%%9%)");
            fmt % source_order.ordering(0) % source_order.ordering(1) % source_order.ordering(2);
            fmt % source_order.ordering(3);
            fmt % (planarconfig == SEPARATE ? "separate" : "contiguous");
            fmt % order.ordering(0) % order.ordering(1) % order.ordering(2);
            fmt % order.ordering(3);
            throw Exception(fmt.str());
          }

        TileInfo info = getTileInfo();

        PlaneRegion region(x, y, w, h);
        std::vector<dimension_size_type> tiles(info.tileCoverage(region));

        WriteVisitor v(*this, *(impl->coverage), *(impl->tilecache), info, region, tiles);
        ome::compat::visit(v, source.vbuffer());
      }

      void
      IFD::writeImage(const VariantPixelBuffer& /* source*/,
                      dimension_size_type       /* x*/,
                      dimension_size_type       /* y*/,
                      dimension_size_type       /* w*/,
                      dimension_size_type       /* h*/,
                      dimension_size_type       /* sample */)
      {
        throw Exception("Writing samples separately is not yet implemented (requires TileCache and WriteVisitor to handle writing and caching of interleaved and non-interleaved samples; currently it handles writing all samples in one call only and can not combine separate samples from separate calls");
      }

      std::shared_ptr<IFD>
      IFD::next() const
      {
        std::shared_ptr<IFD> ret;

        std::shared_ptr<TIFF>& tiff = getTIFF();
        ::TIFF *tiffraw = reinterpret_cast<::TIFF *>(tiff->getWrapped());

        Sentry sentry;

        makeCurrent();

        if (TIFFReadDirectory(tiffraw) == 1)
          {
            uint64_t offset = static_cast<uint64_t>(TIFFCurrentDirOffset(tiffraw));
            ret = openOffset(tiff, offset);
          }

        return ret;
      }

      bool
      IFD::last() const
      {
        std::shared_ptr<TIFF>& tiff = getTIFF();
        ::TIFF *tiffraw = reinterpret_cast<::TIFF *>(tiff->getWrapped());

        Sentry sentry;

        makeCurrent();

        return TIFFLastDirectory(tiffraw) != 0;
      }

    }
  }
}
