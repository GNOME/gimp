/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpfilloptions.h
 * Copyright (C) 2003 Simon Budig
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

#ifndef __GIMP_FILL_OPTIONS_H__
#define __GIMP_FILL_OPTIONS_H__


#include "gimpcontext.h"


#define GIMP_TYPE_FILL_OPTIONS            (gimp_fill_options_get_type ())
#define GIMP_FILL_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILL_OPTIONS, GimpFillOptions))
#define GIMP_FILL_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILL_OPTIONS, GimpFillOptionsClass))
#define GIMP_IS_FILL_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILL_OPTIONS))
#define GIMP_IS_FILL_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILL_OPTIONS))
#define GIMP_FILL_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILL_OPTIONS, GimpFillOptionsClass))


typedef struct _GimpFillOptionsClass GimpFillOptionsClass;

struct _GimpFillOptions
{
  GimpContext  parent_instance;
};

struct _GimpFillOptionsClass
{
  GimpContextClass  parent_class;
};


GType             gimp_fill_options_get_type         (void) G_GNUC_CONST;

GimpFillOptions * gimp_fill_options_new              (Gimp                *gimp,
                                                      GimpContext         *context,
                                                      gboolean             use_context_color);

GimpFillStyle     gimp_fill_options_get_style        (GimpFillOptions     *options);
void              gimp_fill_options_set_style        (GimpFillOptions     *options,
                                                      GimpFillStyle        style);

GimpCustomStyle     gimp_fill_options_get_custom_style
                                                     (GimpFillOptions     *options);
void              gimp_fill_options_set_custom_style (GimpFillOptions     *options,
                                                      GimpCustomStyle      custom_style);

gboolean          gimp_fill_options_get_antialias    (GimpFillOptions     *options);
void              gimp_fill_options_set_antialias    (GimpFillOptions     *options,
                                                      gboolean             antialias);

gboolean          gimp_fill_options_get_feather      (GimpFillOptions     *options,
                                                      gdouble             *radius);
void              gimp_fill_options_set_feather      (GimpFillOptions     *options,
                                                      gboolean             feather,
                                                      gdouble              radius);

gboolean          gimp_fill_options_set_by_fill_type (GimpFillOptions     *options,
                                                      GimpContext         *context,
                                                      GimpFillType         fill_type,
                                                      GError             **error);
gboolean          gimp_fill_options_set_by_fill_mode (GimpFillOptions     *options,
                                                      GimpContext         *context,
                                                      GimpBucketFillMode   fill_mode,
                                                      GError             **error);

const gchar     * gimp_fill_options_get_undo_desc    (GimpFillOptions     *options);

const Babl      * gimp_fill_options_get_format       (GimpFillOptions     *options,
                                                      GimpDrawable        *drawable);

GeglBuffer      * gimp_fill_options_create_buffer    (GimpFillOptions     *options,
                                                      GimpDrawable        *drawable,
                                                      const GeglRectangle *rect,
                                                      gint                 pattern_offset_x,
                                                      gint                 pattern_offset_y);
void              gimp_fill_options_fill_buffer      (GimpFillOptions     *options,
                                                      GimpDrawable        *drawable,
                                                      GeglBuffer          *buffer,
                                                      gint                 pattern_offset_x,
                                                      gint                 pattern_offset_y);


#endif /* __GIMP_FILL_OPTIONS_H__ */
