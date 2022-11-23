/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmapdbcontext.h
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

#ifndef __LIGMA_PDB_CONTEXT_H__
#define __LIGMA_PDB_CONTEXT_H__


#include "core/ligmacontext.h"


#define LIGMA_TYPE_PDB_CONTEXT            (ligma_pdb_context_get_type ())
#define LIGMA_PDB_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PDB_CONTEXT, LigmaPDBContext))
#define LIGMA_PDB_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PDB_CONTEXT, LigmaPDBContextClass))
#define LIGMA_IS_PDB_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PDB_CONTEXT))
#define LIGMA_IS_PDB_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PDB_CONTEXT))
#define LIGMA_PDB_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PDB_CONTEXT, LigmaPDBContextClass))


typedef struct _LigmaPDBContext      LigmaPDBContext;
typedef struct _LigmaPDBContextClass LigmaPDBContextClass;

struct _LigmaPDBContext
{
  LigmaContext             parent_instance;

  gboolean                antialias;
  gboolean                feather;
  gdouble                 feather_radius_x;
  gdouble                 feather_radius_y;
  gboolean                sample_merged;
  LigmaSelectCriterion     sample_criterion;
  gdouble                 sample_threshold;
  gboolean                sample_transparent;
  gboolean                diagonal_neighbors;

  LigmaInterpolationType   interpolation;
  LigmaTransformDirection  transform_direction;
  LigmaTransformResize     transform_resize;

  LigmaContainer          *paint_options_list;
  LigmaStrokeOptions      *stroke_options;

  GeglDistanceMetric      distance_metric;
};

struct _LigmaPDBContextClass
{
  LigmaContextClass  parent_class;
};


GType               ligma_pdb_context_get_type           (void) G_GNUC_CONST;

LigmaContext       * ligma_pdb_context_new                (Ligma           *ligma,
                                                         LigmaContext    *parent,
                                                         gboolean        set_parent);

LigmaContainer     * ligma_pdb_context_get_paint_options_list
                                                        (LigmaPDBContext *context);
LigmaPaintOptions  * ligma_pdb_context_get_paint_options  (LigmaPDBContext *context,
                                                         const gchar    *name);
GList             * ligma_pdb_context_get_brush_options  (LigmaPDBContext *context);

LigmaStrokeOptions * ligma_pdb_context_get_stroke_options (LigmaPDBContext *context);


#endif  /*  __LIGMA_PDB_CONTEXT_H__  */
