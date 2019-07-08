/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * gimplayermodecombobox.h
 * Copyright (C) 2017  Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_LAYER_MODE_COMBO_BOX_H__
#define __GIMP_LAYER_MODE_COMBO_BOX_H__


#define GIMP_TYPE_LAYER_MODE_COMBO_BOX            (gimp_layer_mode_combo_box_get_type ())
#define GIMP_LAYER_MODE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LAYER_MODE_COMBO_BOX, GimpLayerModeComboBox))
#define GIMP_LAYER_MODE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER_MODE_COMBO_BOX, GimpLayerModeComboBoxClass))
#define GIMP_IS_LAYER_MODE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LAYER_MODE_COMBO_BOX))
#define GIMP_IS_LAYER_MODE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER_MODE_COMBO_BOX))
#define GIMP_LAYER_MODE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LAYER_MODE_COMBO_BOX, GimpLayerModeComboBoxClass))


typedef struct _GimpLayerModeComboBoxPrivate GimpLayerModeComboBoxPrivate;
typedef struct _GimpLayerModeComboBoxClass   GimpLayerModeComboBoxClass;

struct _GimpLayerModeComboBox
{
  GimpEnumComboBox              parent_instance;

  GimpLayerModeComboBoxPrivate *priv;
};

struct _GimpLayerModeComboBoxClass
{
  GimpEnumComboBoxClass  parent_class;
};


GType                  gimp_layer_mode_combo_box_get_type    (void) G_GNUC_CONST;

GtkWidget            * gimp_layer_mode_combo_box_new         (GimpLayerModeContext   context);

void                   gimp_layer_mode_combo_box_set_context (GimpLayerModeComboBox *combo,
                                                              GimpLayerModeContext   context);
GimpLayerModeContext   gimp_layer_mode_combo_box_get_context (GimpLayerModeComboBox *combo);

void                   gimp_layer_mode_combo_box_set_mode    (GimpLayerModeComboBox *combo,
                                                              GimpLayerMode          mode);
GimpLayerMode          gimp_layer_mode_combo_box_get_mode    (GimpLayerModeComboBox *combo);

void                   gimp_layer_mode_combo_box_set_group   (GimpLayerModeComboBox *combo,
                                                              GimpLayerModeGroup     group);
GimpLayerModeGroup     gimp_layer_mode_combo_box_get_group   (GimpLayerModeComboBox *combo);


#endif  /* __GIMP_LAYER_MODE_COMBO_BOX_H__ */
