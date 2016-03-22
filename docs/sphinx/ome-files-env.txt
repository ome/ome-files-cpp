.. _ome-files-env:

Environment
===========

The OME C++ libraries and programs are configured and built to use a
set of search paths for different components.  It should not be
necessary to override these defaults.  The :program:`bf` command will
be able to autodetect the installation directory configure paths on
most platforms, and the OME C++ libraries are also able to determine
the paths on most platforms so long as the library search path is
configured correctly.  However, the following environment variables
may be used to override the defaults if this proves necessary:

Installation root
-----------------

:envvar:`OME_FILES_HOME`

  The root of the installation (if applicable).  Setting this will
  allow the installation to be used in a location other than the one
  configured.  It will also default all the following variables unless
  they are explicitly overridden individually.  This is not useful if
  an absolute installation path has been configured (e.g. if using
  :file:`/usr/local`).

Basic paths
-----------

These may be shared with other packages if configured to do so
(e.g. if using :file:`/usr/local`).  See `GNUInstallDirs
<http://www.cmake.org/cmake/help/v3.0/module/GNUInstallDirs.html>`_
for more details.  Not all of these paths are currently used, but may
be used in the future.

:envvar:`OME_FILES_BINDIR`
  Programs invocable directly by an end user (on the default
  :envvar:`PATH`)
:envvar:`OME_FILES_SBINDIR`
  Programs invocable directly by an end user or admin (not on the
  default :envvar:`PATH`)
:envvar:`OME_FILES_LIBEXECDIR`
  Programs not typically invoked directly (called internally by the
  OME-Files tools and libraries as needed)
:envvar:`OME_FILES_SYSCONFDIR`
  Configuration files
:envvar:`OME_FILES_SHAREDSTATEDIR`
  Shared state
:envvar:`OME_FILES_LOCALSTATEDIR`
  Local state
:envvar:`OME_FILES_LIBDIR`
  Libraries
:envvar:`OME_FILES_INCLUDEDIR`
  C and C++ include files
:envvar:`OME_FILES_OLDINCLUDEDIR`
  C and C++ include files (system)
:envvar:`OME_FILES_DATAROOTDIR`
  Read-only architecture-independent data (root)
:envvar:`OME_FILES_SYSDATADIR`
  Read-only architecture-independent data
:envvar:`OME_FILES_INFODIR`
  GNU Info documentation files
:envvar:`OME_FILES_LOCALEDIR`
  Locale data
:envvar:`OME_FILES_MANDIR`
  Manual pages
:envvar:`OME_FILES_DOCDIR`
  Documentation files

Package-specific paths
----------------------

These are used by specific packages and their dependees and are not
shared with other packages.  They are all subdirectories under the
basic paths, above.

:envvar:`OME_FILES_DATADIR`
  OME-Files data files
:envvar:`OME_FILES_LIBEXECDIR`
  OME-Files program executables
:envvar:`OME_QTWIDGETS_ICONDIR`
  OME-QtWidgets icons
:envvar:`OME_XML_SCHEMADIR`
  OME-XML model schemas
:envvar:`OME_XML_TRANSFORMDIR`
  OME-XML model transforms
