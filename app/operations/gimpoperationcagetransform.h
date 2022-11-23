/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmaoperationcagetransform.h
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

#ifndef __LIGMA_OPERATION_CAGE_TRANSFORM_H__
#define __LIGMA_OPERATION_CAGE_TRANSFORM_H__


#include <gegl-plugin.h>
#include <operation/gegl-operation-composer.h>


#define LIGMA_TYPE_OPERATION_CAGE_TRANSFORM            (ligma_operation_cage_transform_get_type ())
#define LIGMA_OPERATION_CAGE_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_CAGE_TRANSFORM, LigmaOperationCageTransform))
#define LIGMA_OPERATION_CAGE_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_CAGE_TRANSFORM, LigmaOperationCageTransformClass))
#define LIGMA_IS_OPERATION_CAGE_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_CAGE_TRANSFORM))
#define LIGMA_IS_OPERATION_CAGE_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_CAGE_TRANSFORM))
#define LIGMA_OPERATION_CAGE_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_CAGE_TRANSFORM, LigmaOperationCageTransformClass))


typedef struct _LigmaOperationCageTransform      LigmaOperationCageTransform;
typedef struct _LigmaOperationCageTransformClass LigmaOperationCageTransformClass;

struct _LigmaOperationCageTransform
{
  GeglOperationComposer  parent_instance;

  LigmaCageConfig        *config;
  gboolean               fill_plain_color;

  const Babl            *format_coords;
};

struct _LigmaOperationCageTransformClass
{
  GeglOperationComposerClass  parent_class;
};


GType   ligma_operation_cage_transform_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_CAGE_TRANSFORM_H__ */
