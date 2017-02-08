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
sphinx-linkcheck=(ON|OFF)
  Check the validity of all internal and external documentation links.
  Disabled by default.  If set to ON, the build will fail if any links
  are invalid; if set to OFF, the linkchecking targets will not be run
  automatically, but may still be run by hand.
test=(ON|OFF)
  Enable unit tests.  Tests are enabled by default.
