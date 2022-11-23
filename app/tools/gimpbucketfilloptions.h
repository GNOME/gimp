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

#ifndef  __LIGMA_BUCKET_FILL_OPTIONS_H__
#define  __LIGMA_BUCKET_FILL_OPTIONS_H__


#include "paint/ligmapaintoptions.h"


#define LIGMA_TYPE_BUCKET_FILL_OPTIONS            (ligma_bucket_fill_options_get_type ())
#define LIGMA_BUCKET_FILL_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BUCKET_FILL_OPTIONS, LigmaBucketFillOptions))
#define LIGMA_BUCKET_FILL_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BUCKET_FILL_OPTIONS, LigmaBucketFillOptionsClass))
#define LIGMA_IS_BUCKET_FILL_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BUCKET_FILL_OPTIONS))
#define LIGMA_IS_BUCKET_FILL_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BUCKET_FILL_OPTIONS))
#define LIGMA_BUCKET_FILL_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BUCKET_FILL_OPTIONS, LigmaBucketFillOptionsClass))


typedef struct _LigmaBucketFillOptions        LigmaBucketFillOptions;
typedef struct _LigmaBucketFillOptionsPrivate LigmaBucketFillOptionsPrivate;
typedef struct _LigmaPaintOptionsClass        LigmaBucketFillOptionsClass;

struct _LigmaBucketFillOptions
{
  LigmaPaintOptions              paint_options;

  LigmaBucketFillMode            fill_mode;
  LigmaBucketFillArea            fill_area;
  gboolean                      fill_transparent;
  gboolean                      sample_merged;
  gboolean                      diagonal_neighbors;
  gboolean                      antialias;
  gboolean                      feather;
  gdouble                       feather_radius;
  gdouble                       threshold;

  GtkWidget                    *line_art_busy_box;
  LigmaLineArtSource             line_art_source;
  gdouble                       line_art_threshold;
  gint                          line_art_max_grow;

  gboolean                      line_art_automatic_closure;
  gint                          line_art_max_gap_length;

  gboolean                      line_art_stroke;
  gchar                        *line_art_stroke_tool;
  LigmaStrokeOptions            *stroke_options;

  gboolean                      fill_as_line_art;
  gdouble                       fill_as_line_art_threshold;

  LigmaSelectCriterion           fill_criterion;

  LigmaBucketFillOptionsPrivate *priv;
};


GType       ligma_bucket_fill_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_bucket_fill_options_gui      (LigmaToolOptions *tool_options);


#endif  /*  __LIGMA_BUCKET_FILL_OPTIONS_H__  */
