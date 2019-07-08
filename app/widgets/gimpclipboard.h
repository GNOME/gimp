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

#ifndef __GIMP_CLIPBOARD_H__
#define __GIMP_CLIPBOARD_H__


void         gimp_clipboard_init       (Gimp        *gimp);
void         gimp_clipboard_exit       (Gimp        *gimp);

gboolean     gimp_clipboard_has_image  (Gimp        *gimp);
gboolean     gimp_clipboard_has_buffer (Gimp        *gimp);
gboolean     gimp_clipboard_has_svg    (Gimp        *gimp);
gboolean     gimp_clipboard_has_curve  (Gimp        *gimp);

GimpObject * gimp_clipboard_get_object (Gimp        *gimp);

GimpImage  * gimp_clipboard_get_image  (Gimp        *gimp);
GimpBuffer * gimp_clipboard_get_buffer (Gimp        *gimp);
gchar      * gimp_clipboard_get_svg    (Gimp        *gimp,
                                        gsize       *svg_length);
GimpCurve  * gimp_clipboard_get_curve  (Gimp        *gimp);

void         gimp_clipboard_set_image  (Gimp        *gimp,
                                        GimpImage   *image);
void         gimp_clipboard_set_buffer (Gimp        *gimp,
                                        GimpBuffer  *buffer);
void         gimp_clipboard_set_svg    (Gimp        *gimp,
                                        const gchar *svg);
void         gimp_clipboard_set_text   (Gimp        *gimp,
                                        const gchar *text);
void         gimp_clipboard_set_curve  (Gimp        *gimp,
                                        GimpCurve   *curve);


#endif /* __GIMP_CLIPBOARD_H__ */
