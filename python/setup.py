from distutils.core import setup, Extension

ext = Extension(
    "ome_files",
    sources=["src/ome_files.cpp"],
    libraries=["ome-common", "ome-files", "ome-xml"],
)

setup(name="ome_files", ext_modules=[ext])
