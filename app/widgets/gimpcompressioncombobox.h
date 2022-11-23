/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacompressioncombobox.h
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

#ifndef __LIGMA_COMPRESSION_COMBO_BOX_H__
#define __LIGMA_COMPRESSION_COMBO_BOX_H__


#define LIGMA_TYPE_COMPRESSION_COMBO_BOX            (ligma_compression_combo_box_get_type ())
#define LIGMA_COMPRESSION_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COMPRESSION_COMBO_BOX, LigmaCompressionComboBox))
#define LIGMA_COMPRESSION_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COMPRESSION_COMBO_BOX, LigmaCompressionComboBoxClass))
#define LIGMA_IS_COMPRESSION_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COMPRESSION_COMBO_BOX))
#define LIGMA_IS_COMPRESSION_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COMPRESSION_COMBO_BOX))
#define LIGMA_COMPRESSION_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COMPRESSION_COMBO_BOX, LigmaCompressionComboBoxClass))


typedef struct _LigmaCompressionComboBoxClass  LigmaCompressionComboBoxClass;


struct _LigmaCompressionComboBox
{
  LigmaStringComboBox  parent_instance;
};

struct _LigmaCompressionComboBoxClass
{
  LigmaStringComboBoxClass  parent_instance;
};


GType       ligma_compression_combo_box_get_type        (void) G_GNUC_CONST;

GtkWidget * ligma_compression_combo_box_new             (void);

void        ligma_compression_combo_box_set_compression (LigmaCompressionComboBox *combo_box,
                                                        const gchar             *compression);
gchar     * ligma_compression_combo_box_get_compression (LigmaCompressionComboBox *combo_box);

#endif  /* __LIGMA_COMPRESSION_COMBO_BOX_H__ */
