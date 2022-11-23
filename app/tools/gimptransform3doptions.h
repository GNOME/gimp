/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_TRANSFORM_3D_OPTIONS_H__
#define __LIGMA_TRANSFORM_3D_OPTIONS_H__


#include "ligmatransformgridoptions.h"


#define LIGMA_TYPE_TRANSFORM_3D_OPTIONS            (ligma_transform_3d_options_get_type ())
#define LIGMA_TRANSFORM_3D_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TRANSFORM_3D_OPTIONS, LigmaTransform3DOptions))
#define LIGMA_TRANSFORM_3D_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TRANSFORM_3D_OPTIONS, LigmaTransform3DOptionsClass))
#define LIGMA_IS_TRANSFORM_3D_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TRANSFORM_3D_OPTIONS))
#define LIGMA_IS_TRANSFORM_3D_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TRANSFORM_3D_OPTIONS))
#define LIGMA_TRANSFORM_3D_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TRANSFORM_3D_OPTIONS, LigmaTransform3DOptionsClass))


typedef struct _LigmaTransform3DOptions      LigmaTransform3DOptions;
typedef struct _LigmaTransform3DOptionsClass LigmaTransform3DOptionsClass;

struct _LigmaTransform3DOptions
{
  LigmaTransformGridOptions  parent_instance;

  LigmaTransform3DMode       mode;
  gboolean                  unified;

  gboolean                  constrain_axis;
  gboolean                  z_axis;
  gboolean                  local_frame;
};

struct _LigmaTransform3DOptionsClass
{
  LigmaTransformGridOptionsClass  parent_class;
};


GType       ligma_transform_3d_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_transform_3d_options_gui      (LigmaToolOptions *tool_options);


#endif /* __LIGMA_TRANSFORM_3D_OPTIONS_H__ */
