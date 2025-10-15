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

#include "path/gimppath.h"
#include "path/gimppath-export.h"
#include "path/gimppath-import.h"
#include "path/gimpvectorlayer.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpclipboard.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "tools/gimppathtool.h"
#include "tools/tool_manager.h"

#include "dialogs/dialogs.h"
#include "dialogs/path-export-dialog.h"
#include "dialogs/path-import-dialog.h"
#include "dialogs/path-options-dialog.h"

#include "actions.h"
#include "items-commands.h"
#include "paths-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   paths_new_callback             (GtkWidget    *dialog,
                                              GimpImage    *image,
                                              GimpPath     *path,
                                              GimpContext  *context,
                                              const gchar  *path_name,
                                              gboolean      path_visible,
                                              GimpColorTag  path_color_tag,
                                              gboolean      path_lock_content,
                                              gboolean      path_lock_position,
                                              gboolean      path_lock_visibility,
                                              gpointer      user_data);
static void   paths_edit_attributes_callback (GtkWidget    *dialog,
                                              GimpImage    *image,
                                              GimpPath     *path,
                                              GimpContext  *context,
                                              const gchar  *path_name,
                                              gboolean      path_visible,
                                              GimpColorTag  path_color_tag,
                                              gboolean      path_lock_content,
                                              gboolean      path_lock_position,
                                              gboolean      path_lock_visibility,
                                              gpointer      user_data);
static void   paths_import_callback          (GtkWidget    *dialog,
                                              GimpImage    *image,
                                              GFile        *file,
                                              GFile        *import_folder,
                                              gboolean      merge_paths,
                                              gboolean      scale_paths,
                                              gpointer      user_data);
static void   paths_export_callback          (GtkWidget    *dialog,
                                              GimpImage    *image,
                                              GFile        *file,
                                              GFile        *export_folder,
                                              gboolean      active_only,
                                              gpointer      user_data);


/*  public functions  */

void
paths_edit_cmd_callback (GimpAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  GimpTool  *active_tool;
  return_if_no_paths (image, paths, data);

  if (g_list_length (paths) != 1)
    return;

  active_tool = tool_manager_get_active (image->gimp);

  if (! GIMP_IS_PATH_TOOL (active_tool))
    {
      GimpToolInfo *tool_info = gimp_get_tool_info (image->gimp,
                                                    "gimp-path-tool");

      if (GIMP_IS_TOOL_INFO (tool_info))
        {
          gimp_context_set_tool (action_data_get_context (data), tool_info);
          active_tool = tool_manager_get_active (image->gimp);
        }
    }

  if (GIMP_IS_PATH_TOOL (active_tool))
    gimp_path_tool_set_path (GIMP_PATH_TOOL (active_tool), NULL, paths->data);
}

void
paths_edit_attributes_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage   *image;
  GList       *paths;
  GimpPath    *path;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_paths (image, paths, data);
  return_if_no_widget (widget, data);

  if (g_list_length (paths) != 1)
    return;

  path = paths->data;

#define EDIT_DIALOG_KEY "gimp-path-edit-attributes-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (path), EDIT_DIALOG_KEY);

  if (! dialog)
    {
      GimpItem *item = GIMP_ITEM (path);

      dialog = path_options_dialog_new (image, path,
                                        action_data_get_context (data),
                                        widget,
                                        _("Path Attributes"),
                                        "gimp-path-edit",
                                        GIMP_ICON_EDIT,
                                        _("Edit Path Attributes"),
                                        GIMP_HELP_PATH_EDIT,
                                        gimp_object_get_name (path),
                                        gimp_item_get_visible (item),
                                        gimp_item_get_color_tag (item),
                                        gimp_item_get_lock_content (item),
                                        gimp_item_get_lock_position (item),
                                        gimp_item_get_lock_visibility (item),
                                        paths_edit_attributes_callback,
                                        NULL);

      dialogs_attach_dialog (G_OBJECT (path), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
paths_new_cmd_callback (GimpAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  GimpImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define NEW_DIALOG_KEY "gimp-path-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), NEW_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

      dialog = path_options_dialog_new (image, NULL,
                                        action_data_get_context (data),
                                        widget,
                                        _("New Path"),
                                        "gimp-path-new",
                                        GIMP_ICON_PATH,
                                        _("Create a New Path"),
                                        GIMP_HELP_PATH_NEW,
                                        config->path_new_name,
                                        FALSE,
                                        GIMP_COLOR_TAG_NONE,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        paths_new_callback,
                                        NULL);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
paths_new_last_vals_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage        *image;
  GimpPath         *path;
  GimpDialogConfig *config;
  return_if_no_image (image, data);

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  path = gimp_path_new (image, config->path_new_name);
  gimp_image_add_path (image, path,
                       GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_image_flush (image);
}

void
paths_raise_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_paths (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      gint index;

      index = gimp_item_get_index (iter->data);
      if (index > 0)
        {
          moved_list = g_list_prepend (moved_list, iter->data);
        }
      else
        {
          gimp_image_flush (image);
          g_list_free (moved_list);
          return;
        }
    }

  if (moved_list)
    {
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Raise Path",
                                             "Raise Paths",
                                             g_list_length (moved_list)));

      moved_list = g_list_reverse (moved_list);
      for (iter = moved_list; iter; iter = iter->next)
        gimp_image_raise_item (image, GIMP_ITEM (iter->data), NULL);

      gimp_image_flush (image);
      gimp_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
paths_raise_to_top_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_paths (image, list, data);

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
paths_lower_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_paths (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      GList *paths_list;
      gint   index;

      paths_list = gimp_item_get_container_iter (GIMP_ITEM (iter->data));
      index = gimp_item_get_index (iter->data);
      if (index < g_list_length (paths_list) - 1)
        {
          moved_list = g_list_prepend (moved_list, iter->data);
        }
      else
        {
          gimp_image_flush (image);
          g_list_free (moved_list);
          return;
        }
    }

  if (moved_list)
    {
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
paths_lower_to_bottom_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_paths (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      GList *paths_list;
      gint   index;

      paths_list = gimp_item_get_container_iter (GIMP_ITEM (iter->data));
      index = gimp_item_get_index (iter->data);
      if (index < g_list_length (paths_list) - 1)
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
paths_duplicate_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  GList     *new_paths = NULL;
  GList     *iter;
  return_if_no_paths (image, paths, data);

  paths = g_list_copy (paths);
  paths = g_list_reverse (paths);

  /* TODO: proper undo group. */
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_PATHS_IMPORT,
                               _("Duplicate Paths"));
  for (iter = paths; iter; iter = iter->next)
    {
      GimpPath *new_path;

      new_path = GIMP_PATH (gimp_item_duplicate (iter->data,
                                                 G_TYPE_FROM_INSTANCE (iter->data)));
      /*  use the actual parent here, not GIMP_IMAGE_ACTIVE_PARENT because
       *  the latter would add a duplicated group inside itself instead of
       *  above it
       */
      gimp_image_add_path (image, new_path,
                           gimp_path_get_parent (iter->data), -1,
                           TRUE);
      new_paths = g_list_prepend (new_paths, new_path);
    }
  if (new_paths)
    {
      gimp_image_set_selected_paths (image, new_paths);
      gimp_image_flush (image);
    }
  gimp_image_undo_group_end (image);
  g_list_free (paths);
}

void
paths_delete_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  gboolean   attached_to_vector_layer = FALSE;
  return_if_no_paths (image, paths, data);

  paths = g_list_copy (paths);
  /* TODO: proper undo group. */
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_PATHS_IMPORT,
                               _("Remove Paths"));

  for (GList *iter = paths; iter; iter = iter->next)
    {
      /* Verify path is not attached to vector layer */
      if (! gimp_path_attached_to_vector_layer (GIMP_PATH (iter->data), image))
        gimp_image_remove_path (image, iter->data, TRUE, NULL);
      else
        attached_to_vector_layer = TRUE;
    }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
  g_list_free (paths);

  if (attached_to_vector_layer)
    gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_WARNING,
                          _("Paths attached to vector layers weren't deleted"));
}

void
paths_merge_visible_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage   *image;
  GList       *paths;
  GtkWidget   *widget;
  GError      *error = NULL;
  return_if_no_paths (image, paths, data);
  return_if_no_widget (widget, data);

  if (! gimp_image_merge_visible_paths (image, &error))
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
path_to_vector_layer_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage       *image;
  GList           *paths;
  GimpVectorLayer *layer;
  return_if_no_paths (image, paths, data);

  layer = gimp_vector_layer_new (image, paths->data,
                                 gimp_get_user_context (image->gimp));
  gimp_image_add_layer (image,
                        GIMP_LAYER (layer),
                        GIMP_IMAGE_ACTIVE_PARENT,
                        -1,
                        TRUE);
  gimp_vector_layer_refresh (layer);

  gimp_image_flush (image);
}

void
paths_to_selection_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage      *image;
  GList          *paths;
  GList          *iter;
  GimpChannelOps  operation;
  return_if_no_paths (image, paths, data);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_DRAWABLE_MOD,
                               _("Paths to selection"));

  operation = (GimpChannelOps) g_variant_get_int32 (value);

  for (iter = paths; iter; iter = iter->next)
    {
      gimp_item_to_selection (iter->data, operation, TRUE, FALSE, 0, 0);

      if (operation == GIMP_CHANNEL_OP_REPLACE && iter == paths)
        operation = GIMP_CHANNEL_OP_ADD;
    }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

void
paths_selection_to_paths_cmd_callback (GimpAction *action,
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
paths_fill_cmd_callback (GimpAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_fill_cmd_callback (action, image, paths,
                           _("Fill Path"),
                           GIMP_ICON_TOOL_BUCKET_FILL,
                           GIMP_HELP_PATH_FILL,
                           data);
}

void
paths_fill_last_vals_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_fill_last_vals_cmd_callback (action, image, paths, data);
}

void
paths_stroke_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_stroke_cmd_callback (action, image, paths,
                             _("Stroke Path"),
                             GIMP_ICON_PATH_STROKE,
                             GIMP_HELP_PATH_STROKE,
                             data);
}

void
paths_stroke_last_vals_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_stroke_last_vals_cmd_callback (action, image, paths, data);
}

void
paths_copy_cmd_callback (GimpAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  GimpImage   *image;
  GList       *paths;
  gchar       *svg;
  return_if_no_paths (image, paths, data);

  svg = gimp_path_export_string (image, paths);

  if (svg)
    {
      gimp_clipboard_set_svg (image->gimp, svg);
      g_free (svg);
    }
}

void
paths_paste_cmd_callback (GimpAction *action,
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

      if (! gimp_path_import_buffer (image, svg, svg_size,
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
paths_export_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage   *image;
  GList       *paths;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_paths (image, paths, data);
  return_if_no_widget (widget, data);

#define EXPORT_DIALOG_KEY "gimp-paths-export-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), EXPORT_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
      GFile            *folder = NULL;

      if (config->path_export_path)
        folder = gimp_file_new_for_config_path (config->path_export_path,
                                                NULL);

      dialog = path_export_dialog_new (image, widget,
                                       folder,
                                       config->path_export_active_only,
                                       paths_export_callback,
                                       NULL);

      if (folder)
        g_object_unref (folder);

      dialogs_attach_dialog (G_OBJECT (image), EXPORT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
paths_import_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define IMPORT_DIALOG_KEY "gimp-paths-import-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), IMPORT_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
      GFile            *folder = NULL;

      if (config->path_import_path)
        folder = gimp_file_new_for_config_path (config->path_import_path,
                                                NULL);

      dialog = path_import_dialog_new (image, widget,
                                       folder,
                                       config->path_import_merge,
                                       config->path_import_scale,
                                       paths_import_callback,
                                       NULL);

      dialogs_attach_dialog (G_OBJECT (image), IMPORT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
paths_visible_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_visible_cmd_callback (action, value, image, paths);
}

void
paths_lock_content_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_lock_content_cmd_callback (action, value, image, paths);
}

void
paths_lock_position_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_lock_position_cmd_callback (action, value, image, paths);
}

void
paths_color_tag_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpImage    *image;
  GList        *paths;
  GimpColorTag  color_tag;
  return_if_no_paths (image, paths, data);

  color_tag = (GimpColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, paths, color_tag);
}


/*  private functions  */

static void
paths_new_callback (GtkWidget    *dialog,
                    GimpImage    *image,
                    GimpPath     *path,
                    GimpContext  *context,
                    const gchar  *paths_name,
                    gboolean      paths_visible,
                    GimpColorTag  paths_color_tag,
                    gboolean      paths_lock_content,
                    gboolean      paths_lock_position,
                    gboolean      paths_lock_visibility,
                    gpointer      user_data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

  g_object_set (config,
                "path-new-name", paths_name,
                NULL);

  path = gimp_path_new (image, config->path_new_name);
  gimp_item_set_visible (GIMP_ITEM (path), paths_visible, FALSE);
  gimp_item_set_color_tag (GIMP_ITEM (path), paths_color_tag, FALSE);
  gimp_item_set_lock_content (GIMP_ITEM (path), paths_lock_content, FALSE);
  gimp_item_set_lock_position (GIMP_ITEM (path), paths_lock_position, FALSE);
  gimp_item_set_lock_visibility (GIMP_ITEM (path), paths_lock_visibility, FALSE);

  gimp_image_add_path (image, path,
                       GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
paths_edit_attributes_callback (GtkWidget    *dialog,
                                GimpImage    *image,
                                GimpPath     *path,
                                GimpContext  *context,
                                const gchar  *paths_name,
                                gboolean      paths_visible,
                                GimpColorTag  paths_color_tag,
                                gboolean      paths_lock_content,
                                gboolean      paths_lock_position,
                                gboolean      paths_lock_visibility,
                                gpointer      user_data)
{
  GimpItem *item = GIMP_ITEM (path);

  if (strcmp (paths_name, gimp_object_get_name (path))            ||
      paths_visible         != gimp_item_get_visible (item)       ||
      paths_color_tag       != gimp_item_get_color_tag (item)     ||
      paths_lock_content    != gimp_item_get_lock_content (item)  ||
      paths_lock_position   != gimp_item_get_lock_position (item) ||
      paths_lock_visibility != gimp_item_get_lock_visibility (item))
    {
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Path Attributes"));

      if (strcmp (paths_name, gimp_object_get_name (path)))
        gimp_item_rename (GIMP_ITEM (path), paths_name, NULL);

      if (paths_visible != gimp_item_get_visible (item))
        gimp_item_set_visible (item, paths_visible, TRUE);

      if (paths_color_tag != gimp_item_get_color_tag (item))
        gimp_item_set_color_tag (item, paths_color_tag, TRUE);

      if (paths_lock_content != gimp_item_get_lock_content (item))
        gimp_item_set_lock_content (item, paths_lock_content, TRUE);

      if (paths_lock_position != gimp_item_get_lock_position (item))
        gimp_item_set_lock_position (item, paths_lock_position, TRUE);

      if (paths_lock_visibility != gimp_item_get_lock_visibility (item))
        gimp_item_set_lock_visibility (item, paths_lock_visibility, TRUE);

      gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}

static void
paths_import_callback (GtkWidget *dialog,
                       GimpImage *image,
                       GFile     *file,
                       GFile     *import_folder,
                       gboolean   merge_paths,
                       gboolean   scale_paths,
                       gpointer   user_data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
  gchar            *path   = NULL;
  GError           *error  = NULL;

  if (import_folder)
    path = gimp_file_get_config_path (import_folder, NULL);

  g_object_set (config,
                "path-import-path",  path,
                "path-import-merge", merge_paths,
                "path-import-scale", scale_paths,
                NULL);

  if (path)
    g_free (path);

  if (gimp_path_import_file (image, file,
                             config->path_import_merge,
                             config->path_import_scale,
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
paths_export_callback (GtkWidget *dialog,
                       GimpImage *image,
                       GFile     *file,
                       GFile     *export_folder,
                       gboolean   active_only,
                       gpointer   user_data)
{
  GimpDialogConfig *config  = GIMP_DIALOG_CONFIG (image->gimp->config);
  GList            *paths   = NULL;
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

  if (config->path_export_active_only)
    paths = gimp_image_get_selected_paths (image);

  if (! gimp_path_export_file (image, paths, file, &error))
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
paths_select_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage            *image;
  GList                *new_paths = NULL;
  GList                *paths;
  GList                *iter;
  GimpActionSelectType  select_type;
  gboolean              run_once;
  return_if_no_image (image, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  paths = gimp_image_get_selected_paths (image);
  run_once = (g_list_length (paths) == 0);

  for (iter = paths; iter || run_once; iter = iter ? iter->next : NULL)
    {
      GimpPath      *new_vec;
      GimpContainer *container;

      if (iter)
        {
          container = gimp_item_get_container (GIMP_ITEM (iter->data));
        }
      else /* run_once */
        {
          container = gimp_image_get_paths (image);
          run_once  = FALSE;
        }
      new_vec = (GimpPath *) action_select_object (select_type,
                                                   container,
                                                   iter ? iter->data : NULL);
      if (new_vec)
        new_paths = g_list_prepend (new_paths, new_vec);
    }

  if (new_paths)
    {
      gimp_image_set_selected_paths (image, new_paths);
      gimp_image_flush (image);
    }

  g_list_free (new_paths);
}
