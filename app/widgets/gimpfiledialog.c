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
#include "gimpuimanager.h"

#include "gimppreviewrendererimagefile.h"
#include "gimppreview.h"

#include "gimp-intl.h"


static void       gimp_file_dialog_class_init   (GimpFileDialogClass *klass);
static void       gimp_file_dialog_init         (GimpFileDialog      *dialog);

static void       gimp_file_dialog_destroy      (GtkObject           *object);

static gboolean   gimp_file_dialog_delete_event (GtkWidget           *widget,
                                                 GdkEventAny         *event);

static void  gimp_file_dialog_selection_changed (GtkFileChooser      *chooser,
                                                 GimpFileDialog      *dialog);
static void  gimp_file_dialog_update_preview    (GtkFileChooser      *chooser,
                                                 GimpFileDialog      *dialog);


static GtkFileChooserDialogClass *parent_class = NULL;


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

      dialog_type = g_type_register_static (GTK_TYPE_FILE_CHOOSER_DIALOG,
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
}

static void
gimp_file_dialog_destroy (GtkObject *object)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (object);

  if (dialog->manager)
    {
      g_object_unref (dialog->manager);
      dialog->manager = NULL;
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
gimp_file_dialog_new (Gimp                 *gimp,
                      GSList               *file_procs,
                      GtkFileChooserAction  action,
                      GimpMenuFactory      *menu_factory,
                      const gchar          *menu_identifier,
                      const gchar          *ui_path,
                      const gchar          *title,
                      const gchar          *role,
                      const gchar          *stock_id,
                      const gchar          *help_id)
{
  GimpFileDialog *dialog;
  GtkWidget      *hbox;
  GtkWidget      *option_menu;
  GtkWidget      *label;
  GtkFileFilter  *filter;
  GSList         *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (file_procs != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (menu_identifier != NULL, NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  dialog = g_object_new (GIMP_TYPE_FILE_DIALOG,
                         "title",  title,
                         "role",   role,
                         "action", action,
                         NULL);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          stock_id,         GTK_RESPONSE_OK,
                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  gimp_help_connect (GTK_WIDGET (dialog), gimp_standard_help_func,
                     help_id, NULL);

  dialog->gimp    = gimp;
  dialog->manager = gimp_menu_factory_manager_new (menu_factory,
                                                   menu_identifier,
                                                   dialog,
                                                   FALSE);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), hbox);
  gtk_widget_show (hbox);

  option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu),
                            gimp_ui_manager_ui_get (dialog->manager, ui_path));
  gtk_box_pack_end (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
  gtk_widget_show (option_menu);

  label = gtk_label_new_with_mnemonic (_("Determine File _Type:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), option_menu);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  if (gimp->config->thumbnail_size > 0)
    {
      g_signal_connect (dialog, "selection-changed",
                        G_CALLBACK (gimp_file_dialog_selection_changed),
                        dialog);
      g_signal_connect (dialog, "update-preview",
                        G_CALLBACK (gimp_file_dialog_update_preview),
                        dialog);

      dialog->thumb_box = gimp_thumb_box_new (gimp);
      gtk_widget_set_sensitive (GTK_WIDGET (dialog->thumb_box), FALSE);
      gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (dialog),
                                           dialog->thumb_box);
      gtk_widget_show (dialog->thumb_box);

#ifdef ENABLE_FILE_SYSTEM_ICONS
      GIMP_PREVIEW_RENDERER_IMAGEFILE (GIMP_PREVIEW (GIMP_THUMB_BOX (dialog->thumb_box)->preview)->renderer)->file_system = _gtk_file_chooser_get_file_system (GTK_FILE_CHOOSER (dialog));
#endif

      gtk_file_chooser_set_use_preview_label (GTK_FILE_CHOOSER (dialog), FALSE);
    }

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All Files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  for (list = file_procs; list; list = g_slist_next (list))
    {
      PlugInProcDef *file_proc = list->data;

      if (file_proc->menu_paths && file_proc->extensions_list)
        {
          gchar  *name;
          GSList *ext;

          name = strrchr (file_proc->menu_paths->data, '/') + 1;

          filter = gtk_file_filter_new ();
          gtk_file_filter_set_name (filter, name);

          for (ext = file_proc->extensions_list; ext; ext = g_slist_next (ext))
            {
              gchar *pattern = g_strdup_printf ("*.%s", (gchar *) ext->data);
              gtk_file_filter_add_pattern (filter, pattern);
              g_free (pattern);
            }

          gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
        }
    }

  return GTK_WIDGET (dialog);
}

void
gimp_file_dialog_set_file_proc (GimpFileDialog *dialog,
                                PlugInProcDef  *file_proc)
{
  GtkFileChooser *chooser;

  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));

  if (file_proc == dialog->file_proc)
    return;

  dialog->file_proc = file_proc;

  chooser = GTK_FILE_CHOOSER (dialog);

  if (file_proc && file_proc->extensions_list &&
      gtk_file_chooser_get_action (chooser) == GTK_FILE_CHOOSER_ACTION_SAVE)
    {
      gchar *uri = gtk_file_chooser_get_uri (chooser);

      if (uri && strlen (uri))
        {
          gchar *last_dot = last_dot = strrchr (uri, '.');

          if (last_dot != uri)
            {
              GString *s = g_string_new (uri);
              gchar   *basename;

              if (last_dot)
                g_string_truncate (s, last_dot - uri);

              g_string_append (s, ".");
              g_string_append (s, (gchar *) file_proc->extensions_list->data);

              gtk_file_chooser_set_uri (chooser, s->str);

              basename = file_utils_uri_to_utf8_basename (s->str);
              gtk_file_chooser_set_current_name (chooser, basename);
              g_free (basename);

              g_string_free (s, TRUE);
            }
        }

      g_free (uri);
    }
}

void
gimp_file_dialog_set_uri (GimpFileDialog  *dialog,
                          GimpImage       *gimage,
                          const gchar     *uri)
{
  gchar    *real_uri  = NULL;
  gboolean  is_folder = FALSE;

  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));
  g_return_if_fail (gimage == NULL || GIMP_IS_IMAGE (gimage));

  if (gimage)
    {
      gchar *filename = gimp_image_get_filename (gimage);

      if (filename)
        {
          gchar *dirname = g_path_get_dirname (filename);

          g_free (filename);

          real_uri = g_filename_to_uri (dirname, NULL, NULL);
          g_free (dirname);

          is_folder = TRUE;
        }
    }
  else if (uri)
    {
      real_uri = g_strdup (uri);
    }
  else
    {
      gchar *current = g_get_current_dir ();

      real_uri = g_filename_to_uri (current, NULL, NULL);
      g_free (current);

      is_folder = TRUE;
    }

  if (is_folder)
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog),
                                             real_uri);
  else
    gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dialog), real_uri);

  g_free (real_uri);
}

void
gimp_file_dialog_set_image (GimpFileDialog *dialog,
                            GimpImage      *gimage,
                            gboolean        set_uri_and_proc,
                            gboolean        set_image_clean)
{
  const gchar *uri;

  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  dialog->gimage           = gimage;
  dialog->set_uri_and_proc = set_uri_and_proc;
  dialog->set_image_clean  = set_image_clean;

  uri = gimp_object_get_name (GIMP_OBJECT (gimage));

  if (uri)
    gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dialog), uri);

  gimp_ui_manager_update (dialog->manager,
                          gimp_image_active_drawable (gimage));
}


/*  private functions  */

static void
gimp_file_dialog_selection_changed (GtkFileChooser *chooser,
                                    GimpFileDialog *dialog)
{
  GSList *uris = gtk_file_chooser_get_uris (chooser);

#ifdef __GNUC__
#warning FIXME: remove version check as soon as we depend on GTK+ 2.4.1
#endif
  if (gtk_check_version (2, 4, 1))
    {
      if (uris)
        gimp_thumb_box_set_uri (GIMP_THUMB_BOX (dialog->thumb_box), uris->data);
      else
        gimp_thumb_box_set_uri (GIMP_THUMB_BOX (dialog->thumb_box), NULL);
    }

  gimp_thumb_box_set_uris (GIMP_THUMB_BOX (dialog->thumb_box), uris);
}

static void
gimp_file_dialog_update_preview (GtkFileChooser *chooser,
                                 GimpFileDialog *dialog)
{
  gchar *uri = gtk_file_chooser_get_preview_uri (chooser);

  gimp_thumb_box_set_uri (GIMP_THUMB_BOX (dialog->thumb_box), uri);

  g_free (uri);
}
