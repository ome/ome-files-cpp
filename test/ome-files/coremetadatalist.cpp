/*
 * #%L
 * OME-FILES C++ library for image IO.
 * %%
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

#include <ome/files/Types.h>
#include <ome/files/MetadataList.h>
#include <ome/files/CoreMetadataList.h>

#include <iostream>

#include <ome/test/test.h>

using ome::files::dimension_size_type;
using ome::files::CoreMetadata;
using ome::files::CoreMetadataList;
using ome::files::MetadataList;
using ome::files::orderResolutions;

namespace
{

  // Create CoreMetadata from x,y,z extents.
  CoreMetadata
  CM(dimension_size_type x,
     dimension_size_type y,
     dimension_size_type z)
  {
    CoreMetadata c;
    c.sizeX = x;
    c.sizeY = y;
    c.sizeZ = z;
    return c;
  }

  // Compare CoreMetadata by x,y,z extents.
  bool
  compare(const CoreMetadata& lhs,
          const CoreMetadata& rhs)
  {
    return lhs.sizeX == rhs.sizeX &&
           lhs.sizeY == rhs.sizeY &&
           lhs.sizeZ == rhs.sizeZ;
  }

}

struct ListTestParameters
{
  // Core metadata to test.
  CoreMetadataList list;
  // Sorted order of subresolutions for each series.
  MetadataList<dimension_size_type> order;

  ListTestParameters(const CoreMetadataList& list,
                     const MetadataList<dimension_size_type>& order):
    list(list),
    order(order)
  {
  }
};

template<class charT, class traits>
inline std::basic_ostream<charT,traits>&
operator<< (std::basic_ostream<charT,traits>& os,
            const ListTestParameters& p)
{
  for (const auto& secondary : p.list)
    {
      os << "{ ";
      for (const auto& core : secondary)
        {
          os << core << ' ';
        }
      os << "} ";
    }
  os << " [";
    for (const auto& s : p.order)
      {
        os << s << ",";
      }
  os << ']';
  return os;
}

template<class charT, class traits>
inline std::basic_ostream<charT,traits>&
operator<< (std::basic_ostream<charT,traits>& os,
            const CoreMetadataList& list)
{
  os << "{ ";
  for (const auto& secondary : list)
    {
      os << "{ ";
      for (const auto& core : secondary)
        {
          os << core.sizeX << ',' << core.sizeY << ',' << core.sizeZ << ' ';
        }
      os << "} ";
    }
  os << "} ";
  return os;
}

class CoreMetadataListTest : public ::testing::TestWithParam<ListTestParameters>
{
public:
  CoreMetadataList list;
};

TEST_P(CoreMetadataListTest, AutomaticOrder)
{
  const ListTestParameters& params = GetParam();

  CoreMetadataList list(params.list);

  std::cout << "Before: " << list << std::endl;

  orderResolutions(list);

  std::cout << "After: " << list << std::endl;

  ASSERT_EQ(params.list.size(), list.size());
  ASSERT_EQ(params.order.size(), list.size());

  for (dimension_size_type i = 0U;
       i < params.list.size();
       ++i)
    {
      const auto& expected = params.list[i];
      const auto& expected_order = params.order[i];
      const auto& observed = list[i];

      ASSERT_EQ(expected.size(), observed.size());
      ASSERT_EQ(expected_order.size(), observed.size());

      for (dimension_size_type j = 0U;
           j < expected.size();
           ++j)
        {
          ASSERT_TRUE(compare(expected[expected_order[j]], observed[j]));
        }
    }
}

std::vector<ListTestParameters> list_params =
  {
    // CoreMetadataList and corresponding sorted sub-resolution orders.
    // Empty
    {{}, {}},
    // Zero size, no reordering
    {{{CM(0,0,0)}}, {{0}}},
    // Multiple series, no reordering
    {{{CM(0,0,0)},{CM(0,0,0)},{CM(0,0,0)},{CM(0,0,0)}}, {{0},{0},{0},{0}}},
    // Single series, reordering
    {{{CM(4096,4096,1024),CM(8192,8192,1024),CM(0,0,0),CM(2048,2048,512),CM(1024,1024,256)}},
     {{1,0,3,4,2}}},
    // Three series, reordering
    {{{CM(4096,4096,1024),CM(8192,8192,1024),CM(0,0,0),CM(2048,2048,512),CM(1024,1024,256)},
      {CM(4096,8192,512),CM(8192,8192,1024),CM(0,0,0),CM(2048,2048,256),CM(1024,1024,128)},
      {CM(8192,4096,512),CM(8192,8192,1024),CM(0,0,0),CM(2048,2048,512),CM(1024,1024,256)}},
     {{1,0,3,4,2},
      {1,0,3,4,2},
      {1,0,3,4,2}}}
  };


// Disable missing-prototypes warning for INSTANTIATE_TEST_CASE_P;
// this is solely to work around a missing prototype in gtest.
#ifdef __GNUC__
#  if defined __clang__ || defined __APPLE__
#    pragma GCC diagnostic ignored "-Wmissing-prototypes"
#  endif
#  pragma GCC diagnostic ignored "-Wmissing-declarations"
#endif

INSTANTIATE_TEST_CASE_P(ListVariants, CoreMetadataListTest, ::testing::ValuesIn(list_params));
