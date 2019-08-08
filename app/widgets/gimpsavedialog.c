/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsavedialog.c
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpimage.h"
#include "core/gimpimage-metadata.h"

#include "file/gimp-file.h"

#include "gimphelp-ids.h"
#include "gimpsavedialog.h"

#include "gimp-intl.h"


typedef struct _GimpSaveDialogState GimpSaveDialogState;

struct _GimpSaveDialogState
{
  gchar    *filter_name;
  gboolean  compression;
};


static void     gimp_save_dialog_constructed       (GObject             *object);

static void     gimp_save_dialog_save_state        (GimpFileDialog      *dialog,
                                                    const gchar         *state_name);
static void     gimp_save_dialog_load_state        (GimpFileDialog      *dialog,
                                                    const gchar         *state_name);

static void     gimp_save_dialog_add_extra_widgets (GimpSaveDialog      *dialog);
static void     gimp_save_dialog_compression_toggled
                                                   (GtkToggleButton     *button,
                                                    GimpSaveDialog      *dialog);

static GimpSaveDialogState
              * gimp_save_dialog_get_state         (GimpSaveDialog      *dialog);
static void     gimp_save_dialog_set_state         (GimpSaveDialog      *dialog,
                                                    GimpSaveDialogState *state);
static void     gimp_save_dialog_state_destroy     (GimpSaveDialogState *state);


G_DEFINE_TYPE (GimpSaveDialog, gimp_save_dialog,
               GIMP_TYPE_FILE_DIALOG)

#define parent_class gimp_save_dialog_parent_class


static void
gimp_save_dialog_class_init (GimpSaveDialogClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpFileDialogClass *fd_class     = GIMP_FILE_DIALOG_CLASS (klass);

  object_class->constructed = gimp_save_dialog_constructed;

  fd_class->save_state      = gimp_save_dialog_save_state;
  fd_class->load_state      = gimp_save_dialog_load_state;
}

static void
gimp_save_dialog_init (GimpSaveDialog *dialog)
{
}

static void
gimp_save_dialog_constructed (GObject *object)
{
  GimpSaveDialog *dialog = GIMP_SAVE_DIALOG (object);

  /* GimpFileDialog's constructed() is doing a few initialization
   * common to all file dialogs.
   */
  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_save_dialog_add_extra_widgets (dialog);
}

static void
gimp_save_dialog_save_state (GimpFileDialog *dialog,
                             const gchar    *state_name)
{
  g_object_set_data_full (G_OBJECT (dialog->gimp), state_name,
                          gimp_save_dialog_get_state (GIMP_SAVE_DIALOG (dialog)),
                          (GDestroyNotify) gimp_save_dialog_state_destroy);
}

static void
gimp_save_dialog_load_state (GimpFileDialog *dialog,
                             const gchar    *state_name)
{
  GimpSaveDialogState *state;

  state = g_object_get_data (G_OBJECT (dialog->gimp), state_name);

  if (state)
    gimp_save_dialog_set_state (GIMP_SAVE_DIALOG (dialog), state);
}


/*  public functions  */

GtkWidget *
gimp_save_dialog_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_SAVE_DIALOG,
                       "gimp",                  gimp,
                       "title",                 _("Save Image"),
                       "role",                  "gimp-file-save",
                       "help-id",               GIMP_HELP_FILE_SAVE,
                       "ok-button-label",       _("_Save"),

                       "automatic-label",       _("By Extension"),
                       "automatic-help-id",     GIMP_HELP_FILE_SAVE_BY_EXTENSION,

                       "action",                GTK_FILE_CHOOSER_ACTION_SAVE,
                       "file-procs",            GIMP_FILE_PROCEDURE_GROUP_SAVE,
                       "file-procs-all-images", GIMP_FILE_PROCEDURE_GROUP_EXPORT,
                       "file-filter-label",     _("All XCF images"),
                       NULL);
}

void
gimp_save_dialog_set_image (GimpSaveDialog *dialog,
                            GimpImage      *image,
                            gboolean        save_a_copy,
                            gboolean        close_after_saving,
                            GimpObject     *display)
{
  GimpFileDialog *file_dialog;
  GtkWidget      *compression_toggle;
  GFile          *dir_file  = NULL;
  GFile          *name_file = NULL;
  GFile          *ext_file  = NULL;
  gchar          *basename;
  const gchar    *version_string;
  gint            rle_version;
  gint            zlib_version;

  g_return_if_fail (GIMP_IS_SAVE_DIALOG (dialog));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  file_dialog = GIMP_FILE_DIALOG (dialog);

  file_dialog->image         = image;
  dialog->save_a_copy        = save_a_copy;
  dialog->close_after_saving = close_after_saving;
  dialog->display_to_close   = display;

  gimp_file_dialog_set_file_proc (file_dialog, NULL);

  /*
   * Priority of default paths for Save:
   *
   *   1. Last Save a copy-path (applies only to Save a copy)
   *   2. Last Save path
   *   3. Path of source XCF
   *   4. Path of Import source
   *   5. Last Save path of any GIMP document
   *   6. The default path (usually the OS 'Documents' path)
   */

  if (save_a_copy)
    dir_file = gimp_image_get_save_a_copy_file (image);

  if (! dir_file)
    dir_file = gimp_image_get_file (image);

  if (! dir_file)
    dir_file = g_object_get_data (G_OBJECT (image),
                                  "gimp-image-source-file");

  if (! dir_file)
    dir_file = gimp_image_get_imported_file (image);

  if (! dir_file)
    dir_file = g_object_get_data (G_OBJECT (file_dialog->gimp),
                                  GIMP_FILE_SAVE_LAST_FILE_KEY);

  if (! dir_file)
    dir_file = gimp_file_dialog_get_default_folder (file_dialog);


  /* Priority of default basenames for Save:
   *
   *   1. Last Save a copy-name (applies only to Save a copy)
   *   2. Last Save name
   *   3. Last Export name
   *   3. The source image path
   *   3. 'Untitled'
   */

  if (save_a_copy)
    name_file = gimp_image_get_save_a_copy_file (image);

  if (! name_file)
    name_file = gimp_image_get_file (image);

  if (! name_file)
    name_file = gimp_image_get_exported_file (image);

  if (! name_file)
    name_file = gimp_image_get_imported_file (image);

  if (! name_file)
    name_file = gimp_image_get_untitled_file (image);


  /* Priority of default type/extension for Save:
   *
   *   1. Type of last Save
   *   2. .xcf (which we don't explicitly append)
   */

  ext_file = gimp_image_get_file (image);

  if (ext_file)
    g_object_ref (ext_file);
  else
    ext_file = g_file_new_for_uri ("file:///we/only/care/about/extension.xcf");

  gimp_image_get_xcf_version (image, FALSE, &rle_version,
                              &version_string, NULL);
  gimp_image_get_xcf_version (image, TRUE,  &zlib_version,
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
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);
      gtk_container_add (GTK_CONTAINER (dialog->compression_frame),
                         label);
      gtk_widget_show (label);
      g_free (text);
    }

  compression_toggle = gtk_frame_get_label_widget (GTK_FRAME (dialog->compression_frame));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (compression_toggle),
                                gimp_image_get_xcf_compression (image));
  /* Force a "toggled" signal since gtk_toggle_button_set_active() won't
   * send it if the button status doesn't change.
   */
  gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON (compression_toggle));

  if (ext_file)
    {
      GFile *tmp_file = gimp_file_with_new_extension (name_file, ext_file);
      basename = g_path_get_basename (gimp_file_get_utf8_name (tmp_file));
      g_object_unref (tmp_file);
      g_object_unref (ext_file);
    }
  else
    {
      basename = g_path_get_basename (gimp_file_get_utf8_name (name_file));
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
gimp_save_dialog_add_extra_widgets (GimpSaveDialog *dialog)
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

  dialog->compression_frame = gimp_frame_new (NULL);
  gtk_frame_set_label_widget (GTK_FRAME (dialog->compression_frame), compression_toggle);
  gtk_widget_show (compression_toggle);
  gimp_file_dialog_add_extra_widget (GIMP_FILE_DIALOG (dialog), dialog->compression_frame,
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

  gimp_file_dialog_add_extra_widget (GIMP_FILE_DIALOG (dialog),
                                     dialog->compat_info,
                                     FALSE, FALSE, 0);
  gtk_widget_show (dialog->compat_info);

  g_signal_connect (compression_toggle, "toggled",
                    G_CALLBACK (gimp_save_dialog_compression_toggled),
                    dialog);
}

static void
gimp_save_dialog_compression_toggled (GtkToggleButton *button,
                                      GimpSaveDialog  *dialog)
{
  const gchar    *version_string = NULL;
  GimpFileDialog *file_dialog    = GIMP_FILE_DIALOG (dialog);
  gchar          *compat_hint    = NULL;
  gchar          *reason         = NULL;
  GtkWidget      *widget;
  GtkTextBuffer  *text_buffer;
  gint            version;

  if (! file_dialog->image)
    return;

  dialog->compression = gtk_toggle_button_get_active (button);

  if (dialog->compression)
    gimp_image_get_xcf_version (file_dialog->image, TRUE,  &version,
                                &version_string, &reason);
  else
    gimp_image_get_xcf_version (file_dialog->image, FALSE, &version,
                                &version_string, &reason);

  /* Only show compatibility information for GIMP over 2.6. The reason
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
                       "won't be readable by older GIMP versions."),
                     version_string);

  if (gimp_image_get_metadata (file_dialog->image))
    {
      gchar *temp_hint;

      temp_hint = g_strconcat (compat_hint, "\n",
                               _("Metadata won't be visible in GIMP "
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

static GimpSaveDialogState *
gimp_save_dialog_get_state (GimpSaveDialog *dialog)
{
  GimpSaveDialogState *state;
  GtkFileFilter       *filter;

  state = g_slice_new0 (GimpSaveDialogState);

  filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (dialog));

  if (filter)
    state->filter_name = g_strdup (gtk_file_filter_get_name (filter));

  state->compression = dialog->compression;

  return state;
}

static void
gimp_save_dialog_set_state (GimpSaveDialog      *dialog,
                            GimpSaveDialogState *state)
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
gimp_save_dialog_state_destroy (GimpSaveDialogState *state)
{
  g_free (state->filter_name);
  g_slice_free (GimpSaveDialogState, state);
}
