/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfiledialog.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_FILE_DIALOG            (gimp_file_dialog_get_type ())
#define GIMP_FILE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILE_DIALOG, GimpFileDialog))
#define GIMP_FILE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILE_DIALOG, GimpFileDialogClass))
#define GIMP_IS_FILE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILE_DIALOG))
#define GIMP_IS_FILE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILE_DIALOG))
#define GIMP_FILE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILE_DIALOG, GimpFileDialogClass))


typedef struct _GimpFileDialogClass  GimpFileDialogClass;

struct _GimpFileDialog
{
  GtkFileChooserDialog  parent_instance;

  GBytes               *window_handle;

  Gimp                 *gimp;
  GimpImage            *image;

  GimpPlugInProcedure  *file_proc;

  GtkWidget            *thumb_box;
  GtkWidget            *extra_vbox;
  GtkWidget            *proc_expander;
  GtkWidget            *proc_view;
  GtkWidget            *progress;

  gboolean              busy;
  gboolean              canceled;

  gchar                *help_id;
  gchar                *ok_button_label;
  gchar                *automatic_help_id;
  gchar                *automatic_label;
  gchar                *file_filter_label;

  GSList               *file_procs;
  GSList               *file_procs_all_images;

  gboolean              show_all_files;
};

struct _GimpFileDialogClass
{
  GtkFileChooserDialogClass  parent_class;

  GFile * (* get_default_folder) (GimpFileDialog *dialog);

  void    (* save_state)         (GimpFileDialog *dialog,
                                  const gchar    *state_name);
  void    (* load_state)         (GimpFileDialog *dialog,
                                  const gchar    *state_name);
};


GType   gimp_file_dialog_get_type           (void) G_GNUC_CONST;

void    gimp_file_dialog_add_extra_widget   (GimpFileDialog      *dialog,
                                             GtkWidget           *widget,
                                             gboolean             expand,
                                             gboolean             fill,
                                             guint                padding);

void    gimp_file_dialog_set_sensitive      (GimpFileDialog      *dialog,
                                             gboolean             sensitive);

void    gimp_file_dialog_set_file_proc      (GimpFileDialog      *dialog,
                                             GimpPlugInProcedure *file_proc);

GFile * gimp_file_dialog_get_default_folder (GimpFileDialog      *dialog);

void    gimp_file_dialog_save_state         (GimpFileDialog      *dialog,
                                             const gchar         *state_name);
void    gimp_file_dialog_load_state         (GimpFileDialog      *dialog,
                                             const gchar         *state_name);
