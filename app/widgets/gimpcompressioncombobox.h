/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcompressioncombobox.h
 * Copyright (C) 2019 Ell
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


#define GIMP_TYPE_COMPRESSION_COMBO_BOX (gimp_compression_combo_box_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpCompressionComboBox,
                          gimp_compression_combo_box,
                          GIMP, COMPRESSION_COMBO_BOX,
                          GimpStringComboBox)


struct _GimpCompressionComboBoxClass
{
  GimpStringComboBoxClass  parent_instance;
};


GtkWidget * gimp_compression_combo_box_new             (void);

void        gimp_compression_combo_box_set_compression (GimpCompressionComboBox *combo_box,
                                                        const gchar             *compression);
gchar     * gimp_compression_combo_box_get_compression (GimpCompressionComboBox *combo_box);
