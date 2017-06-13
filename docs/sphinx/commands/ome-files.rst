ome-files
=========

Synopsis
--------

**ome-files** command [*options*]

Description
-----------

:program:`ome-files` is a front end for running the OME-Files (C++)
command-line tools.

This takes care of setting up the environment to ensure that all
needed libraries, programs and data files are made available.  It is
of course possible to run the tools directly if desired.

Options
-------

.. option:: -h, --help

  Show this manual page.

.. option:: -u, --usage

  Show usage information.

.. option:: -V, --version

  Print version information.

Commands
--------

Commonly-used commands are:

info (or showinf)
  Display and validate image metadata
view (or glview)
  View image pixel data [optional component; only present if built with
  Qt5 and OpenGL support]

See also
--------

:ref:`ome-files-env`, :ref:`ome-files-info`, :ref:`ome-files-view`.
