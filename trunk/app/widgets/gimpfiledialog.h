/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfiledialog.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_FILE_DIALOG_H__
#define __GIMP_FILE_DIALOG_H__

G_BEGIN_DECLS


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

  GimpPlugInProcedure  *file_proc;

  GimpImage            *image;
  gboolean              save_a_copy;
  gboolean              close_after_saving;

  GtkWidget            *thumb_box;
  GtkWidget            *proc_expander;
  GtkWidget            *proc_view;
  GtkWidget            *progress;

  gboolean              busy;
  gboolean              canceled;
};

struct _GimpFileDialogClass
{
  GtkFileChooserDialogClass  parent_class;
};


GType       gimp_file_dialog_get_type      (void) G_GNUC_CONST;

GtkWidget * gimp_file_dialog_new           (Gimp                 *gimp,
                                            GtkFileChooserAction  action,
                                            const gchar          *title,
                                            const gchar          *role,
                                            const gchar          *stock_id,
                                            const gchar          *help_id);

void        gimp_file_dialog_set_sensitive (GimpFileDialog       *dialog,
                                            gboolean              sensitive);

void        gimp_file_dialog_set_file_proc (GimpFileDialog       *dialog,
                                            GimpPlugInProcedure  *file_proc);

void        gimp_file_dialog_set_image     (GimpFileDialog       *dialog,
                                            GimpImage            *image,
                                            gboolean              save_a_copy,
                                            gboolean              close_after_saving);

G_END_DECLS

#endif /* __GIMP_FILE_DIALOG_H__ */
