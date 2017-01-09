/*
 * #%L
 * OME-FILES C++ library for image IO.
 * %%
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

#ifndef TEST_PIXEL_H
#define TEST_PIXEL_H

#include <ome/files/PixelBuffer.h>
#include <ome/files/PixelProperties.h>
#include <ome/files/VariantPixelBuffer.h>

#include <ome/compat/cstdint.h>

/// Helpers to create pixel values of all supported types from integers.

template<typename P>
inline
P
pixel_value(uint32_t value)
{
  return static_cast<P>(value);
}

template<typename C>
inline
C
pixel_value_complex(uint32_t value)
{
  return C(typename C::value_type(value),
           typename C::value_type(0.0f));
}

template<>
inline
::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXFLOAT,
                                          ::ome::files::ENDIAN_BIG>::type
pixel_value< ::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXFLOAT,
                                                       ::ome::files::ENDIAN_BIG>::type>(uint32_t value)
{
  return pixel_value_complex< ::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXFLOAT,
                                                                        ::ome::files::ENDIAN_BIG>::type>(value);
}

template<>
inline
::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXFLOAT,
                                          ::ome::files::ENDIAN_LITTLE>::type
pixel_value< ::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXFLOAT,
                                                       ::ome::files::ENDIAN_LITTLE>::type>(uint32_t value)
{
  return pixel_value_complex< ::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXFLOAT,
                                                                        ::ome::files::ENDIAN_LITTLE>::type>(value);
}

template<>
inline
::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXDOUBLE,
                                          ::ome::files::ENDIAN_BIG>::type
pixel_value< ::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXDOUBLE,
                                                       ::ome::files::ENDIAN_BIG>::type>(uint32_t value)
{
  return pixel_value_complex< ::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXDOUBLE,
                                                                        ::ome::files::ENDIAN_BIG>::type>(value);
}

template<>
inline
::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXDOUBLE,
                                          ::ome::files::ENDIAN_LITTLE>::type
pixel_value< ::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXDOUBLE,
                                                       ::ome::files::ENDIAN_LITTLE>::type>(uint32_t value)
{
  return pixel_value_complex< ::ome::files::PixelEndianProperties< ::ome::xml::model::enums::PixelType::COMPLEXDOUBLE,
                                                                        ::ome::files::ENDIAN_LITTLE>::type>(value);
}

/*
 * Assign buffer with values from a buffer of a different pixel type
 * and check.  This is for testing loading and saving of all pixel
 * types.
 */
template<int P>
struct PixelTypeConversionVisitor : public boost::static_visitor<>
{
  typedef typename ::ome::files::PixelProperties<P>::std_type src_type;
  typedef ::ome::files::PixelProperties< ::ome::xml::model::enums::PixelType::BIT>::std_type bit_type;

  const std::shared_ptr< ::ome::files::PixelBuffer<src_type>> *src;
  ::ome::files::VariantPixelBuffer& dest;

  PixelTypeConversionVisitor(const ::ome::files::VariantPixelBuffer& src,
                             ::ome::files::VariantPixelBuffer& dest):
    src(boost::get<std::shared_ptr< ::ome::files::PixelBuffer<src_type>>>(&src.vbuffer())),
    dest(dest)
  {

    if (!(this->src && *this->src))
      throw std::runtime_error("Null source buffer or incorrect pixel type");

    if((*this->src)->num_elements() != dest.num_elements())
      throw std::runtime_error("Array size mismatch");
  }

  // Expand pixel values to fill the positive pixel value range for
  // the destination pixel type.
  template <typename T>
  typename boost::enable_if_c<
    boost::is_integral<T>::value, void
    >::type
  operator() (std::shared_ptr< ::ome::files::PixelBuffer<T>>& lhs)
  {
    const src_type *src_buf = (*src)->data();
    T *dest_buf = lhs->data();

    float oldmin = static_cast<float>(std::numeric_limits<src_type>::min());
    float oldmax = static_cast<float>(std::numeric_limits<src_type>::max());
    float newmin = static_cast<float>(std::numeric_limits<T>::min());
    float newmax = static_cast<float>(std::numeric_limits<T>::max());

    for (::ome::files::VariantPixelBuffer::size_type i = 0;
         i != (*src)->num_elements();
         ++i)
      {

        dest_buf[i] = static_cast<T>((static_cast<float>(src_buf[i] - oldmin) *
                                      ((newmax - newmin) / (oldmax - oldmin))) + newmin);
      }
  }

  // Normalise pixel values to fill the pixel value range 0..1 for the
  // destination pixel type.
  template <typename T>
  typename boost::enable_if_c<
    boost::is_floating_point<T>::value, void
    >::type
  operator() (std::shared_ptr< ::ome::files::PixelBuffer<T>>& lhs)
  {
    const src_type *src_buf = (*src)->data();
    T *dest_buf = lhs->data();

    float oldmin = static_cast<float>(std::numeric_limits<src_type>::min());
    float oldmax = static_cast<float>(std::numeric_limits<src_type>::max());
    float newmin = 0.0f;
    float newmax = 1.0f;

    for (::ome::files::VariantPixelBuffer::size_type i = 0;
         i != (*src)->num_elements();
         ++i)
      {
        dest_buf[i] = static_cast<T>((static_cast<float>(src_buf[i] - oldmin) *
                                      ((newmax - newmin) / (oldmax - oldmin))) + newmin);
      }
  }

  // Normalise pixel values to fill the pixel value range 0..1+0.0i for the
  // destination complex pixel type.
  template <typename T>
  typename boost::enable_if_c<
    boost::is_complex<T>::value, void
    >::type
  operator() (std::shared_ptr< ::ome::files::PixelBuffer<T>>& lhs)
  {
    const src_type *src_buf = (*src)->data();
    T *dest_buf = lhs->data();

    float oldmin = static_cast<float>(std::numeric_limits<src_type>::min());
    float oldmax = static_cast<float>(std::numeric_limits<src_type>::max());
    float newmin = 0.0f;
    float newmax = 1.0f;

    for (::ome::files::VariantPixelBuffer::size_type i = 0;
         i != (*src)->num_elements();
         ++i)
      {
        dest_buf[i] = T((static_cast<typename T::value_type>((src_buf[i] - oldmin) *
                                                             ((newmax - newmin) / (oldmax - oldmin))) + newmin),
                        0.0f);
      }
  }

  // Split the pixel range into two, the lower part being set to false
  // and the upper part being set to true for the destination boolean
  // pixel type.
  void
  operator() (std::shared_ptr< ::ome::files::PixelBuffer<bit_type>>& lhs)
  {
    const src_type *src_buf = (*src)->data();
    bit_type *dest_buf = lhs->data();

    for (::ome::files::VariantPixelBuffer::size_type i = 0;
         i != (*src)->num_elements();
         ++i)
      {
        dest_buf[i] = (static_cast<float>(src_buf[i] - std::numeric_limits<src_type>::min()) /
                       static_cast<float>(std::numeric_limits<src_type>::max())) < 0.3 ? false : true;
      }
  }
};

struct PixelSubrangeVisitor : public boost::static_visitor<>
{
  ::ome::files::dimension_size_type x;
  ::ome::files::dimension_size_type y;

  PixelSubrangeVisitor(::ome::files::dimension_size_type x,
                       ::ome::files::dimension_size_type y):
    x(x),
    y(y)
  {}

  template<typename T, typename U>
  void
  operator() (const T& /*src*/,
              U& /* dest */) const
  {}

  template<typename T>
  void
  operator() (const T& src,
              T& dest) const
  {
    const ::ome::files::VariantPixelBuffer::size_type *shape = dest->shape();

    ::ome::files::dimension_size_type width = shape[::ome::files::DIM_SPATIAL_X];
    ::ome::files::dimension_size_type height = shape[::ome::files::DIM_SPATIAL_Y];
    ::ome::files::dimension_size_type subchannels = shape[::ome::files::DIM_SUBCHANNEL];

    for (::ome::files::dimension_size_type dx = 0; dx < width; ++dx)
      for (::ome::files::dimension_size_type dy = 0; dy < height; ++dy)
        for (::ome::files::dimension_size_type ds = 0; ds < subchannels; ++ds)
          {
            ::ome::files::VariantPixelBuffer::indices_type srcidx;
            srcidx[::ome::files::DIM_SPATIAL_X] = x + dx;
            srcidx[::ome::files::DIM_SPATIAL_Y] = y + dy;
            srcidx[::ome::files::DIM_SUBCHANNEL] = ds;
            srcidx[2] = srcidx[3] = srcidx[4] = srcidx[6] = srcidx[7] = srcidx[8] = 0;

            ::ome::files::VariantPixelBuffer::indices_type destidx;
            destidx[::ome::files::DIM_SPATIAL_X] = dx;
            destidx[::ome::files::DIM_SPATIAL_Y] = dy;
            destidx[::ome::files::DIM_SUBCHANNEL] = ds;
            destidx[2] = destidx[3] = destidx[4] = destidx[6] = destidx[7] = destidx[8] = 0;

            dest->at(destidx) = src->at(srcidx);
          }
  }
};

namespace std
{
  template<class charT, class traits>
  inline std::basic_ostream<charT,traits>&
  operator<< (std::basic_ostream<charT,traits>& os,
              const ::ome::files::PixelBufferBase::storage_order_type& order)
  {
    os << '(';
    for (uint16_t i = 0; i < ::ome::files::PixelBufferBase::dimensions; ++i)
    {
      os << order.ordering(i) << '/' << order.ascending(i);
      if (i + 1 != ::ome::files::PixelBufferBase::dimensions)
        os << ',';
    }
    os << ')';
    return os;
  }
}

#endif // TEST_PIXEL_H

/*
 * Local Variables:
 * mode:C++
 * End:
 */
