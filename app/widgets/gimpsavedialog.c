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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-metadata.h"

#include "file/file-utils.h"
#include "file/gimp-file.h"

#include "pdb/gimppdb.h"

#include "plug-in/gimppluginmanager.h"

#include "gimpfiledialog.h"
#include "gimphelp-ids.h"
#include "gimpsavedialog.h"

#include "gimp-intl.h"


typedef struct _GimpSaveDialogState GimpSaveDialogState;

struct _GimpSaveDialogState
{
  gchar    *filter_name;
  gboolean  compat;
};


static void     gimp_save_dialog_constructed         (GObject             *object);

static void     gimp_save_dialog_save_state          (GimpFileDialog      *dialog,
                                                      const gchar         *state_name);
static void     gimp_save_dialog_load_state          (GimpFileDialog      *dialog,
                                                      const gchar         *state_name);

static void     gimp_save_dialog_add_compat_toggle   (GimpSaveDialog      *dialog);
static void     gimp_save_dialog_compat_toggled      (GtkToggleButton     *button,
                                                      GimpSaveDialog      *dialog);

static GimpSaveDialogState
              * gimp_save_dialog_get_state           (GimpSaveDialog      *dialog);
static void     gimp_save_dialog_set_state           (GimpSaveDialog      *dialog,
                                                      GimpSaveDialogState *state);
static void     gimp_save_dialog_state_destroy       (GimpSaveDialogState *state);


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

  gimp_save_dialog_add_compat_toggle (dialog);
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
  GimpSaveDialog *dialog;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = g_object_new (GIMP_TYPE_SAVE_DIALOG,
                         "gimp",                      gimp,
                         "title",                     _("Save Image"),
                         "role",                      "gimp-file-save",
                         "help-id",                   GIMP_HELP_FILE_SAVE,
                         "stock-id",                  GTK_STOCK_SAVE,

                         "automatic-label",           _("By Extension"),
                         "automatic-help-id",         GIMP_HELP_FILE_SAVE_BY_EXTENSION,

                         "action",                    GTK_FILE_CHOOSER_ACTION_SAVE,
                         "file-procs",                gimp->plug_in_manager->save_procs,
                         "file-procs-all-images",     gimp->plug_in_manager->export_procs,
                         "file-filter-label",         _("All XCF images"),
                         "local-only",                FALSE,
                         "do-overwrite-confirmation", TRUE,
                         NULL);

  return GTK_WIDGET (dialog);
}

void
gimp_save_dialog_set_image (GimpSaveDialog *dialog,
                            Gimp           *gimp,
                            GimpImage      *image,
                            gboolean        save_a_copy,
                            gboolean        close_after_saving,
                            GimpObject     *display)
{
  GimpFileDialog *file_dialog;
  GFile          *dir_file  = NULL;
  GFile          *name_file = NULL;
  GFile          *ext_file  = NULL;
  gchar          *basename;
  const gchar    *version_string;
  gint            rle_version;
  gint            zlib_version;
  gchar          *tooltip;

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
    dir_file = g_object_get_data (G_OBJECT (gimp),
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

  gimp_image_get_xcf_version (image, FALSE, &rle_version,  &version_string);
  gimp_image_get_xcf_version (image, TRUE,  &zlib_version, NULL);

  if (rle_version == zlib_version)
    {
      gtk_widget_set_sensitive (dialog->compat_toggle, FALSE);
      gtk_widget_set_sensitive (dialog->compat_info, FALSE);

      tooltip = g_strdup_printf (_("The image uses features from %s and "
                                   "cannot be saved for older GIMP "
                                   "versions."),
                                 version_string);
    }
  else
    {
      gtk_widget_set_sensitive (dialog->compat_toggle, TRUE);
      gtk_widget_set_sensitive (dialog->compat_info, TRUE);

      tooltip = g_strdup_printf (_("Disables compression to make the XCF "
                                   "file readable by %s and later."),
                                 version_string);
      if (gimp_image_get_metadata (image))
        {
          gchar *temp_tooltip;

          temp_tooltip = g_strconcat (tooltip, "\n",
                                      _("Metadata won't be visible in GIMP "
                                        "older than version 2.10."), NULL);
          g_free (tooltip);
          tooltip = temp_tooltip;
        }
    }

  gtk_label_set_text (GTK_LABEL (dialog->compat_info), tooltip);
  g_free (tooltip);

  gtk_widget_show (dialog->compat_toggle);
  gtk_widget_show (dialog->compat_info);

  /* We set the compatibility mode by default either if the image was
  * previously saved with the compatibility mode, or if it has never been
  * saved and the last GimpSaveDialogState had compatibility mode ON. */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->compat_toggle),
                                gtk_widget_get_sensitive (dialog->compat_toggle) &&
                                (gimp_image_get_xcf_compat_mode (image) ||
                                 (! gimp_image_get_file (image) && dialog->compat)));

  if (ext_file)
    {
      GFile *tmp_file = file_utils_file_with_new_ext (name_file, ext_file);
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
}


/*  private functions  */

static void
gimp_save_dialog_add_compat_toggle (GimpSaveDialog *dialog)
{
  GtkWidget *compat_frame;

  compat_frame = gimp_frame_new (NULL);

  /* The checkbox. */
  dialog->compat_toggle =
    gtk_check_button_new_with_label (_("Save this XCF file with maximum compatibility"));
  gtk_frame_set_label_widget (GTK_FRAME (compat_frame),
                              dialog->compat_toggle);

  /* Additional information explaining what this mode does. */
  dialog->compat_info = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (dialog->compat_info), 0.0, 0.0);
  gimp_label_set_attributes (GTK_LABEL (dialog->compat_info),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_container_add (GTK_CONTAINER (compat_frame), dialog->compat_info);

  gimp_file_dialog_add_extra_widget (GIMP_FILE_DIALOG (dialog),
                                     compat_frame,
                                     FALSE, FALSE, 0);
  gtk_widget_show (compat_frame);

  g_signal_connect (dialog->compat_toggle, "toggled",
                    G_CALLBACK (gimp_save_dialog_compat_toggled),
                    dialog);
}

static void
gimp_save_dialog_compat_toggled (GtkToggleButton *button,
                                 GimpSaveDialog  *dialog)
{
  dialog->compat = gtk_toggle_button_get_active (button);
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

  state->compat = dialog->compat;

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

  dialog->compat = state->compat;
}

static void
gimp_save_dialog_state_destroy (GimpSaveDialogState *state)
{
  g_free (state->filter_name);
  g_slice_free (GimpSaveDialogState, state);
}
