#define NO_IMPORT_PYGOBJECT

#include "pygimpcolor.h"

static PyObject *
rgb_set(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *r = NULL, *g = NULL, *b = NULL, *a = NULL;
    GimpRGB tmprgb, *rgb;
    static char *kwlist[] = { "r", "g", "b", "a", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOOO:set", kwlist,
				     &r, &g, &b, &a))
        return NULL;

    if (!r && !g && !b && !a) {
	PyErr_SetString(PyExc_TypeError, "must provide r,g,b or a arguments");
	return NULL;
    }

    if ((r && (!g || !b)) ||
        (g && (!r || !b)) ||
        (b && (!r || !g))) {
	PyErr_SetString(PyExc_TypeError, "must provide all 3 r,g,b arguments");
	return NULL;
    }

    rgb = pyg_boxed_get(self, GimpRGB);
    tmprgb = *rgb;

#define SET_MEMBER(m)	G_STMT_START {				\
    if (PyInt_Check(m))						\
	tmprgb.m = (double) PyInt_AS_LONG(m) / 255.0;		\
    else if (PyFloat_Check(m))					\
        tmprgb.m = PyFloat_AS_DOUBLE(m);			\
    else {							\
	PyErr_SetString(PyExc_TypeError,			\
			#m " must be an int or a float");	\
	return NULL;						\
    }								\
} G_STMT_END

    if (r) {
	SET_MEMBER(r);
	SET_MEMBER(g);
	SET_MEMBER(b);
    }

    if (a)
	SET_MEMBER(a);

#undef SET_MEMBER

    *rgb = tmprgb;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
rgb_set_alpha(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *py_a;
    GimpRGB *rgb;
    static char *kwlist[] = { "a", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
	                             "O:set_alpha", kwlist,
				     &py_a))
        return NULL;

    rgb = pyg_boxed_get(self, GimpRGB);

    if (PyInt_Check(py_a))
	rgb->a = (double) PyInt_AS_LONG(py_a) / 255.0;
    else if (PyFloat_Check(py_a))
        rgb->a = PyFloat_AS_DOUBLE(py_a);
    else {
	PyErr_SetString(PyExc_TypeError, "a must be an int or a float");
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
rgb_add(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *color;
    gboolean with_alpha = FALSE;
    static char *kwlist[] = { "color", "with_alpha", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i:add", kwlist,
				     &PyGimpRGB_Type, &color, &with_alpha))
        return NULL;

    if (with_alpha)
	gimp_rgba_add(pyg_boxed_get(self, GimpRGB),
		      pyg_boxed_get(color, GimpRGB));
    else
	gimp_rgb_add(pyg_boxed_get(self, GimpRGB),
		     pyg_boxed_get(color, GimpRGB));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
rgb_subtract(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *color;
    gboolean with_alpha = FALSE;
    static char *kwlist[] = { "color", "with_alpha", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i:subtract", kwlist,
				     &PyGimpRGB_Type, &color, &with_alpha))
        return NULL;

    if (with_alpha)
	gimp_rgba_subtract(pyg_boxed_get(self, GimpRGB),
			   pyg_boxed_get(color, GimpRGB));
    else
	gimp_rgb_subtract(pyg_boxed_get(self, GimpRGB),
			  pyg_boxed_get(color, GimpRGB));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
rgb_multiply(PyObject *self, PyObject *args, PyObject *kwargs)
{
    double factor;
    gboolean with_alpha = FALSE;
    static char *kwlist[] = { "factor", "with_alpha", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d|i:multiply", kwlist,
				     &factor, &with_alpha))
        return NULL;

    if (with_alpha)
	gimp_rgba_multiply(pyg_boxed_get(self, GimpRGB), factor);
    else
	gimp_rgb_multiply(pyg_boxed_get(self, GimpRGB), factor);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
rgb_distance(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *color;
    gboolean alpha = FALSE;
    double ret;
    static char *kwlist[] = { "color", "alpha", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|i:distance", kwlist,
				     &PyGimpRGB_Type, &color, &alpha))
        return NULL;

    ret = gimp_rgb_distance(pyg_boxed_get(self, GimpRGB),
			    pyg_boxed_get(color, GimpRGB));


    return PyFloat_FromDouble(ret);
}

static PyObject *
rgb_max(PyObject *self)
{
    return PyFloat_FromDouble(gimp_rgb_max(pyg_boxed_get(self, GimpRGB)));
}

static PyObject *
rgb_min(PyObject *self)
{
    return PyFloat_FromDouble(gimp_rgb_min(pyg_boxed_get(self, GimpRGB)));
}

static PyObject *
rgb_clamp(PyObject *self)
{
    gimp_rgb_clamp(pyg_boxed_get(self, GimpRGB));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
rgb_gamma(PyObject *self, PyObject *args, PyObject *kwargs)
{
    double gamma;
    static char *kwlist[] = { "gamma", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d:gamma", kwlist, &gamma))
        return NULL;

    gimp_rgb_gamma(pyg_boxed_get(self, GimpRGB), gamma);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
rgb_luminance(PyObject *self)
{
    return PyFloat_FromDouble(gimp_rgb_luminance(pyg_boxed_get(self, GimpRGB)));
}

static PyObject *
rgb_composite(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *color;
    int mode = GIMP_RGB_COMPOSITE_NORMAL;
    static char *kwlist[] = { "color", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "O!|i:composite", kwlist,
				     &PyGimpRGB_Type, &color, &mode))
        return NULL;

    if (mode < GIMP_RGB_COMPOSITE_NONE || mode > GIMP_RGB_COMPOSITE_BEHIND) {
	PyErr_SetString(PyExc_TypeError, "composite type is not valid");
	return NULL;
    }

    gimp_rgb_composite(pyg_boxed_get(self, GimpRGB),
		       pyg_boxed_get(color, GimpRGB),
		       mode);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
rgb_parse_name(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *name;
    int len;
    gboolean success;
    static char *kwlist[] = { "name", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#:parse_name", kwlist,
				     &name, &len))
        return NULL;

    success = gimp_rgb_parse_name(pyg_boxed_get(self, GimpRGB), name, len);
    PyMem_Free(name);

    if (!success) {
	PyErr_SetString(PyExc_ValueError, "unable to parse color name");
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
rgb_parse_hex(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *hex;
    int len;
    gboolean success;
    static char *kwlist[] = { "hex", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#:parse_hex", kwlist,
				     &hex, &len))
        return NULL;

    success = gimp_rgb_parse_hex(pyg_boxed_get(self, GimpRGB), hex, len);
    PyMem_Free(hex);

    if (!success) {
	PyErr_SetString(PyExc_ValueError, "unable to parse hex value");
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
rgb_parse_css(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *css;
    int len;
    gboolean success, with_alpha = FALSE;
    static char *kwlist[] = { "css", "with_alpha", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s#|i:parse_css", kwlist,
				     &css, &len, &with_alpha))
        return NULL;

    if (with_alpha)
	success = gimp_rgba_parse_css(pyg_boxed_get(self, GimpRGB), css, len);
    else
	success = gimp_rgb_parse_css(pyg_boxed_get(self, GimpRGB), css, len);

    PyMem_Free(css);

    if (!success) {
	PyErr_SetString(PyExc_ValueError, "unable to parse CSS color");
	return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef rgb_methods[] = {
    { "set", (PyCFunction)rgb_set, METH_VARARGS|METH_KEYWORDS },
    { "set_alpha", (PyCFunction)rgb_set_alpha, METH_VARARGS|METH_KEYWORDS },
    { "add", (PyCFunction)rgb_add, METH_VARARGS|METH_KEYWORDS },
    { "subtract", (PyCFunction)rgb_subtract, METH_VARARGS|METH_KEYWORDS },
    { "multiply", (PyCFunction)rgb_multiply, METH_VARARGS|METH_KEYWORDS },
    { "distance", (PyCFunction)rgb_distance, METH_VARARGS|METH_KEYWORDS },
    { "max", (PyCFunction)rgb_max, METH_NOARGS },
    { "min", (PyCFunction)rgb_min, METH_NOARGS },
    { "clamp", (PyCFunction)rgb_clamp, METH_NOARGS },
    { "gamma", (PyCFunction)rgb_gamma, METH_VARARGS|METH_KEYWORDS },
    { "luminance", (PyCFunction)rgb_luminance, METH_NOARGS },
    { "composite", (PyCFunction)rgb_composite, METH_VARARGS|METH_KEYWORDS },
    { "parse_name", (PyCFunction)rgb_parse_name, METH_VARARGS|METH_KEYWORDS },
    { "parse_hex", (PyCFunction)rgb_parse_hex, METH_VARARGS|METH_KEYWORDS },
    { "parse_css", (PyCFunction)rgb_parse_css, METH_VARARGS|METH_KEYWORDS },
    { NULL, NULL, 0 }
};

#define MEMBER_ACCESSOR(m) \
static PyObject *							\
rgb_get_ ## m(PyObject *self, void *closure)				\
{									\
    return PyFloat_FromDouble(pyg_boxed_get(self, GimpRGB)->m);		\
}									\
static int								\
rgb_set_ ## m(PyObject *self, PyObject *value, void *closure)		\
{									\
    GimpRGB *rgb = pyg_boxed_get(self, GimpRGB);			\
    if (value == NULL) {						\
	PyErr_SetString(PyExc_TypeError, "cannot delete value");	\
	return -1;							\
    }									\
    else if (PyInt_Check(value))					\
	rgb->m = (double) PyInt_AS_LONG(value) / 255.0;			\
    else if (PyFloat_Check(value))					\
        rgb->m = PyFloat_AS_DOUBLE(value);				\
    else {								\
	PyErr_SetString(PyExc_TypeError, "type mismatch");		\
	return -1;							\
    }									\
    return 0;								\
}

MEMBER_ACCESSOR(r);
MEMBER_ACCESSOR(g);
MEMBER_ACCESSOR(b);
MEMBER_ACCESSOR(a);

#undef MEMBER_ACCESSOR

static PyGetSetDef rgb_getsets[] = {
    { "r", (getter)rgb_get_r, (setter)rgb_set_r },
    { "g", (getter)rgb_get_g, (setter)rgb_set_g },
    { "b", (getter)rgb_get_b, (setter)rgb_set_b },
    { "a", (getter)rgb_get_a, (setter)rgb_set_a },
    { "red",   (getter)rgb_get_r, (setter)rgb_set_r },
    { "green", (getter)rgb_get_g, (setter)rgb_set_g },
    { "blue",  (getter)rgb_get_b, (setter)rgb_set_b },
    { "alpha", (getter)rgb_get_a, (setter)rgb_set_a },
    { NULL, (getter)0, (setter)0 },
};

static PyObject *
rgb_init(PyGBoxed *self, PyObject *args, PyObject *kwargs)
{
    PyObject *r, *g, *b, *a = NULL;
    GimpRGB rgb;
    static char *kwlist[] = { "r", "g", "b", "a", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
	                             "OOO|O:set", kwlist,
				     &r, &g, &b, &a))
        return NULL;

#define SET_MEMBER(m)	G_STMT_START {				\
    if (PyInt_Check(m))						\
	rgb.m = (double) PyInt_AS_LONG(m) / 255.0;		\
    else if (PyFloat_Check(m))					\
        rgb.m = PyFloat_AS_DOUBLE(m);				\
    else {							\
	PyErr_SetString(PyExc_TypeError,			\
			#m " must be an int or a float");	\
	return NULL;						\
    }								\
} G_STMT_END

    SET_MEMBER(r);
    SET_MEMBER(g);
    SET_MEMBER(b);

    if (a)
	SET_MEMBER(a);
    else
        rgb.a = 1.0;

#undef SET_MEMBER

    self->gtype = GIMP_TYPE_RGB;
    self->free_on_dealloc = TRUE;
    self->boxed = g_boxed_copy(GIMP_TYPE_RGB, &rgb);

    Py_INCREF(Py_None);
    return Py_None;
}

PyTypeObject PyGimpRGB_Type = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "gimpcolor.RGB",			/* tp_name */
    sizeof(PyGBoxed),			/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)0,			/* tp_dealloc */
    (printfunc)0,			/* tp_print */
    (getattrfunc)0,			/* tp_getattr */
    (setattrfunc)0,			/* tp_setattr */
    (cmpfunc)0,				/* tp_compare */
    (reprfunc)0,			/* tp_repr */
    (PyNumberMethods*)0,		/* tp_as_number */
    (PySequenceMethods*)0,		/* tp_as_sequence */
    (PyMappingMethods*)0,		/* tp_as_mapping */
    (hashfunc)0,			/* tp_hash */
    (ternaryfunc)0,			/* tp_call */
    (reprfunc)0,			/* tp_repr */
    (getattrofunc)0,			/* tp_getattro */
    (setattrofunc)0,			/* tp_setattro */
    (PyBufferProcs*)0,			/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,			/* tp_flags */
    NULL, 				/* Documentation string */
    (traverseproc)0,			/* tp_traverse */
    (inquiry)0,				/* tp_clear */
    (richcmpfunc)0,			/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    (getiterfunc)0,			/* tp_iter */
    (iternextfunc)0,			/* tp_iternext */
    rgb_methods,			/* tp_methods */
    0,					/* tp_members */
    rgb_getsets,			/* tp_getset */
    NULL,				/* tp_base */
    NULL,				/* tp_dict */
    (descrgetfunc)0,			/* tp_descr_get */
    (descrsetfunc)0,			/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)rgb_init,			/* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
    (freefunc)0,			/* tp_free */
    (inquiry)0				/* tp_is_gc */
};

PyObject *
pygimp_rgb_new(GimpRGB *rgb)
{
    return pyg_boxed_new(GIMP_TYPE_RGB, rgb, TRUE, TRUE);
}
