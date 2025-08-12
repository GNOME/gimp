/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsettingsbox.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_SETTINGS_BOX            (gimp_settings_box_get_type ())
#define GIMP_SETTINGS_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SETTINGS_BOX, GimpSettingsBox))
#define GIMP_SETTINGS_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SETTINGS_BOX, GimpSettingsBoxClass))
#define GIMP_IS_SETTINGS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SETTINGS_BOX))
#define GIMP_IS_SETTINGS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SETTINGS_BOX))
#define GIMP_SETTINGS_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SETTINGS_BOX, GimpSettingsBoxClass))


typedef struct _GimpSettingsBoxClass GimpSettingsBoxClass;

struct _GimpSettingsBox
{
  GtkBox  parent_instance;
};

struct _GimpSettingsBoxClass
{
  GtkBoxClass  parent_class;

  /*  signals  */
  void (* file_dialog_setup) (GimpSettingsBox      *box,
                              GtkFileChooserDialog *dialog,
                              gboolean              export);
  void (* import)            (GimpSettingsBox      *box,
                              GFile                *file);
  void (* export)            (GimpSettingsBox      *box,
                              GFile                *file);
  void (* selected)          (GimpSettingsBox      *box,
                              GObject              *config);
};


GType       gimp_settings_box_get_type    (void) G_GNUC_CONST;

GtkWidget * gimp_settings_box_new         (Gimp            *gimp,
                                           GObject         *config,
                                           GimpContainer   *container,
                                           const gchar     *import_dialog_title,
                                           const gchar     *export_dialog_title,
                                           const gchar     *file_dialog_help_id,
                                           GFile           *default_folder,
                                           GFile           *last_file);

GtkWidget * gimp_settings_box_get_combo   (GimpSettingsBox *box);

void        gimp_settings_box_add_current (GimpSettingsBox *box,
                                           gint             max_recent);

void        gimp_settings_box_unset       (GimpSettingsBox *box);
