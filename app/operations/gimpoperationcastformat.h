/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcastformat.h
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_OPERATION_CAST_FORMAT_H__
#define __GIMP_OPERATION_CAST_FORMAT_H__


#include <gegl-plugin.h>


#define GIMP_TYPE_OPERATION_CAST_FORMAT            (gimp_operation_cast_format_get_type ())
#define GIMP_OPERATION_CAST_FORMAT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_CAST_FORMAT, GimpOperationCastFormat))
#define GIMP_OPERATION_CAST_FORMAT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_CAST_FORMAT, GimpOperationCastFormatClass))
#define GEGL_IS_OPERATION_CAST_FORMAT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_CAST_FORMAT))
#define GEGL_IS_OPERATION_CAST_FORMAT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_CAST_FORMAT))
#define GIMP_OPERATION_CAST_FORMAT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_CAST_FORMAT, GimpOperationCastFormatClass))


typedef struct _GimpOperationCastFormat      GimpOperationCastFormat;
typedef struct _GimpOperationCastFormatClass GimpOperationCastFormatClass;

struct _GimpOperationCastFormat
{
  GeglOperationFilter  parent_instance;

  const Babl          *input_format;
  const Babl          *output_format;
};

struct _GimpOperationCastFormatClass
{
  GeglOperationFilterClass  parent_class;
};


GType   gimp_operation_cast_format_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_OPERATION_CAST_FORMAT_H__ */
