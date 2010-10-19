/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpscalecombobox.h
 * Copyright (C) 2004, 2008  Sven Neumann <sven@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_SCALE_COMBO_BOX_H__
#define __GIMP_SCALE_COMBO_BOX_H__


#define GIMP_TYPE_SCALE_COMBO_BOX            (gimp_scale_combo_box_get_type ())
#define GIMP_SCALE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SCALE_COMBO_BOX, GimpScaleComboBox))
#define GIMP_SCALE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SCALE_COMBO_BOX, GimpScaleComboBoxClass))
#define GIMP_IS_SCALE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SCALE_COMBO_BOX))
#define GIMP_IS_SCALE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SCALE_COMBO_BOX))
#define GIMP_SCALE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SCALE_COMBO_BOX, GimpScaleComboBoxClass))


typedef struct _GimpScaleComboBoxClass  GimpScaleComboBoxClass;

struct _GimpScaleComboBox
{
  GtkComboBox  parent_instance;

  gdouble      scale;
  GtkTreePath *last_path;
  GList       *mru;
};

struct _GimpScaleComboBoxClass
{
  GtkComboBoxClass  parent_instance;

  void (* entry_activated) (GimpScaleComboBox *combo_box);
};


GType       gimp_scale_combo_box_get_type   (void) G_GNUC_CONST;

GtkWidget * gimp_scale_combo_box_new        (void);
void        gimp_scale_combo_box_set_scale  (GimpScaleComboBox *combo_box,
                                             gdouble            scale);
gdouble     gimp_scale_combo_box_get_scale  (GimpScaleComboBox *combo_box);


#endif  /* __GIMP_SCALE_COMBO_BOX_H__ */
