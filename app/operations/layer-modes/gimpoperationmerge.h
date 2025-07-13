/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationmerge.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2017 Ell
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

#pragma once

#include "gimpoperationlayermode.h"


#define GIMP_TYPE_OPERATION_MERGE            (gimp_operation_merge_get_type ())
#define GIMP_OPERATION_MERGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_MERGE, GimpOperationMerge))
#define GIMP_OPERATION_MERGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_MERGE, GimpOperationMergeClass))
#define GIMP_IS_OPERATION_MERGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_MERGE))
#define GIMP_IS_OPERATION_MERGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_MERGE))
#define GIMP_OPERATION_MERGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_MERGE, GimpOperationMergeClass))


typedef struct _GimpOperationMerge      GimpOperationMerge;
typedef struct _GimpOperationMergeClass GimpOperationMergeClass;

struct _GimpOperationMerge
{
  GimpOperationLayerMode  parent_instance;
};

struct _GimpOperationMergeClass
{
  GimpOperationLayerModeClass  parent_class;
};


GType   gimp_operation_merge_get_type (void) G_GNUC_CONST;
