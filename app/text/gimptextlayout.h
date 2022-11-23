/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaText
 * Copyright (C) 2002-2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_TEXT_LAYOUT_H__
#define __LIGMA_TEXT_LAYOUT_H__


#define LIGMA_TYPE_TEXT_LAYOUT    (ligma_text_layout_get_type ())
#define LIGMA_TEXT_LAYOUT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEXT_LAYOUT, LigmaTextLayout))
#define LIGMA_IS_TEXT_LAYOUT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEXT_LAYOUT))


typedef struct _LigmaTextLayoutClass LigmaTextLayoutClass;

struct _LigmaTextLayoutClass
{
  GObjectClass   parent_class;
};


GType            ligma_text_layout_get_type             (void) G_GNUC_CONST;

LigmaTextLayout * ligma_text_layout_new                  (LigmaText       *text,
                                                        gdouble         xres,
                                                        gdouble         yres,
                                                        GError        **error);
gboolean         ligma_text_layout_get_size             (LigmaTextLayout *layout,
                                                        gint           *width,
                                                        gint           *height);
void             ligma_text_layout_get_offsets          (LigmaTextLayout *layout,
                                                        gint           *x,
                                                        gint           *y);
void             ligma_text_layout_get_resolution       (LigmaTextLayout *layout,
                                                        gdouble        *xres,
                                                        gdouble        *yres);

LigmaText       * ligma_text_layout_get_text             (LigmaTextLayout *layout);
PangoLayout    * ligma_text_layout_get_pango_layout     (LigmaTextLayout *layout);

void             ligma_text_layout_get_transform        (LigmaTextLayout *layout,
                                                        cairo_matrix_t *matrix);

void             ligma_text_layout_transform_rect       (LigmaTextLayout *layout,
                                                        PangoRectangle *rect);
void             ligma_text_layout_transform_point      (LigmaTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);
void             ligma_text_layout_transform_distance   (LigmaTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);

void             ligma_text_layout_untransform_rect     (LigmaTextLayout *layout,
                                                        PangoRectangle *rect);
void             ligma_text_layout_untransform_point    (LigmaTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);
void             ligma_text_layout_untransform_distance (LigmaTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);


#endif /* __LIGMA_TEXT_LAYOUT_H__ */
