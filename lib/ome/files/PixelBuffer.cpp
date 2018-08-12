/*
 * #%L
 * OME-FILES C++ library for image IO.
 * Copyright © 2006 - 2015 Open Microscopy Environment:
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

#include <ome/files/PixelBuffer.h>

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

    constexpr uint16_t PixelBufferBase::dimensions;

    PixelBufferBase::storage_order_type
    PixelBufferBase::make_storage_order(bool interleaved)
    {
      size_type ordering[dimensions];
      bool ascending[dimensions] = {true, true, true, true};

      if (interleaved)
        {
          ordering[0] = DIM_SAMPLE;
          ordering[1] = DIM_SPATIAL_X;
          ordering[2] = DIM_SPATIAL_Y;
          ordering[3] = DIM_SPATIAL_Z;
        }
      else
        {
          ordering[0] = DIM_SPATIAL_X;
          ordering[1] = DIM_SPATIAL_Y;
          ordering[2] = DIM_SPATIAL_Z;
          ordering[3] = DIM_SAMPLE;
        }

      return storage_order_type(ordering, ascending);
    }

    PixelBufferBase::storage_order_type
    PixelBufferBase::make_storage_order(ome::xml::model::enums::DimensionOrder /* order */,
                                        bool                                   interleaved)
    {
      return make_storage_order(interleaved);
    }

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

    PixelBufferBase::storage_order_type
    PixelBufferBase::default_storage_order()
    {
      return make_storage_order(true);
    }

  }
}
