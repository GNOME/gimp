/* GIMP - The GNU Image Manipulation Program
 *
 * gimpoperationcagetransform.h
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#include <gegl-plugin.h>
#include <operation/gegl-operation-composer.h>


#define GIMP_TYPE_OPERATION_CAGE_TRANSFORM            (gimp_operation_cage_transform_get_type ())
#define GIMP_OPERATION_CAGE_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_CAGE_TRANSFORM, GimpOperationCageTransform))
#define GIMP_OPERATION_CAGE_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_CAGE_TRANSFORM, GimpOperationCageTransformClass))
#define GIMP_IS_OPERATION_CAGE_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_CAGE_TRANSFORM))
#define GIMP_IS_OPERATION_CAGE_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_CAGE_TRANSFORM))
#define GIMP_OPERATION_CAGE_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_CAGE_TRANSFORM, GimpOperationCageTransformClass))


typedef struct _GimpOperationCageTransform      GimpOperationCageTransform;
typedef struct _GimpOperationCageTransformClass GimpOperationCageTransformClass;

struct _GimpOperationCageTransform
{
  GeglOperationComposer  parent_instance;

  GimpCageConfig        *config;
  gboolean               fill_plain_color;

  const Babl            *format_coords;
};

struct _GimpOperationCageTransformClass
{
  GeglOperationComposerClass  parent_class;
};


GType   gimp_operation_cage_transform_get_type (void) G_GNUC_CONST;
