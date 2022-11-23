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

#ifndef __LIGMA_CLIPBOARD_H__
#define __LIGMA_CLIPBOARD_H__


void         ligma_clipboard_init       (Ligma        *ligma);
void         ligma_clipboard_exit       (Ligma        *ligma);

gboolean     ligma_clipboard_has_image  (Ligma        *ligma);
gboolean     ligma_clipboard_has_buffer (Ligma        *ligma);
gboolean     ligma_clipboard_has_svg    (Ligma        *ligma);
gboolean     ligma_clipboard_has_curve  (Ligma        *ligma);

LigmaObject * ligma_clipboard_get_object (Ligma        *ligma);

LigmaImage  * ligma_clipboard_get_image  (Ligma        *ligma);
LigmaBuffer * ligma_clipboard_get_buffer (Ligma        *ligma);
gchar      * ligma_clipboard_get_svg    (Ligma        *ligma,
                                        gsize       *svg_length);
LigmaCurve  * ligma_clipboard_get_curve  (Ligma        *ligma);

void         ligma_clipboard_set_image  (Ligma        *ligma,
                                        LigmaImage   *image);
void         ligma_clipboard_set_buffer (Ligma        *ligma,
                                        LigmaBuffer  *buffer);
void         ligma_clipboard_set_svg    (Ligma        *ligma,
                                        const gchar *svg);
void         ligma_clipboard_set_text   (Ligma        *ligma,
                                        const gchar *text);
void         ligma_clipboard_set_curve  (Ligma        *ligma,
                                        LigmaCurve   *curve);


#endif /* __LIGMA_CLIPBOARD_H__ */
