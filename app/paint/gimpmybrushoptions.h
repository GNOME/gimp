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

#ifndef  __GIMP_MYBRUSH_OPTIONS_H__
#define  __GIMP_MYBRUSH_OPTIONS_H__


#include "gimppaintoptions.h"


#define GIMP_TYPE_MYBRUSH_OPTIONS            (gimp_mybrush_options_get_type ())
#define GIMP_MYBRUSH_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MYBRUSH_OPTIONS, GimpMybrushOptions))
#define GIMP_MYBRUSH_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MYBRUSH_OPTIONS, GimpMybrushOptionsClass))
#define GIMP_IS_MYBRUSH_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MYBRUSH_OPTIONS))
#define GIMP_IS_MYBRUSH_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MYBRUSH_OPTIONS))
#define GIMP_MYBRUSH_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MYBRUSH_OPTIONS, GimpMybrushOptionsClass))


typedef struct _GimpMybrushOptionsClass GimpMybrushOptionsClass;

struct _GimpMybrushOptions
{
  GimpPaintOptions  parent_instance;

  gdouble           radius;
  gdouble           opaque;
  gdouble           hardness;
  gboolean          eraser;
  gboolean          no_erasing;
};

struct _GimpMybrushOptionsClass
{
  GimpPaintOptionsClass  parent_instance;
};


GType   gimp_mybrush_options_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_MYBRUSH_OPTIONS_H__  */
