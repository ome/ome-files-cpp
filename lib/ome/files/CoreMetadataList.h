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

#ifndef OME_FILES_COREMETADATALIST_H
#define OME_FILES_COREMETADATALIST_H

#include <ome/files/CoreMetadata.h>
#include <ome/files/MetadataList.h>

#include <iostream>

namespace ome
{
  namespace files
  {

    /**
     * A list of lists of core metadata.
     *
     * This is intended for storing series and resolution core
     * metadata, where series is an index into the primary list, and
     * resolution is an index into the secondary list for a given
     * series.
     */
    using CoreMetadataList = MetadataList<CoreMetadata>;

    /**
     * Order resolution levels in a CoreMetadataList.
     *
     * For each series, order the resolutions from largest to
     * smallest.
     *
     * @param list the core metadata list to use.
     */
    inline void
    orderResolutions(CoreMetadataList& list)
    {
      for (auto& secondary : list)
        {
          std::sort(secondary.begin(), secondary.end(),
                    [](const auto& lhs, const auto& rhs)
                    { return lhs.sizeX > rhs.sizeX ||
                        lhs.sizeY > rhs.sizeY ||
                        lhs.sizeZ > rhs.sizeZ; });
        }
    }

  }
}

#endif // OME_FILES_METADATALIST_H

/*
 * Local Variables:
 * mode:C++
 * End:
 */
