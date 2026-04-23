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

#pragma once

#include "gimpcontext.h"


#define GIMP_TYPE_FILL_OPTIONS (gimp_fill_options_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpFillOptions,
                          gimp_fill_options,
                          GIMP, FILL_OPTIONS,
                          GimpContext)


struct _GimpFillOptionsClass
{
  GimpContextClass  parent_class;
};


GimpFillOptions * gimp_fill_options_new              (Gimp                *gimp,
                                                      GimpContext         *context,
                                                      gboolean             use_context_color);

GimpFillStyle     gimp_fill_options_get_style        (GimpFillOptions     *options);
void              gimp_fill_options_set_style        (GimpFillOptions     *options,
                                                      GimpFillStyle        style);

GimpCustomStyle   gimp_fill_options_get_custom_style (GimpFillOptions     *options);
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

void              gimp_fill_options_enable_color_history
                                                     (GimpFillOptions     *options,
                                                      gboolean             enable);
