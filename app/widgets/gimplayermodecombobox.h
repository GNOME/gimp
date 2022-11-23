/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * ligmalayermodecombobox.h
 * Copyright (C) 2017  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_LAYER_MODE_COMBO_BOX_H__
#define __LIGMA_LAYER_MODE_COMBO_BOX_H__


#define LIGMA_TYPE_LAYER_MODE_COMBO_BOX            (ligma_layer_mode_combo_box_get_type ())
#define LIGMA_LAYER_MODE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LAYER_MODE_COMBO_BOX, LigmaLayerModeComboBox))
#define LIGMA_LAYER_MODE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LAYER_MODE_COMBO_BOX, LigmaLayerModeComboBoxClass))
#define LIGMA_IS_LAYER_MODE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LAYER_MODE_COMBO_BOX))
#define LIGMA_IS_LAYER_MODE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LAYER_MODE_COMBO_BOX))
#define LIGMA_LAYER_MODE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LAYER_MODE_COMBO_BOX, LigmaLayerModeComboBoxClass))


typedef struct _LigmaLayerModeComboBoxPrivate LigmaLayerModeComboBoxPrivate;
typedef struct _LigmaLayerModeComboBoxClass   LigmaLayerModeComboBoxClass;

struct _LigmaLayerModeComboBox
{
  LigmaEnumComboBox              parent_instance;

  LigmaLayerModeComboBoxPrivate *priv;
};

struct _LigmaLayerModeComboBoxClass
{
  LigmaEnumComboBoxClass  parent_class;
};


GType                  ligma_layer_mode_combo_box_get_type    (void) G_GNUC_CONST;

GtkWidget            * ligma_layer_mode_combo_box_new         (LigmaLayerModeContext   context);

void                   ligma_layer_mode_combo_box_set_context (LigmaLayerModeComboBox *combo,
                                                              LigmaLayerModeContext   context);
LigmaLayerModeContext   ligma_layer_mode_combo_box_get_context (LigmaLayerModeComboBox *combo);

void                   ligma_layer_mode_combo_box_set_mode    (LigmaLayerModeComboBox *combo,
                                                              LigmaLayerMode          mode);
LigmaLayerMode          ligma_layer_mode_combo_box_get_mode    (LigmaLayerModeComboBox *combo);

void                   ligma_layer_mode_combo_box_set_group   (LigmaLayerModeComboBox *combo,
                                                              LigmaLayerModeGroup     group);
LigmaLayerModeGroup     ligma_layer_mode_combo_box_get_group   (LigmaLayerModeComboBox *combo);


#endif  /* __LIGMA_LAYER_MODE_COMBO_BOX_H__ */
