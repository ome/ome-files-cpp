Building
========

CMake options
-------------

This project uses a standard CMake-based build.  It may be configured
and built using any supported CMake generator using any of the
standard CMake options and variables, plus the additional
project-specific options detailed below.  Run ``cmake -LH`` to see the
configurable project options; use ``-LAH`` to see advanced options.
The following basic options are supported:

cxxstd-autodetect=(ON|OFF)
  Enable or disable (default) C++ compiler standard autodetection.  If
  enabled, the compiler will be put into C++11 mode if available,
  otherwise falling back to C++03 or C++98.  If disabled, the default
  compiler standard mode is used, and it is the responsibility of the
  user to add the appropriate compiler options to build using the
  required standard.  This is useful if autodetection fails or a
  compiler is buggy in certain modes (e.g. GCC 4.4 or 4.6 require
  ``-std=gnu++98`` or else ``stdarg`` support is broken).
doxygen=(ON|OFF)
  Enable doxygen documentation.  These will be enabled by default if
  doxygen is found.
extended-tests=(ON|OFF)
  Some of the unit tests are comprehensive and run many thousands of
  tests.  These are enabled by default, but by setting to OFF a
  representative subset of the tests will be run instead to save time.
extra-warnings=(ON|OFF)
  Enable or disable additional compiler warnings in addition to the
  default set.  These are disabled by default since they trigger a large
  number of false positives, particularly in third-party libraries
  outside our control.
fatal-warnings=(ON|OFF)
  Make compiler warnings into fatal errors.  This is disabled by
  default.
relocatable-install=(ON|OFF)
  Make the installed libraries, programs and datafiles relocatable;
  this means that they may be moved from their installation prefix to
  another location without breaking them.  If OFF, the installation
  prefix is assumed to contain the libraries and datafiles.  If ON, no
  assumptions are made, and a slower fallback is used to introspect
  the location.  In all cases the location may be set in the
  environment to override the compiled-in defaults.  This is OFF by
  default for a regular build, and ON by default for a superbuild.
sphinx=(ON|OFF)
  Build manual pages and HTML documentation with Sphinx.  Enabled by
  default if Sphinx is autodetected.
sphinx-pdf=(ON|OFF)
  Build PDF documentation with Sphinx.  Enabled by default if Sphinx
  and XeLaTeX are autodetected.
test=(ON|OFF)
  Enable unit tests.  Tests are enabled by default.

C++11
^^^^^

C++11 features such as :cpp:class:`std::shared_ptr` are used when
using a C++11 or C++14 compiler, or when ``-Dcxxstd-autodetect=ON`` is
used and the compiler can be put into a C++11 or C++14 compatibility
mode.  When using an older compatbility mode such as C++98, the Boost
equivalents of C++11 library features will be used as fallbacks to
provide the same functionality.  In both cases these types are
imported into the :cpp:class:`ome::compat` namespace, for example as
:cpp:class:`ome::compat::shared_ptr`, and the types in this namespace
should be used for portability when using any part of the API which
use types from this namespace.
