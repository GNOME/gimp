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

#ifndef __GIMP_CONVOLVE_OPTIONS_H__
#define __GIMP_CONVOLVE_OPTIONS_H__


#include "gimppaintoptions.h"


#define GIMP_TYPE_CONVOLVE_OPTIONS            (gimp_convolve_options_get_type ())
#define GIMP_CONVOLVE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONVOLVE_OPTIONS, GimpConvolveOptions))
#define GIMP_CONVOLVE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONVOLVE_OPTIONS, GimpConvolveOptionsClass))
#define GIMP_IS_CONVOLVE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONVOLVE_OPTIONS))
#define GIMP_IS_CONVOLVE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONVOLVE_OPTIONS))
#define GIMP_CONVOLVE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONVOLVE_OPTIONS, GimpConvolveOptionsClass))


typedef struct _GimpConvolveOptionsClass GimpConvolveOptionsClass;

struct _GimpConvolveOptions
{
  GimpPaintOptions  parent_instance;

  GimpConvolveType  type;
  gdouble           rate;
};

struct _GimpConvolveOptionsClass
{
  GimpPaintOptionsClass  parent_class;
};


GType   gimp_convolve_options_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_CONVOLVE_OPTIONS_H__  */
