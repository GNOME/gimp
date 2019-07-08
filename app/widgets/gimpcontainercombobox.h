/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainercombobox.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_CONTAINER_COMBO_BOX_H__
#define __GIMP_CONTAINER_COMBO_BOX_H__


#define GIMP_TYPE_CONTAINER_COMBO_BOX            (gimp_container_combo_box_get_type ())
#define GIMP_CONTAINER_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_COMBO_BOX, GimpContainerComboBox))
#define GIMP_CONTAINER_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_COMBO_BOX, GimpContainerComboBoxClass))
#define GIMP_IS_CONTAINER_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_COMBO_BOX))
#define GIMP_IS_CONTAINER_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_COMBO_BOX))
#define GIMP_CONTAINER_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_COMBO_BOX, GimpContainerComboBoxClass))


typedef struct _GimpContainerComboBoxClass  GimpContainerComboBoxClass;

struct _GimpContainerComboBox
{
  GtkComboBox      parent_instance;

  GtkCellRenderer *text_renderer;
  GtkCellRenderer *viewable_renderer;
};

struct _GimpContainerComboBoxClass
{
  GtkComboBoxClass  parent_class;
};


GType       gimp_container_combo_box_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_container_combo_box_new      (GimpContainer *container,
                                               GimpContext   *context,
                                               gint           view_size,
                                               gint           view_border_width);


#endif  /*  __GIMP_CONTAINER_COMBO_BOX_H__  */
