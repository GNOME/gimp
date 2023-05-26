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
                                                GimpColorTag  vectors_color_tag,
                                                gboolean      vectors_lock_content,
                                                gboolean      vectors_lock_position,
                                                gboolean      vectors_lock_visibility,
                                                gpointer      user_data);
static void   vectors_edit_attributes_callback (GtkWidget    *dialog,
                                                GimpImage    *image,
                                                GimpVectors  *vectors,
                                                GimpContext  *context,
                                                const gchar  *vectors_name,
                                                gboolean      vectors_visible,
                                                GimpColorTag  vectors_color_tag,
                                                gboolean      vectors_lock_content,
                                                gboolean      vectors_lock_position,
                                                gboolean      vectors_lock_visibility,
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
  GimpImage *image;
  GList     *vectors;
  GimpTool  *active_tool;
  return_if_no_vectors_list (image, vectors, data);

  if (g_list_length (vectors) != 1)
    return;

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
    gimp_vector_tool_set_vectors (GIMP_VECTOR_TOOL (active_tool), vectors->data);
}

void
vectors_edit_attributes_cmd_callback (GimpAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GimpImage   *image;
  GList       *paths;
  GimpVectors *vectors;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_vectors_list (image, paths, data);
  return_if_no_widget (widget, data);

  if (g_list_length (paths) != 1)
    return;

  vectors = paths->data;

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
                                           gimp_item_get_color_tag (item),
                                           gimp_item_get_lock_content (item),
                                           gimp_item_get_lock_position (item),
                                           gimp_item_get_lock_visibility (item),
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
                                           GIMP_COLOR_TAG_NONE,
                                           FALSE,
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
  GimpImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_vectors_list (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      gint index;

      index = gimp_item_get_index (iter->data);
      if (index > 0)
        moved_list = g_list_prepend (moved_list, iter->data);
    }

  if (moved_list)
    {
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Raise Path",
                                             "Raise Paths",
                                             g_list_length (moved_list)));
      for (iter = moved_list; iter; iter = iter->next)
        gimp_image_raise_item (image, GIMP_ITEM (iter->data), NULL);

      gimp_image_flush (image);
      gimp_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
vectors_raise_to_top_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_vectors_list (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      gint index;

      index = gimp_item_get_index (iter->data);
      if (index > 0)
        moved_list = g_list_prepend (moved_list, iter->data);
    }

  if (moved_list)
    {
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Raise Path to Top",
                                             "Raise Paths to Top",
                                             g_list_length (moved_list)));

      for (iter = moved_list; iter; iter = iter->next)
        gimp_image_raise_item_to_top (image, GIMP_ITEM (iter->data));

      gimp_image_flush (image);
      gimp_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
vectors_lower_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_vectors_list (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      GList *vectors_list;
      gint   index;

      vectors_list = gimp_item_get_container_iter (GIMP_ITEM (iter->data));
      index = gimp_item_get_index (iter->data);
      if (index < g_list_length (vectors_list) - 1)
        moved_list = g_list_prepend (moved_list, iter->data);
    }

  if (moved_list)
    {
      moved_list = g_list_reverse (moved_list);
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Lower Path",
                                             "Lower Paths",
                                             g_list_length (moved_list)));

      for (iter = moved_list; iter; iter = iter->next)
        gimp_image_lower_item (image, GIMP_ITEM (iter->data), NULL);

      gimp_image_flush (image);
      gimp_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
vectors_lower_to_bottom_cmd_callback (GimpAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GimpImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_vectors_list (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      GList *vectors_list;
      gint   index;

      vectors_list = gimp_item_get_container_iter (GIMP_ITEM (iter->data));
      index = gimp_item_get_index (iter->data);
      if (index < g_list_length (vectors_list) - 1)
        moved_list = g_list_prepend (moved_list, iter->data);
    }

  if (moved_list)
    {
      moved_list = g_list_reverse (moved_list);
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Lower Path to Bottom",
                                             "Lower Paths to Bottom",
                                             g_list_length (moved_list)));

      for (iter = moved_list; iter; iter = iter->next)
        gimp_image_lower_item_to_bottom (image, GIMP_ITEM (iter->data));

      gimp_image_flush (image);
      gimp_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
vectors_duplicate_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  GList     *new_paths = NULL;
  GList     *iter;
  return_if_no_vectors_list (image, paths, data);

  paths = g_list_copy (paths);
  paths = g_list_reverse (paths);

  /* TODO: proper undo group. */
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_VECTORS_IMPORT,
                               _("Duplicate Paths"));
  for (iter = paths; iter; iter = iter->next)
    {
      GimpVectors *new_vectors;

      new_vectors = GIMP_VECTORS (gimp_item_duplicate (iter->data,
                                                       G_TYPE_FROM_INSTANCE (iter->data)));
      /*  use the actual parent here, not GIMP_IMAGE_ACTIVE_PARENT because
       *  the latter would add a duplicated group inside itself instead of
       *  above it
       */
      gimp_image_add_vectors (image, new_vectors,
                              gimp_vectors_get_parent (iter->data), -1,
                              TRUE);
      new_paths = g_list_prepend (new_paths, new_vectors);
    }
  if (new_paths)
    {
      gimp_image_set_selected_vectors (image, new_paths);
      gimp_image_flush (image);
    }
  gimp_image_undo_group_end (image);
  g_list_free (paths);
}

void
vectors_delete_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  return_if_no_vectors_list (image, paths, data);

  paths = g_list_copy (paths);
  /* TODO: proper undo group. */
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_VECTORS_IMPORT,
                               _("Remove Paths"));

  for (GList *iter = paths; iter; iter = iter->next)
    gimp_image_remove_vectors (image, iter->data, TRUE, NULL);

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
  g_list_free (paths);
}

void
vectors_merge_visible_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage   *image;
  GList       *vectors;
  GtkWidget   *widget;
  GError      *error = NULL;
  return_if_no_vectors_list (image, vectors, data);
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
  GList          *vectors;
  GList          *iter;
  GimpChannelOps  operation;
  return_if_no_vectors_list (image, vectors, data);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_DRAWABLE_MOD,
                               _("Paths to selection"));

  operation = (GimpChannelOps) g_variant_get_int32 (value);

  for (iter = vectors; iter; iter = iter->next)
    {
      gimp_item_to_selection (iter->data, operation, TRUE, FALSE, 0, 0);

      if (operation == GIMP_CHANNEL_OP_REPLACE && iter == vectors)
        operation = GIMP_CHANNEL_OP_ADD;
    }

  gimp_image_undo_group_end (image);
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

  g_value_set_enum   (gimp_value_array_index (args, 0),
                      advanced ?
                      GIMP_RUN_INTERACTIVE : GIMP_RUN_NONINTERACTIVE);
  g_value_set_object (gimp_value_array_index (args, 1),
                      image);

  gimp_procedure_execute_async (procedure, image->gimp,
                                action_data_get_context (data),
                                GIMP_PROGRESS (display), args,
                                display, &error);

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
  GimpImage *image;
  GList     *vectors;
  return_if_no_vectors_list (image, vectors, data);

  items_fill_cmd_callback (action, image, vectors,
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
  GimpImage *image;
  GList     *vectors;
  return_if_no_vectors_list (image, vectors, data);

  items_fill_last_vals_cmd_callback (action, image, vectors, data);
}

void
vectors_stroke_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage *image;
  GList     *vectors;
  return_if_no_vectors_list (image, vectors, data);

  items_stroke_cmd_callback (action, image, vectors,
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
  GimpImage *image;
  GList     *vectors;
  return_if_no_vectors_list (image, vectors, data);

  items_stroke_last_vals_cmd_callback (action, image, vectors, data);
}

void
vectors_copy_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage   *image;
  GList       *vectors;
  gchar       *svg;
  return_if_no_vectors_list (image, vectors, data);

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
  GList       *vectors;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_vectors_list (image, vectors, data);
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
  GimpImage *image;
  GList     *vectors;
  return_if_no_vectors_list (image, vectors, data);

  items_visible_cmd_callback (action, value, image, vectors);
}

void
vectors_lock_content_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage *image;
  GList     *vectors;
  return_if_no_vectors_list (image, vectors, data);

  items_lock_content_cmd_callback (action, value, image, vectors);
}

void
vectors_lock_position_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage *image;
  GList     *vectors;
  return_if_no_vectors_list (image, vectors, data);

  items_lock_position_cmd_callback (action, value, image, vectors);
}

void
vectors_color_tag_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpImage    *image;
  GList        *vectors;
  GimpColorTag  color_tag;
  return_if_no_vectors_list (image, vectors, data);

  color_tag = (GimpColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, vectors, color_tag);
}


/*  private functions  */

static void
vectors_new_callback (GtkWidget    *dialog,
                      GimpImage    *image,
                      GimpVectors  *vectors,
                      GimpContext  *context,
                      const gchar  *vectors_name,
                      gboolean      vectors_visible,
                      GimpColorTag  vectors_color_tag,
                      gboolean      vectors_lock_content,
                      gboolean      vectors_lock_position,
                      gboolean      vectors_lock_visibility,
                      gpointer      user_data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

  g_object_set (config,
                "path-new-name", vectors_name,
                NULL);

  vectors = gimp_vectors_new (image, config->vectors_new_name);
  gimp_item_set_visible (GIMP_ITEM (vectors), vectors_visible, FALSE);
  gimp_item_set_color_tag (GIMP_ITEM (vectors), vectors_color_tag, FALSE);
  gimp_item_set_lock_content (GIMP_ITEM (vectors), vectors_lock_content, FALSE);
  gimp_item_set_lock_position (GIMP_ITEM (vectors), vectors_lock_position, FALSE);
  gimp_item_set_lock_visibility (GIMP_ITEM (vectors), vectors_lock_visibility, FALSE);

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
                                  GimpColorTag  vectors_color_tag,
                                  gboolean      vectors_lock_content,
                                  gboolean      vectors_lock_position,
                                  gboolean      vectors_lock_visibility,
                                  gpointer      user_data)
{
  GimpItem *item = GIMP_ITEM (vectors);

  if (strcmp (vectors_name, gimp_object_get_name (vectors))         ||
      vectors_visible         != gimp_item_get_visible (item)       ||
      vectors_color_tag       != gimp_item_get_color_tag (item)     ||
      vectors_lock_content    != gimp_item_get_lock_content (item)  ||
      vectors_lock_position   != gimp_item_get_lock_position (item) ||
      vectors_lock_visibility != gimp_item_get_lock_visibility (item))
    {
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Path Attributes"));

      if (strcmp (vectors_name, gimp_object_get_name (vectors)))
        gimp_item_rename (GIMP_ITEM (vectors), vectors_name, NULL);

      if (vectors_visible != gimp_item_get_visible (item))
        gimp_item_set_visible (item, vectors_visible, TRUE);

      if (vectors_color_tag != gimp_item_get_color_tag (item))
        gimp_item_set_color_tag (item, vectors_color_tag, TRUE);

      if (vectors_lock_content != gimp_item_get_lock_content (item))
        gimp_item_set_lock_content (item, vectors_lock_content, TRUE);

      if (vectors_lock_position != gimp_item_get_lock_position (item))
        gimp_item_set_lock_position (item, vectors_lock_position, TRUE);

      if (vectors_lock_visibility != gimp_item_get_lock_visibility (item))
        gimp_item_set_lock_visibility (item, vectors_lock_visibility, TRUE);

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
  GList            *vectors = NULL;
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
    vectors = gimp_image_get_selected_vectors (image);

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
  GimpImage            *image;
  GList                *new_vectors = NULL;
  GList                *vectors;
  GList                *iter;
  GimpActionSelectType  select_type;
  gboolean              run_once;
  return_if_no_image (image, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  vectors = gimp_image_get_selected_vectors (image);
  run_once = (g_list_length (vectors) == 0);

  for (iter = vectors; iter || run_once; iter = iter ? iter->next : NULL)
    {
      GimpVectors   *new_vec;
      GimpContainer *container;

      if (iter)
        {
          container = gimp_item_get_container (GIMP_ITEM (iter->data));
        }
      else /* run_once */
        {
          container = gimp_image_get_vectors (image);
          run_once  = FALSE;
        }
      new_vec = (GimpVectors *) action_select_object (select_type,
                                                      container,
                                                      iter ? iter->data : NULL);
      if (new_vec)
        new_vectors = g_list_prepend (new_vectors, new_vec);
    }

  if (new_vectors)
    {
      gimp_image_set_selected_vectors (image, new_vectors);
      gimp_image_flush (image);
    }

  g_list_free (new_vectors);
}
