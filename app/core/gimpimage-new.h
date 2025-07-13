/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#pragma once


GimpTemplate * gimp_image_new_get_last_template (Gimp            *gimp,
                                                 GimpImage       *image);
void           gimp_image_new_set_last_template (Gimp            *gimp,
                                                 GimpTemplate    *template);

GimpImage    * gimp_image_new_from_template     (Gimp            *gimp,
                                                 GimpTemplate    *template,
                                                 GimpContext     *context);
GimpImage    * gimp_image_new_from_drawable     (Gimp            *gimp,
                                                 GimpDrawable    *drawable);
GimpImage    * gimp_image_new_from_drawables    (Gimp            *gimp,
                                                 GList           *drawables,
                                                 gboolean         copy_selection,
                                                 gboolean         tag_copies);
GimpImage    * gimp_image_new_from_component    (Gimp            *gimp,
                                                 GimpImage       *image,
                                                 GimpChannelType  component);
GimpImage    * gimp_image_new_from_buffer       (Gimp            *gimp,
                                                 GimpBuffer      *buffer);
GimpImage    * gimp_image_new_from_pixbuf       (Gimp            *gimp,
                                                 GdkPixbuf       *pixbuf,
                                                 const gchar     *layer_name);
