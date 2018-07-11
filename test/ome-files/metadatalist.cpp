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

#include <ome/files/MetadataList.h>

#include <iostream>

#include <ome/test/test.h>

using ome::files::dimension_size_type;
using ome::files::MetadataList;

struct ListTestParameters
{
  std::vector<std::vector<int>> list;
  std::vector<dimension_size_type> sizes;

  ListTestParameters(const std::vector<std::vector<int>>& list,
                     const std::vector<dimension_size_type> sizes):
    list(list),
    sizes(sizes)
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
      for (const auto& elem : secondary)
        {
          os << elem << ' ';
        }
      os << "} ";
    }
  os << " [";
    for (const auto& s : p.sizes)
      {
        os << s << ",";
      }
  os << ']';
  return os;
}

template<class charT, class traits>
inline std::basic_ostream<charT,traits>&
operator<< (std::basic_ostream<charT,traits>& os,
            const MetadataList<int>& list)
{
  os << "{ ";
  for (const auto& secondary : list)
    {
      os << "{ ";
      for (const auto& elem : secondary)
        {
          os << elem << ' ';
        }
      os << "} ";
    }
  os << "} ";
  return os;
}

class MetadataListTest : public ::testing::TestWithParam<ListTestParameters>
{
public:
  MetadataList<int> list;
};

TEST_P(MetadataListTest, CreateFromNestedList)
{
  const ListTestParameters& params = GetParam();

  MetadataList<int> list(params.list);

  std::cout << list << std::endl;

  ASSERT_EQ(params.sizes.size(), list.size());
  ASSERT_EQ(params.sizes, ome::files::sizes(list));
}

TEST_P(MetadataListTest, CreateByAppend)
{
  const ListTestParameters& params = GetParam();

  MetadataList<int> list;

  for (const auto& secondary : params.list)
    {
      std::vector<int> l2;
      for (const auto& elem : secondary)
        {
          l2.push_back(elem);
        }
      list.push_back(l2);
    }

  std::cout << list << std::endl;

  ASSERT_EQ(params.sizes.size(), list.size());
  ASSERT_EQ(params.sizes, ome::files::sizes(list));
}

std::vector<ListTestParameters> list_params =
  {
    {{}, {}},
    {{{2}}, {1}},
    {{{2,4}}, {2}},
    {{{0,1,2}}, {3}},
    {{{2,4,5}}, {3}},
    {{{8},{9}}, {{1,1}}},
    {{{4,5},{6,7}}, {{2,2}}},
    {{{1},{2},{3},{4},{5},{6}}, {1,1,1,1,1,1}},
    {{{0,1,2},{3},{4,5},{6,7,8}}, {3,1,2,3}}
  };


// Disable missing-prototypes warning for INSTANTIATE_TEST_CASE_P;
// this is solely to work around a missing prototype in gtest.
#ifdef __GNUC__
#  if defined __clang__ || defined __APPLE__
#    pragma GCC diagnostic ignored "-Wmissing-prototypes"
#  endif
#  pragma GCC diagnostic ignored "-Wmissing-declarations"
#endif

INSTANTIATE_TEST_CASE_P(ListVariants, MetadataListTest, ::testing::ValuesIn(list_params));
