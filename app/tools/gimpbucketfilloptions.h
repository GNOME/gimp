/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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


typedef struct _GimpBucketFillOptions GimpBucketFillOptions;
typedef struct _GimpPaintOptionsClass GimpBucketFillOptionsClass;

struct _GimpBucketFillOptions
{
  GimpPaintOptions    paint_options;

  gboolean            fill_transparent;
  gboolean            fill_transparent_d;
  GtkWidget          *fill_transparent_w;

  gboolean            sample_merged;
  gboolean            sample_merged_d;
  GtkWidget          *sample_merged_w;

  gdouble             threshold;
  GtkObject          *threshold_w;

  GimpBucketFillMode  fill_mode;
  GimpBucketFillMode  fill_mode_d;
  GtkWidget          *fill_mode_w;
};


GType   gimp_bucket_fill_options_get_type (void) G_GNUC_CONST;

void    gimp_bucket_fill_options_gui      (GimpToolOptions *tool_options);


#endif  /*  __GIMP_BUCKET_FILL_OPTIONS_H__  */
