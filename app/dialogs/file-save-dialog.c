/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginprocedure.h"

#include "file/file-procedure.h"
#include "file/file-save.h"
#include "file/file-utils.h"
#include "file/gimp-file.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpfiledialog.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "file-save-dialog.h"

#include "gimp-log.h"
#include "gimp-intl.h"


typedef enum
{
  CHECK_URI_FAIL,
  CHECK_URI_OK,
  CHECK_URI_SWITCH_DIALOGS
} CheckUriResult;


/*  local function prototypes  */

static GtkFileChooserConfirmation
                 file_save_dialog_confirm_overwrite         (GtkWidget            *save_dialog,
                                                             Gimp                 *gimp);
static void      file_save_dialog_response                  (GtkWidget            *save_dialog,
                                                             gint                  response_id,
                                                             Gimp                 *gimp);
static CheckUriResult file_save_dialog_check_uri            (GtkWidget            *save_dialog,
                                                             Gimp                 *gimp,
                                                             gchar               **ret_uri,
                                                             gchar               **ret_basename,
                                                             GimpPlugInProcedure **ret_save_proc);
static gboolean  file_save_dialog_no_overwrite_confirmation (GimpFileDialog       *dialog,
                                                             Gimp                 *gimp);
static gchar *   file_save_dialog_get_uri                   (GimpFileDialog       *dialog);
static GSList *  file_save_dialog_get_procs                 (GimpFileDialog       *dialog,
                                                             Gimp                 *gimp);
static gboolean  file_save_dialog_switch_dialogs            (GimpFileDialog       *file_dialog,
                                                             Gimp                 *gimp,
                                                             const gchar          *basename);
static gboolean  file_save_dialog_use_extension             (GtkWidget            *save_dialog,
                                                             const gchar          *uri);


/*  public functions  */

GtkWidget *
file_save_dialog_new (Gimp     *gimp,
                      gboolean  export)
{
  GtkWidget           *dialog;
  GimpFileDialogState *state;

  if (! export)
    {
      dialog = gimp_file_dialog_new (gimp,
                                     GIMP_FILE_CHOOSER_ACTION_SAVE,
                                     _("Save Image"), "gimp-file-save",
                                     GTK_STOCK_SAVE,
                                     GIMP_HELP_FILE_SAVE);

      state = g_object_get_data (G_OBJECT (gimp), "gimp-file-save-dialog-state");
    }
  else
    {
      dialog = gimp_file_dialog_new (gimp,
                                     GIMP_FILE_CHOOSER_ACTION_EXPORT,
                                     _("Export Image"), "gimp-file-export",
                                     _("_Export"),
                                     GIMP_HELP_FILE_EXPORT);

      state = g_object_get_data (G_OBJECT (gimp), "gimp-file-export-dialog-state");
    }

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (state)
    gimp_file_dialog_set_state (GIMP_FILE_DIALOG (dialog), state);

  g_signal_connect (dialog, "confirm-overwrite",
                    G_CALLBACK (file_save_dialog_confirm_overwrite),
                    gimp);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_save_dialog_response),
                    gimp);

  return dialog;
}


/*  private functions  */

static GtkFileChooserConfirmation
file_save_dialog_confirm_overwrite (GtkWidget *save_dialog,
                                    Gimp      *gimp)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (save_dialog);

  if (file_save_dialog_no_overwrite_confirmation (dialog, gimp))
    /* The URI will not be accepted whatever happens, so don't
     * bother asking the user about overwriting files
     */
    return GTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME;
  else
    return GTK_FILE_CHOOSER_CONFIRMATION_CONFIRM;
}

static void
file_save_dialog_response (GtkWidget *save_dialog,
                           gint       response_id,
                           Gimp      *gimp)
{
  GimpFileDialog      *dialog = GIMP_FILE_DIALOG (save_dialog);
  gchar               *uri;
  gchar               *basename;
  GimpPlugInProcedure *save_proc;

  if (! dialog->export)
    {
      g_object_set_data_full (G_OBJECT (gimp), "gimp-file-save-dialog-state",
                              gimp_file_dialog_get_state (dialog),
                              (GDestroyNotify) gimp_file_dialog_state_destroy);
    }
  else
    {
      g_object_set_data_full (G_OBJECT (gimp), "gimp-file-export-dialog-state",
                              gimp_file_dialog_get_state (dialog),
                              (GDestroyNotify) gimp_file_dialog_state_destroy);
    }

  if (response_id != GTK_RESPONSE_OK)
    {
      if (! dialog->busy)
        gtk_widget_destroy (save_dialog);

      return;
    }

  g_object_ref (dialog);
  g_object_ref (dialog->image);

  switch (file_save_dialog_check_uri (save_dialog, gimp,
                                      &uri, &basename, &save_proc))
    {
    case CHECK_URI_FAIL:
      break;

    case CHECK_URI_OK:
      gimp_file_dialog_set_sensitive (dialog, FALSE);

      if (file_save_dialog_save_image (GIMP_PROGRESS (save_dialog),
                                       gimp,
                                       dialog->image,
                                       uri,
                                       save_proc,
                                       GIMP_RUN_INTERACTIVE,
                                       ! dialog->save_a_copy && ! dialog->export,
                                       FALSE,
                                       dialog->export,
                                       FALSE))
        {
          /* Save was successful, now store the URI in a couple of
           * places that depend on it being the user that made a
           * save. Lower-level URI management is handled in
           * file_save()
           */
          if (dialog->save_a_copy)
            gimp_image_set_save_a_copy_uri (dialog->image, uri);

          if (! dialog->export)
            g_object_set_data_full (G_OBJECT (dialog->image->gimp),
                                    GIMP_FILE_SAVE_LAST_URI_KEY,
                                    g_strdup (uri), (GDestroyNotify) g_free);
          else
            g_object_set_data_full (G_OBJECT (dialog->image->gimp),
                                    GIMP_FILE_EXPORT_LAST_URI_KEY,
                                    g_strdup (uri), (GDestroyNotify) g_free);

          /*  make sure the menus are updated with the keys we've just set  */
          gimp_image_flush (dialog->image);

          /* Handle close-after-saving */
          if (dialog->close_after_saving && dialog->display_to_close)
            {
              GimpDisplay *display = GIMP_DISPLAY (dialog->display_to_close);

              if (display &&
                  ! gimp_image_is_dirty (gimp_display_get_image (display)))
                {
                  gimp_display_close (display);
                }
            }

          gtk_widget_destroy (save_dialog);
        }

      g_free (uri);
      g_free (basename);

      gimp_file_dialog_set_sensitive (dialog, TRUE);
      break;

    case CHECK_URI_SWITCH_DIALOGS:
      dialog->busy = TRUE; /* prevent destruction */
      gtk_dialog_response (GTK_DIALOG (dialog), FILE_SAVE_RESPONSE_OTHER_DIALOG);
      dialog->busy = FALSE;

      gtk_widget_destroy (save_dialog);
      break;
    }

  g_object_unref (dialog->image);
  g_object_unref (dialog);
}

/* IMPORTANT: When changing this function, keep
 * file_save_dialog_no_overwrite_confirmation() up to date. It is
 * difficult to move logic to a common place due to how the dialog is
 * implemented in GTK+ in combination with how we use it.
 */
static CheckUriResult
file_save_dialog_check_uri (GtkWidget            *save_dialog,
                            Gimp                 *gimp,
                            gchar               **ret_uri,
                            gchar               **ret_basename,
                            GimpPlugInProcedure **ret_save_proc)
{
  GimpFileDialog      *dialog = GIMP_FILE_DIALOG (save_dialog);
  gchar               *uri;
  gchar               *basename;
  GimpPlugInProcedure *save_proc;
  GimpPlugInProcedure *uri_proc;
  GimpPlugInProcedure *basename_proc;

  uri = file_save_dialog_get_uri (dialog);

  if (! uri)
    return CHECK_URI_FAIL;

  basename      = file_utils_uri_display_basename (uri);

  save_proc     = dialog->file_proc;
  uri_proc      = file_procedure_find (file_save_dialog_get_procs (dialog, gimp),
                                       uri, NULL);
  basename_proc = file_procedure_find (file_save_dialog_get_procs (dialog, gimp),
                                       basename, NULL);

  GIMP_LOG (SAVE_DIALOG, "URI = %s", uri);
  GIMP_LOG (SAVE_DIALOG, "basename = %s", basename);
  GIMP_LOG (SAVE_DIALOG, "selected save_proc: %s",
            save_proc && save_proc->menu_label ?
            save_proc->menu_label : "NULL");
  GIMP_LOG (SAVE_DIALOG, "URI save_proc: %s",
            uri_proc ? uri_proc->menu_label : "NULL");
  GIMP_LOG (SAVE_DIALOG, "basename save_proc: %s",
            basename_proc && basename_proc->menu_label ?
            basename_proc->menu_label : "NULL");

  /*  first check if the user entered an extension at all  */
  if (! basename_proc)
    {
      GIMP_LOG (SAVE_DIALOG, "basename has no valid extension");

      if (! strchr (basename, '.'))
        {
          const gchar *ext = NULL;

          GIMP_LOG (SAVE_DIALOG, "basename has no '.', trying to add extension");

          if (! save_proc && ! dialog->export)
            {
              ext = "xcf";
            }
          else if (save_proc && save_proc->extensions_list)
            {
              ext = save_proc->extensions_list->data;
            }

          if (ext)
            {
              gchar *ext_basename;
              gchar *utf8;

              GIMP_LOG (SAVE_DIALOG, "appending .%s to basename", ext);

              ext_basename = g_strconcat (basename, ".", ext, NULL);

              g_free (uri);
              g_free (basename);

              basename = ext_basename;

              utf8 = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);
              gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (save_dialog),
                                                 utf8);
              g_free (utf8);

              GIMP_LOG (SAVE_DIALOG,
                        "set basename to %s, rerunning response and bailing out",
                        basename);

              /*  call the response callback again, so the
               *  overwrite-confirm logic can check the changed uri
               */
              gtk_dialog_response (GTK_DIALOG (save_dialog), GTK_RESPONSE_OK);

              g_free (basename);

              return CHECK_URI_FAIL;
            }
          else
            {
              GIMP_LOG (SAVE_DIALOG,
                        "save_proc has no extensions, continuing without");

              /*  there may be file formats with no extension at all, use
               *  the selected proc in this case.
               */
              basename_proc = save_proc;

              if (! uri_proc)
                uri_proc = basename_proc;
            }

          if (! basename_proc)
            {
              GIMP_LOG (SAVE_DIALOG,
                        "unable to figure save_proc, bailing out");

              if (file_save_dialog_switch_dialogs (dialog, gimp, basename))
                {
                  g_free (uri);
                  g_free (basename);

                  return CHECK_URI_SWITCH_DIALOGS;
                }

              g_free (uri);
              g_free (basename);

              return CHECK_URI_FAIL;
            }
        }
      else if (save_proc && ! save_proc->extensions_list)
        {
          GIMP_LOG (SAVE_DIALOG,
                    "basename has '.', but save_proc has no extensions, "
                    "accepting random extension");

          /*  accept any random extension if the file format has
           *  no extensions at all
           */
          basename_proc = save_proc;

          if (! uri_proc)
            uri_proc = basename_proc;
        }
    }

  /*  then check if the selected format matches the entered extension  */
  if (! save_proc)
    {
      GIMP_LOG (SAVE_DIALOG, "no save_proc was selected from the list");

      if (! basename_proc)
        {
          GIMP_LOG (SAVE_DIALOG,
                    "basename has no useful extension, bailing out");

          if (file_save_dialog_switch_dialogs (dialog, gimp, basename))
            {
              g_free (uri);
              g_free (basename);

              return CHECK_URI_SWITCH_DIALOGS;
            }

          g_free (uri);
          g_free (basename);

          return CHECK_URI_FAIL;
        }

      GIMP_LOG (SAVE_DIALOG, "use URI's proc '%s' so indirect saving works",
                uri_proc->menu_label ? uri_proc->menu_label : "<unnamed>");

      /*  use the URI's proc if no save proc was selected  */
      save_proc = uri_proc;
    }
  else
    {
      GIMP_LOG (SAVE_DIALOG, "save_proc '%s' was selected from the list",
                save_proc->menu_label ? save_proc->menu_label : "<unnamed>");

      if (save_proc != basename_proc)
        {
          GIMP_LOG (SAVE_DIALOG, "however the basename's proc is '%s'",
                    basename_proc ? basename_proc->menu_label : "NULL");

          if (uri_proc != basename_proc)
            {
              GIMP_LOG (SAVE_DIALOG,
                        "that's impossible for remote URIs, bailing out");

              /*  remote URI  */

              gimp_message (gimp, G_OBJECT (save_dialog), GIMP_MESSAGE_WARNING,
                            _("Saving remote files needs to determine the "
                              "file format from the file extension. "
                              "Please enter a file extension that matches "
                              "the selected file format or enter no file "
                              "extension at all."));
              g_free (uri);
              g_free (basename);

              return CHECK_URI_FAIL;
            }
          else
            {
              GIMP_LOG (SAVE_DIALOG,
                        "ask the user if she really wants that filename");

              /*  local URI  */

              if (! file_save_dialog_use_extension (save_dialog, uri))
                {
                  g_free (uri);
                  g_free (basename);

                  return CHECK_URI_FAIL;
                }
            }
        }
      else if (save_proc != uri_proc)
        {
          GIMP_LOG (SAVE_DIALOG,
                    "use URI's proc '%s' so indirect saving works",
                    uri_proc->menu_label ? uri_proc->menu_label : "<unnamed>");

          /*  need to use the URI's proc for saving because e.g.
           *  the GIF plug-in can't save a GIF to sftp://
           */
          save_proc = uri_proc;
        }
    }

  if (! save_proc)
    {
      g_warning ("%s: EEEEEEK", G_STRFUNC);

      return CHECK_URI_FAIL;
    }

  *ret_uri       = uri;
  *ret_basename  = basename;
  *ret_save_proc = save_proc;

  return CHECK_URI_OK;
}

/*
 * IMPORTANT: Keep this up to date with file_save_dialog_check_uri().
 */
static gboolean
file_save_dialog_no_overwrite_confirmation (GimpFileDialog *dialog,
                                            Gimp           *gimp)
{
  gboolean             uri_will_change = FALSE;
  gboolean             unknown_ext     = FALSE;
  gchar               *uri             = NULL;
  gchar               *basename        = NULL;
  GimpPlugInProcedure *basename_proc   = NULL;
  GimpPlugInProcedure *save_proc       = NULL;

  uri = file_save_dialog_get_uri (dialog);

  if (! uri)
    return FALSE;

  basename      = file_utils_uri_display_basename (uri);
  save_proc     = dialog->file_proc;
  basename_proc = file_procedure_find (file_save_dialog_get_procs (dialog, gimp),
                                       basename, NULL);

  uri_will_change = (! basename_proc &&
                     ! strchr (basename, '.') &&
                     (! save_proc || save_proc->extensions_list));

  unknown_ext     = (! save_proc &&
                     ! basename_proc);

  g_free (basename);
  g_free (uri);

  return uri_will_change || unknown_ext;
}

static gchar *
file_save_dialog_get_uri (GimpFileDialog *dialog)
{
  gchar *uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));

  if (uri && ! strlen (uri))
    {
      g_free (uri);
      uri = NULL;
    }

  return uri;
}

static GSList *
file_save_dialog_get_procs (GimpFileDialog *dialog,
                            Gimp           *gimp)
{
  return (! dialog->export ?
          gimp->plug_in_manager->save_procs :
          gimp->plug_in_manager->export_procs);
}

static gboolean
file_save_other_dialog_activated (GtkWidget   *label,
                                  const gchar *uri,
                                  GtkDialog   *dialog)
{
  gtk_dialog_response (dialog, FILE_SAVE_RESPONSE_OTHER_DIALOG);

  return TRUE;
}

static gboolean
file_save_dialog_switch_dialogs (GimpFileDialog *file_dialog,
                                 Gimp           *gimp,
                                 const gchar    *basename)
{
  GimpPlugInProcedure *proc_in_other_group;
  gboolean             switch_dialogs = FALSE;

  proc_in_other_group =
    file_procedure_find (file_dialog->export ?
                         gimp->plug_in_manager->save_procs :
                         gimp->plug_in_manager->export_procs,
                         basename,
                         NULL);

  if (proc_in_other_group)
    {
      GtkWidget   *dialog;
      const gchar *primary;
      const gchar *message;
      const gchar *link;

      if (file_dialog->export)
        {
          primary = _("The given filename cannot be used for exporting");
          message = _("You can use this dialog to export to various file formats. "
                      "If you want to save the image to the GIMP XCF format, use "
                      "File→Save instead.");
          link    = _("Take me to the Save dialog");
        }
      else
        {
          primary = _("The given filename cannot be used for saving");
          message = _("You can use this dialog to save to the GIMP XCF "
                      "format. Use File→Export to export to other file formats.");
          link    = _("Take me to the Export dialog");
        }

      dialog = gimp_message_dialog_new (_("Extension Mismatch"),
                                        GIMP_STOCK_WARNING,
                                        GTK_WIDGET (file_dialog),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        gimp_standard_help_func, NULL,

                                        GTK_STOCK_OK, GTK_RESPONSE_OK,

                                        NULL);

      gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                         "%s", primary);

      gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                 "%s", message);

      if (! file_dialog->save_a_copy && ! file_dialog->close_after_saving)
        {
          GtkWidget *label;
          gchar     *markup;

          markup = g_strdup_printf ("<a href=\"other-dialog\">%s</a>", link);
          label = gtk_label_new (markup);
          g_free (markup);

          gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
          gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
          gtk_box_pack_start (GTK_BOX (GIMP_MESSAGE_DIALOG (dialog)->box), label,
                              FALSE, FALSE, 0);
          gtk_widget_show (label);

          g_signal_connect (label, "activate-link",
                            G_CALLBACK (file_save_other_dialog_activated),
                            dialog);
        }

      gtk_dialog_set_response_sensitive (GTK_DIALOG (file_dialog),
                                         GTK_RESPONSE_CANCEL, FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (file_dialog),
                                         GTK_RESPONSE_OK, FALSE);

      g_object_ref (dialog);

      if (gimp_dialog_run (GIMP_DIALOG (dialog)) == FILE_SAVE_RESPONSE_OTHER_DIALOG)
        {
          switch_dialogs = TRUE;
        }

      gtk_widget_destroy (dialog);
      g_object_unref (dialog);

      gtk_dialog_set_response_sensitive (GTK_DIALOG (file_dialog),
                                         GTK_RESPONSE_CANCEL, TRUE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (file_dialog),
                                         GTK_RESPONSE_OK, TRUE);
    }
  else
    {
      gimp_message (gimp, G_OBJECT (file_dialog), GIMP_MESSAGE_WARNING,
                    _("The given filename does not have any known "
                      "file extension. Please enter a known file "
                      "extension or select a file format from the "
                      "file format list."));
    }

  return switch_dialogs;
}

static gboolean
file_save_dialog_use_extension (GtkWidget   *save_dialog,
                                const gchar *uri)
{
  GtkWidget *dialog;
  gboolean   use_name = FALSE;

  dialog = gimp_message_dialog_new (_("Extension Mismatch"),
                                    GIMP_STOCK_QUESTION,
                                    save_dialog, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func, NULL,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("The given file extension does "
                                       "not match the chosen file type."));

  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                             _("Do you want to save the image using this "
                               "name anyway?"));

  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_CANCEL, FALSE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_OK, FALSE);

  g_object_ref (dialog);

  use_name = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);
  g_object_unref (dialog);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_CANCEL, TRUE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_OK, TRUE);

  return use_name;
}

gboolean
file_save_dialog_save_image (GimpProgress        *progress,
                             Gimp                *gimp,
                             GimpImage           *image,
                             const gchar         *uri,
                             GimpPlugInProcedure *save_proc,
                             GimpRunMode          run_mode,
                             gboolean             change_saved_state,
                             gboolean             export_backward,
                             gboolean             export_forward,
                             gboolean             verbose_cancel)
{
  GimpPDBStatusType  status;
  GError            *error   = NULL;
  GList             *list;
  gboolean           success = FALSE;

  for (list = gimp_action_groups_from_name ("file");
       list;
       list = g_list_next (list))
    {
      gimp_action_group_set_action_sensitive (list->data, "file-quit", FALSE);
    }

  status = file_save (gimp, image, progress, uri,
                      save_proc, run_mode,
                      change_saved_state, export_backward, export_forward,
                      &error);

  switch (status)
    {
    case GIMP_PDB_SUCCESS:
      success = TRUE;
      break;

    case GIMP_PDB_CANCEL:
      if (verbose_cancel)
        gimp_message_literal (gimp,
                              G_OBJECT (progress), GIMP_MESSAGE_INFO,
                              _("Saving canceled"));
      break;

    default:
      {
        gchar *filename = file_utils_uri_display_name (uri);

        gimp_message (gimp, G_OBJECT (progress), GIMP_MESSAGE_ERROR,
                      _("Saving '%s' failed:\n\n%s"), filename, error->message);
        g_clear_error (&error);
        g_free (filename);
      }
      break;
    }

  for (list = gimp_action_groups_from_name ("file");
       list;
       list = g_list_next (list))
    {
      gimp_action_group_set_action_sensitive (list->data, "file-quit", TRUE);
    }

  return success;
}
