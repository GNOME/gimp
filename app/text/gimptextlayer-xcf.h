/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_TEXT_LAYER_XCF_H__
#define __GIMP_TEXT_LAYER_XCF_H__


gboolean  gimp_text_layer_xcf_load_hack    (GimpLayer     **layer);

void      gimp_text_layer_xcf_save_prepare (GimpTextLayer  *text_layer);

guint32   gimp_text_layer_get_xcf_flags    (GimpTextLayer  *text_layer);
void      gimp_text_layer_set_xcf_flags    (GimpTextLayer  *text_layer,
                                            guint32         flags);


#endif /* __GIMP_TEXT_LAYER_XCF_H__ */
