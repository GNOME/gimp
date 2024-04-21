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

#ifndef __JPEG_EXPORT_H__
#define __JPEG_EXPORT_H__


extern GimpImage       *orig_image_global;
extern GimpDrawable    *drawable_global;


gboolean    export_image       (GFile                *file,
                                GimpProcedureConfig *config,
                                GimpImage            *image,
                                GimpDrawable         *drawable,
                                GimpImage            *orig_image,
                                gboolean              preview,
                                GError              **error);
gboolean    save_dialog        (GimpProcedure        *procedure,
                                GimpProcedureConfig  *config,
                                GimpDrawable         *drawable,
                                GimpImage            *image);

#endif /* __JPEG_EXPORT_H__ */
