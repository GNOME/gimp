/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
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
#include "file/gimp-file.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpfiledialog.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "dialogs/file-save-dialog.h"

#include "actions.h"
#include "file-commands.h"

#include "gimp-intl.h"


#define REVERT_DATA_KEY "revert-confirm-dialog"


/*  local function prototypes  */

static void        file_open_dialog_show        (Gimp         *gimp,
                                                 GtkWidget    *parent,
                                                 const gchar  *title,
                                                 GimpImage    *image,
                                                 const gchar  *uri,
                                                 gboolean      open_as_layers);
static GtkWidget * file_save_dialog_show        (Gimp         *gimp,
                                                 GimpImage    *image,
                                                 GtkWidget    *parent,
                                                 const gchar  *title,
                                                 gboolean      save_a_copy,
                                                 gboolean      close_after_saving,
                                                 GimpDisplay  *display);
static GtkWidget * file_export_dialog_show      (Gimp         *gimp,
                                                 GimpImage    *image,
                                                 GtkWidget    *parent);
static void        file_save_dialog_response    (GtkWidget    *dialog,
                                                 gint          response_id,
                                                 gpointer      data);
static void        file_save_dialog_destroyed   (GtkWidget    *dialog,
                                                 GimpImage    *image);
static void        file_export_dialog_response  (GtkWidget    *dialog,
                                                 gint          response_id,
                                                 gpointer      data);
static void        file_export_dialog_destroyed (GtkWidget    *dialog,
                                                 GimpImage    *image);
static void        file_new_template_callback   (GtkWidget    *widget,
                                                 const gchar  *name,
                                                 gpointer      data);
static void        file_revert_confirm_response (GtkWidget    *dialog,
                                                 gint          response_id,
                                                 GimpDisplay  *display);



/*  public functions  */


void
file_open_cmd_callback (GtkAction *action,
                        gpointer   data)
{
  Gimp        *gimp;
  GtkWidget   *widget;
  GimpImage   *image;
  return_if_no_gimp (gimp, data);
  return_if_no_widget (widget, data);

  image = action_data_get_image (data);

  file_open_dialog_show (gimp, widget,
                         _("Open Image"),
                         image, NULL, FALSE);
}

void
file_open_as_layers_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  Gimp        *gimp;
  GtkWidget   *widget;
  GimpDisplay *display;
  GimpImage   *image = NULL;
  return_if_no_gimp (gimp, data);
  return_if_no_widget (widget, data);

  display = action_data_get_display (data);

  if (display)
    image = gimp_display_get_image (display);

  file_open_dialog_show (gimp, widget,
                         _("Open Image as Layers"),
                         image, NULL, TRUE);
}

void
file_open_location_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                  gtk_widget_get_screen (widget),
                                  NULL /*ui_manager*/,
                                  "gimp-file-open-location-dialog", -1, TRUE);
}

void
file_open_recent_cmd_callback (GtkAction *action,
                               gint       value,
                               gpointer   data)
{
  Gimp          *gimp;
  GimpImagefile *imagefile;
  gint           num_entries;
  return_if_no_gimp (gimp, data);

  num_entries = gimp_container_get_n_children (gimp->documents);

  if (value >= num_entries)
    return;

  imagefile = (GimpImagefile *)
    gimp_container_get_child_by_index (gimp->documents, value);

  if (imagefile)
    {
      GimpDisplay       *display;
      GimpProgress      *progress;
      GimpImage         *image;
      GimpPDBStatusType  status;
      GError            *error = NULL;
      return_if_no_display (display, data);

      g_object_ref (display);
      g_object_ref (imagefile);

      progress = gimp_display_get_image (display) ?
                 NULL : GIMP_PROGRESS (display);

      image = file_open_with_display (gimp, action_data_get_context (data),
                                      progress,
                                      gimp_object_get_name (imagefile), FALSE,
                                      &status, &error);

      if (! image && status != GIMP_PDB_CANCEL)
        {
          gchar *filename =
            file_utils_uri_display_name (gimp_object_get_name (imagefile));

          gimp_message (gimp, G_OBJECT (display), GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        filename, error->message);
          g_clear_error (&error);

          g_free (filename);
        }

      g_object_unref (imagefile);
      g_object_unref (display);
    }
}

void
file_save_cmd_callback (GtkAction *action,
                        gint       value,
                        gpointer   data)
{
  Gimp         *gimp;
  GimpDisplay  *display;
  GimpImage    *image;
  GtkWidget    *widget;
  GimpSaveMode  save_mode;
  const gchar  *uri;
  gboolean      saved = FALSE;
  return_if_no_gimp (gimp, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = gimp_display_get_image (display);

  save_mode = (GimpSaveMode) value;

  if (! gimp_image_get_active_drawable (image))
    return;

  uri = gimp_image_get_uri (image);

  switch (save_mode)
    {
    case GIMP_SAVE_MODE_SAVE:
    case GIMP_SAVE_MODE_SAVE_AND_CLOSE:
      /*  Only save if the image has been modified, or if it is new.  */
      if ((gimp_image_is_dirty (image) ||
           ! GIMP_GUI_CONFIG (image->gimp->config)->trust_dirty_flag) ||
          uri == NULL)
        {
          GimpPlugInProcedure *save_proc = gimp_image_get_save_proc (image);

          if (uri && ! save_proc)
            {
              save_proc =
                file_procedure_find (image->gimp->plug_in_manager->save_procs,
                                     uri, NULL);
            }

          if (uri && save_proc)
            {
              saved = file_save_dialog_save_image (GIMP_PROGRESS (display),
                                                   gimp, image, uri,
                                                   save_proc,
                                                   GIMP_RUN_WITH_LAST_VALS,
                                                   TRUE, FALSE, FALSE, TRUE);
              break;
            }

          /* fall thru */
        }
      else
        {
          gimp_message_literal (image->gimp,
				G_OBJECT (display), GIMP_MESSAGE_INFO,
				_("No changes need to be saved"));
          saved = TRUE;
          break;
        }

    case GIMP_SAVE_MODE_SAVE_AS:
      file_save_dialog_show (gimp, image, widget,
                             _("Save Image"), FALSE,
                             save_mode == GIMP_SAVE_MODE_SAVE_AND_CLOSE, display);
      break;

    case GIMP_SAVE_MODE_SAVE_A_COPY:
      file_save_dialog_show (gimp, image, widget,
                             _("Save a Copy of the Image"), TRUE,
                             FALSE, display);
      break;

    case GIMP_SAVE_MODE_EXPORT:
      file_export_dialog_show (gimp, image, widget);
      break;

    case GIMP_SAVE_MODE_EXPORT_TO:
    case GIMP_SAVE_MODE_OVERWRITE:
      {
        const gchar         *uri         = NULL;
        GimpPlugInProcedure *export_proc = NULL;
        gboolean             overwrite   = FALSE;

        if (save_mode == GIMP_SAVE_MODE_EXPORT_TO)
          {
            uri         = gimp_image_get_exported_uri (image);
            export_proc = gimp_image_get_export_proc (image);

            if (! uri)
              {
                /* Behave as if Export... was invoked */
                file_export_dialog_show (gimp, image, widget);
                break;
              }

            overwrite = FALSE;
          }
        else if (save_mode == GIMP_SAVE_MODE_OVERWRITE)
          {
            uri = gimp_image_get_imported_uri (image);

            overwrite = TRUE;
          }

        if (uri && ! export_proc)
          {
            export_proc =
              file_procedure_find (image->gimp->plug_in_manager->export_procs,
                                   uri, NULL);
          }

        if (uri && export_proc)
          {
            char *uri_copy;

            /* The memory that 'uri' points to can be freed by
               file_save_dialog_save_image(), when it eventually calls
               gimp_image_set_imported_uri() to reset the imported uri,
               resulting in garbage. So make a duplicate of it here. */

            uri_copy = g_strdup (uri);

            saved = file_save_dialog_save_image (GIMP_PROGRESS (display),
                                                 gimp, image, uri_copy,
                                                 export_proc,
                                                 GIMP_RUN_WITH_LAST_VALS,
                                                 FALSE,
                                                 overwrite, ! overwrite,
                                                 TRUE);
            g_free (uri_copy);
          }
      }
      break;
    }

  if (save_mode == GIMP_SAVE_MODE_SAVE_AND_CLOSE &&
      saved &&
      ! gimp_image_is_dirty (image))
    {
      gimp_display_close (display);
    }
}

void
file_create_template_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplay *display;
  GimpImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

  dialog = gimp_query_string_box (_("Create New Template"),
                                  GTK_WIDGET (gimp_display_get_shell (display)),
                                  gimp_standard_help_func,
                                  GIMP_HELP_FILE_CREATE_TEMPLATE,
                                  _("Enter a name for this template"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  file_new_template_callback, image);
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

  image = gimp_display_get_image (display);

  uri = gimp_image_get_uri (image);

  if (! uri)
    uri = gimp_image_get_imported_uri (image);

  dialog = g_object_get_data (G_OBJECT (image), REVERT_DATA_KEY);

  if (! uri)
    {
      gimp_message_literal (image->gimp,
			    G_OBJECT (display), GIMP_MESSAGE_ERROR,
			    _("Revert failed. "
			      "No file name associated with this image."));
    }
  else if (dialog)
    {
      gtk_window_present (GTK_WINDOW (dialog));
    }
  else
    {
      gchar *filename;

      dialog =
        gimp_message_dialog_new (_("Revert Image"), GTK_STOCK_REVERT_TO_SAVED,
                                 GTK_WIDGET (gimp_display_get_shell (display)),
                                 0,
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

      filename = file_utils_uri_display_name (uri);

      gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                         _("Revert '%s' to '%s'?"),
                                         gimp_image_get_display_name (image),
                                         filename);
      g_free (filename);

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
      gimp_displays_close (gimp);
    }
  else
    {
      GtkWidget *widget;
      return_if_no_widget (widget, data);

      gimp_dialog_factory_dialog_raise (gimp_dialog_factory_get_singleton (),
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
  file_open_dialog_show (gimp, parent,
                         _("Open Image"),
                         NULL, uri, FALSE);
}


/*  private functions  */

static void
file_open_dialog_show (Gimp        *gimp,
                       GtkWidget   *parent,
                       const gchar *title,
                       GimpImage   *image,
                       const gchar *uri,
                       gboolean     open_as_layers)
{
  GtkWidget *dialog;

  dialog = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                           gtk_widget_get_screen (parent),
                                           NULL /*ui_manager*/,
                                           "gimp-file-open-dialog", -1, FALSE);

  if (dialog)
    {
      if (! uri && image)
        uri = gimp_image_get_uri (image);

      if (! uri)
        uri = g_object_get_data (G_OBJECT (gimp), GIMP_FILE_OPEN_LAST_URI_KEY);

      if (uri)
        gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dialog), uri);

      gtk_window_set_title (GTK_WINDOW (dialog), title);

      gimp_file_dialog_set_open_image (GIMP_FILE_DIALOG (dialog),
                                       image, open_as_layers);

      gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                    GTK_WINDOW (gtk_widget_get_toplevel (parent)));

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static GtkWidget *
file_save_dialog_show (Gimp        *gimp,
                       GimpImage   *image,
                       GtkWidget   *parent,
                       const gchar *title,
                       gboolean     save_a_copy,
                       gboolean     close_after_saving,
                       GimpDisplay *display)
{
  GtkWidget *dialog;

  dialog = g_object_get_data (G_OBJECT (image), "gimp-file-save-dialog");

  if (! dialog)
    {
      dialog = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                               gtk_widget_get_screen (parent),
                                               NULL /*ui_manager*/,
                                               "gimp-file-save-dialog",
                                               -1, FALSE);

      if (dialog)
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (gtk_widget_get_toplevel (parent)));

          g_object_set_data_full (G_OBJECT (image),
                                  "gimp-file-save-dialog", dialog,
                                  (GDestroyNotify) gtk_widget_destroy);
          g_signal_connect (dialog, "response",
                            G_CALLBACK (file_save_dialog_response),
                            image);
          g_signal_connect (dialog, "destroy",
                            G_CALLBACK (file_save_dialog_destroyed),
                            image);
        }
    }

  if (dialog)
    {
      gtk_window_set_title (GTK_WINDOW (dialog), title);

      gimp_file_dialog_set_save_image (GIMP_FILE_DIALOG (dialog),
                                       gimp, image, save_a_copy, FALSE,
                                       close_after_saving, GIMP_OBJECT (display));

      gtk_window_present (GTK_WINDOW (dialog));
    }

  return dialog;
}

static void
file_save_dialog_response (GtkWidget *dialog,
                           gint       response_id,
                           gpointer   data)
{
  if (response_id == FILE_SAVE_RESPONSE_OTHER_DIALOG)
    {
      GimpFileDialog *file_dialog = GIMP_FILE_DIALOG (dialog);
      GtkWindow      *parent;
      GtkWidget      *other;
      gchar          *folder;
      gchar          *uri;
      gchar          *name;

      parent = gtk_window_get_transient_for (GTK_WINDOW (dialog));
      folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dialog));
      uri    = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
      name   = file_utils_uri_display_basename (uri);
      g_free (uri);

      other = file_export_dialog_show (file_dialog->image->gimp,
                                       file_dialog->image,
                                       GTK_WIDGET (parent));

      gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (other), folder);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (other), name);

      g_free (folder);
      g_free (name);
    }
}

static void
file_save_dialog_destroyed (GtkWidget *dialog,
                            GimpImage *image)
{
  if (GIMP_FILE_DIALOG (dialog)->image == image)
    g_object_set_data (G_OBJECT (image), "gimp-file-save-dialog", NULL);
}

static GtkWidget *
file_export_dialog_show (Gimp      *gimp,
                         GimpImage *image,
                         GtkWidget *parent)
{
  GtkWidget *dialog;

  dialog = g_object_get_data (G_OBJECT (image), "gimp-file-export-dialog");

  if (! dialog)
    {
      dialog = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                               gtk_widget_get_screen (parent),
                                               NULL /*ui_manager*/,
                                               "gimp-file-export-dialog",
                                               -1, FALSE);

      if (dialog)
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (gtk_widget_get_toplevel (parent)));

          g_object_set_data_full (G_OBJECT (image),
                                  "gimp-file-export-dialog", dialog,
                                  (GDestroyNotify) gtk_widget_destroy);
          g_signal_connect (dialog, "response",
                            G_CALLBACK (file_export_dialog_response),
                            image);
          g_signal_connect (dialog, "destroy",
                            G_CALLBACK (file_export_dialog_destroyed),
                            image);
        }
    }

  if (dialog)
    {
      gimp_file_dialog_set_save_image (GIMP_FILE_DIALOG (dialog),
                                       gimp, image, FALSE, TRUE,
                                       FALSE, NULL);

      gtk_window_present (GTK_WINDOW (dialog));
    }

  return dialog;
}

static void
file_export_dialog_response (GtkWidget *dialog,
                             gint       response_id,
                             gpointer   data)
{
  if (response_id == FILE_SAVE_RESPONSE_OTHER_DIALOG)
    {
      GimpFileDialog *file_dialog = GIMP_FILE_DIALOG (dialog);
      GtkWindow      *parent;
      GtkWidget      *other;
      gchar          *folder;
      gchar          *uri;
      gchar          *name;

      parent = gtk_window_get_transient_for (GTK_WINDOW (dialog));
      folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dialog));
      uri    = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
      name   = file_utils_uri_display_basename (uri);
      g_free (uri);

      other = file_save_dialog_show (file_dialog->image->gimp,
                                     file_dialog->image,
                                     GTK_WIDGET (parent),
                                     _("Save Image"),
                                     FALSE, FALSE, NULL);

      gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (other), folder);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (other), name);

      g_free (folder);
      g_free (name);
    }
}

static void
file_export_dialog_destroyed (GtkWidget *dialog,
                              GimpImage *image)
{
  if (GIMP_FILE_DIALOG (dialog)->image == image)
    g_object_set_data (G_OBJECT (image), "gimp-file-export-dialog", NULL);
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
  GimpImage *old_image = gimp_display_get_image (display);

  gtk_widget_destroy (dialog);

  g_object_set_data (G_OBJECT (old_image), REVERT_DATA_KEY, NULL);

  if (response_id == GTK_RESPONSE_OK)
    {
      Gimp              *gimp = old_image->gimp;
      GimpImage         *new_image;
      const gchar       *uri;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      uri = gimp_image_get_uri (old_image);

      if (! uri)
        uri = gimp_image_get_imported_uri (old_image);

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
