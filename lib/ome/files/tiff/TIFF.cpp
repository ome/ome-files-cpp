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
#include <vector>

#include <fcntl.h> // For O_RDONLY on Unix and Windows

// Include before boost headers to ensure the MPL limits get defined.
#include <ome/common/config.h>

#include <boost/range/size.hpp>
#include <boost/thread.hpp>

#include <ome/files/Version.h>
#include <ome/files/tiff/Field.h>
#include <ome/files/tiff/Tags.h>
#include <ome/files/tiff/TIFF.h>
#include <ome/files/tiff/IFD.h>
#include <ome/files/tiff/Sentry.h>
#include <ome/files/tiff/Exception.h>
#include <ome/files/detail/tiff/Tags.h>

#include <ome/common/string.h>

#include <tiffio.h>

using boost::mutex;
using boost::lock_guard;

namespace ome
{
  namespace files
  {
    namespace tiff
    {

      /// @copydoc IFDIterator::increment()
      template<>
      void
      IFDIterator<IFD>::increment()
      {
        pos = pos->next();
      }

      /// @copydoc IFDIterator::increment()
      template<>
      void
      IFDIterator<const IFD>::increment()
      {
        pos = pos->next();
      }

      namespace
      {

        class TIFFConcrete : public TIFF
        {
        public:
          TIFFConcrete(const boost::filesystem::path& filename,
                       const std::string&             mode):
            TIFF(filename, mode)
          {
          }

          virtual
          ~TIFFConcrete()
          {
          }
        };

      }

      /**
       * Internal implementation details of TIFF.
       */
      class TIFF::Impl
      {
      public:
        /// The libtiff file handle.
        ::TIFF *tiff;
        /// Directory offsets
        std::vector<offset_type> offsets;

        /**
         * The constructor.
         *
         * Opens the TIFF using TIFFOpen().
         *
         * @param filename the filename to open.
         * @param mode the file open mode.
         */
        Impl(const boost::filesystem::path& filename,
             const std::string&             mode):
          tiff(),
          offsets()
        {
          Sentry sentry;

#ifdef _MSC_VER
          tiff = TIFFOpenW(filename.wstring().c_str(), mode.c_str());
#else
          tiff = TIFFOpen(filename.string().c_str(), mode.c_str());
#endif
          if (!tiff)
            sentry.error();
        }

        /**
         * The destructor.
         *
         * The open TIFF will be closed if open.
         */
        ~Impl()
        {
          try
            {
              close();
            }
          catch (const Exception&)
            {
              /// @todo Log the error elsewhere.
            }
          catch (...)
            {
              // Catch any exception thrown by closing.
            }
        }

      private:
        /// Copy constructor (deleted).
        Impl (const Impl&);

        /// Assignment operator (deleted).
        Impl&
        operator= (const Impl&);

      public:
        /**
         * Close the libtiff file handle.
         *
         * If open, the file handle will be closed with TIFFClose.
         */
        void
        close()
        {
          if (tiff)
            {
              Sentry sentry;

              TIFFClose(tiff);
              if (!sentry.getMessage().empty())
                sentry.error();
              tiff = 0;
            }
        }
      };

      // Note boost::make_shared can't be used here.
      TIFF::TIFF(const boost::filesystem::path& filename,
                 const std::string&             mode):
        impl(std::shared_ptr<Impl>(new Impl(filename, mode)))
      {
        registerImageJTags();

        // When reading, cache all directory offsets.  When writing,
        // we don't have any offsets until we write a directory, so
        // ignore caching entirely.
        if(TIFFGetMode(impl->tiff) == O_RDONLY)
          {
            do
              {
                offset_type offset = static_cast<offset_type>(TIFFCurrentDirOffset(impl->tiff));
                impl->offsets.push_back(offset);
              }
            while (TIFFReadDirectory(impl->tiff) == 1);
          }
      }

      TIFF::~TIFF()
      {
      }

      TIFF::wrapped_type *
      TIFF::getWrapped() const
      {
        return reinterpret_cast<wrapped_type *>(impl->tiff);
      }

      std::shared_ptr<TIFF>
      TIFF::open(const boost::filesystem::path& filename,
                 const std::string& mode)
      {
        std::shared_ptr<TIFF> ret;
        try
          {
            // Note boost::make_shared can't be used here.
            ret = std::shared_ptr<TIFF>(new TIFFConcrete(filename, mode));
          }
        catch (const std::exception& e)
          {
            // All exception types are propagated as an Exception.
            throw Exception(e.what());
          }
        return ret;
      }

      void
      TIFF::close()
      {
        impl->close();
      }

      TIFF::operator bool ()
      {
        return impl && impl->tiff;
      }

      directory_index_type
      TIFF::directoryCount() const
      {
        return impl->offsets.size();
      }

      std::shared_ptr<IFD>
      TIFF::getDirectoryByIndex(directory_index_type index) const
      {
        offset_type offset;
        try
          {
            offset = impl->offsets.at(index);
          }
        catch (const std::out_of_range& e)
          {
            // Invalid index.
            throw Exception(e.what());
          }
        return getDirectoryByOffset(offset);
      }

      std::shared_ptr<IFD>
      TIFF::getDirectoryByOffset(offset_type offset) const
      {
        Sentry sentry;

        std::shared_ptr<TIFF> t(std::const_pointer_cast<TIFF>(shared_from_this()));
        std::shared_ptr<IFD> ifd = IFD::openOffset(t, offset);
        ifd->makeCurrent(); // Validate offset.
        return ifd;
      }

      std::shared_ptr<IFD>
      TIFF::getCurrentDirectory() const
      {
        std::shared_ptr<TIFF> t(std::const_pointer_cast<TIFF>(shared_from_this()));
        return IFD::current(t);
      }

      void
      TIFF::writeCurrentDirectory()
      {
        Sentry sentry;

        static const std::string software("OME Files (C++) " OME_FILES_VERSION_MAJOR_S "." OME_FILES_VERSION_MINOR_S "." OME_FILES_VERSION_PATCH_S);
        getCurrentDirectory()->getField(SOFTWARE).set(software);

        if (!TIFFWriteDirectory(impl->tiff))
          sentry.error("Failed to write current directory");
      }

      TIFF::iterator
      TIFF::begin()
      {
        std::shared_ptr<IFD> ifd(getDirectoryByIndex(0U));
        return iterator(ifd);
      }

      TIFF::const_iterator
      TIFF::begin() const
      {
        std::shared_ptr<IFD> ifd(getDirectoryByIndex(0U));
        return const_iterator(ifd);
      }

      TIFF::iterator
      TIFF::end()
      {
        return iterator();
      }

      TIFF::const_iterator
      TIFF::end() const
      {
        return const_iterator();
      }

      void
      TIFF::registerImageJTags()
      {
        // This is optional, used for quieting libtiff messages about
        // unknown tags by registering them.  This doesn't work
        // completely since some warnings will be issued reading the
        // first directory, before we can register them.  This is
        // deprecated in libtiff4, so guard against future removal.
        //
        // These static strings are to provide a writable string to
        // comply with the TIFFFieldInfo interface which can't be
        // assigned const string literals.  They must outlive the
        // registered field info.
        static std::string ijbc("ImageJMetadataByteCounts");
        static std::string ij("ImageJMetadata");
        static const TIFFFieldInfo ImageJFieldInfo[] =
          {
            {
              TIFFTAG_IMAGEJ_META_DATA_BYTE_COUNTS,
              TIFF_VARIABLE2, TIFF_VARIABLE2, TIFF_LONG, FIELD_CUSTOM,
              true, true, const_cast<char *>(ijbc.c_str())
            },
            {
              TIFFTAG_IMAGEJ_META_DATA,
              TIFF_VARIABLE2, TIFF_VARIABLE2, TIFF_BYTE, FIELD_CUSTOM,
              true, true, const_cast<char *>(ij.c_str())
            }
          };

        ::TIFF *tiffraw = reinterpret_cast<::TIFF *>(getWrapped());

        Sentry sentry;

        int e = TIFFMergeFieldInfo(tiffraw, ImageJFieldInfo, boost::size(ImageJFieldInfo));
        if (e)
          sentry.error();
      }

    }
  }
}
