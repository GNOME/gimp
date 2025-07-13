/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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


#define GIMP_TYPE_TEXT_LAYOUT    (gimp_text_layout_get_type ())
#define GIMP_TEXT_LAYOUT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT_LAYOUT, GimpTextLayout))
#define GIMP_IS_TEXT_LAYOUT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT_LAYOUT))


typedef struct _GimpTextLayoutClass GimpTextLayoutClass;

struct _GimpTextLayoutClass
{
  GObjectClass   parent_class;
};


GType            gimp_text_layout_get_type             (void) G_GNUC_CONST;

GimpTextLayout * gimp_text_layout_new                  (GimpText       *text,
                                                        GimpImage      *target_image,
                                                        gdouble         xres,
                                                        gdouble         yres,
                                                        GError        **error);

const Babl     * gimp_text_layout_get_space            (GimpTextLayout *layout);
const GimpTRCType gimp_text_layout_get_trc             (GimpTextLayout *layout);
const Babl *     gimp_text_layout_get_format           (GimpTextLayout *layout,
                                                        const gchar    *babl_type);

gboolean         gimp_text_layout_get_size             (GimpTextLayout *layout,
                                                        gint           *width,
                                                        gint           *height);
void             gimp_text_layout_get_offsets          (GimpTextLayout *layout,
                                                        gint           *x,
                                                        gint           *y);
void             gimp_text_layout_get_resolution       (GimpTextLayout *layout,
                                                        gdouble        *xres,
                                                        gdouble        *yres);

GimpText       * gimp_text_layout_get_text             (GimpTextLayout *layout);
PangoLayout    * gimp_text_layout_get_pango_layout     (GimpTextLayout *layout);

void             gimp_text_layout_get_transform        (GimpTextLayout *layout,
                                                        cairo_matrix_t *matrix);

void             gimp_text_layout_transform_rect       (GimpTextLayout *layout,
                                                        PangoRectangle *rect);
void             gimp_text_layout_transform_point      (GimpTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);
void             gimp_text_layout_transform_distance   (GimpTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);

void             gimp_text_layout_untransform_rect     (GimpTextLayout *layout,
                                                        PangoRectangle *rect);
void             gimp_text_layout_untransform_point    (GimpTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);
void             gimp_text_layout_untransform_distance (GimpTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);
