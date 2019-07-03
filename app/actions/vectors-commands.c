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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpdialogconfig.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-undo.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"
#include "core/gimptoolinfo.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpvectors-export.h"
#include "vectors/gimpvectors-import.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpclipboard.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "tools/gimpvectortool.h"
#include "tools/tool_manager.h"

#include "dialogs/dialogs.h"
#include "dialogs/vectors-export-dialog.h"
#include "dialogs/vectors-import-dialog.h"
#include "dialogs/vectors-options-dialog.h"

#include "actions.h"
#include "items-commands.h"
#include "vectors-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   vectors_new_callback             (GtkWidget    *dialog,
                                                GimpImage    *image,
                                                GimpVectors  *vectors,
                                                GimpContext  *context,
                                                const gchar  *vectors_name,
                                                gboolean      vectors_visible,
                                                gboolean      vectors_linked,
                                                GimpColorTag  vectors_color_tag,
                                                gboolean      vectors_lock_content,
                                                gboolean      vectors_lock_position,
                                                gpointer      user_data);
static void   vectors_edit_attributes_callback (GtkWidget    *dialog,
                                                GimpImage    *image,
                                                GimpVectors  *vectors,
                                                GimpContext  *context,
                                                const gchar  *vectors_name,
                                                gboolean      vectors_visible,
                                                gboolean      vectors_linked,
                                                GimpColorTag  vectors_color_tag,
                                                gboolean      vectors_lock_content,
                                                gboolean      vectors_lock_position,
                                                gpointer      user_data);
static void   vectors_import_callback          (GtkWidget    *dialog,
                                                GimpImage    *image,
                                                GFile        *file,
                                                GFile        *import_folder,
                                                gboolean      merge_vectors,
                                                gboolean      scale_vectors,
                                                gpointer      user_data);
static void   vectors_export_callback          (GtkWidget    *dialog,
                                                GimpImage    *image,
                                                GFile        *file,
                                                GFile        *export_folder,
                                                gboolean      active_only,
                                                gpointer      user_data);


/*  public functions  */

void
vectors_edit_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  GimpTool    *active_tool;
  return_if_no_vectors (image, vectors, data);

  active_tool = tool_manager_get_active (image->gimp);

  if (! GIMP_IS_VECTOR_TOOL (active_tool))
    {
      GimpToolInfo  *tool_info = gimp_get_tool_info (image->gimp,
                                                     "gimp-vector-tool");

      if (GIMP_IS_TOOL_INFO (tool_info))
        {
          gimp_context_set_tool (action_data_get_context (data), tool_info);
          active_tool = tool_manager_get_active (image->gimp);
        }
    }

  if (GIMP_IS_VECTOR_TOOL (active_tool))
    gimp_vector_tool_set_vectors (GIMP_VECTOR_TOOL (active_tool), vectors);
}

void
vectors_edit_attributes_cmd_callback (GimpAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_vectors (image, vectors, data);
  return_if_no_widget (widget, data);

#define EDIT_DIALOG_KEY "gimp-vectors-edit-attributes-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (vectors), EDIT_DIALOG_KEY);

  if (! dialog)
    {
      GimpItem *item = GIMP_ITEM (vectors);

      dialog = vectors_options_dialog_new (image, vectors,
                                           action_data_get_context (data),
                                           widget,
                                           _("Path Attributes"),
                                           "gimp-vectors-edit",
                                           GIMP_ICON_EDIT,
                                           _("Edit Path Attributes"),
                                           GIMP_HELP_PATH_EDIT,
                                           gimp_object_get_name (vectors),
                                           gimp_item_get_visible (item),
                                           gimp_item_get_linked (item),
                                           gimp_item_get_color_tag (item),
                                           gimp_item_get_lock_content (item),
                                           gimp_item_get_lock_position (item),
                                           vectors_edit_attributes_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (vectors), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
vectors_new_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define NEW_DIALOG_KEY "gimp-vectors-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), NEW_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

      dialog = vectors_options_dialog_new (image, NULL,
                                           action_data_get_context (data),
                                           widget,
                                           _("New Path"),
                                           "gimp-vectors-new",
                                           GIMP_ICON_PATH,
                                           _("Create a New Path"),
                                           GIMP_HELP_PATH_NEW,
                                           config->vectors_new_name,
                                           FALSE,
                                           FALSE,
                                           GIMP_COLOR_TAG_NONE,
                                           FALSE,
                                           FALSE,
                                           vectors_new_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
vectors_new_last_vals_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage        *image;
  GimpVectors      *vectors;
  GimpDialogConfig *config;
  return_if_no_image (image, data);

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  vectors = gimp_vectors_new (image, config->vectors_new_name);
  gimp_image_add_vectors (image, vectors,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_image_flush (image);
}

void
vectors_raise_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_image_raise_item (image, GIMP_ITEM (vectors), NULL);
  gimp_image_flush (image);
}

void
vectors_raise_to_top_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_image_raise_item_to_top (image, GIMP_ITEM (vectors));
  gimp_image_flush (image);
}

void
vectors_lower_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_image_lower_item (image, GIMP_ITEM (vectors), NULL);
  gimp_image_flush (image);
}

void
vectors_lower_to_bottom_cmd_callback (GimpAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_image_lower_item_to_bottom (image, GIMP_ITEM (vectors));
  gimp_image_flush (image);
}

void
vectors_duplicate_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  GimpVectors *new_vectors;
  return_if_no_vectors (image, vectors, data);

  new_vectors = GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                                   G_TYPE_FROM_INSTANCE (vectors)));
  /*  use the actual parent here, not GIMP_IMAGE_ACTIVE_PARENT because
   *  the latter would add a duplicated group inside itself instead of
   *  above it
   */
  gimp_image_add_vectors (image, new_vectors,
                          gimp_vectors_get_parent (vectors), -1,
                          TRUE);
  gimp_image_flush (image);
}

void
vectors_delete_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_image_remove_vectors (image, vectors, TRUE, NULL);
  gimp_image_flush (image);
}

void
vectors_merge_visible_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  GtkWidget   *widget;
  GError      *error = NULL;
  return_if_no_vectors (image, vectors, data);
  return_if_no_widget (widget, data);

  if (! gimp_image_merge_visible_vectors (image, &error))
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
      return;
    }

  gimp_image_flush (image);
}

void
vectors_to_selection_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage      *image;
  GimpVectors    *vectors;
  GimpChannelOps  operation;
  return_if_no_vectors (image, vectors, data);

  operation = (GimpChannelOps) g_variant_get_int32 (value);

  gimp_item_to_selection (GIMP_ITEM (vectors), operation,
                          TRUE, FALSE, 0, 0);
  gimp_image_flush (image);
}

void
vectors_selection_to_vectors_cmd_callback (GimpAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  GimpImage      *image;
  GtkWidget      *widget;
  GimpProcedure  *procedure;
  GimpValueArray *args;
  GimpDisplay    *display;
  gboolean        advanced;
  GError         *error = NULL;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  advanced = (gboolean) g_variant_get_int32 (value);

  if (advanced)
    procedure = gimp_pdb_lookup_procedure (image->gimp->pdb,
                                           "plug-in-sel2path-advanced");
  else
    procedure = gimp_pdb_lookup_procedure (image->gimp->pdb,
                                           "plug-in-sel2path");

  if (! procedure)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_ERROR,
                            "Selection to path procedure lookup failed.");
      return;
    }

  display = gimp_context_get_display (action_data_get_context (data));

  args = gimp_procedure_get_arguments (procedure);
  gimp_value_array_truncate (args, 2);

  g_value_set_int      (gimp_value_array_index (args, 0),
                        GIMP_RUN_INTERACTIVE);
  gimp_value_set_image (gimp_value_array_index (args, 1),
                        image);

  gimp_procedure_execute_async (procedure, image->gimp,
                                action_data_get_context (data),
                                GIMP_PROGRESS (display), args,
                                GIMP_OBJECT (display), &error);

  gimp_value_array_unref (args);

  if (error)
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_ERROR,
                            error->message);
      g_error_free (error);
    }
}

void
vectors_fill_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_fill_cmd_callback (action,
                           image, GIMP_ITEM (vectors),
                           "gimp-vectors-fill-dialog",
                           _("Fill Path"),
                           GIMP_ICON_TOOL_BUCKET_FILL,
                           GIMP_HELP_PATH_FILL,
                           data);
}

void
vectors_fill_last_vals_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_fill_last_vals_cmd_callback (action,
                                     image, GIMP_ITEM (vectors),
                                     data);
}

void
vectors_stroke_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_stroke_cmd_callback (action,
                             image, GIMP_ITEM (vectors),
                             "gimp-vectors-stroke-dialog",
                             _("Stroke Path"),
                             GIMP_ICON_PATH_STROKE,
                             GIMP_HELP_PATH_STROKE,
                             data);
}

void
vectors_stroke_last_vals_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_stroke_last_vals_cmd_callback (action,
                                       image, GIMP_ITEM (vectors),
                                       data);
}

void
vectors_copy_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  gchar       *svg;
  return_if_no_vectors (image, vectors, data);

  svg = gimp_vectors_export_string (image, vectors);

  if (svg)
    {
      gimp_clipboard_set_svg (image->gimp, svg);
      g_free (svg);
    }
}

void
vectors_paste_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage *image;
  GtkWidget *widget;
  gchar     *svg;
  gsize      svg_size;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  svg = gimp_clipboard_get_svg (image->gimp, &svg_size);

  if (svg)
    {
      GError *error = NULL;

      if (! gimp_vectors_import_buffer (image, svg, svg_size,
                                        TRUE, FALSE,
                                        GIMP_IMAGE_ACTIVE_PARENT, -1,
                                        NULL, &error))
        {
          gimp_message (image->gimp, G_OBJECT (widget), GIMP_MESSAGE_ERROR,
                        "%s", error->message);
          g_clear_error (&error);
        }
      else
        {
          gimp_image_flush (image);
        }

      g_free (svg);
    }
}

void
vectors_export_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_vectors (image, vectors, data);
  return_if_no_widget (widget, data);

#define EXPORT_DIALOG_KEY "gimp-vectors-export-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), EXPORT_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
      GFile            *folder = NULL;

      if (config->vectors_export_path)
        folder = gimp_file_new_for_config_path (config->vectors_export_path,
                                                NULL);

      dialog = vectors_export_dialog_new (image, widget,
                                          folder,
                                          config->vectors_export_active_only,
                                          vectors_export_callback,
                                          NULL);

      if (folder)
        g_object_unref (folder);

      dialogs_attach_dialog (G_OBJECT (image), EXPORT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
vectors_import_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define IMPORT_DIALOG_KEY "gimp-vectors-import-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), IMPORT_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
      GFile            *folder = NULL;

      if (config->vectors_import_path)
        folder = gimp_file_new_for_config_path (config->vectors_import_path,
                                                NULL);

      dialog = vectors_import_dialog_new (image, widget,
                                          folder,
                                          config->vectors_import_merge,
                                          config->vectors_import_scale,
                                          vectors_import_callback,
                                          NULL);

      dialogs_attach_dialog (G_OBJECT (image), IMPORT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
vectors_visible_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_visible_cmd_callback (action, value, image, GIMP_ITEM (vectors));
}

void
vectors_linked_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_linked_cmd_callback (action, value, image, GIMP_ITEM (vectors));
}

void
vectors_lock_content_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_lock_content_cmd_callback (action, value, image, GIMP_ITEM (vectors));
}

void
vectors_lock_position_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_lock_position_cmd_callback (action, value, image, GIMP_ITEM (vectors));
}

void
vectors_color_tag_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpImage    *image;
  GimpVectors  *vectors;
  GimpColorTag  color_tag;
  return_if_no_vectors (image, vectors, data);

  color_tag = (GimpColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, GIMP_ITEM (vectors),
                                color_tag);
}


/*  private functions  */

static void
vectors_new_callback (GtkWidget    *dialog,
                      GimpImage    *image,
                      GimpVectors  *vectors,
                      GimpContext  *context,
                      const gchar  *vectors_name,
                      gboolean      vectors_visible,
                      gboolean      vectors_linked,
                      GimpColorTag  vectors_color_tag,
                      gboolean      vectors_lock_content,
                      gboolean      vectors_lock_position,
                      gpointer      user_data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

  g_object_set (config,
                "path-new-name", vectors_name,
                NULL);

  vectors = gimp_vectors_new (image, config->vectors_new_name);
  gimp_item_set_visible (GIMP_ITEM (vectors), vectors_visible, FALSE);
  gimp_item_set_linked (GIMP_ITEM (vectors), vectors_linked, FALSE);
  gimp_item_set_color_tag (GIMP_ITEM (vectors), vectors_color_tag, FALSE);
  gimp_item_set_lock_content (GIMP_ITEM (vectors), vectors_lock_content, FALSE);
  gimp_item_set_lock_position (GIMP_ITEM (vectors), vectors_lock_position, FALSE);

  gimp_image_add_vectors (image, vectors,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
vectors_edit_attributes_callback (GtkWidget    *dialog,
                                  GimpImage    *image,
                                  GimpVectors  *vectors,
                                  GimpContext  *context,
                                  const gchar  *vectors_name,
                                  gboolean      vectors_visible,
                                  gboolean      vectors_linked,
                                  GimpColorTag  vectors_color_tag,
                                  gboolean      vectors_lock_content,
                                  gboolean      vectors_lock_position,
                                  gpointer      user_data)
{
  GimpItem *item = GIMP_ITEM (vectors);

  if (strcmp (vectors_name, gimp_object_get_name (vectors))      ||
      vectors_visible       != gimp_item_get_visible (item)      ||
      vectors_linked        != gimp_item_get_linked (item)       ||
      vectors_color_tag     != gimp_item_get_color_tag (item)    ||
      vectors_lock_content  != gimp_item_get_lock_content (item) ||
      vectors_lock_position != gimp_item_get_lock_position (item))
    {
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Path Attributes"));

      if (strcmp (vectors_name, gimp_object_get_name (vectors)))
        gimp_item_rename (GIMP_ITEM (vectors), vectors_name, NULL);

      if (vectors_visible != gimp_item_get_visible (item))
        gimp_item_set_visible (item, vectors_visible, TRUE);

      if (vectors_linked != gimp_item_get_linked (item))
        gimp_item_set_linked (item, vectors_linked, TRUE);

      if (vectors_color_tag != gimp_item_get_color_tag (item))
        gimp_item_set_color_tag (item, vectors_color_tag, TRUE);

      if (vectors_lock_content != gimp_item_get_lock_content (item))
        gimp_item_set_lock_content (item, vectors_lock_content, TRUE);

      if (vectors_lock_position != gimp_item_get_lock_position (item))
        gimp_item_set_lock_position (item, vectors_lock_position, TRUE);

      gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}

static void
vectors_import_callback (GtkWidget *dialog,
                         GimpImage *image,
                         GFile     *file,
                         GFile     *import_folder,
                         gboolean   merge_vectors,
                         gboolean   scale_vectors,
                         gpointer   user_data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
  gchar            *path   = NULL;
  GError           *error  = NULL;

  if (import_folder)
    path = gimp_file_get_config_path (import_folder, NULL);

  g_object_set (config,
                "path-import-path",  path,
                "path-import-merge", merge_vectors,
                "path-import-scale", scale_vectors,
                NULL);

  if (path)
    g_free (path);

  if (gimp_vectors_import_file (image, file,
                                config->vectors_import_merge,
                                config->vectors_import_scale,
                                GIMP_IMAGE_ACTIVE_PARENT, -1,
                                NULL, &error))
    {
      gimp_image_flush (image);
    }
  else
    {
      gimp_message (image->gimp, G_OBJECT (dialog),
                    GIMP_MESSAGE_ERROR,
                    "%s", error->message);
      g_error_free (error);
      return;
    }

  gtk_widget_destroy (dialog);
}

static void
vectors_export_callback (GtkWidget *dialog,
                         GimpImage *image,
                         GFile     *file,
                         GFile     *export_folder,
                         gboolean   active_only,
                         gpointer   user_data)
{
  GimpDialogConfig *config  = GIMP_DIALOG_CONFIG (image->gimp->config);
  GimpVectors      *vectors = NULL;
  gchar            *path    = NULL;
  GError           *error   = NULL;

  if (export_folder)
    path = gimp_file_get_config_path (export_folder, NULL);

  g_object_set (config,
                "path-export-path",        path,
                "path-export-active-only", active_only,
                NULL);

  if (path)
    g_free (path);

  if (config->vectors_export_active_only)
    vectors = gimp_image_get_active_vectors (image);

  if (! gimp_vectors_export_file (image, vectors, file, &error))
    {
      gimp_message (image->gimp, G_OBJECT (dialog),
                    GIMP_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);
      return;
    }

  gtk_widget_destroy (dialog);
}

void
vectors_select_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage      *image;
  GimpVectors    *vectors;
  GimpContainer  *container;
  GimpVectors    *new_vectors;
  return_if_no_image (image, data);

  vectors = gimp_image_get_active_vectors (image);

  if (vectors)
    container = gimp_item_get_container (GIMP_ITEM (vectors));
  else
    container = gimp_image_get_vectors (image);

  new_vectors = (GimpVectors *) action_select_object ((GimpActionSelectType) value,
                                                       container,
                                                      (GimpObject *) vectors);

  if (new_vectors && new_vectors != vectors)
    {
      gimp_image_set_active_vectors (image, new_vectors);
      gimp_image_flush (image);
    }
}
