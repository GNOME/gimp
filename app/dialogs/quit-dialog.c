/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "widgets/gimpcontainertreestore.h"
#include "widgets/gimpcontainertreeview.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimpviewrenderer.h"
#include "widgets/gimpwidgets-utils.h"

#include "quit-dialog.h"

#include "gimp-intl.h"


typedef struct _QuitDialog QuitDialog;

struct _QuitDialog
{
  Gimp            *gimp;
  GimpContainer   *images;
  GimpContext     *context;

  gboolean         do_quit;

  GtkWidget       *dialog;
  GtkWidget       *ok_button;
  GimpMessageBox  *box;
  GtkWidget       *lost_label;
  GtkWidget       *hint_label;

  guint            accel_key;
  GdkModifierType  accel_mods;
};


static GtkWidget * quit_close_all_dialog_new               (Gimp              *gimp,
                                                            gboolean           do_quit);
static void        quit_close_all_dialog_free              (QuitDialog        *dialog);
static void        quit_close_all_dialog_response          (GtkWidget         *widget,
                                                            gint               response_id,
                                                            QuitDialog        *dialog);
static void        quit_close_all_dialog_accel_marshal     (GClosure          *closure,
                                                            GValue            *return_value,
                                                            guint              n_param_values,
                                                            const GValue      *param_values,
                                                            gpointer           invocation_hint,
                                                            gpointer           marshal_data);
static void        quit_close_all_dialog_container_changed (GimpContainer     *images,
                                                            GimpObject        *image,
                                                            GtkWidget         *widget);
static void        quit_close_all_dialog_image_activated   (GimpContainerView *view,
                                                            GimpImage         *image,
                                                            gpointer           insert_data,
                                                            QuitDialog        *dialog);
static void        quit_close_all_dialog_name_cell_func    (GtkTreeViewColumn *tree_column,
                                                            GtkCellRenderer   *cell,
                                                            GtkTreeModel      *tree_model,
                                                            GtkTreeIter       *iter,
                                                            gpointer           data);


/*  public functions  */

GtkWidget *
quit_dialog_new (Gimp *gimp)
{
  return quit_close_all_dialog_new (gimp, TRUE);
}

GtkWidget *
close_all_dialog_new (Gimp *gimp)
{
  return quit_close_all_dialog_new (gimp, FALSE);
}

static GtkWidget *
quit_close_all_dialog_new (Gimp     *gimp,
                           gboolean  do_quit)
{
  QuitDialog            *dialog;
  GtkWidget             *view;
  GimpContainerTreeView *tree_view;
  GtkCellRenderer       *renderer;
  GtkWidget             *dnd_widget;
  GtkAccelGroup         *accel_group;
  GClosure              *closure;
  gint                   rows;
  gint                   view_size;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = g_new0 (QuitDialog, 1);

  dialog->gimp    = gimp;
  dialog->do_quit = do_quit;
  dialog->images  = gimp_displays_get_dirty_images (gimp);
  dialog->context = gimp_context_new (gimp, "close-all-dialog",
                                      gimp_get_user_context (gimp));

  g_return_val_if_fail (dialog->images != NULL, NULL);

  dialog->dialog =
    gimp_message_dialog_new (do_quit ? _("Quit GIMP") : _("Close All Images"),
                             GIMP_STOCK_WARNING,
                             NULL, 0,
                             gimp_standard_help_func,
                             do_quit ?
                             GIMP_HELP_FILE_QUIT : GIMP_HELP_FILE_CLOSE_ALL,

                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

                             NULL);

  g_object_set_data_full (G_OBJECT (dialog->dialog), "quit-dialog",
                          dialog, (GDestroyNotify) quit_close_all_dialog_free);

  dialog->ok_button = gtk_dialog_add_button (GTK_DIALOG (dialog->dialog),
                                             "", GTK_RESPONSE_OK);

  g_object_set (gtk_widget_get_settings (dialog->dialog),
                "gtk-alternative-button-order", TRUE,
                NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (quit_close_all_dialog_response),
                    dialog);

  /* connect <Primary>D to the quit/close button */
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dialog->dialog), accel_group);
  g_object_unref (accel_group);

  closure = g_closure_new_object (sizeof (GClosure), G_OBJECT (dialog->dialog));
  g_closure_set_marshal (closure, quit_close_all_dialog_accel_marshal);
  gtk_accelerator_parse ("<Primary>D",
                         &dialog->accel_key, &dialog->accel_mods);
  gtk_accel_group_connect (accel_group,
                           dialog->accel_key, dialog->accel_mods,
                           0, closure);

  dialog->box = GIMP_MESSAGE_DIALOG (dialog->dialog)->box;

  view_size = gimp->config->layer_preview_size;
  rows      = CLAMP (gimp_container_get_n_children (dialog->images), 3, 6);

  view = gimp_container_tree_view_new (dialog->images, dialog->context,
                                       view_size, 1);
  tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  renderer = gimp_container_tree_view_get_name_cell (tree_view);
  gtk_tree_view_column_set_cell_data_func (tree_view->main_column,
                                           renderer,
                                           quit_close_all_dialog_name_cell_func,
                                           NULL, NULL);
  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (view),
                                       -1,
                                       rows * (view_size + 2));
  gtk_box_pack_start (GTK_BOX (dialog->box), view, TRUE, TRUE, 0);
  gtk_widget_show (view);

  g_signal_connect (view, "activate-item",
                    G_CALLBACK (quit_close_all_dialog_image_activated),
                    dialog);

  dnd_widget = gimp_container_view_get_dnd_widget (GIMP_CONTAINER_VIEW (view));
  gimp_dnd_xds_source_add (dnd_widget,
                           (GimpDndDragViewableFunc) gimp_dnd_get_drag_data,
                           NULL);

  if (do_quit)
    dialog->lost_label = gtk_label_new (_("If you quit GIMP now, "
                                          "these changes will be lost."));
  else
    dialog->lost_label = gtk_label_new (_("If you close these images now, "
                                          "changes will be lost."));
  gtk_misc_set_alignment (GTK_MISC (dialog->lost_label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (dialog->lost_label), TRUE);
  gtk_box_pack_start (GTK_BOX (dialog->box), dialog->lost_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (dialog->lost_label);

  dialog->hint_label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (dialog->hint_label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (dialog->hint_label), TRUE);
  gtk_box_pack_start (GTK_BOX (dialog->box), dialog->hint_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (dialog->hint_label);

  g_signal_connect_object (dialog->images, "add",
                           G_CALLBACK (quit_close_all_dialog_container_changed),
                           dialog->dialog, 0);
  g_signal_connect_object (dialog->images, "remove",
                           G_CALLBACK (quit_close_all_dialog_container_changed),
                           dialog->dialog, 0);

  quit_close_all_dialog_container_changed (dialog->images, NULL,
                                           dialog->dialog);

  return dialog->dialog;
}

static void
quit_close_all_dialog_free (QuitDialog *dialog)
{
  g_object_unref (dialog->images);
  g_object_unref (dialog->context);

  g_free (dialog);
}

static void
quit_close_all_dialog_response (GtkWidget  *widget,
                                gint        response_id,
                                QuitDialog *dialog)
{
  Gimp     *gimp    = dialog->gimp;
  gboolean  do_quit = dialog->do_quit;

  gtk_widget_destroy (dialog->dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      if (do_quit)
        gimp_exit (gimp, TRUE);
      else
        gimp_displays_close (gimp);
    }
}

static void
quit_close_all_dialog_accel_marshal (GClosure     *closure,
                                     GValue       *return_value,
                                     guint         n_param_values,
                                     const GValue *param_values,
                                     gpointer      invocation_hint,
                                     gpointer      marshal_data)
{
  gtk_dialog_response (GTK_DIALOG (closure->data), GTK_RESPONSE_OK);

  /* we handled the accelerator */
  g_value_set_boolean (return_value, TRUE);
}

static void
quit_close_all_dialog_container_changed (GimpContainer  *images,
                                         GimpObject     *image,
                                         GtkWidget      *widget)
{
  QuitDialog *dialog     = g_object_get_data (G_OBJECT (widget), "quit-dialog");
  gint        num_images = gimp_container_get_n_children (images);
  gchar      *accel_string;
  gchar      *hint;
  gchar      *markup;

  accel_string = gtk_accelerator_get_label (dialog->accel_key,
                                            dialog->accel_mods);

  gimp_message_box_set_primary_text (dialog->box,
                                     ngettext ("There is one image with "
                                               "unsaved changes:",
                                               "There are %d images with "
                                               "unsaved changes:",
                                               num_images), num_images);

  if (num_images == 0)
    {
      gtk_widget_hide (dialog->lost_label);

      if (dialog->do_quit)
        hint = g_strdup_printf (_("Press %s to quit."),
                                accel_string);
      else
        hint = g_strdup_printf (_("Press %s to close all images."),
                                accel_string);

      g_object_set (dialog->ok_button,
                    "label",     dialog->do_quit ? GTK_STOCK_QUIT : GTK_STOCK_CLOSE,
                    "use-stock", TRUE,
                    "image",     NULL,
                    NULL);

      gtk_widget_grab_default (dialog->ok_button);
    }
  else
    {
      GtkWidget *icon;

      if (dialog->do_quit)
        hint = g_strdup_printf (_("Press %s to discard all changes and quit."),
                                accel_string);
      else
        hint = g_strdup_printf (_("Press %s to discard all changes and close all images."),
                                accel_string);

      gtk_widget_show (dialog->lost_label);

      icon = gtk_image_new_from_icon_name ("edit-delete",
                                           GTK_ICON_SIZE_BUTTON);
      g_object_set (dialog->ok_button,
                    "label",     _("_Discard Changes"),
                    "use-stock", FALSE,
                    "image",     icon,
                    NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
                                       GTK_RESPONSE_CANCEL);
    }

  markup = g_strdup_printf ("<i><small>%s</small></i>", hint);

  gtk_label_set_markup (GTK_LABEL (dialog->hint_label), markup);

  g_free (markup);
  g_free (hint);
  g_free (accel_string);
}

static void
quit_close_all_dialog_image_activated (GimpContainerView *view,
                                       GimpImage         *image,
                                       gpointer           insert_data,
                                       QuitDialog        *dialog)
{
  GList *list;

  for (list = gimp_get_display_iter (dialog->gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      if (gimp_display_get_image (display) == image)
        gimp_display_shell_present (gimp_display_get_shell (display));
    }
}

static void
quit_close_all_dialog_name_cell_func (GtkTreeViewColumn *tree_column,
                                      GtkCellRenderer   *cell,
                                      GtkTreeModel      *tree_model,
                                      GtkTreeIter       *iter,
                                      gpointer           data)
{
  GimpViewRenderer *renderer;
  GimpImage        *image;
  gchar            *name;

  gtk_tree_model_get (tree_model, iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_NAME,     &name,
                      -1);

  image = GIMP_IMAGE (renderer->viewable);

  if (gimp_image_is_export_dirty (image))
    {
      g_object_set (cell,
                    "markup", NULL,
                    "text",   name,
                    NULL);
    }
  else
    {
      GFile       *file;
      const gchar *filename;
      gchar       *escaped_name;
      gchar       *escaped_filename;
      gchar       *exported;
      gchar       *markup;

      file = gimp_image_get_exported_file (image);
      if (! file)
        file = gimp_image_get_imported_file (image);

      filename = gimp_file_get_utf8_name (file);

      escaped_name     = g_markup_escape_text (name, -1);
      escaped_filename = g_markup_escape_text (filename, -1);

      exported = g_strdup_printf (_("Exported to %s"), escaped_filename);
      markup = g_strdup_printf ("%s\n<i>%s</i>", escaped_name, exported);
      g_free (exported);

      g_free (escaped_name);
      g_free (escaped_filename);

      g_object_set (cell,
                    "text",   NULL,
                    "markup", markup,
                    NULL);

      g_free (markup);
    }

  g_object_unref (renderer);
  g_free (name);
}
