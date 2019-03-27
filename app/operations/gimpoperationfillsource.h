/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationfillsource.h
 * Copyright (C) 2019 Ell
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

#ifndef __GIMP_OPERATION_FILL_SOURCE_H__
#define __GIMP_OPERATION_FILL_SOURCE_H__


#define GIMP_TYPE_OPERATION_FILL_SOURCE            (gimp_operation_fill_source_get_type ())
#define GIMP_OPERATION_FILL_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_FILL_SOURCE, GimpOperationFillSource))
#define GIMP_OPERATION_FILL_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_FILL_SOURCE, GimpOperationFillSourceClass))
#define GIMP_IS_OPERATION_FILL_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_FILL_SOURCE))
#define GIMP_IS_OPERATION_FILL_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_FILL_SOURCE))
#define GIMP_OPERATION_FILL_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_FILL_SOURCE, GimpOperationFillSourceClass))


typedef struct _GimpOperationFillSource      GimpOperationFillSource;
typedef struct _GimpOperationFillSourceClass GimpOperationFillSourceClass;

struct _GimpOperationFillSource
{
  GeglOperationSource  parent_instance;

  GimpFillOptions     *options;
  GimpDrawable        *drawable;
  gint                 pattern_offset_x;
  gint                 pattern_offset_y;
};

struct _GimpOperationFillSourceClass
{
  GeglOperationSourceClass  parent_class;
};


GType   gimp_operation_fill_source_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_OPERATION_FILL_SOURCE_H__ */
