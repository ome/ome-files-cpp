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

#include <ome/common/config.h>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <algorithm>
#include <list>

#include <ome/files/Types.h>
#include <ome/files/TileCoverage.h>

using ome::files::dimension_size_type;
namespace geom = boost::geometry;
namespace geomi = boost::geometry::index;

typedef geom::model::point<dimension_size_type, 2, geom::cs::cartesian> point;
typedef geom::model::box<point> box;

namespace
{

  box
  box_from_region(const ::ome::files::PlaneRegion &r)
  {
    return box(point(r.x, r.y),
               point(r.x + r.w, r.y + r.h));
  }

  ::ome::files::PlaneRegion
  region_from_box(const box& b)
  {
    const point& bmin = b.min_corner();
    const point& bmax = b.max_corner();
    return ::ome::files::PlaneRegion
      (bmin.get<0>(), bmin.get<1>(),
       bmax.get<0>() - bmin.get<0>(), bmax.get<1>() - bmin.get<1>());
  }

}

namespace ome
{
  namespace files
  {

    /**
     * Internal implementation details of TileCoverage.
     */
    class TileCoverage::Impl
    {
    public:
      /// Region coverage stored as box ranges.
      geomi::rtree<box, geomi::quadratic<16>> rtree;

      /**
       * Constructor.
       */
      Impl():
        rtree()
      {
      }

      /// Destructor.
      ~Impl()
      {
      }

      /**
       * Boxes intersecting with the specified box.
       *
       * @param b the box to intersect with.
       * @returns a list of intersecting boxes.
       */
      std::vector<box>
      intersecting(const box& b)
      {
        std::vector<box> results;

        rtree.query(geomi::intersects(b),
                    std::back_inserter(results));

        return results;
      }
    };

    TileCoverage::TileCoverage():
      impl(std::shared_ptr<Impl>(new Impl()))
    {
    }

    TileCoverage::~TileCoverage()
    {
    }

    bool
    TileCoverage::insert(const PlaneRegion& region,
                         bool               coalesce)
    {
      bool inserted = false;

      if (coverage(region) == 0)
        {
          box b(box_from_region(region));

          if (!coalesce)
            {
              impl->rtree.insert(b);
              inserted = true;
            }
          else // Merge adjacent regions
            {
              // Merged regions to remove
              std::vector<box> remove;

              PlaneRegion merged_region = region;

              // Merge any adjacent regions and then loop and retry
              // with the resulting enlarged region until no further
              // merges are possible
              bool merged = true;
              while(merged)
                {
                  merged = false;
                  std::vector<box> results = impl->intersecting(b);

                  for(std::vector<box>::const_iterator i = results.begin();
                      i != results.end();
                      ++i)
                    {
                      PlaneRegion test(region_from_box(*i));
                      PlaneRegion m = merged_region | test;

                      if (m.valid())
                        {
                          merged_region = m;
                          remove.push_back(*i);
                          merged = true;
                        }
                    }
                }

              // Remove merged regions
              if (!remove.empty())
                {
                  for (std::vector<box>::const_iterator r = remove.begin();
                       r != remove.end();
                       ++r)
                    {
                      impl->rtree.remove(*r);
                    }
                }

              // Insert merged region
              box mb(box_from_region(merged_region));

              impl->rtree.insert(mb);
              inserted = true;
            }
        }

      return inserted;
    }

    bool
    TileCoverage::remove(const PlaneRegion& region)
    {
      box b(box_from_region(region));

      return (impl->rtree.remove(b));
    }

    dimension_size_type
    TileCoverage::size() const
    {
      return impl->rtree.size();
    }

    void
    TileCoverage::clear()
    {
      impl->rtree.clear();
    }

    dimension_size_type
    TileCoverage::coverage(const PlaneRegion& region) const
    {
      box b(box_from_region(region));
      std::vector<box> results = impl->intersecting(b);

      dimension_size_type area = 0;
      for(std::vector<box>::const_iterator i = results.begin();
          i != results.end();
          ++i)
        {
          PlaneRegion test(region_from_box(*i));
          PlaneRegion intersection = region & test;

          if (intersection.valid())
            area += intersection.area();
        }

      return area;
    }

    bool
    TileCoverage::covered(const PlaneRegion& region) const
    {
      return (region.w * region.h) == coverage(region);
    }

  }
}
