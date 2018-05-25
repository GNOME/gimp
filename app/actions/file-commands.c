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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"
#include "core/gimpprogress.h"
#include "core/gimptemplate.h"

#include "plug-in/gimppluginmanager-file.h"

#include "file/file-open.h"
#include "file/file-save.h"
#include "file/gimp-file.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpclipboard.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpexportdialog.h"
#include "widgets/gimpfiledialog.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimpopendialog.h"
#include "widgets/gimpsavedialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "dialogs/dialogs.h"
#include "dialogs/file-save-dialog.h"

#include "actions.h"
#include "file-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void        file_open_dialog_show        (Gimp         *gimp,
                                                 GtkWidget    *parent,
                                                 const gchar  *title,
                                                 GimpImage    *image,
                                                 GFile        *file,
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
static void        file_export_dialog_response  (GtkWidget    *dialog,
                                                 gint          response_id,
                                                 gpointer      data);
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
                                  gimp_widget_get_monitor (widget),
                                  NULL /*ui_manager*/,
                                  widget,
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
      GFile             *file;
      GimpDisplay       *display;
      GtkWidget         *widget;
      GimpProgress      *progress;
      GimpImage         *image;
      GimpPDBStatusType  status;
      GError            *error = NULL;
      return_if_no_display (display, data);
      return_if_no_widget (widget, data);

      g_object_ref (display);
      g_object_ref (imagefile);

      file = gimp_imagefile_get_file (imagefile);

      progress = gimp_display_get_image (display) ?
                 NULL : GIMP_PROGRESS (display);

      image = file_open_with_display (gimp, action_data_get_context (data),
                                      progress,
                                      file, FALSE,
                                      G_OBJECT (gimp_widget_get_monitor (widget)),
                                      &status, &error);

      if (! image && status != GIMP_PDB_CANCEL)
        {
          gimp_message (gimp, G_OBJECT (display), GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        gimp_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
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
  GFile        *file  = NULL;
  gboolean      saved = FALSE;
  return_if_no_gimp (gimp, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = gimp_display_get_image (display);

  save_mode = (GimpSaveMode) value;

  if (! gimp_image_get_active_drawable (image))
    return;

  file = gimp_image_get_file (image);

  switch (save_mode)
    {
    case GIMP_SAVE_MODE_SAVE:
    case GIMP_SAVE_MODE_SAVE_AND_CLOSE:
      /*  Only save if the image has been modified, or if it is new.  */
      if ((gimp_image_is_dirty (image) ||
           ! GIMP_GUI_CONFIG (image->gimp->config)->trust_dirty_flag) ||
          file == NULL)
        {
          GimpPlugInProcedure *save_proc = gimp_image_get_save_proc (image);

          if (file && ! save_proc)
            {
              save_proc =
                gimp_plug_in_manager_file_procedure_find (image->gimp->plug_in_manager,
                                                          GIMP_FILE_PROCEDURE_GROUP_SAVE,
                                                          file, NULL);
            }

          if (file && save_proc)
            {
              saved = file_save_dialog_save_image (GIMP_PROGRESS (display),
                                                   gimp, image, file,
                                                   save_proc,
                                                   GIMP_RUN_WITH_LAST_VALS,
                                                   TRUE, FALSE, FALSE,
                                                   gimp_image_get_xcf_compression (image),
                                                   TRUE);
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

    case GIMP_SAVE_MODE_EXPORT_AS:
      file_export_dialog_show (gimp, image, widget);
      break;

    case GIMP_SAVE_MODE_EXPORT:
    case GIMP_SAVE_MODE_OVERWRITE:
      {
        GFile               *file        = NULL;
        GimpPlugInProcedure *export_proc = NULL;
        gboolean             overwrite   = FALSE;

        if (save_mode == GIMP_SAVE_MODE_EXPORT)
          {
            file        = gimp_image_get_exported_file (image);
            export_proc = gimp_image_get_export_proc (image);

            if (! file)
              {
                /* Behave as if Export As... was invoked */
                file_export_dialog_show (gimp, image, widget);
                break;
              }

            overwrite = FALSE;
          }
        else if (save_mode == GIMP_SAVE_MODE_OVERWRITE)
          {
            file = gimp_image_get_imported_file (image);

            overwrite = TRUE;
          }

        if (file && ! export_proc)
          {
            export_proc =
              gimp_plug_in_manager_file_procedure_find (image->gimp->plug_in_manager,
                                                        GIMP_FILE_PROCEDURE_GROUP_EXPORT,
                                                        file, NULL);
          }

        if (file && export_proc)
          {
            saved = file_save_dialog_save_image (GIMP_PROGRESS (display),
                                                 gimp, image, file,
                                                 export_proc,
                                                 GIMP_RUN_WITH_LAST_VALS,
                                                 FALSE,
                                                 overwrite, ! overwrite,
                                                 FALSE, TRUE);
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
  GFile       *file;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

  file = gimp_image_get_file (image);

  if (! file)
    file = gimp_image_get_imported_file (image);

  if (! file)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (display), GIMP_MESSAGE_ERROR,
                            _("Revert failed. "
                              "No file name associated with this image."));
      return;
    }

#define REVERT_DIALOG_KEY "gimp-revert-confirm-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), REVERT_DIALOG_KEY);

  if (! dialog)
    {
      dialog =
        gimp_message_dialog_new (_("Revert Image"), GIMP_ICON_DOCUMENT_REVERT,
                                 GTK_WIDGET (gimp_display_get_shell (display)),
                                 0,
                                 gimp_standard_help_func, GIMP_HELP_FILE_REVERT,

                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                 _("_Revert"), GTK_RESPONSE_OK,

                                 NULL);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect_object (display, "disconnect",
                               G_CALLBACK (gtk_widget_destroy),
                               dialog, G_CONNECT_SWAPPED);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (file_revert_confirm_response),
                        display);

      gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                         _("Revert '%s' to '%s'?"),
                                         gimp_image_get_display_name (image),
                                         gimp_file_get_utf8_name (file));

      gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                 _("By reverting the image to the state saved "
                                   "on disk, you will lose all changes, "
                                   "including all undo information."));

      dialogs_attach_dialog (G_OBJECT (image), REVERT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
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
                                        gimp_widget_get_monitor (widget),
                                        widget,
                                        "gimp-close-all-dialog", -1);
    }
}

void
file_copy_location_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  Gimp         *gimp;
  GimpDisplay  *display;
  GimpImage    *image;
  GFile        *file;
  return_if_no_gimp (gimp, data);
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

  file = gimp_image_get_any_file (image);

  if (file)
    {
      gchar *uri = g_file_get_uri (file);

      gimp_clipboard_set_text (gimp, uri);
      g_free (uri);
    }
}

void
file_show_in_file_manager_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  Gimp         *gimp;
  GimpDisplay  *display;
  GimpImage    *image;
  GFile        *file;
  return_if_no_gimp (gimp, data);
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

  file = gimp_image_get_any_file (image);

  if (file)
    {
      GError *error = NULL;

      if (! gimp_file_show_in_file_manager (file, &error))
        {
          gimp_message (gimp, G_OBJECT (display), GIMP_MESSAGE_ERROR,
                        _("Can't show file in file manager: %s"),
                        error->message);
          g_clear_error (&error);
        }
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
file_file_open_dialog (Gimp      *gimp,
                       GFile     *file,
                       GtkWidget *parent)
{
  file_open_dialog_show (gimp, parent,
                         _("Open Image"),
                         NULL, file, FALSE);
}


/*  private functions  */

static void
file_open_dialog_show (Gimp        *gimp,
                       GtkWidget   *parent,
                       const gchar *title,
                       GimpImage   *image,
                       GFile       *file,
                       gboolean     open_as_layers)
{
  GtkWidget *dialog;

  dialog = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                           gimp_widget_get_monitor (parent),
                                           NULL /*ui_manager*/,
                                           parent,
                                           "gimp-file-open-dialog", -1, FALSE);

  if (dialog)
    {
      if (! file && image)
        file = gimp_image_get_file (image);

      if (! file)
        file = g_object_get_data (G_OBJECT (gimp),
                                  GIMP_FILE_OPEN_LAST_FILE_KEY);

      if (file)
        {
          gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog), file, NULL);
        }
      else if (gimp->default_folder)
        {
          gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                    gimp->default_folder, NULL);
        }

      gtk_window_set_title (GTK_WINDOW (dialog), title);

      gimp_open_dialog_set_image (GIMP_OPEN_DIALOG (dialog),
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

#define SAVE_DIALOG_KEY "gimp-file-save-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), SAVE_DIALOG_KEY);

  if (! dialog)
    {
      dialog = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                               gimp_widget_get_monitor (parent),
                                               NULL /*ui_manager*/,
                                               parent,
                                               "gimp-file-save-dialog",
                                               -1, FALSE);

      if (dialog)
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (gtk_widget_get_toplevel (parent)));

          dialogs_attach_dialog (G_OBJECT (image), SAVE_DIALOG_KEY, dialog);
          g_signal_connect_object (image, "disconnect",
                                   G_CALLBACK (gtk_widget_destroy),
                                   dialog, G_CONNECT_SWAPPED);

          g_signal_connect (dialog, "response",
                            G_CALLBACK (file_save_dialog_response),
                            image);
        }
    }

  if (dialog)
    {
      gtk_window_set_title (GTK_WINDOW (dialog), title);

      gimp_save_dialog_set_image (GIMP_SAVE_DIALOG (dialog),
                                  image, save_a_copy,
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
      GFile          *file;
      gchar          *folder;
      gchar          *basename;

      parent   = gtk_window_get_transient_for (GTK_WINDOW (dialog));
      file     = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      folder   = g_path_get_dirname (gimp_file_get_utf8_name (file));
      basename = g_path_get_basename (gimp_file_get_utf8_name (file));
      g_object_unref (file);

      other = file_export_dialog_show (GIMP_FILE_DIALOG (file_dialog)->image->gimp,
                                       GIMP_FILE_DIALOG (file_dialog)->image,
                                       GTK_WIDGET (parent));

      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (other), folder);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (other), basename);

      g_free (folder);
      g_free (basename);
    }
}

static GtkWidget *
file_export_dialog_show (Gimp      *gimp,
                         GimpImage *image,
                         GtkWidget *parent)
{
  GtkWidget *dialog;

#define EXPORT_DIALOG_KEY "gimp-file-export-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), EXPORT_DIALOG_KEY);

  if (! dialog)
    {
      dialog = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                               gimp_widget_get_monitor (parent),
                                               NULL /*ui_manager*/,
                                               parent,
                                               "gimp-file-export-dialog",
                                               -1, FALSE);

      if (dialog)
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (gtk_widget_get_toplevel (parent)));

          dialogs_attach_dialog (G_OBJECT (image), EXPORT_DIALOG_KEY, dialog);
          g_signal_connect_object (image, "disconnect",
                                   G_CALLBACK (gtk_widget_destroy),
                                   dialog, G_CONNECT_SWAPPED);

          g_signal_connect (dialog, "response",
                            G_CALLBACK (file_export_dialog_response),
                            image);
        }
    }

  if (dialog)
    {
      gimp_export_dialog_set_image (GIMP_EXPORT_DIALOG (dialog), image);

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
      GFile          *file;
      gchar          *folder;
      gchar          *basename;

      parent   = gtk_window_get_transient_for (GTK_WINDOW (dialog));
      file     = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      folder   = g_path_get_dirname (gimp_file_get_utf8_name (file));
      basename = g_path_get_basename (gimp_file_get_utf8_name (file));
      g_object_unref (file);

      other = file_save_dialog_show (GIMP_FILE_DIALOG (file_dialog)->image->gimp,
                                     GIMP_FILE_DIALOG (file_dialog)->image,
                                     GTK_WIDGET (parent),
                                     _("Save Image"),
                                     FALSE, FALSE, NULL);

      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (other), folder);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (other), basename);

      g_free (folder);
      g_free (basename);
    }
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

  if (response_id == GTK_RESPONSE_OK)
    {
      Gimp              *gimp = old_image->gimp;
      GimpImage         *new_image;
      GFile             *file;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      file = gimp_image_get_file (old_image);

      if (! file)
        file = gimp_image_get_imported_file (old_image);

      new_image = file_open_image (gimp, gimp_get_user_context (gimp),
                                   GIMP_PROGRESS (display),
                                   file, file, FALSE, NULL,
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
          gimp_message (gimp, G_OBJECT (display), GIMP_MESSAGE_ERROR,
                        _("Reverting to '%s' failed:\n\n%s"),
                        gimp_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }
    }
}
