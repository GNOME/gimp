/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationbehind.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_OPERATION_BEHIND_H__
#define __GIMP_OPERATION_BEHIND_H__


#include "gimpoperationlayermode.h"


#define GIMP_TYPE_OPERATION_BEHIND            (gimp_operation_behind_get_type ())
#define GIMP_OPERATION_BEHIND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_BEHIND, GimpOperationBehind))
#define GIMP_OPERATION_BEHIND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_BEHIND, GimpOperationBehindClass))
#define GIMP_IS_OPERATION_BEHIND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_BEHIND))
#define GIMP_IS_OPERATION_BEHIND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_BEHIND))
#define GIMP_OPERATION_BEHIND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_BEHIND, GimpOperationBehindClass))


typedef struct _GimpOperationBehind      GimpOperationBehind;
typedef struct _GimpOperationBehindClass GimpOperationBehindClass;

struct _GimpOperationBehind
{
  GimpOperationLayerMode  parent_instance;
};

struct _GimpOperationBehindClass
{
  GimpOperationLayerModeClass  parent_class;
};


GType    gimp_operation_behind_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_OPERATION_BEHIND_H__ */
