/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationprofiletransform.h
 * Copyright (C) 2016 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_OPERATION_PROFILE_TRANSFORM_H__
#define __GIMP_OPERATION_PROFILE_TRANSFORM_H__


#include <gegl-plugin.h>
#include <operation/gegl-operation-point-filter.h>


#define GIMP_TYPE_OPERATION_PROFILE_TRANSFORM            (gimp_operation_profile_transform_get_type ())
#define GIMP_OPERATION_PROFILE_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_PROFILE_TRANSFORM, GimpOperationProfileTransform))
#define GIMP_OPERATION_PROFILE_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_PROFILE_TRANSFORM, GimpOperationProfileTransformClass))
#define GIMP_IS_OPERATION_PROFILE_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_PROFILE_TRANSFORM))
#define GIMP_IS_OPERATION_PROFILE_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_PROFILE_TRANSFORM))
#define GIMP_OPERATION_PROFILE_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_PROFILE_TRANSFORM, GimpOperationProfileTransformClass))


typedef struct _GimpOperationProfileTransform      GimpOperationProfileTransform;
typedef struct _GimpOperationProfileTransformClass GimpOperationProfileTransformClass;

struct _GimpOperationProfileTransform
{
  GeglOperationPointFilter  parent_instance;

  GimpColorProfile         *src_profile;
  const Babl               *src_format;

  GimpColorProfile         *dest_profile;
  const Babl               *dest_format;

  GimpColorRenderingIntent  rendering_intent;
  gboolean                  black_point_compensation;

  GimpColorTransform       *transform;
};

struct _GimpOperationProfileTransformClass
{
  GeglOperationPointFilterClass  parent_class;
};


GType   gimp_operation_profile_transform_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_OPERATION_PROFILE_TRANSFORM_H__ */
