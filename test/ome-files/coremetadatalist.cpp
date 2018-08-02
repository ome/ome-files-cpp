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
#include <memory>

#include <ome/test/test.h>

using ome::files::dimension_size_type;
using ome::files::CoreMetadata;
using ome::files::CoreMetadataList;
using ome::files::MetadataList;
using ome::files::orderResolutions;

namespace
{

  // Create CoreMetadata from x,y,z extents.
  std::unique_ptr<CoreMetadata>
  CM(dimension_size_type x,
     dimension_size_type y,
     dimension_size_type z)
  {
    auto c = std::make_unique<CoreMetadata>();
    c->sizeX = x;
    c->sizeY = y;
    c->sizeZ = z;
    return c;
  }

  // Compare CoreMetadata by x,y,z extents.
  bool
  compare(const std::unique_ptr<CoreMetadata>& lhs,
          const std::unique_ptr<CoreMetadata>& rhs)
  {
    if (!lhs && !rhs)
      return true;
    if (!lhs || !rhs)
      return false;
    return lhs->sizeX == rhs->sizeX &&
           lhs->sizeY == rhs->sizeY &&
           lhs->sizeZ == rhs->sizeZ;
  }

}

struct ListTestParameters
{
  // Core metadata to test.
  CoreMetadataList list;
  // Sorted order of subresolutions for each series.
  MetadataList<dimension_size_type> order;
  // Throws on reorder.
  bool reorder_throw;

  ListTestParameters():
    list(),
    order(),
    reorder_throw(false)
  {
  }

  ListTestParameters(CoreMetadataList&& list,
                     MetadataList<dimension_size_type>&& order,
                     bool reorder_throw):
    list(std::move(list)),
    order(std::move(order)),
    reorder_throw(reorder_throw)
  {
  }

  ListTestParameters(CoreMetadataList& list,
                     MetadataList<dimension_size_type>& order,
                     bool reorder_throw):
    list(ome::files::copy(list)),
    order(order),
    reorder_throw(reorder_throw)
  {
  }

  ListTestParameters(const ListTestParameters& rhs):
    list(ome::files::copy(rhs.list)),
    order(rhs.order),
    reorder_throw(rhs.reorder_throw)
  {
  }

  // Not assignable.
  ListTestParameters& operator=(const ListTestParameters&) = delete;
  // Is moveable.
  ListTestParameters(ListTestParameters&&) = default;
  // Is move-assignable.
  ListTestParameters& operator=(ListTestParameters&&) = default;
};

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
          if (core)
            os << core->sizeX << ',' << core->sizeY << ',' << core->sizeZ << ' ';
          else
            os << "null ";
        }
      os << "} ";
    }
  os << "} ";
  return os;
}

template<class charT, class traits>
inline std::basic_ostream<charT,traits>&
operator<< (std::basic_ostream<charT,traits>& os,
            const ListTestParameters& p)
{
  os << p.list;
  os << " [ ";
    for (const auto& s : p.order)
      {
        os << '(';
        for (const auto& r : s)
          os << r << ",";
        os << "), ";
      }
  os << ']';
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

  CoreMetadataList list;
  ome::files::append(params.list, list);

  std::cout << "Before: " << list << std::endl;

  if (params.reorder_throw)
    {
      ASSERT_THROW(orderResolutions(list), std::logic_error);
      return; // No need to check ordering.
    }
  else
    {
      ASSERT_NO_THROW(orderResolutions(list));
    }

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

std::vector<ListTestParameters> list_params = [](){
  using LTP = ListTestParameters;

  std::vector<ListTestParameters> data;

  // CoreMetadataList and corresponding sorted sub-resolution orders.

  // Empty
  auto test1 = LTP{};
  data.emplace_back(std::move(test1));

  // Null
  auto test2 = LTP{};
  test2.list.resize(1);
  test2.order.resize(1);
  test2.list[0].emplace_back(std::unique_ptr<CoreMetadata>());
  test2.order[0] = {0};
  test2.reorder_throw = false; // only one item; comparison does not occur
  data.emplace_back(std::move(test2));

  // Zero size, no reordering
  auto test3 = LTP{};
  test3.list.resize(1);
  test3.order.resize(1);
  test3.list[0].emplace_back(CM(0,0,0));
  test3.order[0] = {0};
  data.emplace_back(std::move(test3));

  // Multiple series, no reordering
  auto test4 = LTP{};
  test4.list.resize(4);
  test4.order.resize(4);
  test4.list[0].emplace_back(CM(0,0,0));
  test4.list[1].emplace_back(CM(0,0,0));
  test4.list[2].emplace_back(CM(0,0,0));
  test4.list[3].emplace_back(CM(0,0,0));
  test4.order[0] = {0};
  test4.order[1] = {0};
  test4.order[2] = {0};
  test4.order[3] = {0};
  data.emplace_back(std::move(test4));

  // Single series, reordering
  auto test5 = LTP{};
  test5.list.resize(1);
  test5.order.resize(1);
  test5.list[0].emplace_back(CM(4096,4096,1024));
  test5.list[0].emplace_back(CM(8192,8192,1024));
  test5.list[0].emplace_back(CM(0,0,0));
  test5.list[0].emplace_back(CM(2048,2048,512));
  test5.list[0].emplace_back(CM(1024,1024,256));
  test5.order[0] = {1,0,3,4,2};
  data.emplace_back(std::move(test5));

  // Single series including null, reordering
  auto test6 = LTP{};
  test6.list.resize(1);
  test6.order.resize(1);
  test6.list[0].emplace_back(CM(4096,4096,1024));
  test6.list[0].emplace_back(CM(8192,8192,1024));
  test6.list[0].emplace_back(std::unique_ptr<CoreMetadata>());
  test6.list[0].emplace_back(CM(2048,2048,512));
  test6.list[0].emplace_back(CM(1024,1024,256));
  test6.order[0] = {1,0,3,4,2};
  test6.reorder_throw = true;
  data.emplace_back(std::move(test6));

  // Three series, reordering
  auto test7 = LTP{};
  test7.list.resize(3);
  test7.order.resize(3);

  test7.list[0].emplace_back(CM(4096,4096,1024));
  test7.list[0].emplace_back(CM(8192,8192,1024));
  test7.list[0].emplace_back(CM(0,0,0));
  test7.list[0].emplace_back(CM(2048,2048,512));
  test7.list[0].emplace_back(CM(1024,1024,256));
  test7.order[0] = {1,0,3,4,2};

  test7.list[1].emplace_back(CM(8192,8192,1024));
  test7.list[1].emplace_back(CM(0,0,0));
  test7.list[1].emplace_back(CM(2048,2048,512));
  test7.list[1].emplace_back(CM(1024,1024,256));
  test7.list[1].emplace_back(CM(4096,8192,512));
  test7.order[1] = {0,4,2,3,1};

  test7.list[2].emplace_back(CM(2048,2048,512));
  test7.list[2].emplace_back(CM(1024,1024,256));
  test7.list[2].emplace_back(CM(8192,8192,1024));
  test7.list[2].emplace_back(CM(0,0,0));
  test7.list[2].emplace_back(CM(8192,4096,512));
  test7.order[2] = {2,4,0,1,3};

  data.emplace_back(std::move(test7));

  return data;
}();


// Disable missing-prototypes warning for INSTANTIATE_TEST_CASE_P;
// this is solely to work around a missing prototype in gtest.
#ifdef __GNUC__
#  if defined __clang__ || defined __APPLE__
#    pragma GCC diagnostic ignored "-Wmissing-prototypes"
#  endif
#  pragma GCC diagnostic ignored "-Wmissing-declarations"
#endif

INSTANTIATE_TEST_CASE_P(ListVariants, CoreMetadataListTest, ::testing::ValuesIn(list_params));
