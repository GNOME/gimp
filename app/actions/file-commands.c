/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"
#include "core/gimptemplate.h"

#include "file/file-open.h"
#include "file/file-save.h"
#include "file/file-utils.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpfiledialog.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "file-commands.h"

#include "gimp-intl.h"


#define REVERT_DATA_KEY "revert-confirm-dialog"


/*  local function prototypes  */

static void   file_open_dialog_show        (GtkWidget   *parent,
                                            GimpImage   *gimage,
                                            const gchar *uri,
                                            gboolean     open_as_layer);
static void   file_save_dialog_show        (GimpImage   *gimage,
                                            GtkWidget   *parent,
                                            const gchar *title,
                                            gboolean     save_a_copy);
static void   file_save_dialog_destroyed   (GtkWidget   *dialog,
                                            GimpImage   *gimage);
static void   file_new_template_callback   (GtkWidget   *widget,
                                            const gchar *name,
                                            gpointer     data);
static void   file_revert_confirm_response (GtkWidget   *dialog,
                                            gint         response_id,
                                            GimpDisplay *gdisp);


/*  public functions  */

void
file_open_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  file_open_dialog_show (widget, NULL, NULL, FALSE);
}

void
file_open_from_image_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpImage   *image;
  GtkWidget   *widget;
  const gchar *uri = NULL;
  return_if_no_widget (widget, data);

  image = action_data_get_image (data);

  if (image)
    uri = gimp_object_get_name (GIMP_OBJECT (image));

  file_open_dialog_show (widget, NULL, uri, FALSE);
}

void
file_open_as_layer_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *widget;
  GimpImage   *image;
  const gchar *uri;
  return_if_no_display (gdisp, data);
  return_if_no_widget (widget, data);

  image = gdisp->gimage;
  uri = gimp_object_get_name (GIMP_OBJECT (image));

  file_open_dialog_show (widget, image, uri, TRUE);
}

void
file_open_location_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  gimp_dialog_factory_dialog_new (global_dialog_factory,
                                  gtk_widget_get_screen (widget),
                                  "gimp-file-open-location-dialog", -1, TRUE);
}

void
file_last_opened_cmd_callback (GtkAction *action,
                               gint       value,
                               gpointer   data)
{
  Gimp          *gimp;
  GimpImagefile *imagefile;
  gint           num_entries;
  return_if_no_gimp (gimp, data);

  num_entries = gimp_container_num_children (gimp->documents);

  if (value >= num_entries)
    return;

  imagefile = (GimpImagefile *)
    gimp_container_get_child_by_index (gimp->documents, value);

  if (imagefile)
    {
      GimpImage         *gimage;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      gimage = file_open_with_display (gimp, action_data_get_context (data),
                                       NULL,
                                       GIMP_OBJECT (imagefile)->name,
                                       &status, &error);

      if (! gimage && status != GIMP_PDB_CANCEL)
        {
          gchar *filename =
            file_utils_uri_display_name (GIMP_OBJECT (imagefile)->name);

          g_message (_("Opening '%s' failed:\n\n%s"),
                     filename, error->message);
          g_clear_error (&error);

          g_free (filename);
        }
    }
}

void
file_save_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  GimpDisplay *gdisp;
  GimpImage   *gimage;
  return_if_no_display (gdisp, data);

  gimage = gdisp->gimage;

  if (! gimp_image_active_drawable (gimage))
    return;

  /*  Only save if the gimage has been modified  */
  if (gimage->dirty ||
      ! GIMP_GUI_CONFIG (gimage->gimp->config)->trust_dirty_flag)
    {
      const gchar   *uri;
      PlugInProcDef *save_proc = NULL;

      uri       = gimp_object_get_name (GIMP_OBJECT (gimage));
      save_proc = gimp_image_get_save_proc (gimage);

      if (uri && ! save_proc)
        save_proc = file_utils_find_proc (gimage->gimp->save_procs, uri);

      if (! (uri && save_proc))
        {
          file_save_as_cmd_callback (action, data);
        }
      else
        {
          GimpPDBStatusType  status;
          GError            *error = NULL;
          GList             *list;

          for (list = gimp_action_groups_from_name ("file");
               list;
               list = g_list_next (list))
            {
              gimp_action_group_set_action_sensitive (list->data, "file-quit",
                                                      FALSE);
            }

          status = file_save (gimage, action_data_get_context (data),
                              GIMP_PROGRESS (gdisp),
                              uri, save_proc,
                              GIMP_RUN_WITH_LAST_VALS, FALSE, &error);

          if (status != GIMP_PDB_SUCCESS &&
              status != GIMP_PDB_CANCEL)
            {
              gchar *filename = file_utils_uri_display_name (uri);

              g_message (_("Saving '%s' failed:\n\n%s"),
                         filename, error->message);
              g_clear_error (&error);

              g_free (filename);
            }

          for (list = gimp_action_groups_from_name ("file");
               list;
               list = g_list_next (list))
            {
              gimp_action_group_set_action_sensitive (list->data, "file-quit",
                                                      TRUE);
            }
        }
    }
}

void
file_save_as_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *widget;
  return_if_no_display (gdisp, data);
  return_if_no_widget (widget, data);

  if (! gimp_image_active_drawable (gdisp->gimage))
    return;

  file_save_dialog_show (gdisp->gimage, widget,
                         _("Save Image"), FALSE);
}

void
file_save_a_copy_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *widget;
  return_if_no_display (gdisp, data);
  return_if_no_widget (widget, data);

  if (! gimp_image_active_drawable (gdisp->gimage))
    return;

  file_save_dialog_show (gdisp->gimage, widget,
                         _("Save a Copy of the Image"), TRUE);
}

void
file_save_template_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *dialog;
  return_if_no_display (gdisp, data);

  dialog = gimp_query_string_box (_("Create New Template"),
                                  gdisp->shell,
                                  gimp_standard_help_func,
                                  GIMP_HELP_FILE_SAVE_AS_TEMPLATE,
                                  _("Enter a name for this template"),
                                  NULL,
                                  G_OBJECT (gdisp->gimage), "disconnect",
                                  file_new_template_callback, gdisp->gimage);
  gtk_widget_show (dialog);
}

void
file_revert_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *dialog;
  const gchar *uri;
  return_if_no_display (gdisp, data);

  uri = gimp_object_get_name (GIMP_OBJECT (gdisp->gimage));

  dialog = g_object_get_data (G_OBJECT (gdisp->gimage), REVERT_DATA_KEY);

  if (! uri)
    {
      g_message (_("Revert failed. No file name associated with this image."));
    }
  else if (dialog)
    {
      gtk_window_present (GTK_WINDOW (dialog));
    }
  else
    {
      gchar *basename;
      gchar *filename;

      dialog =
        gimp_message_dialog_new (_("Revert Image"), GIMP_STOCK_QUESTION,
                                 gdisp->shell, 0,
                                 gimp_standard_help_func, GIMP_HELP_FILE_REVERT,

                                 GTK_STOCK_CANCEL,          GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_REVERT_TO_SAVED, GTK_RESPONSE_OK,

                                 NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect_object (gdisp, "disconnect",
                               G_CALLBACK (gtk_widget_destroy),
                               dialog, G_CONNECT_SWAPPED);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (file_revert_confirm_response),
                        gdisp);

      basename = file_utils_uri_display_basename (uri);
      filename = file_utils_uri_display_name (uri);

      gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                         _("Revert '%s' to '%s'?"),
                                         basename, filename);
      g_free (filename);
      g_free (basename);

      gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                 _("By reverting the image to the state saved "
                                   "on disk, you will lose all changes, "
                                   "including all undo information."));

      g_object_set_data (G_OBJECT (gdisp->gimage), REVERT_DATA_KEY, dialog);

      gtk_widget_show (dialog);
    }
}

void
file_close_all_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  Gimp *gimp;
  return_if_no_gimp (gimp, data);

  if (! gimp_displays_dirty (gimp))
    {
      gimp_displays_delete (gimp);
    }
  else
    {
      GtkWidget *widget;
      return_if_no_widget (widget, data);

      gimp_dialog_factory_dialog_raise (global_dialog_factory,
                                        gtk_widget_get_screen (widget),
                                        "gimp-close-all-dialog", -1);
    }
}

void
file_quit_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  Gimp *gimp;
  return_if_no_gimp (gimp, data);

  gimp_exit (gimp, FALSE);
}

void
file_file_open_dialog (Gimp        *gimp,
                       const gchar *uri,
                       GtkWidget   *parent)
{
  file_open_dialog_show (parent, NULL, uri, FALSE);
}


/*  private functions  */

static void
file_open_dialog_show (GtkWidget   *parent,
                       GimpImage   *gimage,
                       const gchar *uri,
                       gboolean     open_as_layer)
{
  GtkWidget *dialog;

  dialog = gimp_dialog_factory_dialog_new (global_dialog_factory,
                                           gtk_widget_get_screen (parent),
                                           "gimp-file-open-dialog", -1, FALSE);

  if (dialog)
    {
      if (uri)
        gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dialog), uri);

      if (open_as_layer)
        {
          gtk_window_set_title (GTK_WINDOW (dialog), _("Open Image as Layer"));
          GIMP_FILE_DIALOG (dialog)->gimage = gimage;
        }
      else
        {
          gtk_window_set_title (GTK_WINDOW (dialog), _("Open Image"));
          GIMP_FILE_DIALOG (dialog)->gimage = NULL;
        }

      gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static void
file_save_dialog_show (GimpImage   *gimage,
                       GtkWidget   *parent,
                       const gchar *title,
                       gboolean     save_a_copy)
{
  GtkWidget *dialog;

  dialog = g_object_get_data (G_OBJECT (gimage), "gimp-file-save-dialog");

  if (! dialog)
    {
      dialog = gimp_dialog_factory_dialog_new (global_dialog_factory,
                                               gtk_widget_get_screen (parent),
                                               "gimp-file-save-dialog",
                                               -1, FALSE);

      if (dialog)
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (parent));

          g_object_set_data_full (G_OBJECT (gimage),
                                  "gimp-file-save-dialog", dialog,
                                  (GDestroyNotify) gtk_widget_destroy);
          g_signal_connect (dialog, "destroy",
                            G_CALLBACK (file_save_dialog_destroyed),
                            gimage);
        }
    }

  if (dialog)
    {
      gtk_window_set_title (GTK_WINDOW (dialog), title);

      gimp_file_dialog_set_image (GIMP_FILE_DIALOG (dialog),
                                  gimage, save_a_copy);

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static void
file_save_dialog_destroyed (GtkWidget *dialog,
                            GimpImage *gimage)
{
  if (GIMP_FILE_DIALOG (dialog)->gimage == gimage)
    g_object_set_data (G_OBJECT (gimage), "gimp-file-save-dialog", NULL);
}

static void
file_new_template_callback (GtkWidget   *widget,
                            const gchar *name,
                            gpointer     data)
{
  GimpTemplate *template;
  GimpImage    *gimage;

  gimage = (GimpImage *) data;

  if (! (name && strlen (name)))
    name = _("(Unnamed Template)");

  template = gimp_template_new (name);
  gimp_template_set_from_image (template, gimage);
  gimp_container_add (gimage->gimp->templates, GIMP_OBJECT (template));
  g_object_unref (template);
}

static void
file_revert_confirm_response (GtkWidget   *dialog,
                              gint         response_id,
                              GimpDisplay *gdisp)
{
  GimpImage *old_gimage = gdisp->gimage;

  gtk_widget_destroy (dialog);

  g_object_set_data (G_OBJECT (old_gimage), REVERT_DATA_KEY, NULL);

  if (response_id == GTK_RESPONSE_OK)
    {
      Gimp              *gimp = old_gimage->gimp;
      GimpImage         *new_gimage;
      const gchar       *uri;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      uri = gimp_object_get_name (GIMP_OBJECT (old_gimage));

      new_gimage = file_open_image (gimp, gimp_get_user_context (gimp),
                                    GIMP_PROGRESS (gdisp),
                                    uri, uri, NULL,
                                    GIMP_RUN_INTERACTIVE,
                                    &status, NULL, &error);

      if (new_gimage)
        {
          gimp_displays_reconnect (gimp, old_gimage, new_gimage);
          gimp_image_flush (new_gimage);

          /*  the displays own the image now  */
          g_object_unref (new_gimage);
        }
      else if (status != GIMP_PDB_CANCEL)
        {
          gchar *filename;

          filename = file_utils_uri_display_name (uri);

          g_message (_("Reverting to '%s' failed:\n\n%s"),
                     filename, error->message);
          g_clear_error (&error);

          g_free (filename);
        }
    }
}
