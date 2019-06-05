/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationoffset.h
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

#ifndef __GIMP_OPERATION_OFFSET_H__
#define __GIMP_OPERATION_OFFSET_H__


#define GIMP_TYPE_OPERATION_OFFSET            (gimp_operation_offset_get_type ())
#define GIMP_OPERATION_OFFSET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_OFFSET, GimpOperationOffset))
#define GIMP_OPERATION_OFFSET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_OFFSET, GimpOperationOffsetClass))
#define GIMP_IS_OPERATION_OFFSET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_OFFSET))
#define GIMP_IS_OPERATION_OFFSET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_OFFSET))
#define GIMP_OPERATION_OFFSET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_OFFSET, GimpOperationOffsetClass))


typedef struct _GimpOperationOffset      GimpOperationOffset;
typedef struct _GimpOperationOffsetClass GimpOperationOffsetClass;

struct _GimpOperationOffset
{
  GeglOperationFilter  parent_instance;

  GimpContext         *context;
  GimpOffsetType       type;
  gint                 x;
  gint                 y;
};

struct _GimpOperationOffsetClass
{
  GeglOperationFilterClass  parent_class;
};


GType   gimp_operation_offset_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_OPERATION_OFFSET_H__ */
