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

#ifndef __LIGMA_DRAWABLE__PREVIEW_H__
#define __LIGMA_DRAWABLE__PREVIEW_H__


/*
 *  virtual functions of LigmaDrawable -- don't call directly
 */
LigmaTempBuf * ligma_drawable_get_new_preview       (LigmaViewable *viewable,
                                                   LigmaContext  *context,
                                                   gint          width,
                                                   gint          height);
GdkPixbuf   * ligma_drawable_get_new_pixbuf        (LigmaViewable *viewable,
                                                   LigmaContext  *context,
                                                   gint          width,
                                                   gint          height);

/*
 *  normal functions (no virtuals)
 */
const Babl  * ligma_drawable_get_preview_format    (LigmaDrawable *drawable);

LigmaTempBuf * ligma_drawable_get_sub_preview       (LigmaDrawable *drawable,
                                                   gint          src_x,
                                                   gint          src_y,
                                                   gint          src_width,
                                                   gint          src_height,
                                                   gint          dest_width,
                                                   gint          dest_height);
GdkPixbuf   * ligma_drawable_get_sub_pixbuf        (LigmaDrawable *drawable,
                                                   gint          src_x,
                                                   gint          src_y,
                                                   gint          src_width,
                                                   gint          src_height,
                                                   gint          dest_width,
                                                   gint          dest_height);

LigmaAsync   * ligma_drawable_get_sub_preview_async (LigmaDrawable *drawable,
                                                   gint          src_x,
                                                   gint          src_y,
                                                   gint          src_width,
                                                   gint          src_height,
                                                   gint          dest_width,
                                                   gint          dest_height);


#endif /* __LIGMA_DRAWABLE__PREVIEW_H__ */
