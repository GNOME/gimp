/* -*- Mode: C; c-basic-offset: 4 -*-
 * Gimp-Python - allows the writing of Gimp plugins in Python.
 * Copyright (C) 2003  Manish Singh <yosh@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _PYGIMPCOLOR_H_
#define _PYGIMPCOLOR_H_

#include <Python.h>

#include <pygobject.h>

#include <libgimp/gimp.h>

G_BEGIN_DECLS

extern PyTypeObject PyGimpRGB_Type;
#define pygimp_rgb_check(v) (pyg_boxed_check((v), GIMP_TYPE_RGB))
PyObject *pygimp_rgb_new(const GimpRGB *rgb);

extern PyTypeObject PyGimpHSV_Type;
#define pygimp_hsv_check(v) (pyg_boxed_check((v), GIMP_TYPE_HSV))
PyObject *pygimp_hsv_new(const GimpHSV *hsv);

extern PyTypeObject PyGimpHSL_Type;
#define pygimp_hsl_check(v) (pyg_boxed_check((v), GIMP_TYPE_HSL))
PyObject *pygimp_hsl_new(const GimpHSL *hsl);

extern PyTypeObject PyGimpCMYK_Type;
#define pygimp_cmyk_check(v) (pyg_boxed_check((v), GIMP_TYPE_CMYK))
PyObject *pygimp_cmyk_new(const GimpCMYK *cmyk);

int pygimp_rgb_from_pyobject(PyObject *object, GimpRGB *color);

G_END_DECLS

#endif
