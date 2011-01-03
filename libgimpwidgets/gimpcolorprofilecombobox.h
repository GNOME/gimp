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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_PROFILE_COMBO_BOX_H__
#define __GIMP_COLOR_PROFILE_COMBO_BOX_H__

G_BEGIN_DECLS

#define GIMP_TYPE_COLOR_PROFILE_COMBO_BOX            (gimp_color_profile_combo_box_get_type ())
#define GIMP_COLOR_PROFILE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_PROFILE_COMBO_BOX, GimpColorProfileComboBox))
#define GIMP_COLOR_PROFILE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_PROFILE_COMBO_BOX, GimpColorProfileComboBoxClass))
#define GIMP_IS_COLOR_PROFILE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_PROFILE_COMBO_BOX))
#define GIMP_IS_COLOR_PROFILE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_PROFILE_COMBO_BOX))
#define GIMP_COLOR_PROFILE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_PROFILE_COMBO_BOX, GimpColorProfileComboBoxClass))


typedef struct _GimpColorProfileComboBoxClass  GimpColorProfileComboBoxClass;

struct _GimpColorProfileComboBox
{
  GtkComboBox  parent_instance;
};

struct _GimpColorProfileComboBoxClass
{
  GtkComboBoxClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_color_profile_combo_box_get_type         (void) G_GNUC_CONST;

GtkWidget * gimp_color_profile_combo_box_new              (GtkWidget    *dialog,
                                                           const gchar  *history);
GtkWidget * gimp_color_profile_combo_box_new_with_model   (GtkWidget    *dialog,
                                                           GtkTreeModel *model);

GIMP_DEPRECATED_FOR (gimp_color_profile_combo_box_add_file)
void        gimp_color_profile_combo_box_add              (GimpColorProfileComboBox *combo,
                                                           const gchar              *filename,
                                                           const gchar              *label);
void        gimp_color_profile_combo_box_add_file         (GimpColorProfileComboBox *combo,
                                                           GFile                    *file,
                                                           const gchar              *label);

GIMP_DEPRECATED_FOR (gimp_color_profile_combo_box_set_active_file)
void        gimp_color_profile_combo_box_set_active       (GimpColorProfileComboBox *combo,
                                                           const gchar              *filename,
                                                           const gchar              *label);
void        gimp_color_profile_combo_box_set_active_file  (GimpColorProfileComboBox *combo,
                                                           GFile                    *file,
                                                           const gchar              *label);

GIMP_DEPRECATED_FOR (gimp_color_profile_combo_box_get_active_file)
gchar *     gimp_color_profile_combo_box_get_active       (GimpColorProfileComboBox *combo);
GFile *     gimp_color_profile_combo_box_get_active_file  (GimpColorProfileComboBox *combo);


G_END_DECLS

#endif  /* __GIMP_COLOR_PROFILE_COMBO_BOX_H__ */
