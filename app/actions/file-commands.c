/* GIMP - The GNU Image Manipulation Program
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

#include "plug-in/gimppluginmanager.h"

#include "file/file-open.h"
#include "file/file-procedure.h"
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
                                            GimpImage   *image,
                                            const gchar *uri,
                                            gboolean     open_as_layers);
static void   file_save_dialog_show        (GimpImage   *image,
                                            GtkWidget   *parent,
                                            const gchar *title,
                                            gboolean     save_a_copy,
                                            gboolean     close_after_saving);
static void   file_save_dialog_destroyed   (GtkWidget   *dialog,
                                            GimpImage   *image);
static void   file_new_template_callback   (GtkWidget   *widget,
                                            const gchar *name,
                                            gpointer     data);
static void   file_revert_confirm_response (GtkWidget   *dialog,
                                            gint         response_id,
                                            GimpDisplay *display);


/*  public functions  */


void
file_open_cmd_callback (GtkAction *action,
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
file_open_as_layers_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpDisplay *display;
  GtkWidget   *widget;
  GimpImage   *image;
  const gchar *uri;
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = display->image;
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
      GimpImage         *image;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      image = file_open_with_display (gimp, action_data_get_context (data),
                                      NULL,
                                      GIMP_OBJECT (imagefile)->name, FALSE,
                                      &status, &error);

      if (! image && status != GIMP_PDB_CANCEL)
        {
          gchar *filename =
            file_utils_uri_display_name (GIMP_OBJECT (imagefile)->name);

          gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        filename, error->message);
          g_clear_error (&error);

          g_free (filename);
        }
    }
}

void
file_save_cmd_callback (GtkAction *action,
                        gint       value,
                        gpointer   data)
{
  GimpDisplay  *display;
  GimpImage    *image;
  GtkWidget    *widget;
  GimpSaveMode  save_mode;
  gboolean      saved = FALSE;
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = display->image;

  save_mode = (GimpSaveMode) value;

  if (! gimp_image_get_active_drawable (image))
    return;

  switch (save_mode)
    {
    case GIMP_SAVE_MODE_SAVE:
    case GIMP_SAVE_MODE_SAVE_AND_CLOSE:
      /*  Only save if the image has been modified  */
      if (image->dirty ||
          ! GIMP_GUI_CONFIG (image->gimp->config)->trust_dirty_flag)
        {
          const gchar         *uri;
          GimpPlugInProcedure *save_proc = NULL;

          uri       = gimp_object_get_name (GIMP_OBJECT (image));
          save_proc = gimp_image_get_save_proc (image);

          if (uri && ! save_proc)
            save_proc =
              file_procedure_find (image->gimp->plug_in_manager->save_procs,
                                   uri, NULL);

          if (uri && save_proc)
            {
              GimpPDBStatusType  status;
              GError            *error = NULL;
              GList             *list;

              for (list = gimp_action_groups_from_name ("file");
                   list;
                   list = g_list_next (list))
                {
                  gimp_action_group_set_action_sensitive (list->data,
                                                          "file-quit",
                                                          FALSE);
                }

              status = file_save (image, action_data_get_context (data),
                                  GIMP_PROGRESS (display),
                                  uri, save_proc,
                                  GIMP_RUN_WITH_LAST_VALS, FALSE, &error);

              switch (status)
                {
                case GIMP_PDB_SUCCESS:
                  saved = TRUE;
                  break;

                case GIMP_PDB_CANCEL:
                  gimp_message (image->gimp, G_OBJECT (display),
                                GIMP_MESSAGE_INFO,
                                _("Saving canceled"));
                  break;

                default:
                  {
                    gchar *filename = file_utils_uri_display_name (uri);

                    gimp_message (image->gimp, G_OBJECT (display),
                                  GIMP_MESSAGE_ERROR,
                                  _("Saving '%s' failed:\n\n%s"),
                                  filename, error->message);
                    g_free (filename);
                    g_clear_error (&error);
                  }
                  break;
                }

              for (list = gimp_action_groups_from_name ("file");
                   list;
                   list = g_list_next (list))
                {
                  gimp_action_group_set_action_sensitive (list->data,
                                                          "file-quit",
                                                          TRUE);
                }

              break;
            }

          /* fall thru */
        }
      else
        {
          saved = TRUE;
          break;
        }

    case GIMP_SAVE_MODE_SAVE_AS:
      file_save_dialog_show (display->image, widget,
                             _("Save Image"), FALSE,
                             save_mode == GIMP_SAVE_MODE_SAVE_AND_CLOSE);
      break;

    case GIMP_SAVE_MODE_SAVE_A_COPY:
      file_save_dialog_show (display->image, widget,
                             _("Save a Copy of the Image"), TRUE,
                             FALSE);
      break;
    }

  if (save_mode == GIMP_SAVE_MODE_SAVE_AND_CLOSE &&
      saved && ! display->image->dirty)
    {
      gimp_display_delete (display);
    }
}

void
file_save_template_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpDisplay *display;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  dialog = gimp_query_string_box (_("Create New Template"),
                                  display->shell,
                                  gimp_standard_help_func,
                                  GIMP_HELP_FILE_SAVE_AS_TEMPLATE,
                                  _("Enter a name for this template"),
                                  NULL,
                                  G_OBJECT (display->image), "disconnect",
                                  file_new_template_callback, display->image);
  gtk_widget_show (dialog);
}

void
file_revert_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  GimpDisplay *display;
  GimpImage   *image;
  GtkWidget   *dialog;
  const gchar *uri;
  return_if_no_display (display, data);

  image = display->image;

  uri = gimp_object_get_name (GIMP_OBJECT (image));

  dialog = g_object_get_data (G_OBJECT (image), REVERT_DATA_KEY);

  if (! uri)
    {
      gimp_message (image->gimp, G_OBJECT (display), GIMP_MESSAGE_ERROR,
                    _("Revert failed. "
                      "No file name associated with this image."));
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
        gimp_message_dialog_new (_("Revert Image"), GTK_STOCK_REVERT_TO_SAVED,
                                 display->shell, 0,
                                 gimp_standard_help_func, GIMP_HELP_FILE_REVERT,

                                 GTK_STOCK_CANCEL,          GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_REVERT_TO_SAVED, GTK_RESPONSE_OK,

                                 NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect_object (display, "disconnect",
                               G_CALLBACK (gtk_widget_destroy),
                               dialog, G_CONNECT_SWAPPED);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (file_revert_confirm_response),
                        display);

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

      g_object_set_data (G_OBJECT (image), REVERT_DATA_KEY, dialog);

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
                       GimpImage   *image,
                       const gchar *uri,
                       gboolean     open_as_layers)
{
  GtkWidget *dialog;

  dialog = gimp_dialog_factory_dialog_new (global_dialog_factory,
                                           gtk_widget_get_screen (parent),
                                           "gimp-file-open-dialog", -1, FALSE);

  if (dialog)
    {
      if (uri)
        gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dialog), uri);

      if (open_as_layers)
        {
          gtk_window_set_title (GTK_WINDOW (dialog), _("Open Image as Layers"));
          GIMP_FILE_DIALOG (dialog)->image = image;
        }
      else
        {
          gtk_window_set_title (GTK_WINDOW (dialog), _("Open Image"));
          GIMP_FILE_DIALOG (dialog)->image = NULL;
        }

      parent = gtk_widget_get_toplevel (parent);

      if (GTK_IS_WINDOW (parent))
        gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static void
file_save_dialog_show (GimpImage   *image,
                       GtkWidget   *parent,
                       const gchar *title,
                       gboolean     save_a_copy,
                       gboolean     close_after_saving)
{
  GtkWidget *dialog;

  dialog = g_object_get_data (G_OBJECT (image), "gimp-file-save-dialog");

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

          g_object_set_data_full (G_OBJECT (image),
                                  "gimp-file-save-dialog", dialog,
                                  (GDestroyNotify) gtk_widget_destroy);
          g_signal_connect (dialog, "destroy",
                            G_CALLBACK (file_save_dialog_destroyed),
                            image);
        }
    }

  if (dialog)
    {
      gtk_window_set_title (GTK_WINDOW (dialog), title);

      gimp_file_dialog_set_image (GIMP_FILE_DIALOG (dialog),
                                  image, save_a_copy, close_after_saving);

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static void
file_save_dialog_destroyed (GtkWidget *dialog,
                            GimpImage *image)
{
  if (GIMP_FILE_DIALOG (dialog)->image == image)
    g_object_set_data (G_OBJECT (image), "gimp-file-save-dialog", NULL);
}

static void
file_new_template_callback (GtkWidget   *widget,
                            const gchar *name,
                            gpointer     data)
{
  GimpTemplate *template;
  GimpImage    *image;

  image = (GimpImage *) data;

  if (! (name && strlen (name)))
    name = _("(Unnamed Template)");

  template = gimp_template_new (name);
  gimp_template_set_from_image (template, image);
  gimp_container_add (image->gimp->templates, GIMP_OBJECT (template));
  g_object_unref (template);
}

static void
file_revert_confirm_response (GtkWidget   *dialog,
                              gint         response_id,
                              GimpDisplay *display)
{
  GimpImage *old_image = display->image;

  gtk_widget_destroy (dialog);

  g_object_set_data (G_OBJECT (old_image), REVERT_DATA_KEY, NULL);

  if (response_id == GTK_RESPONSE_OK)
    {
      Gimp              *gimp = old_image->gimp;
      GimpImage         *new_image;
      const gchar       *uri;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      uri = gimp_object_get_name (GIMP_OBJECT (old_image));

      new_image = file_open_image (gimp, gimp_get_user_context (gimp),
                                   GIMP_PROGRESS (display),
                                   uri, uri, FALSE, NULL,
                                   GIMP_RUN_INTERACTIVE,
                                   &status, NULL, &error);

      if (new_image)
        {
          gimp_displays_reconnect (gimp, old_image, new_image);
          gimp_image_flush (new_image);

          /*  the displays own the image now  */
          g_object_unref (new_image);
        }
      else if (status != GIMP_PDB_CANCEL)
        {
          gchar *filename = file_utils_uri_display_name (uri);

          gimp_message (gimp, G_OBJECT (display), GIMP_MESSAGE_ERROR,
                        _("Reverting to '%s' failed:\n\n%s"),
                        filename, error->message);
          g_clear_error (&error);

          g_free (filename);
        }
    }
}
