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

#ifndef __GIMP_EDIT_H__
#define __GIMP_EDIT_H__


GimpObject  * gimp_edit_cut                (GimpImage       *image,
                                            GimpDrawable    *drawable,
                                            GimpContext     *context,
                                            GError         **error);
GimpObject  * gimp_edit_copy               (GimpImage       *image,
                                            GimpDrawable    *drawable,
                                            GimpContext     *context,
                                            GError         **error);
GimpBuffer  * gimp_edit_copy_visible       (GimpImage       *image,
                                            GimpContext     *context,
                                            GError         **error);

GimpLayer   * gimp_edit_paste              (GimpImage       *image,
                                            GimpDrawable    *drawable,
                                            GimpObject      *paste,
                                            GimpPasteType    paste_type,
                                            gint             viewport_x,
                                            gint             viewport_y,
                                            gint             viewport_width,
                                            gint             viewport_height);
GimpImage   * gimp_edit_paste_as_new_image (Gimp            *gimp,
                                            GimpObject      *paste);

const gchar * gimp_edit_named_cut          (GimpImage       *image,
                                            const gchar     *name,
                                            GimpDrawable    *drawable,
                                            GimpContext     *context,
                                            GError         **error);
const gchar * gimp_edit_named_copy         (GimpImage       *image,
                                            const gchar     *name,
                                            GimpDrawable    *drawable,
                                            GimpContext     *context,
                                            GError         **error);
const gchar * gimp_edit_named_copy_visible (GimpImage       *image,
                                            const gchar     *name,
                                            GimpContext     *context,
                                            GError         **error);


#endif  /*  __GIMP_EDIT_H__  */
