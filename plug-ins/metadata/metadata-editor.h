/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2016, 2017 Ben Touchette
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

#ifndef __METADATA_EDITOR_H__
#define __METADATA_EDITOR_H__

void        metadata_editor_write_callback      (GtkWidget           *dialog,
                                                 metadata_editor     *meta_info,
                                                 GimpImage           *image);

GtkWidget * metadata_editor_get_widget          (metadata_editor     *meta_info,
                                                 const gchar         *name);

#endif /* __METADATA_EDITOR_H__ */
