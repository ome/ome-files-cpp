#include <Python.h>

#include <string>

#include <ome/compat/memory.h>
#include <ome/files/FormatReader.h>
#include <ome/files/in/OMETIFFReader.h>


static PyObject *Error;


typedef struct {
    PyObject_HEAD
    ome::compat::shared_ptr<ome::files::FormatReader> reader;
} PyOMETIFFReader;


static int
PyOMETIFFReader_init(PyOMETIFFReader *self, PyObject *args, PyObject *kwds) {
  self->reader = ome::compat::make_shared<ome::files::in::OMETIFFReader>();
  return 0;
}


static PyObject *
PyOMETIFFReader_setId(PyOMETIFFReader *self, PyObject *args) {
  const char *filename;
  if (!PyArg_ParseTuple(args, "s", &filename)) {
    return NULL;
  }
  try {
    self->reader->setId(std::string(filename));
  } catch (const std::exception& e) {
    PyErr_SetString(Error, e.what());
    return NULL;
  }
  Py_RETURN_NONE;
}


static PyObject *
PyOMETIFFReader_getImageCount(PyOMETIFFReader *self, PyObject *args) {
  try {
    return PyInt_FromSize_t(self->reader->getImageCount());
  } catch (const std::exception& e) {
    PyErr_SetString(Error, e.what());
    return NULL;
  }
}


static PyMethodDef PyOMETIFFReader_methods[] = {
  {"set_id", (PyCFunction)PyOMETIFFReader_setId, METH_VARARGS,
   "set the current file name"},
  {"get_image_count", (PyCFunction)PyOMETIFFReader_getImageCount, METH_VARARGS,
   "get the number of image planes in the current series"},
  {NULL}                                       /* Sentinel */
};


PyTypeObject PyOMETIFFReaderType = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /* ob_size */
    "ome_files.OMETIFFReader",                 /* tp_name */
    sizeof(PyOMETIFFReader),                   /* tp_basicsize */
    0,                                         /* tp_itemsize */
    0,                                         /* tp_dealloc */
    0,                                         /* tp_print */
    0,                                         /* tp_getattr */
    0,                                         /* tp_setattr */
    0,                                         /* tp_compare */
    0,                                         /* tp_repr */
    0,                                         /* tp_as_number */
    0,                                         /* tp_as_sequence */
    0,                                         /* tp_as_mapping */
    0,                                         /* tp_hash */
    0,                                         /* tp_call */
    0,                                         /* tp_str */
    0,                                         /* tp_getattro */
    0,                                         /* tp_setattro */
    0,                                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags */
    "PyOMETIFFReader objects",                 /* tp_doc */
    0,                                         /* tp_traverse */
    0,                                         /* tp_clear */
    0,                                         /* tp_richcompare */
    0,                                         /* tp_weaklistoffset */
    0,                                         /* tp_iter */
    0,                                         /* tp_iternext */
    PyOMETIFFReader_methods,                   /* tp_methods */
    0,                                         /* tp_members */
    0,                                         /* tp_getset */
    0,                                         /* tp_base */
    0,                                         /* tp_dict */
    0,                                         /* tp_descr_get */
    0,                                         /* tp_descr_set */
    0,                                         /* tp_dictoffset */
    (initproc)PyOMETIFFReader_init,            /* tp_init */
    0,                                         /* tp_alloc */
    0,                                         /* tp_new */
};


static PyMethodDef OMEFilesMethods[] = {
  {NULL}                                       /* Sentinel */
};


PyMODINIT_FUNC
initome_files(void) {
  PyObject *m;
  PyOMETIFFReaderType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PyOMETIFFReaderType) < 0) {
    return;
  }
  m = Py_InitModule3("ome_files", OMEFilesMethods, "OME Files wrapper");
  if (!m) {
    return;
  }
  Py_INCREF(&PyOMETIFFReaderType);
  PyModule_AddObject(m, "OMETIFFReader", (PyObject *)&PyOMETIFFReaderType);
  Error = PyErr_NewException(const_cast<char*>("ome_files.Error"), NULL, NULL);
  Py_INCREF(Error);
  PyModule_AddObject(m, "Error", Error);
}
