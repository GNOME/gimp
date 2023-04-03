/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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

#ifndef __PSD_LOAD_H__
#define __PSD_LOAD_H__


GimpImage * load_image  (GFile        *file,
                         gboolean      merged_image_only,
                         gboolean     *resolution_loaded,
                         gboolean     *profile_loaded,
                         PSDSupport   *unsupported_features,
                         GError      **error);

GimpImage * load_image_metadata
                        (GFile        *file,
                         gint          data_length,
                         GimpImage    *image,
                         gboolean      for_layers,
                         gboolean      is_cmyk,
                         PSDSupport   *unsupported_features,
                         GError      **error);

void        load_dialog (const gchar  *title,
                         PSDSupport   *unsupported_features);


#endif /* __PSD_LOAD_H__ */
