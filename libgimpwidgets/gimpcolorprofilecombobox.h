/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorprofilecombobox.h
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_PROFILE_COMBO_BOX_H__
#define __GIMP_COLOR_PROFILE_COMBO_BOX_H__

G_BEGIN_DECLS

#define GIMP_TYPE_COLOR_PROFILE_COMBO_BOX (gimp_color_profile_combo_box_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorProfileComboBox, gimp_color_profile_combo_box, GIMP, COLOR_PROFILE_COMBO_BOX, GtkComboBox)


GtkWidget * gimp_color_profile_combo_box_new                (GtkWidget    *dialog,
                                                             GFile        *history);
GtkWidget * gimp_color_profile_combo_box_new_with_model     (GtkWidget    *dialog,
                                                             GtkTreeModel *model);

void        gimp_color_profile_combo_box_add_file           (GimpColorProfileComboBox *combo,
                                                             GFile                    *file,
                                                             const gchar              *label);

void        gimp_color_profile_combo_box_set_active_file    (GimpColorProfileComboBox *combo,
                                                             GFile                    *file,
                                                             const gchar              *label);
void        gimp_color_profile_combo_box_set_active_profile (GimpColorProfileComboBox *combo,
                                                             GimpColorProfile         *profile);

GFile *     gimp_color_profile_combo_box_get_active_file    (GimpColorProfileComboBox *combo);


G_END_DECLS

#endif  /* __GIMP_COLOR_PROFILE_COMBO_BOX_H__ */
