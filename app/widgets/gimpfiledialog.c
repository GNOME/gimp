/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfiledialog.c
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "config/gimpcoreconfig.h"

#include "file/file-utils.h"

#include "plug-in/plug-in-proc.h"

#include "gimpfiledialog.h"
#include "gimpmenufactory.h"
#include "gimpthumbbox.h"

#include "gimp-intl.h"


static void       gimp_file_dialog_class_init   (GimpFileDialogClass *klass);
static void       gimp_file_dialog_init         (GimpFileDialog      *dialog);

static void       gimp_file_dialog_destroy      (GtkObject           *object);

static gboolean   gimp_file_dialog_delete_event (GtkWidget           *widget,
                                                 GdkEventAny         *event);

static void  gimp_file_dialog_selection_changed (GtkTreeSelection    *sel,
                                                 GimpFileDialog      *dialog);


static GtkFileSelectionClass *parent_class = NULL;


GType
gimp_file_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (! dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GimpFileDialogClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_file_dialog_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpFileDialog),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_file_dialog_init,
      };

      dialog_type = g_type_register_static (GTK_TYPE_FILE_SELECTION,
                                            "GimpFileDialog",
                                            &dialog_info, 0);
    }

  return dialog_type;
}

static void
gimp_file_dialog_class_init (GimpFileDialogClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy      = gimp_file_dialog_destroy;

  widget_class->delete_event = gimp_file_dialog_delete_event;
}

static void
gimp_file_dialog_init (GimpFileDialog *dialog)
{
  GtkFileSelection *fs = GTK_FILE_SELECTION (dialog);

  gtk_container_set_border_width (GTK_CONTAINER (fs), 6);
  gtk_container_set_border_width (GTK_CONTAINER (fs->button_area), 4);
}

static void
gimp_file_dialog_destroy (GtkObject *object)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (object);

  if (dialog->item_factory)
    {
      g_object_unref (dialog->item_factory);
      dialog->item_factory = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
gimp_file_dialog_delete_event (GtkWidget   *widget,
                               GdkEventAny *event)
{
  return TRUE;
}


/*  public functions  */

GtkWidget *
gimp_file_dialog_new (Gimp            *gimp,
                      GSList          *file_procs,
                      GimpMenuFactory *menu_factory,
                      const gchar     *menu_identifier,
                      const gchar     *title,
                      const gchar     *role,
                      const gchar     *stock_id,
                      const gchar     *help_id)
{
  GimpFileDialog *dialog;
  GtkWidget      *hbox;
  GtkWidget      *option_menu;
  GtkWidget      *label;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (file_procs != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (menu_identifier != NULL, NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  dialog = g_object_new (GIMP_TYPE_FILE_DIALOG,
                         "title", title,
                         NULL);

  gtk_window_set_role (GTK_WINDOW (dialog), role);

  gimp_help_connect (GTK_WIDGET (dialog), gimp_standard_help_func,
                     help_id, NULL);

  dialog->gimp         = gimp;
  dialog->item_factory = gimp_menu_factory_menu_new (menu_factory,
                                                     menu_identifier,
                                                     GTK_TYPE_MENU,
                                                     dialog,
                                                     FALSE);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION (dialog)->main_vbox), hbox,
                    FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu),
                            GTK_ITEM_FACTORY (dialog->item_factory)->widget);
  gtk_box_pack_end (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
  gtk_widget_show (option_menu);

  label = gtk_label_new_with_mnemonic (_("Determine File _Type:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), option_menu);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  if (gimp->config->thumbnail_size > 0)
    {
      GtkFileSelection *fs = GTK_FILE_SELECTION (dialog);
      GtkTreeSelection *tree_sel;

      tree_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (fs->file_list));

      /* Catch file-list clicks so we can update the preview thumbnail */
      g_signal_connect (tree_sel, "changed",
                        G_CALLBACK (gimp_file_dialog_selection_changed),
                        dialog);

      /* EEK */
      for (hbox = fs->dir_list; ! GTK_IS_HBOX (hbox); hbox = hbox->parent);

      dialog->thumb_box = gimp_thumb_box_new (gimp);
      gtk_widget_set_sensitive (GTK_WIDGET (dialog->thumb_box), FALSE);
      gtk_box_pack_end (GTK_BOX (hbox), dialog->thumb_box, FALSE, FALSE, 0);
      gtk_widget_show (dialog->thumb_box);
    }

  return GTK_WIDGET (dialog);
}

void
gimp_file_dialog_set_file_proc (GimpFileDialog *dialog,
                                PlugInProcDef  *file_proc)
{
  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));

  if (file_proc == dialog->file_proc)
    return;

  dialog->file_proc = file_proc;

  if (file_proc && file_proc->extensions_list && dialog->gimage)
    {
      GtkFileSelection *fs = GTK_FILE_SELECTION (dialog);
      const gchar      *text;
      gchar            *last_dot;
      GString          *s;

      text = gtk_entry_get_text (GTK_ENTRY (fs->selection_entry));
      last_dot = strrchr (text, '.');

      if (last_dot == text || !text[0])
	return;

      s = g_string_new (text);

      if (last_dot)
	g_string_truncate (s, last_dot-text);

      g_string_append (s, ".");
      g_string_append (s, (gchar *) file_proc->extensions_list->data);

      gtk_entry_set_text (GTK_ENTRY (fs->selection_entry), s->str);

      g_string_free (s, TRUE);
    }
}


/*  private functions  */

static void
selchanged_foreach (GtkTreeModel *model,
		    GtkTreePath  *path,
		    GtkTreeIter  *iter,
		    gpointer      data)
{
  gboolean *selected = data;

  *selected = TRUE;
}

static void
gimp_file_dialog_selection_changed (GtkTreeSelection *sel,
                                    GimpFileDialog   *dialog)
{
  GtkFileSelection *fs       = GTK_FILE_SELECTION (dialog);
  const gchar      *fullfname;
  gboolean          selected = FALSE;

  gtk_tree_selection_selected_foreach (sel,
				       selchanged_foreach,
				       &selected);

  if (selected)
    {
      gchar *uri;

      fullfname = gtk_file_selection_get_filename (fs);

      uri = file_utils_filename_to_uri (dialog->gimp->load_procs,
                                        fullfname, NULL);
      gimp_thumb_box_set_uri (GIMP_THUMB_BOX (dialog->thumb_box), uri);
      g_free (uri);
    }
  else
    {
      gimp_thumb_box_set_uri (GIMP_THUMB_BOX (dialog->thumb_box), NULL);
    }

  {
    gchar **selections;
    GSList *uris = NULL;
    gint    i;

    selections = gtk_file_selection_get_selections (fs);

    for (i = 0; selections[i] != NULL; i++)
      uris = g_slist_prepend (uris, g_filename_to_uri (selections[i],
                                                       NULL, NULL));

    g_strfreev (selections);

    uris = g_slist_reverse (uris);

    gimp_thumb_box_set_uris (GIMP_THUMB_BOX (dialog->thumb_box), uris);
  }
}
