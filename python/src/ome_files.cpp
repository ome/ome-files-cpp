#include <Python.h>

#include <string>

#include <ome/compat/memory.h>
#include <ome/files/FormatReader.h>
#include <ome/files/in/OMETIFFReader.h>


static PyObject *Error;


static PyObject *
get_image_count(PyObject *self, PyObject *args) {
    const char *filename;
    ome::compat::shared_ptr<ome::files::FormatReader> reader;
    PyObject* image_count;
    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return NULL;
    }
    try {
      reader = ome::compat::make_shared<ome::files::in::OMETIFFReader>();
      reader->setId(std::string(filename));
      image_count = PyInt_FromSize_t(reader->getImageCount());
      reader->close();
    } catch (const std::exception& e) {
      PyErr_SetString(Error, e.what());
      return NULL;
    }
    return image_count;
}


static PyMethodDef OMEFilesMethods[] = {
  {"get_image_count", get_image_count, METH_VARARGS,
   "Open file and get image count"},
  {NULL, NULL, 0, NULL} /* Sentinel */
};


PyMODINIT_FUNC
initome_files(void) {
  PyObject *m;
  m = Py_InitModule3("ome_files", OMEFilesMethods, "OME Files wrapper");
  if (!m) {
    return;
  }
  Error = PyErr_NewException(const_cast<char*>("ome_files.Error"), NULL, NULL);
  Py_INCREF(Error);
  PyModule_AddObject(m, "Error", Error);
}
