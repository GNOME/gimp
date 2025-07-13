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

#pragma once


/*  public guide adding API
 */
GimpGuide * gimp_image_add_hguide     (GimpImage *image,
                                       gint       position,
                                       gboolean   push_undo);
GimpGuide * gimp_image_add_vguide     (GimpImage *image,
                                       gint       position,
                                       gboolean   push_undo);

/*  internal guide adding API, does not check the guide's position and
 *  is publicly declared only to be used from undo
 */
void        gimp_image_add_guide      (GimpImage *image,
                                       GimpGuide *guide,
                                       gint       position);

void        gimp_image_remove_guide   (GimpImage *image,
                                       GimpGuide *guide,
                                       gboolean   push_undo);
void        gimp_image_move_guide     (GimpImage *image,
                                       GimpGuide *guide,
                                       gint       position,
                                       gboolean   push_undo);

GList     * gimp_image_get_guides     (GimpImage *image);
GimpGuide * gimp_image_get_guide      (GimpImage *image,
                                       guint32    id);
GimpGuide * gimp_image_get_next_guide (GimpImage *image,
                                       guint32    id,
                                       gboolean  *guide_found);
