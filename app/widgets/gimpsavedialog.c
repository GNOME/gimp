/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasavedialog.c
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligma-utils.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-metadata.h"

#include "file/ligma-file.h"

#include "ligmahelp-ids.h"
#include "ligmasavedialog.h"

#include "ligma-intl.h"


typedef struct _LigmaSaveDialogState LigmaSaveDialogState;

struct _LigmaSaveDialogState
{
  gchar    *filter_name;
  gboolean  compression;
};


static void     ligma_save_dialog_constructed       (GObject             *object);

static void     ligma_save_dialog_save_state        (LigmaFileDialog      *dialog,
                                                    const gchar         *state_name);
static void     ligma_save_dialog_load_state        (LigmaFileDialog      *dialog,
                                                    const gchar         *state_name);

static void     ligma_save_dialog_add_extra_widgets (LigmaSaveDialog      *dialog);
static void     ligma_save_dialog_compression_toggled
                                                   (GtkToggleButton     *button,
                                                    LigmaSaveDialog      *dialog);

static LigmaSaveDialogState
              * ligma_save_dialog_get_state         (LigmaSaveDialog      *dialog);
static void     ligma_save_dialog_set_state         (LigmaSaveDialog      *dialog,
                                                    LigmaSaveDialogState *state);
static void     ligma_save_dialog_state_destroy     (LigmaSaveDialogState *state);


G_DEFINE_TYPE (LigmaSaveDialog, ligma_save_dialog,
               LIGMA_TYPE_FILE_DIALOG)

#define parent_class ligma_save_dialog_parent_class


static void
ligma_save_dialog_class_init (LigmaSaveDialogClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaFileDialogClass *fd_class     = LIGMA_FILE_DIALOG_CLASS (klass);

  object_class->constructed = ligma_save_dialog_constructed;

  fd_class->save_state      = ligma_save_dialog_save_state;
  fd_class->load_state      = ligma_save_dialog_load_state;
}

static void
ligma_save_dialog_init (LigmaSaveDialog *dialog)
{
}

static void
ligma_save_dialog_constructed (GObject *object)
{
  LigmaSaveDialog *dialog = LIGMA_SAVE_DIALOG (object);

  /* LigmaFileDialog's constructed() is doing a few initialization
   * common to all file dialogs.
   */
  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_save_dialog_add_extra_widgets (dialog);
}

static void
ligma_save_dialog_save_state (LigmaFileDialog *dialog,
                             const gchar    *state_name)
{
  g_object_set_data_full (G_OBJECT (dialog->ligma), state_name,
                          ligma_save_dialog_get_state (LIGMA_SAVE_DIALOG (dialog)),
                          (GDestroyNotify) ligma_save_dialog_state_destroy);
}

static void
ligma_save_dialog_load_state (LigmaFileDialog *dialog,
                             const gchar    *state_name)
{
  LigmaSaveDialogState *state;

  state = g_object_get_data (G_OBJECT (dialog->ligma), state_name);

  if (state)
    ligma_save_dialog_set_state (LIGMA_SAVE_DIALOG (dialog), state);
}


/*  public functions  */

GtkWidget *
ligma_save_dialog_new (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_new (LIGMA_TYPE_SAVE_DIALOG,
                       "ligma",                  ligma,
                       "title",                 _("Save Image"),
                       "role",                  "ligma-file-save",
                       "help-id",               LIGMA_HELP_FILE_SAVE,
                       "ok-button-label",       _("_Save"),

                       "automatic-label",       _("By Extension"),
                       "automatic-help-id",     LIGMA_HELP_FILE_SAVE_BY_EXTENSION,

                       "action",                GTK_FILE_CHOOSER_ACTION_SAVE,
                       "file-procs",            LIGMA_FILE_PROCEDURE_GROUP_SAVE,
                       "file-procs-all-images", LIGMA_FILE_PROCEDURE_GROUP_EXPORT,
                       "file-filter-label",     _("All XCF images"),
                       NULL);
}

void
ligma_save_dialog_set_image (LigmaSaveDialog *dialog,
                            LigmaImage      *image,
                            gboolean        save_a_copy,
                            gboolean        close_after_saving,
                            LigmaObject     *display)
{
  LigmaFileDialog *file_dialog;
  GtkWidget      *compression_toggle;
  GFile          *dir_file  = NULL;
  GFile          *name_file = NULL;
  GFile          *ext_file  = NULL;
  gchar          *basename;
  const gchar    *version_string;
  gint            rle_version;
  gint            zlib_version;

  g_return_if_fail (LIGMA_IS_SAVE_DIALOG (dialog));
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  file_dialog = LIGMA_FILE_DIALOG (dialog);

  file_dialog->image         = image;
  dialog->save_a_copy        = save_a_copy;
  dialog->close_after_saving = close_after_saving;
  dialog->display_to_close   = display;

  ligma_file_dialog_set_file_proc (file_dialog, NULL);

  /*
   * Priority of default paths for Save:
   *
   *   1. Last Save a copy-path (applies only to Save a copy)
   *   2. Last Save path
   *   3. Path of source XCF
   *   4. Path of Import source
   *   5. Last Save path of any LIGMA document
   *   6. The default path (usually the OS 'Documents' path)
   */

  if (save_a_copy)
    dir_file = ligma_image_get_save_a_copy_file (image);

  if (! dir_file)
    dir_file = ligma_image_get_file (image);

  if (! dir_file)
    dir_file = g_object_get_data (G_OBJECT (image),
                                  "ligma-image-source-file");

  if (! dir_file)
    dir_file = ligma_image_get_imported_file (image);

  if (! dir_file)
    dir_file = g_object_get_data (G_OBJECT (file_dialog->ligma),
                                  LIGMA_FILE_SAVE_LAST_FILE_KEY);

  if (! dir_file)
    dir_file = ligma_file_dialog_get_default_folder (file_dialog);


  /* Priority of default basenames for Save:
   *
   *   1. Last Save a copy-name (applies only to Save a copy)
   *   2. Last Save name
   *   3. Last Export name
   *   3. The source image path
   *   3. 'Untitled'
   */

  if (save_a_copy)
    name_file = ligma_image_get_save_a_copy_file (image);

  if (! name_file)
    name_file = ligma_image_get_file (image);

  if (! name_file)
    name_file = ligma_image_get_exported_file (image);

  if (! name_file)
    name_file = ligma_image_get_imported_file (image);

  if (! name_file)
    name_file = ligma_image_get_untitled_file (image);


  /* Priority of default type/extension for Save:
   *
   *   1. Type of last Save
   *   2. .xcf (which we don't explicitly append)
   */

  ext_file = ligma_image_get_file (image);

  if (ext_file)
    g_object_ref (ext_file);
  else
    ext_file = g_file_new_for_uri ("file:///we/only/care/about/extension.xcf");

  ligma_image_get_xcf_version (image, FALSE, &rle_version,
                              &version_string, NULL);
  ligma_image_get_xcf_version (image, TRUE,  &zlib_version,
                              NULL, NULL);
  if (rle_version != zlib_version)
    {
      GtkWidget *label;
      gchar     *text;

      text = g_strdup_printf (_("Keep compression disabled to make the XCF "
                                "file readable by %s and later."),
                              version_string);
      label = gtk_label_new (text);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      ligma_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);
      gtk_container_add (GTK_CONTAINER (dialog->compression_frame),
                         label);
      gtk_widget_show (label);
      g_free (text);
    }

  compression_toggle = gtk_frame_get_label_widget (GTK_FRAME (dialog->compression_frame));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (compression_toggle),
                                ligma_image_get_xcf_compression (image));
  /* Force a "toggled" signal since gtk_toggle_button_set_active() won't
   * send it if the button status doesn't change.
   */
  gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON (compression_toggle));

  if (ext_file)
    {
      GFile *tmp_file = ligma_file_with_new_extension (name_file, ext_file);
      basename = g_path_get_basename (ligma_file_get_utf8_name (tmp_file));
      g_object_unref (tmp_file);
      g_object_unref (ext_file);
    }
  else
    {
      basename = g_path_get_basename (ligma_file_get_utf8_name (name_file));
    }

  if (g_file_query_file_type (dir_file, G_FILE_QUERY_INFO_NONE, NULL) ==
      G_FILE_TYPE_DIRECTORY)
    {
      gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                dir_file, NULL);
    }
  else
    {
      GFile *parent_file = g_file_get_parent (dir_file);
      gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                parent_file, NULL);
      g_object_unref (parent_file);
    }

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), basename);
  g_free (basename);
}


/*  private functions  */

static void
ligma_save_dialog_add_extra_widgets (LigmaSaveDialog *dialog)
{
  GtkWidget *label;
  GtkWidget *reasons;
  GtkWidget *compression_toggle;

  /* Compression toggle. */
  compression_toggle =
    gtk_check_button_new_with_mnemonic (_("Save this _XCF file with better but slower compression"));
  gtk_widget_set_tooltip_text (compression_toggle,
                               _("On edge cases, better compression algorithms might still "
                                 "end up on bigger file size; manual check recommended"));

  dialog->compression_frame = ligma_frame_new (NULL);
  gtk_frame_set_label_widget (GTK_FRAME (dialog->compression_frame), compression_toggle);
  gtk_widget_show (compression_toggle);
  ligma_file_dialog_add_extra_widget (LIGMA_FILE_DIALOG (dialog), dialog->compression_frame,
                                     FALSE, FALSE, 0);
  gtk_widget_show (dialog->compression_frame);

  /* Additional information explaining file compatibility things */
  dialog->compat_info = gtk_expander_new (NULL);
  label = gtk_label_new ("");
  gtk_expander_set_label_widget (GTK_EXPANDER (dialog->compat_info), label);
  gtk_widget_show (label);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  reasons = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (reasons), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (reasons), FALSE);
  gtk_container_add (GTK_CONTAINER (dialog->compat_info), reasons);
  gtk_widget_show (reasons);

  ligma_file_dialog_add_extra_widget (LIGMA_FILE_DIALOG (dialog),
                                     dialog->compat_info,
                                     FALSE, FALSE, 0);
  gtk_widget_show (dialog->compat_info);

  g_signal_connect (compression_toggle, "toggled",
                    G_CALLBACK (ligma_save_dialog_compression_toggled),
                    dialog);
}

static void
ligma_save_dialog_compression_toggled (GtkToggleButton *button,
                                      LigmaSaveDialog  *dialog)
{
  const gchar    *version_string = NULL;
  LigmaFileDialog *file_dialog    = LIGMA_FILE_DIALOG (dialog);
  gchar          *compat_hint    = NULL;
  gchar          *reason         = NULL;
  GtkWidget      *widget;
  GtkTextBuffer  *text_buffer;
  gint            version;

  if (! file_dialog->image)
    return;

  dialog->compression = gtk_toggle_button_get_active (button);

  if (dialog->compression)
    ligma_image_get_xcf_version (file_dialog->image, TRUE,  &version,
                                &version_string, &reason);
  else
    ligma_image_get_xcf_version (file_dialog->image, FALSE, &version,
                                &version_string, &reason);

  /* Only show compatibility information for LIGMA over 2.6. The reason
   * is mostly that we don't have details to make a compatibility list
   * with this older version.
   * It's anyway so prehistorical that we are not really caring about
   * compatibility with older version.
   */
  if (version <= 206)
    gtk_widget_hide (dialog->compat_info);
  else
    gtk_widget_show (dialog->compat_info);

  /* Set the compatibility label. */
  compat_hint =
    g_strdup_printf (_("The image uses features from %s and "
                       "won't be readable by older LIGMA versions."),
                     version_string);

  if (ligma_image_get_metadata (file_dialog->image))
    {
      gchar *temp_hint;

      temp_hint = g_strconcat (compat_hint, "\n",
                               _("Metadata won't be visible in LIGMA "
                                 "older than version 2.10."), NULL);
      g_free (compat_hint);
      compat_hint = temp_hint;
    }

  widget = gtk_expander_get_label_widget (GTK_EXPANDER (dialog->compat_info));
  gtk_label_set_text (GTK_LABEL (widget), compat_hint);
  g_free (compat_hint);

  /* Fill in the details (list of compatibility reasons). */
  widget = gtk_bin_get_child (GTK_BIN (dialog->compat_info));
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  gtk_text_buffer_set_text (text_buffer, reason ? reason : "", -1);
  if (reason)
    g_free (reason);
}

static LigmaSaveDialogState *
ligma_save_dialog_get_state (LigmaSaveDialog *dialog)
{
  LigmaSaveDialogState *state;
  GtkFileFilter       *filter;

  state = g_slice_new0 (LigmaSaveDialogState);

  filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (dialog));

  if (filter)
    state->filter_name = g_strdup (gtk_file_filter_get_name (filter));

  state->compression = dialog->compression;

  return state;
}

static void
ligma_save_dialog_set_state (LigmaSaveDialog      *dialog,
                            LigmaSaveDialogState *state)
{
  if (state->filter_name)
    {
      GSList *filters;
      GSList *list;

      filters = gtk_file_chooser_list_filters (GTK_FILE_CHOOSER (dialog));

      for (list = filters; list; list = list->next)
        {
          GtkFileFilter *filter = GTK_FILE_FILTER (list->data);
          const gchar   *name   = gtk_file_filter_get_name (filter);

          if (name && strcmp (state->filter_name, name) == 0)
            {
              gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);
              break;
            }
        }

      g_slist_free (filters);
    }

  dialog->compression = state->compression;
}

static void
ligma_save_dialog_state_destroy (LigmaSaveDialogState *state)
{
  g_free (state->filter_name);
  g_slice_free (LigmaSaveDialogState, state);
}
