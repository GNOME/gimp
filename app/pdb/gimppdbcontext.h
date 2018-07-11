/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimppdbcontext.h
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

#ifndef __GIMP_PDB_CONTEXT_H__
#define __GIMP_PDB_CONTEXT_H__


#include "core/gimpcontext.h"


#define GIMP_TYPE_PDB_CONTEXT            (gimp_pdb_context_get_type ())
#define GIMP_PDB_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PDB_CONTEXT, GimpPDBContext))
#define GIMP_PDB_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PDB_CONTEXT, GimpPDBContextClass))
#define GIMP_IS_PDB_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PDB_CONTEXT))
#define GIMP_IS_PDB_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PDB_CONTEXT))
#define GIMP_PDB_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PDB_CONTEXT, GimpPDBContextClass))


typedef struct _GimpPDBContext      GimpPDBContext;
typedef struct _GimpPDBContextClass GimpPDBContextClass;

struct _GimpPDBContext
{
  GimpContext             parent_instance;

  gboolean                antialias;
  gboolean                feather;
  gdouble                 feather_radius_x;
  gdouble                 feather_radius_y;
  gboolean                sample_merged;
  GimpSelectCriterion     sample_criterion;
  gdouble                 sample_threshold;
  gboolean                sample_transparent;
  gboolean                diagonal_neighbors;

  GimpInterpolationType   interpolation;
  GimpTransformDirection  transform_direction;
  GimpTransformResize     transform_resize;

  GimpContainer          *paint_options_list;
  GimpStrokeOptions      *stroke_options;

  GeglDistanceMetric      distance_metric;
};

struct _GimpPDBContextClass
{
  GimpContextClass  parent_class;
};


GType               gimp_pdb_context_get_type           (void) G_GNUC_CONST;

GimpContext       * gimp_pdb_context_new                (Gimp           *gimp,
                                                         GimpContext    *parent,
                                                         gboolean        set_parent);

GimpContainer     * gimp_pdb_context_get_paint_options_list
                                                        (GimpPDBContext *context);
GimpPaintOptions  * gimp_pdb_context_get_paint_options  (GimpPDBContext *context,
                                                         const gchar    *name);
GList             * gimp_pdb_context_get_brush_options  (GimpPDBContext *context);

GimpStrokeOptions * gimp_pdb_context_get_stroke_options (GimpPDBContext *context);


#endif  /*  __GIMP_PDB_CONTEXT_H__  */
