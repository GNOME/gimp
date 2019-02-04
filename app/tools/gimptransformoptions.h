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

#ifndef __GIMP_TRANSFORM_OPTIONS_H__
#define __GIMP_TRANSFORM_OPTIONS_H__


#include "core/gimptooloptions.h"


#define GIMP_TYPE_TRANSFORM_OPTIONS            (gimp_transform_options_get_type ())
#define GIMP_TRANSFORM_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSFORM_OPTIONS, GimpTransformOptions))
#define GIMP_TRANSFORM_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSFORM_OPTIONS, GimpTransformOptionsClass))
#define GIMP_IS_TRANSFORM_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSFORM_OPTIONS))
#define GIMP_IS_TRANSFORM_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TRANSFORM_OPTIONS))
#define GIMP_TRANSFORM_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSFORM_OPTIONS, GimpTransformOptionsClass))


typedef struct _GimpTransformOptions      GimpTransformOptions;
typedef struct _GimpTransformOptionsClass GimpTransformOptionsClass;

struct _GimpTransformOptions
{
  GimpToolOptions         parent_instance;

  GimpTransformType       type;
  GimpTransformDirection  direction;
  GimpInterpolationType   interpolation;
  GimpTransformResize     clip;

  /*  options gui  */
  GtkWidget              *type_box;
  GtkWidget              *direction_frame;
};

struct _GimpTransformOptionsClass
{
  GimpToolOptionsClass  parent_class;
};


GType       gimp_transform_options_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_transform_options_gui      (GimpToolOptions *tool_options,
                                             gboolean         direction,
                                             gboolean         interpolation,
                                             gboolean         clipping);


#endif /* __GIMP_TRANSFORM_OPTIONS_H__ */
