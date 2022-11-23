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

#ifndef __LIGMA_TRANSFORM_GRID_OPTIONS_H__
#define __LIGMA_TRANSFORM_GRID_OPTIONS_H__


#include "ligmatransformoptions.h"


#define LIGMA_TYPE_TRANSFORM_GRID_OPTIONS            (ligma_transform_grid_options_get_type ())
#define LIGMA_TRANSFORM_GRID_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TRANSFORM_GRID_OPTIONS, LigmaTransformGridOptions))
#define LIGMA_TRANSFORM_GRID_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TRANSFORM_GRID_OPTIONS, LigmaTransformGridOptionsClass))
#define LIGMA_IS_TRANSFORM_GRID_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TRANSFORM_GRID_OPTIONS))
#define LIGMA_IS_TRANSFORM_GRID_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TRANSFORM_GRID_OPTIONS))
#define LIGMA_TRANSFORM_GRID_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TRANSFORM_GRID_OPTIONS, LigmaTransformGridOptionsClass))


typedef struct _LigmaTransformGridOptions      LigmaTransformGridOptions;
typedef struct _LigmaTransformGridOptionsClass LigmaTransformGridOptionsClass;

struct _LigmaTransformGridOptions
{
  LigmaTransformOptions  parent_instance;

  gboolean              direction_linked;
  gboolean              show_preview;
  gboolean              composited_preview;
  gboolean              synchronous_preview;
  gdouble               preview_opacity;
  LigmaGuidesType        grid_type;
  gint                  grid_size;
  gboolean              constrain_move;
  gboolean              constrain_scale;
  gboolean              constrain_rotate;
  gboolean              constrain_shear;
  gboolean              constrain_perspective;
  gboolean              frompivot_scale;
  gboolean              frompivot_shear;
  gboolean              frompivot_perspective;
  gboolean              cornersnap;
  gboolean              fixedpivot;

  /*  options gui  */
  GtkWidget            *direction_chain_button;
};

struct _LigmaTransformGridOptionsClass
{
  LigmaTransformOptionsClass  parent_class;
};


GType       ligma_transform_grid_options_get_type     (void) G_GNUC_CONST;

GtkWidget * ligma_transform_grid_options_gui          (LigmaToolOptions          *tool_options);

gboolean    ligma_transform_grid_options_show_preview (LigmaTransformGridOptions *options);


#endif /* __LIGMA_TRANSFORM_GRID_OPTIONS_H__ */
