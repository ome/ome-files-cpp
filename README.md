# OME Files C++

OME Files is a standalone C++ library for reading and writing life sciences
image file formats.

Links
-----

- [Overview of all components](https://docs.openmicroscopy.org/latest/ome-files-cpp/)
- [Documentation](https://docs.openmicroscopy.org/latest/ome-files-cpp/ome-files/manual/html/index.html)
- [Tutorial](https://docs.openmicroscopy.org/latest/ome-files-cpp/ome-files/manual/html/tutorial.html)
- [API reference](https://docs.openmicroscopy.org/latest/ome-files-cpp/ome-files/api/html/namespaces.html)
- [Performance testing](https://github.com/openmicroscopy/ome-files-performance)

Purpose
-------

OME Files' primary purpose is to convert proprietary microscopy data
into an open standard called the OME data model, particularly into the
OME-TIFF file format. See the [statement of
purpose](https://docs.openmicroscopy.org/latest/bio-formats/about/index.html)
for a thorough explanation and rationale.  OME Files provides support
for reading and writing files using the OME-TIFF file format and for
the OME metadata model which is the basis for the file format.


Supported formats
-----------------

OME-TIFF using the OME metadata model.

For users
---------

[Many software
packages](https://docs.openmicroscopy.org/latest/bio-formats/users/index.html)
use Bio-Formats to read and write open microscopy formats such as
OME-TIFF in Java programs.  OME Files provides equivalent
functionality for C++ programs.


For developers
--------------

You can use OME Files C++ to easily support OME-TIFF in your software.


More information
----------------

For more information, see the [documentation](https://docs.openmicroscopy.org/latest/ome-files-cpp/).


Pull request testing
--------------------

We welcome pull requests from anyone, but ask that you please verify the
following before submitting a pull request:

 * verify that the branch merges cleanly into ```master```
 * verify that the branch compiles
 * run the unit tests (```ctest -V```) and correct any failures
 * make sure that your commits contain the correct authorship information and,
   if necessary, a signed-off-by line
 * make sure that the commit messages or pull request comment contains
   sufficient information for the reviewer(s) to understand what problem was
   fixed and how to test it
