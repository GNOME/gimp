/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationprofiletransform.h
 * Copyright (C) 2016 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OPERATION_PROFILE_TRANSFORM_H__
#define __LIGMA_OPERATION_PROFILE_TRANSFORM_H__


#include <gegl-plugin.h>
#include <operation/gegl-operation-point-filter.h>


#define LIGMA_TYPE_OPERATION_PROFILE_TRANSFORM            (ligma_operation_profile_transform_get_type ())
#define LIGMA_OPERATION_PROFILE_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_PROFILE_TRANSFORM, LigmaOperationProfileTransform))
#define LIGMA_OPERATION_PROFILE_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_PROFILE_TRANSFORM, LigmaOperationProfileTransformClass))
#define LIGMA_IS_OPERATION_PROFILE_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_PROFILE_TRANSFORM))
#define LIGMA_IS_OPERATION_PROFILE_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_PROFILE_TRANSFORM))
#define LIGMA_OPERATION_PROFILE_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_PROFILE_TRANSFORM, LigmaOperationProfileTransformClass))


typedef struct _LigmaOperationProfileTransform      LigmaOperationProfileTransform;
typedef struct _LigmaOperationProfileTransformClass LigmaOperationProfileTransformClass;

struct _LigmaOperationProfileTransform
{
  GeglOperationPointFilter  parent_instance;

  LigmaColorProfile         *src_profile;
  const Babl               *src_format;

  LigmaColorProfile         *dest_profile;
  const Babl               *dest_format;

  LigmaColorRenderingIntent  rendering_intent;
  gboolean                  black_point_compensation;

  LigmaColorTransform       *transform;
};

struct _LigmaOperationProfileTransformClass
{
  GeglOperationPointFilterClass  parent_class;
};


GType   ligma_operation_profile_transform_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_PROFILE_TRANSFORM_H__ */
