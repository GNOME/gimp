/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasettingsbox.h
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_SETTINGS_BOX_H__
#define __LIGMA_SETTINGS_BOX_H__


#define LIGMA_TYPE_SETTINGS_BOX            (ligma_settings_box_get_type ())
#define LIGMA_SETTINGS_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SETTINGS_BOX, LigmaSettingsBox))
#define LIGMA_SETTINGS_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SETTINGS_BOX, LigmaSettingsBoxClass))
#define LIGMA_IS_SETTINGS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SETTINGS_BOX))
#define LIGMA_IS_SETTINGS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SETTINGS_BOX))
#define LIGMA_SETTINGS_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SETTINGS_BOX, LigmaSettingsBoxClass))


typedef struct _LigmaSettingsBoxClass LigmaSettingsBoxClass;

struct _LigmaSettingsBox
{
  GtkBox  parent_instance;
};

struct _LigmaSettingsBoxClass
{
  GtkBoxClass  parent_class;

  /*  signals  */
  void (* file_dialog_setup) (LigmaSettingsBox      *box,
                              GtkFileChooserDialog *dialog,
                              gboolean              export);
  void (* import)            (LigmaSettingsBox      *box,
                              GFile                *file);
  void (* export)            (LigmaSettingsBox      *box,
                              GFile                *file);
  void (* selected)          (LigmaSettingsBox      *box,
                              GObject              *config);
};


GType       ligma_settings_box_get_type    (void) G_GNUC_CONST;

GtkWidget * ligma_settings_box_new         (Ligma            *ligma,
                                           GObject         *config,
                                           LigmaContainer   *container,
                                           const gchar     *import_dialog_title,
                                           const gchar     *export_dialog_title,
                                           const gchar     *file_dialog_help_id,
                                           GFile           *default_folder,
                                           GFile           *last_file);

GtkWidget * ligma_settings_box_get_combo   (LigmaSettingsBox *box);

void        ligma_settings_box_add_current (LigmaSettingsBox *box,
                                           gint             max_recent);

void        ligma_settings_box_unset       (LigmaSettingsBox *box);


#endif  /*  __LIGMA_SETTINGS_BOX_H__  */
