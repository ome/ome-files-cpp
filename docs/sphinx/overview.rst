OME Files C++ overview
======================

OME Files C++ is a reference implementation of the OME data model and
the OME-TIFF file format for C++ developers who wish to integrate
support for the OME data model and reading and writing the standard
OME-TIFF file format into their software.  Potential uses include
export of images using OME-TIFF, saving of acquired image data in
OME-TIFF, reading metadata and image data from OME-TIFF for
visualisation and analysis, or use of the data model metadata APIs for
handling metadata.

OME Files C++ is a re-implementation of the Bio-Formats Java API,
providing equivalent data model, metadata and reading and writing
interfaces.  Unlike the Bio-Formats Java API, which supports reading
of over 130 file formats and writing of over 15 file formats at the
time of writing, OME Files C++ restricts itself to reading and writing
OME-TIFF and plain TIFF, and does not at present support additional
formats.  At present it is a reference implementation for the OME-TIFF
format, and the focus for development is upon improving and extending
the data model metadata APIs and the reader and writer APIs, rather
than adding additional formats.

.. note::
  Due to the renaming of Bio-Formats C++ to OME Files, this will
  result in an API break between version Bio-Formats C++ 5.1 and OME
  Files C++ 0.x as a result of the renamed namespaces.  Further
  breaking changes are planned as the basic interfaces are cleaned up
  to make them more flexible and efficient.
