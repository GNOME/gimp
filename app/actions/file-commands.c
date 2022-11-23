/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimagefile.h"
#include "core/ligmaprogress.h"
#include "core/ligmatemplate.h"

#include "plug-in/ligmapluginmanager-file.h"

#include "file/file-open.h"
#include "file/file-save.h"
#include "file/ligma-file.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmaclipboard.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmaexportdialog.h"
#include "widgets/ligmafiledialog.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"
#include "widgets/ligmaopendialog.h"
#include "widgets/ligmasavedialog.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplay-foreach.h"

#include "dialogs/dialogs.h"
#include "dialogs/file-save-dialog.h"

#include "actions.h"
#include "file-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void        file_open_dialog_show        (Ligma         *ligma,
                                                 GtkWidget    *parent,
                                                 const gchar  *title,
                                                 LigmaImage    *image,
                                                 GFile        *file,
                                                 gboolean      open_as_layers);
static GtkWidget * file_save_dialog_show        (Ligma         *ligma,
                                                 LigmaImage    *image,
                                                 GtkWidget    *parent,
                                                 const gchar  *title,
                                                 gboolean      save_a_copy,
                                                 gboolean      close_after_saving,
                                                 LigmaDisplay  *display);
static GtkWidget * file_export_dialog_show      (Ligma         *ligma,
                                                 LigmaImage    *image,
                                                 GtkWidget    *parent,
                                                 LigmaDisplay  *display);
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
                                                 LigmaDisplay  *display);



/*  public functions  */


void
file_open_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  Ligma        *ligma;
  GtkWidget   *widget;
  LigmaImage   *image;
  return_if_no_ligma (ligma, data);
  return_if_no_widget (widget, data);

  image = action_data_get_image (data);

  file_open_dialog_show (ligma, widget,
                         _("Open Image"),
                         image, NULL, FALSE);
}

void
file_open_as_layers_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  Ligma        *ligma;
  GtkWidget   *widget;
  LigmaDisplay *display;
  LigmaImage   *image = NULL;
  return_if_no_ligma (ligma, data);
  return_if_no_widget (widget, data);

  display = action_data_get_display (data);

  if (display)
    image = ligma_display_get_image (display);

  file_open_dialog_show (ligma, widget,
                         _("Open Image as Layers"),
                         image, NULL, TRUE);
}

void
file_open_location_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  ligma_dialog_factory_dialog_new (ligma_dialog_factory_get_singleton (),
                                  ligma_widget_get_monitor (widget),
                                  NULL /*ui_manager*/,
                                  widget,
                                  "ligma-file-open-location-dialog", -1, TRUE);
}

void
file_open_recent_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  Ligma          *ligma;
  LigmaImagefile *imagefile;
  gint           index;
  gint           num_entries;
  return_if_no_ligma (ligma, data);

  index = g_variant_get_int32 (value);

  num_entries = ligma_container_get_n_children (ligma->documents);

  if (index >= num_entries)
    return;

  imagefile = (LigmaImagefile *)
    ligma_container_get_child_by_index (ligma->documents, index);

  if (imagefile)
    {
      GFile             *file;
      LigmaDisplay       *display;
      GtkWidget         *widget;
      LigmaProgress      *progress;
      LigmaImage         *image;
      LigmaPDBStatusType  status;
      GError            *error = NULL;
      return_if_no_display (display, data);
      return_if_no_widget (widget, data);

      g_object_ref (display);
      g_object_ref (imagefile);

      file = ligma_imagefile_get_file (imagefile);

      progress = ligma_display_get_image (display) ?
                 NULL : LIGMA_PROGRESS (display);

      image = file_open_with_display (ligma, action_data_get_context (data),
                                      progress,
                                      file, FALSE,
                                      G_OBJECT (ligma_widget_get_monitor (widget)),
                                      &status, &error);

      if (! image && status != LIGMA_PDB_CANCEL)
        {
          ligma_message (ligma, G_OBJECT (display), LIGMA_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        ligma_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (imagefile);
      g_object_unref (display);
    }
}

void
file_save_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  Ligma         *ligma;
  LigmaDisplay  *display;
  LigmaImage    *image;
  GList        *drawables;
  GtkWidget    *widget;
  LigmaSaveMode  save_mode;
  GFile        *file  = NULL;
  gboolean      saved = FALSE;
  return_if_no_ligma (ligma, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = ligma_display_get_image (display);

  save_mode = (LigmaSaveMode) g_variant_get_int32 (value);

  drawables = ligma_image_get_selected_drawables (image);
  if (! drawables)
    {
      g_list_free (drawables);
      return;
    }
  g_list_free (drawables);

  file = ligma_image_get_file (image);

  switch (save_mode)
    {
    case LIGMA_SAVE_MODE_SAVE:
    case LIGMA_SAVE_MODE_SAVE_AND_CLOSE:
      /*  Only save if the image has been modified, or if it is new.  */
      if ((ligma_image_is_dirty (image) ||
           ! LIGMA_GUI_CONFIG (image->ligma->config)->trust_dirty_flag) ||
          file == NULL)
        {
          LigmaPlugInProcedure *save_proc = ligma_image_get_save_proc (image);

          if (file && ! save_proc)
            {
              save_proc =
                ligma_plug_in_manager_file_procedure_find (image->ligma->plug_in_manager,
                                                          LIGMA_FILE_PROCEDURE_GROUP_SAVE,
                                                          file, NULL);
            }

          if (file && save_proc)
            {
              saved = file_save_dialog_save_image (LIGMA_PROGRESS (display),
                                                   ligma, image, file,
                                                   save_proc,
                                                   LIGMA_RUN_WITH_LAST_VALS,
                                                   TRUE, FALSE, FALSE,
                                                   ligma_image_get_xcf_compression (image),
                                                   TRUE);
              break;
            }

          /* fall thru */
        }
      else
        {
          ligma_message_literal (image->ligma,
                                G_OBJECT (display), LIGMA_MESSAGE_INFO,
                                _("No changes need to be saved"));
          saved = TRUE;
          break;
        }

    case LIGMA_SAVE_MODE_SAVE_AS:
      file_save_dialog_show (ligma, image, widget,
                             _("Save Image"), FALSE,
                             save_mode == LIGMA_SAVE_MODE_SAVE_AND_CLOSE, display);
      break;

    case LIGMA_SAVE_MODE_SAVE_A_COPY:
      file_save_dialog_show (ligma, image, widget,
                             _("Save a Copy of the Image"), TRUE,
                             FALSE, display);
      break;

    case LIGMA_SAVE_MODE_EXPORT_AS:
      file_export_dialog_show (ligma, image, widget, display);
      break;

    case LIGMA_SAVE_MODE_EXPORT:
    case LIGMA_SAVE_MODE_OVERWRITE:
      {
        GFile               *file        = NULL;
        LigmaPlugInProcedure *export_proc = NULL;
        gboolean             overwrite   = FALSE;

        if (save_mode == LIGMA_SAVE_MODE_EXPORT)
          {
            file        = ligma_image_get_exported_file (image);
            export_proc = ligma_image_get_export_proc (image);

            if (! file)
              {
                /* Behave as if Export As... was invoked */
                file_export_dialog_show (ligma, image, widget, display);
                break;
              }

            overwrite = FALSE;
          }
        else if (save_mode == LIGMA_SAVE_MODE_OVERWRITE)
          {
            file = ligma_image_get_imported_file (image);

            overwrite = TRUE;
          }

        if (file && ! export_proc)
          {
            export_proc =
              ligma_plug_in_manager_file_procedure_find (image->ligma->plug_in_manager,
                                                        LIGMA_FILE_PROCEDURE_GROUP_EXPORT,
                                                        file, NULL);
          }

        if (file && export_proc)
          {
            saved = file_save_dialog_save_image (LIGMA_PROGRESS (display),
                                                 ligma, image, file,
                                                 export_proc,
                                                 LIGMA_RUN_WITH_LAST_VALS,
                                                 FALSE,
                                                 overwrite, ! overwrite,
                                                 FALSE, TRUE);
          }
      }
      break;
    }

  if (save_mode == LIGMA_SAVE_MODE_SAVE_AND_CLOSE &&
      saved &&
      ! ligma_image_is_dirty (image))
    {
      ligma_display_close (display);
    }
}

void
file_create_template_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

  dialog = ligma_query_string_box (_("Create New Template"),
                                  GTK_WIDGET (ligma_display_get_shell (display)),
                                  ligma_standard_help_func,
                                  LIGMA_HELP_FILE_CREATE_TEMPLATE,
                                  _("Enter a name for this template"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  file_new_template_callback,
                                  image, NULL);
  gtk_widget_show (dialog);
}

void
file_revert_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  GtkWidget   *dialog;
  GFile       *file;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

  file = ligma_image_get_file (image);

  if (! file)
    file = ligma_image_get_imported_file (image);

  if (! file)
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (display), LIGMA_MESSAGE_ERROR,
                            _("Revert failed. "
                              "No file name associated with this image."));
      return;
    }

#define REVERT_DIALOG_KEY "ligma-revert-confirm-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), REVERT_DIALOG_KEY);

  if (! dialog)
    {
      dialog =
        ligma_message_dialog_new (_("Revert Image"), LIGMA_ICON_DOCUMENT_REVERT,
                                 GTK_WIDGET (ligma_display_get_shell (display)),
                                 0,
                                 ligma_standard_help_func, LIGMA_HELP_FILE_REVERT,

                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                 _("_Revert"), GTK_RESPONSE_OK,

                                 NULL);

      ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect_object (display, "disconnect",
                               G_CALLBACK (gtk_widget_destroy),
                               dialog, G_CONNECT_SWAPPED);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (file_revert_confirm_response),
                        display);

      ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                         _("Revert '%s' to '%s'?"),
                                         ligma_image_get_display_name (image),
                                         ligma_file_get_utf8_name (file));

      ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                 _("By reverting the image to the state saved "
                                   "on disk, you will lose all changes, "
                                   "including all undo information."));

      dialogs_attach_dialog (G_OBJECT (image), REVERT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
file_close_all_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  Ligma *ligma;
  return_if_no_ligma (ligma, data);

  if (! ligma_displays_dirty (ligma))
    {
      ligma_displays_close (ligma);
    }
  else
    {
      GtkWidget *widget;
      return_if_no_widget (widget, data);

      ligma_dialog_factory_dialog_raise (ligma_dialog_factory_get_singleton (),
                                        ligma_widget_get_monitor (widget),
                                        widget,
                                        "ligma-close-all-dialog", -1);
    }
}

void
file_copy_location_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  Ligma         *ligma;
  LigmaDisplay  *display;
  LigmaImage    *image;
  GFile        *file;
  return_if_no_ligma (ligma, data);
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

  file = ligma_image_get_any_file (image);

  if (file)
    {
      gchar *uri = g_file_get_uri (file);

      ligma_clipboard_set_text (ligma, uri);
      g_free (uri);
    }
}

void
file_show_in_file_manager_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  Ligma         *ligma;
  LigmaDisplay  *display;
  LigmaImage    *image;
  GFile        *file;
  return_if_no_ligma (ligma, data);
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

  file = ligma_image_get_any_file (image);

  if (file)
    {
      GError *error = NULL;

      if (! ligma_file_show_in_file_manager (file, &error))
        {
          ligma_message (ligma, G_OBJECT (display), LIGMA_MESSAGE_ERROR,
                        _("Can't show file in file manager: %s"),
                        error->message);
          g_clear_error (&error);
        }
    }
}

void
file_quit_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  Ligma *ligma;
  return_if_no_ligma (ligma, data);

  ligma_exit (ligma, FALSE);
}

void
file_file_open_dialog (Ligma      *ligma,
                       GFile     *file,
                       GtkWidget *parent)
{
  file_open_dialog_show (ligma, parent,
                         _("Open Image"),
                         NULL, file, FALSE);
}


/*  private functions  */

static void
file_open_dialog_show (Ligma        *ligma,
                       GtkWidget   *parent,
                       const gchar *title,
                       LigmaImage   *image,
                       GFile       *file,
                       gboolean     open_as_layers)
{
  GtkWidget *dialog;

  dialog = ligma_dialog_factory_dialog_new (ligma_dialog_factory_get_singleton (),
                                           ligma_widget_get_monitor (parent),
                                           NULL /*ui_manager*/,
                                           parent,
                                           "ligma-file-open-dialog", -1, FALSE);

  if (dialog)
    {
      if (! file && image)
        file = ligma_image_get_file (image);

      if (! file)
        file = g_object_get_data (G_OBJECT (ligma),
                                  LIGMA_FILE_OPEN_LAST_FILE_KEY);

      if (file)
        {
          gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog), file, NULL);
        }
      else if (ligma->default_folder)
        {
          gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                    ligma->default_folder, NULL);
        }

      gtk_window_set_title (GTK_WINDOW (dialog), title);

      ligma_open_dialog_set_image (LIGMA_OPEN_DIALOG (dialog),
                                  image, open_as_layers);

      gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                    GTK_WINDOW (gtk_widget_get_toplevel (parent)));

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static GtkWidget *
file_save_dialog_show (Ligma        *ligma,
                       LigmaImage   *image,
                       GtkWidget   *parent,
                       const gchar *title,
                       gboolean     save_a_copy,
                       gboolean     close_after_saving,
                       LigmaDisplay *display)
{
  GtkWidget *dialog;

#define SAVE_DIALOG_KEY "ligma-file-save-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), SAVE_DIALOG_KEY);

  if (! dialog)
    {
      dialog = ligma_dialog_factory_dialog_new (ligma_dialog_factory_get_singleton (),
                                               ligma_widget_get_monitor (parent),
                                               NULL /*ui_manager*/,
                                               parent,
                                               "ligma-file-save-dialog",
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

      ligma_save_dialog_set_image (LIGMA_SAVE_DIALOG (dialog),
                                  image, save_a_copy,
                                  close_after_saving, LIGMA_OBJECT (display));

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
      LigmaFileDialog *file_dialog = LIGMA_FILE_DIALOG (dialog);
      GtkWindow      *parent;
      GtkWidget      *other;
      GFile          *file;
      gchar          *folder;
      gchar          *basename;

      parent   = gtk_window_get_transient_for (GTK_WINDOW (dialog));
      file     = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      folder   = g_path_get_dirname (ligma_file_get_utf8_name (file));
      basename = g_path_get_basename (ligma_file_get_utf8_name (file));
      g_object_unref (file);

      other = file_export_dialog_show (LIGMA_FILE_DIALOG (file_dialog)->image->ligma,
                                       LIGMA_FILE_DIALOG (file_dialog)->image,
                                       GTK_WIDGET (parent), NULL);

      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (other), folder);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (other), basename);

      g_free (folder);
      g_free (basename);
    }
}

static GtkWidget *
file_export_dialog_show (Ligma        *ligma,
                         LigmaImage   *image,
                         GtkWidget   *parent,
                         LigmaDisplay *display)
{
  GtkWidget *dialog;

#define EXPORT_DIALOG_KEY "ligma-file-export-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), EXPORT_DIALOG_KEY);

  if (! dialog)
    {
      dialog = ligma_dialog_factory_dialog_new (ligma_dialog_factory_get_singleton (),
                                               ligma_widget_get_monitor (parent),
                                               NULL /*ui_manager*/,
                                               parent,
                                               "ligma-file-export-dialog",
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
      ligma_export_dialog_set_image (LIGMA_EXPORT_DIALOG (dialog), image,
                                    LIGMA_OBJECT (display));

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
      LigmaFileDialog *file_dialog = LIGMA_FILE_DIALOG (dialog);
      GtkWindow      *parent;
      GtkWidget      *other;
      GFile          *file;
      gchar          *folder;
      gchar          *basename;

      parent   = gtk_window_get_transient_for (GTK_WINDOW (dialog));
      file     = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      folder   = g_path_get_dirname (ligma_file_get_utf8_name (file));
      basename = g_path_get_basename (ligma_file_get_utf8_name (file));
      g_object_unref (file);

      other = file_save_dialog_show (LIGMA_FILE_DIALOG (file_dialog)->image->ligma,
                                     LIGMA_FILE_DIALOG (file_dialog)->image,
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
  LigmaTemplate *template;
  LigmaImage    *image;

  image = (LigmaImage *) data;

  if (! (name && strlen (name)))
    name = _("(Unnamed Template)");

  template = ligma_template_new (name);
  ligma_template_set_from_image (template, image);
  ligma_container_add (image->ligma->templates, LIGMA_OBJECT (template));
  g_object_unref (template);
}

static void
file_revert_confirm_response (GtkWidget   *dialog,
                              gint         response_id,
                              LigmaDisplay *display)
{
  LigmaImage *old_image = ligma_display_get_image (display);

  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      Ligma              *ligma = old_image->ligma;
      LigmaImage         *new_image;
      GFile             *file;
      LigmaPDBStatusType  status;
      GError            *error = NULL;

      file = ligma_image_get_file (old_image);

      if (! file)
        file = ligma_image_get_imported_file (old_image);

      new_image = file_open_image (ligma, ligma_get_user_context (ligma),
                                   LIGMA_PROGRESS (display),
                                   file, FALSE, NULL,
                                   LIGMA_RUN_INTERACTIVE,
                                   &status, NULL, &error);

      if (new_image)
        {
          ligma_displays_reconnect (ligma, old_image, new_image);
          ligma_image_flush (new_image);

          /*  the displays own the image now  */
          g_object_unref (new_image);
        }
      else if (status != LIGMA_PDB_CANCEL)
        {
          ligma_message (ligma, G_OBJECT (display), LIGMA_MESSAGE_ERROR,
                        _("Reverting to '%s' failed:\n\n%s"),
                        ligma_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }
    }
}
