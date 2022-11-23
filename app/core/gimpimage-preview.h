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

#ifndef __LIGMA_IMAGE_PREVIEW_H__
#define __LIGMA_IMAGE_PREVIEW_H__


const Babl  * ligma_image_get_preview_format (LigmaImage    *image);


/*
 *  virtual functions of LigmaImage -- don't call directly
 */

void          ligma_image_get_preview_size   (LigmaViewable *viewable,
                                             gint          size,
                                             gboolean      is_popup,
                                             gboolean      dot_for_dot,
                                             gint         *width,
                                             gint         *height);
gboolean      ligma_image_get_popup_size     (LigmaViewable *viewable,
                                             gint          width,
                                             gint          height,
                                             gboolean      dot_for_dot,
                                             gint         *popup_width,
                                             gint         *popup_height);
LigmaTempBuf * ligma_image_get_new_preview    (LigmaViewable *viewable,
                                             LigmaContext  *context,
                                             gint          width,
                                             gint          height);
GdkPixbuf   * ligma_image_get_new_pixbuf     (LigmaViewable *viewable,
                                             LigmaContext  *context,
                                             gint          width,
                                             gint          height);


#endif /* __LIGMA_IMAGE_PREVIEW_H__ */
