/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmascalecombobox.h
 * Copyright (C) 2004, 2008  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_SCALE_COMBO_BOX_H__
#define __LIGMA_SCALE_COMBO_BOX_H__


#define LIGMA_TYPE_SCALE_COMBO_BOX            (ligma_scale_combo_box_get_type ())
#define LIGMA_SCALE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SCALE_COMBO_BOX, LigmaScaleComboBox))
#define LIGMA_SCALE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SCALE_COMBO_BOX, LigmaScaleComboBoxClass))
#define LIGMA_IS_SCALE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SCALE_COMBO_BOX))
#define LIGMA_IS_SCALE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SCALE_COMBO_BOX))
#define LIGMA_SCALE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SCALE_COMBO_BOX, LigmaScaleComboBoxClass))


typedef struct _LigmaScaleComboBoxClass  LigmaScaleComboBoxClass;

struct _LigmaScaleComboBox
{
  GtkComboBox  parent_instance;

  gdouble      scale;
  GtkTreePath *last_path;
  GList       *mru;
};

struct _LigmaScaleComboBoxClass
{
  GtkComboBoxClass  parent_instance;

  void (* entry_activated) (LigmaScaleComboBox *combo_box);
};


GType       ligma_scale_combo_box_get_type   (void) G_GNUC_CONST;

GtkWidget * ligma_scale_combo_box_new        (void);
void        ligma_scale_combo_box_set_scale  (LigmaScaleComboBox *combo_box,
                                             gdouble            scale);
gdouble     ligma_scale_combo_box_get_scale  (LigmaScaleComboBox *combo_box);


#endif  /* __LIGMA_SCALE_COMBO_BOX_H__ */
