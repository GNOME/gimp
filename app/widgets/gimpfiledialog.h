/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafiledialog.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_FILE_DIALOG_H__
#define __LIGMA_FILE_DIALOG_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_FILE_DIALOG            (ligma_file_dialog_get_type ())
#define LIGMA_FILE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FILE_DIALOG, LigmaFileDialog))
#define LIGMA_FILE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FILE_DIALOG, LigmaFileDialogClass))
#define LIGMA_IS_FILE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FILE_DIALOG))
#define LIGMA_IS_FILE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FILE_DIALOG))
#define LIGMA_FILE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FILE_DIALOG, LigmaFileDialogClass))


typedef struct _LigmaFileDialogClass  LigmaFileDialogClass;

struct _LigmaFileDialog
{
  GtkFileChooserDialog  parent_instance;

  Ligma                 *ligma;
  LigmaImage            *image;

  LigmaPlugInProcedure  *file_proc;

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

struct _LigmaFileDialogClass
{
  GtkFileChooserDialogClass  parent_class;

  GFile * (* get_default_folder) (LigmaFileDialog *dialog);

  void    (* save_state)         (LigmaFileDialog *dialog,
                                  const gchar    *state_name);
  void    (* load_state)         (LigmaFileDialog *dialog,
                                  const gchar    *state_name);
};


GType   ligma_file_dialog_get_type           (void) G_GNUC_CONST;

void    ligma_file_dialog_add_extra_widget   (LigmaFileDialog      *dialog,
                                             GtkWidget           *widget,
                                             gboolean             expand,
                                             gboolean             fill,
                                             guint                padding);

void    ligma_file_dialog_set_sensitive      (LigmaFileDialog      *dialog,
                                             gboolean             sensitive);

void    ligma_file_dialog_set_file_proc      (LigmaFileDialog      *dialog,
                                             LigmaPlugInProcedure *file_proc);

GFile * ligma_file_dialog_get_default_folder (LigmaFileDialog      *dialog);

void    ligma_file_dialog_save_state         (LigmaFileDialog      *dialog,
                                             const gchar         *state_name);
void    ligma_file_dialog_load_state         (LigmaFileDialog      *dialog,
                                             const gchar         *state_name);


G_END_DECLS

#endif /* __LIGMA_FILE_DIALOG_H__ */
