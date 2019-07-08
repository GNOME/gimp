/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_CONVOLVE_H__
#define __GIMP_CONVOLVE_H__


#include "gimpbrushcore.h"


#define GIMP_TYPE_CONVOLVE            (gimp_convolve_get_type ())
#define GIMP_CONVOLVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONVOLVE, GimpConvolve))
#define GIMP_CONVOLVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONVOLVE, GimpConvolveClass))
#define GIMP_IS_CONVOLVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONVOLVE))
#define GIMP_IS_CONVOLVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONVOLVE))
#define GIMP_CONVOLVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONVOLVE, GimpConvolveClass))


typedef struct _GimpConvolveClass GimpConvolveClass;

struct _GimpConvolve
{
  GimpBrushCore  parent_instance;
  gfloat         matrix[9];
  gfloat         matrix_divisor;
};

struct _GimpConvolveClass
{
  GimpBrushCoreClass parent_class;
};


void    gimp_convolve_register (Gimp                      *gimp,
                                GimpPaintRegisterCallback  callback);

GType   gimp_convolve_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_CONVOLVE_H__  */
