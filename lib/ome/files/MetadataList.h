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

#ifndef OME_FILES_METADATALIST_H
#define OME_FILES_METADATALIST_H

#include <algorithm>
#include <vector>

#include <ome/files/Types.h>

namespace ome
{
  namespace files
  {

    /**
     * A list of lists of an arbitrary metadata type.
     *
     * This is intended for storing series and resolution metadata,
     * where series is an index into the primary list, and resolution
     * is an index into the secondary list for a given series.
     */
    template<typename T>
    using MetadataList = std::vector<std::vector<T>>;

    /**
     * Get the sizes of each secondary list in a MetadataList.
     *
     * This is the number of secondary elements in each primary
     *
     * @param list the list to use.
     * @returns the sizes.
     */
    template<typename T>
    inline std::vector<dimension_size_type>
    sizes(const MetadataList<T>& list)
    {
      std::vector<dimension_size_type> ret(list.size());
      std::transform(list.begin(), list.end(), ret.begin(),
                     [](const auto& item){ return item.size(); });
      return ret;
    }

  }
}

#endif // OME_FILES_METADATALIST_H

/*
 * Local Variables:
 * mode:C++
 * End:
 */
