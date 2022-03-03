/* GIMP - The GNU Image Manipulation Program
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

#ifndef  __GIMP_BUCKET_FILL_OPTIONS_H__
#define  __GIMP_BUCKET_FILL_OPTIONS_H__


#include "paint/gimppaintoptions.h"


#define GIMP_TYPE_BUCKET_FILL_OPTIONS            (gimp_bucket_fill_options_get_type ())
#define GIMP_BUCKET_FILL_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BUCKET_FILL_OPTIONS, GimpBucketFillOptions))
#define GIMP_BUCKET_FILL_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BUCKET_FILL_OPTIONS, GimpBucketFillOptionsClass))
#define GIMP_IS_BUCKET_FILL_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BUCKET_FILL_OPTIONS))
#define GIMP_IS_BUCKET_FILL_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BUCKET_FILL_OPTIONS))
#define GIMP_BUCKET_FILL_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BUCKET_FILL_OPTIONS, GimpBucketFillOptionsClass))


typedef struct _GimpBucketFillOptions        GimpBucketFillOptions;
typedef struct _GimpBucketFillOptionsPrivate GimpBucketFillOptionsPrivate;
typedef struct _GimpPaintOptionsClass        GimpBucketFillOptionsClass;

struct _GimpBucketFillOptions
{
  GimpPaintOptions              paint_options;

  GimpBucketFillMode            fill_mode;
  GimpBucketFillArea            fill_area;
  gboolean                      fill_transparent;
  gboolean                      sample_merged;
  gboolean                      diagonal_neighbors;
  gboolean                      antialias;
  gboolean                      feather;
  gdouble                       feather_radius;
  gdouble                       threshold;

  GtkWidget                    *line_art_busy_box;
  GimpLineArtSource             line_art_source;
  gdouble                       line_art_threshold;
  gint                          line_art_max_grow;

  gboolean                      line_art_automatic_closure;
  gint                          line_art_max_gap_length;

  gboolean                      line_art_stroke;
  gchar                        *line_art_stroke_tool;
  GimpStrokeOptions            *stroke_options;

  gboolean                      fill_as_line_art;
  gdouble                       fill_as_line_art_threshold;

  GimpSelectCriterion           fill_criterion;

  GimpBucketFillOptionsPrivate *priv;
};


GType       gimp_bucket_fill_options_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_bucket_fill_options_gui      (GimpToolOptions *tool_options);


#endif  /*  __GIMP_BUCKET_FILL_OPTIONS_H__  */
