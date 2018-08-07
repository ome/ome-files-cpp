/*
 * #%L
 * OME-FILES C++ library for image IO.
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

#include <algorithm>
#include <complex>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include <ome/files/PixelBuffer.h>
#include <ome/files/PixelProperties.h>
#include <ome/files/VariantPixelBuffer.h>

#include "fractal.h"

using std::make_shared;
using std::shared_ptr;
using ome::files::dimension_size_type;
using ome::files::DIM_SPATIAL_X;
using ome::files::DIM_SPATIAL_Y;
using ome::files::DIM_SAMPLE;
using ome::files::FormatWriter;
using ome::files::PixelBuffer;
using ome::files::PixelBufferBase;
using ome::files::PixelProperties;
using ome::files::VariantPixelBuffer;
using ome::xml::model::enums::DimensionOrder;
using ome::xml::model::enums::PixelType;

namespace
{

  // Convenience types.
  using coord = std::array<dimension_size_type, 2>;
  using dcoord = std::array<double, 2>;
  using range = std::array<dimension_size_type, 2>;
  using drange = std::array<double, 2>;
  using area = std::array<range, 2>;
  using darea = std::array<drange, 2>;

  /*
   * Set an x/y pixel value in the pixel buffer to the specified RGB
   * value.
   */
  inline
  void
  set_pixel(PixelBuffer<PixelProperties<PixelType::UINT8>::std_type>& buffer,
            dimension_size_type                                       x,
            dimension_size_type                                       y,
            std::array<uint8_t, 3>                                    value)
  {
    PixelBufferBase::indices_type idx;
    std::fill(idx.begin(), idx.end(), 0);
    idx[DIM_SPATIAL_X] = x;
    idx[DIM_SPATIAL_Y] = y;

    for (dimension_size_type i = 0; i < 3; ++i)
      {
        idx[DIM_SAMPLE] = i;
        buffer.array()(idx) = value[i];
      }
  }

  /*
   * Polynomial B G R colour scheme.
   */
  std::array<uint8_t, 3>
  lookup_colour(int iterations,
                int iter_max)
  {
    std::array<uint8_t, 3> col{0U, 0U, 0U};

    // map n on the 0..1 interval
    double t = static_cast<double>(iterations) / static_cast<double>(iter_max);

    // Use smooth polynomials for R, G, B
    col[0] = static_cast<uint8_t>(9*(1-t)*t*t*t*255);
    col[1] = static_cast<uint8_t>(15*(1-t)*(1-t)*t*t*255);
    col[2] = static_cast<uint8_t>(8.5*(1-t)*(1-t)*(1-t)*t*255);

    return col;
  }

  /*
   * Compute a lookup table of the specified size.
   */
  std::vector<std::array<uint8_t, 3>>
  create_lut(int size)
  {
    std::vector<std::array<uint8_t, 3>> lut(size);

    for (decltype(lut.size()) i = 0; i < lut.size(); ++i)
      {
        lut[i] = lookup_colour(i, size);
      }

    return lut;
  }

  /*
   * Compute the mean RGB values of the specified list of RGB samples.
   */
  template<long unsigned int S>
  std::array<uint8_t, 3>
  blend_samples(const std::array<std::array<uint8_t, 3>, S>& samples)
  {
    std::array<uint16_t, 3> accum {0U, 0U, 0U};
    std::array<uint8_t, 3> ret {0U, 0U, 0U};

    for (int i = 0; i < 3; ++i)
      {
        for (long unsigned int j = 0; j < S; ++j)
          {
            accum[i] += samples[j][i];
          }
        ret[i] = static_cast<uint8_t>(accum[i] / S);
        if (accum[i] % S >= (S/2)) // round to nearest
          ++ret[i];
      }

    return ret;
  }

  /*
   * Scale a floating point value in range1 into the equivalent
   * position in floating point range2
   */
  double
  scale (double val,
         drange range1,
         drange range2)
  {
    // Normalised value
    double nval = (val - range1[0]) / (range1[1] - range1[0]);
    // Rescaled value
    double scaleval = (nval * (range2[1] - range2[0])) + range2[0];

    return scaleval;
  }

  /*
   * Scale an integer value in range1 into the equivalent position in
   * floating point range2.
   */
  double
  scale (dimension_size_type val,
         range               range1,
         drange              range2)
  {
    return scale(static_cast<double>(val),
                 {static_cast<double>(range1[0]), static_cast<double>(range1[1])},
                 range2);
  }

  /*
   * Convert a pixel coordinate to a complex number, scaled by the tile area.
   *
   * dcoord - x and y values
   * tilearea - the pixel area to fill
   * fractarea - the fractal area to visit (to map onto the tile area)
   */
  std::complex<double> make_c(const dcoord& coord,
                              const darea&  tilearea,
                              const darea&  fractarea) {
    // x and y are rescaled as real and imaginary components
    return std::complex<double>(scale(coord[0], tilearea[0], fractarea[0]),
                                scale(coord[1], tilearea[1], fractarea[1]));
  }

  /*
   * Render loop.  Step over every pixel in the tile area and compute
   * the RGB value for the fractal function.  Multisample each pixel
   * using a 16× sampling in x and y and computing the mean value.
   *
   * buffer - the buffer to store the fractal
   * tilearea - the pixel area to fill
   * fractarea - the fractal area to visit (to map onto the tile area)
   * func - the fractal function to use
   * escape - the escape function to use
   * iter_max - the maximum number of iterations
   * lut - the RGB lookup table
   */
  void
  render_fractal(PixelBuffer<PixelProperties<PixelType::UINT8>::std_type>& buffer,
                 const area& tilearea,
                 const darea& fractarea,
                 const std::function<std::complex<double>(std::complex<double>, std::complex<double>)> &func,
                 const std::function<int(std::complex<double>, const std::function<std::complex<double>(std::complex<double>, std::complex<double>)> &, int)> &escape,
                 int iter_max,
                 const std::vector<std::array<uint8_t, 3>>& lut)
  {
    // 16× multisample offsets
    constexpr std::array<std::array<double, 2>, 16> ms_offsets =
      {{
          {+1.0/16.0, +1.0/16.0},
          {-1.0/16.0, -3.0/16.0},
          {-3.0/16.0, +2.0/16.0},
          {+4.0/16.0, -1.0/16.0},
          {-5.0/16.0, -2.0/16.0},
          {+2.0/16.0, +5.0/16.0},
          {+5.0/16.0, +3.0/16.0},
          {+3.0/16.0, -5.0/16.0},
          {-2.0/16.0, +6.0/16.0},
          {+0.0/16.0, -7.0/16.0},
          {-4.0/16.0, -6.0/16.0},
          {-6.0/16.0, +4.0/16.0},
          {-8.0/16.0, +0.0/16.0},
          {+7.0/16.0, -4.0/16.0},
          {+6.0/16.0, +7.0/16.0},
          {-7.0/16.0, -8.0/16.0}
      }};

    // Tile area as double precision
    const darea dtilearea =
      {{
          {static_cast<double>(tilearea[0][0]),
           static_cast<double>(tilearea[0][1])},
          {static_cast<double>(tilearea[1][0]),
           static_cast<double>(tilearea[1][1])}
      }};

    for(dimension_size_type i = tilearea[1][0]; i < tilearea[1][1]; ++i)
      {
        for(dimension_size_type j = tilearea[0][0]; j < tilearea[0][1]; ++j)
          {
            // RGB values (16× multisample)
            std::array<std::array<uint8_t, 3>, ms_offsets.size()> samples;
            for (decltype(ms_offsets.size()) k = 0; k < ms_offsets.size(); ++k)
              {
                // Convert x (j) and y (i) values to a complex number,
                // scaled by the tile and fractal area, with
                // multisample offsets applied.
                auto c = make_c({static_cast<double>(j)+ms_offsets[k][0],
                                 static_cast<double>(i)+ms_offsets[k][1]},
                                dtilearea,
                                fractarea);
                // Compute the number of iterations with the escape
                // function and fractal function.
                auto iterations = escape(c, func, iter_max);
                assert(iterations < (iter_max + 1));

                // Look up RGB value.
                samples[k] = lut[iterations];
              }

            // Set pixel value by computing the mean of the 16 samples.
            set_pixel(buffer, j, i, blend_samples(samples));
          }
      }
  }

  /*
   * Generate fractal for single tile
   *
   * fractal - the type of fractal to generate
   * buffer - the buffer to store the fractal
   * tile_coord - the tile x and y coordinates
   * tile_size - the tile x and y sizes
   * image_size - the image x and y sizes
   * iter_max - the maximum number of iterations
   * lut - the RGB lookup table
   */
  void
  fill_fractal(FractalType fractal,
               PixelBuffer<PixelProperties<PixelType::UINT8>::std_type>& buffer,
               coord tile_coord,
               range tile_size,
               range image_size,
               int iter_max,
               const std::vector<std::array<uint8_t, 3>>& lut)
  {
    area tile_area =
      {{
          {0, tile_size[0]},
          {0, tile_size[1]}
      }};
    if (tile_coord[0] + tile_size[0] > image_size[0])
      tile_area[0][1] = image_size[0] - tile_coord[0];
    if (tile_coord[1] + tile_size[1] > image_size[1])
      tile_area[1][1] = image_size[1] - tile_coord[1];

    darea fract_area;

    auto func = [] (std::complex <double> z, std::complex<double> c) -> std::complex<double>
      { return (z * z) + c; };

    std::function<int(std::complex<double>, const std::function<std::complex<double>(std::complex<double>, std::complex<double>)> &, int)> escape;

    switch (fractal)
      {
      case FractalType::JULIA:
        fract_area =
          {{
            {-2.2, 2.2},
            {-2.2, 2.2}
          }};
        escape = [] (std::complex<double> c,
                     const std::function<std::complex<double>(std::complex<double>, std::complex<double>)> &func,
                     int iter_max) -> int
          {
            std::complex<double> z(c);
            std::complex<double> constant(-0.83, 0.2);
            int iter = 0;

            while (std::abs(z) < 2.0 && iter < iter_max) {
              z = func(z, constant);
              ++iter;
            }

            return iter;
          };
        break;
      case FractalType::MANDELBROT:
      default:
        fract_area =
          {{
            {-2.2, 1.2},
            {-1.7, 1.7}
          }};
        escape = [] (std::complex<double> c,
                     const std::function<std::complex<double>(std::complex<double>, std::complex<double>)> &func,
                     int iter_max) -> int
          {
            std::complex<double> z(0);
            int iter = 0;

            while (std::abs(z) < 2.0 && iter < iter_max) {
              z = func(z, c);
              ++iter;
            }

            return iter;
          };
        break;
      }

    const darea scaled_fract_area =
      {{
        {scale((tile_coord[0] * tile_size[0]),
          {0, image_size[0]}, fract_area[0]),
         scale((tile_coord[0] * tile_size[0]) + tile_area[0][1],
          {0, image_size[0]}, fract_area[0])},
        {scale((tile_coord[1] * tile_size[1]),
          {0, image_size[1]}, fract_area[1]),
         scale((tile_coord[1] * tile_size[1]) + tile_area[1][1],
          {0, image_size[1]}, fract_area[1])},
      }};

    render_fractal(buffer, tile_area, scaled_fract_area, func, escape, iter_max, lut);
  }

}

void
write_fractal(FormatWriter& writer,
              FractalType   fractal,
              std::ostream& stream)
{
  // Get dimension sizes and compute tile counts.
  dimension_size_type image_size_x = writer.getSizeX();
  dimension_size_type image_size_y = writer.getSizeY();
  dimension_size_type tile_size_x = writer.getTileSizeX();
  dimension_size_type tile_size_y = writer.getTileSizeY();

  dimension_size_type ntile_x = image_size_x / tile_size_x;
  if (image_size_x % tile_size_x)
    ++ntile_x;
  dimension_size_type ntile_y = image_size_y / tile_size_y;
  if (image_size_y % tile_size_y)
    ++ntile_y;

  // List of threads; sized by threads supported by hardware, but if
  // the tile count is less than this value, use the tile count
  // instead so we don't create threads with nothing to do.
  std::vector<std::thread> threads(std::min(static_cast<dimension_size_type>(std::thread::hardware_concurrency()),
                                            ntile_x * ntile_y));

  // Data for a single unit of work.
  struct work_item
  {
    coord tile_coord;
    range tile_size;
    range image_size;
  };

  // List of work units for each thread.
  std::vector<std::vector<work_item>> work(threads.size());
  // This mutex gates access to the writer object.
  std::mutex writer_lock;

  // Loop over tiles in this image.  Push tiles onto work list for
  // worker threads to pick up.  This should result in evenly
  // distributed amounts of work between threads, running in roughly
  // sequential tile order.
  dimension_size_type item = 0;
  for (dimension_size_type tile_x = 0; tile_x < ntile_x; ++tile_x)
    {
      for (dimension_size_type tile_y = 0; tile_y < ntile_y; ++tile_y, ++item)
        {
          work[item % work.size()].push_back
            ({{tile_x, tile_y},
                {tile_size_x, tile_size_y},
                  {image_size_x, image_size_y}});
        }
    }

  // Maximum number of iterations in fractal calculation.
  constexpr int iter_max = 255;
  // Lookup table for mapping iteration count to RGB values.
  auto lut=create_lut(iter_max + 1);

  stream << "Writing tiles (" << threads.size() << " threads): ";

  // Create the threads
  for (decltype(threads.size()) t = 0; t < threads.size(); ++t)
    {
      // The thread lambda function captures some of the local state,
      // and is passed an identifier as an index into the work list to
      // delegate the work items to this thread.
      auto func = [&fractal, &writer, &writer_lock, &work, &iter_max, &lut, &stream] (dimension_size_type id)
        {
          // Pixel buffer; sized from writer with 3 samples of type
          // uint8_t.  It uses the native endianness and has
          // interleaved storage (samples are chunky).
          auto buffer = make_shared<PixelBuffer<PixelProperties<PixelType::UINT8>::std_type>>
          (boost::extents[writer.getTileSizeX()][writer.getTileSizeY()][1][3],
           PixelType::UINT8, ome::files::ENDIAN_NATIVE,
           PixelBufferBase::make_storage_order(true));
          VariantPixelBuffer vbuffer(buffer);

          for (const auto& item : work.at(id))
            {
              // Fill each tile with a section of the specified fractal.
              fill_fractal(fractal, *buffer, item.tile_coord, item.tile_size, item.image_size, iter_max, lut);

              std::lock_guard<std::mutex> lock(writer_lock);
              // Write the the entire pixel buffer to the plane.
              writer.saveBytes(0U, vbuffer,
                               item.tile_coord[0] * item.tile_size[0],
                               item.tile_coord[1] * item.tile_size[1],
                               item.tile_size[0],
                               item.tile_size[1]);
              stream << '.' << std::flush;
            }
        };

      threads[t] = std::thread(func, t);
    }

  // Wait for completion.
  for (auto& t : threads)
    {
      t.join();
    }

  stream << '\n';
}
