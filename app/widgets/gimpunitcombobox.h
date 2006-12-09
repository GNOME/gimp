/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpunitcombobox.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_UNIT_COMBO_BOX_H__
#define __GIMP_UNIT_COMBO_BOX_H__

#include <gtk/gtkcombobox.h>


#define GIMP_TYPE_UNIT_COMBO_BOX            (gimp_unit_combo_box_get_type ())
#define GIMP_UNIT_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNIT_COMBO_BOX, GimpUnitComboBox))
#define GIMP_UNIT_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNIT_COMBO_BOX, GimpUnitComboBoxClass))
#define GIMP_IS_UNIT_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_UNIT_COMBO_BOX))
#define GIMP_IS_UNIT_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNIT_COMBO_BOX))
#define GIMP_UNIT_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNIT_COMBO_BOX, GimpUnitComboBoxClass))


typedef struct _GimpUnitComboBoxClass  GimpUnitComboBoxClass;

struct _GimpUnitComboBoxClass
{
  GtkComboBoxClass  parent_instance;
};

struct _GimpUnitComboBox
{
  GtkComboBox       parent_instance;
};


GType       gimp_unit_combo_box_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_unit_combo_box_new            (void);
GtkWidget * gimp_unit_combo_box_new_with_model (GimpUnitStore    *model);

GimpUnit    gimp_unit_combo_box_get_active     (GimpUnitComboBox *combo);
void        gimp_unit_combo_box_set_active     (GimpUnitComboBox *combo,
                                                GimpUnit          unit);


#endif  /* __GIMP_UNIT_COMBO_BOX_H__ */
