/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#ifndef __ICO_LOAD_H__
#define __ICO_LOAD_H__


GimpImage * ico_load_image           (GFile         *file,
                                      gint32        *file_offset,
                                      gint           frame_num,
                                      GError       **error);
GimpImage * ani_load_image           (GFile         *file,
                                      gboolean       load_thumb,
                                      gint          *width,
                                      gint          *height,
                                      GError       **error);
GimpImage * ico_load_thumbnail_image (GFile         *file,
                                      gint          *width,
                                      gint          *height,
                                      gint32         file_offset,
                                      GError       **error);

gint        ico_get_bit_from_data    (const guint8  *data,
                                      gint           line_width,
                                      gint           bit);
gint        ico_get_nibble_from_data (const guint8  *data,
                                      gint           line_width,
                                      gint           nibble);
gint        ico_get_byte_from_data   (const guint8  *data,
                                      gint           line_width,
                                      gint           byte);


#endif /* __ICO_LOAD_H__ */
