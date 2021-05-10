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

#ifndef __PSD_IMAGE_RES_LOAD_H__
#define __PSD_IMAGE_RES_LOAD_H__


gint  get_image_resource_header (PSDimageres   *res_a,
                                 GInputStream  *input,
                                 GError       **error);

gint  load_image_resource       (PSDimageres   *res_a,
                                 GimpImage     *image,
                                 PSDimage      *img_a,
                                 GInputStream  *input,
                                 gboolean      *resolution_loaded,
                                 gboolean      *profile_loaded,
                                 GError       **error);

gint  load_thumbnail_resource   (PSDimageres   *res_a,
                                 GimpImage     *image,
                                 GFile         *file,
                                 GInputStream  *input,
                                 GError       **error);


#endif /* __PSD_IMAGE_RES_LOAD_H__ */
