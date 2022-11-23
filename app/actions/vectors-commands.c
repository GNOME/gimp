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
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmadialogconfig.h"

#include "core/ligma.h"
#include "core/ligma-utils.h"
#include "core/ligmachannel.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-merge.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmaprogress.h"
#include "core/ligmatoolinfo.h"

#include "pdb/ligmapdb.h"
#include "pdb/ligmaprocedure.h"

#include "vectors/ligmavectors.h"
#include "vectors/ligmavectors-export.h"
#include "vectors/ligmavectors-import.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmaclipboard.h"
#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"

#include "tools/ligmavectortool.h"
#include "tools/tool_manager.h"

#include "dialogs/dialogs.h"
#include "dialogs/vectors-export-dialog.h"
#include "dialogs/vectors-import-dialog.h"
#include "dialogs/vectors-options-dialog.h"

#include "actions.h"
#include "items-commands.h"
#include "vectors-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   vectors_new_callback             (GtkWidget    *dialog,
                                                LigmaImage    *image,
                                                LigmaVectors  *vectors,
                                                LigmaContext  *context,
                                                const gchar  *vectors_name,
                                                gboolean      vectors_visible,
                                                LigmaColorTag  vectors_color_tag,
                                                gboolean      vectors_lock_content,
                                                gboolean      vectors_lock_position,
                                                gpointer      user_data);
static void   vectors_edit_attributes_callback (GtkWidget    *dialog,
                                                LigmaImage    *image,
                                                LigmaVectors  *vectors,
                                                LigmaContext  *context,
                                                const gchar  *vectors_name,
                                                gboolean      vectors_visible,
                                                LigmaColorTag  vectors_color_tag,
                                                gboolean      vectors_lock_content,
                                                gboolean      vectors_lock_position,
                                                gpointer      user_data);
static void   vectors_import_callback          (GtkWidget    *dialog,
                                                LigmaImage    *image,
                                                GFile        *file,
                                                GFile        *import_folder,
                                                gboolean      merge_vectors,
                                                gboolean      scale_vectors,
                                                gpointer      user_data);
static void   vectors_export_callback          (GtkWidget    *dialog,
                                                LigmaImage    *image,
                                                GFile        *file,
                                                GFile        *export_folder,
                                                gboolean      active_only,
                                                gpointer      user_data);


/*  public functions  */

void
vectors_edit_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  LigmaTool    *active_tool;
  return_if_no_vectors (image, vectors, data);

  active_tool = tool_manager_get_active (image->ligma);

  if (! LIGMA_IS_VECTOR_TOOL (active_tool))
    {
      LigmaToolInfo  *tool_info = ligma_get_tool_info (image->ligma,
                                                     "ligma-vector-tool");

      if (LIGMA_IS_TOOL_INFO (tool_info))
        {
          ligma_context_set_tool (action_data_get_context (data), tool_info);
          active_tool = tool_manager_get_active (image->ligma);
        }
    }

  if (LIGMA_IS_VECTOR_TOOL (active_tool))
    ligma_vector_tool_set_vectors (LIGMA_VECTOR_TOOL (active_tool), vectors);
}

void
vectors_edit_attributes_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_vectors (image, vectors, data);
  return_if_no_widget (widget, data);

#define EDIT_DIALOG_KEY "ligma-vectors-edit-attributes-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (vectors), EDIT_DIALOG_KEY);

  if (! dialog)
    {
      LigmaItem *item = LIGMA_ITEM (vectors);

      dialog = vectors_options_dialog_new (image, vectors,
                                           action_data_get_context (data),
                                           widget,
                                           _("Path Attributes"),
                                           "ligma-vectors-edit",
                                           LIGMA_ICON_EDIT,
                                           _("Edit Path Attributes"),
                                           LIGMA_HELP_PATH_EDIT,
                                           ligma_object_get_name (vectors),
                                           ligma_item_get_visible (item),
                                           ligma_item_get_color_tag (item),
                                           ligma_item_get_lock_content (item),
                                           ligma_item_get_lock_position (item),
                                           vectors_edit_attributes_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (vectors), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
vectors_new_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define NEW_DIALOG_KEY "ligma-vectors-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), NEW_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);

      dialog = vectors_options_dialog_new (image, NULL,
                                           action_data_get_context (data),
                                           widget,
                                           _("New Path"),
                                           "ligma-vectors-new",
                                           LIGMA_ICON_PATH,
                                           _("Create a New Path"),
                                           LIGMA_HELP_PATH_NEW,
                                           config->vectors_new_name,
                                           FALSE,
                                           LIGMA_COLOR_TAG_NONE,
                                           FALSE,
                                           FALSE,
                                           vectors_new_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
vectors_new_last_vals_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaImage        *image;
  LigmaVectors      *vectors;
  LigmaDialogConfig *config;
  return_if_no_image (image, data);

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  vectors = ligma_vectors_new (image, config->vectors_new_name);
  ligma_image_add_vectors (image, vectors,
                          LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);
  ligma_image_flush (image);
}

void
vectors_raise_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_vectors_list (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      gint index;

      index = ligma_item_get_index (iter->data);
      if (index > 0)
        moved_list = g_list_prepend (moved_list, iter->data);
    }

  if (moved_list)
    {
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Raise Path",
                                             "Raise Paths",
                                             g_list_length (moved_list)));
      for (iter = moved_list; iter; iter = iter->next)
        ligma_image_raise_item (image, LIGMA_ITEM (iter->data), NULL);

      ligma_image_flush (image);
      ligma_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
vectors_raise_to_top_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_vectors_list (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      gint index;

      index = ligma_item_get_index (iter->data);
      if (index > 0)
        moved_list = g_list_prepend (moved_list, iter->data);
    }

  if (moved_list)
    {
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Raise Path to Top",
                                             "Raise Paths to Top",
                                             g_list_length (moved_list)));

      for (iter = moved_list; iter; iter = iter->next)
        ligma_image_raise_item_to_top (image, LIGMA_ITEM (iter->data));

      ligma_image_flush (image);
      ligma_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
vectors_lower_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_vectors_list (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      GList *vectors_list;
      gint   index;

      vectors_list = ligma_item_get_container_iter (LIGMA_ITEM (iter->data));
      index = ligma_item_get_index (iter->data);
      if (index < g_list_length (vectors_list) - 1)
        moved_list = g_list_prepend (moved_list, iter->data);
    }

  if (moved_list)
    {
      moved_list = g_list_reverse (moved_list);
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Lower Path",
                                             "Lower Paths",
                                             g_list_length (moved_list)));

      for (iter = moved_list; iter; iter = iter->next)
        ligma_image_lower_item (image, LIGMA_ITEM (iter->data), NULL);

      ligma_image_flush (image);
      ligma_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
vectors_lower_to_bottom_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_vectors_list (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      GList *vectors_list;
      gint   index;

      vectors_list = ligma_item_get_container_iter (LIGMA_ITEM (iter->data));
      index = ligma_item_get_index (iter->data);
      if (index < g_list_length (vectors_list) - 1)
        moved_list = g_list_prepend (moved_list, iter->data);
    }

  if (moved_list)
    {
      moved_list = g_list_reverse (moved_list);
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Lower Path to Bottom",
                                             "Lower Paths to Bottom",
                                             g_list_length (moved_list)));

      for (iter = moved_list; iter; iter = iter->next)
        ligma_image_lower_item_to_bottom (image, LIGMA_ITEM (iter->data));

      ligma_image_flush (image);
      ligma_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
vectors_duplicate_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  LigmaVectors *new_vectors;
  return_if_no_vectors (image, vectors, data);

  new_vectors = LIGMA_VECTORS (ligma_item_duplicate (LIGMA_ITEM (vectors),
                                                   G_TYPE_FROM_INSTANCE (vectors)));
  /*  use the actual parent here, not LIGMA_IMAGE_ACTIVE_PARENT because
   *  the latter would add a duplicated group inside itself instead of
   *  above it
   */
  ligma_image_add_vectors (image, new_vectors,
                          ligma_vectors_get_parent (vectors), -1,
                          TRUE);
  ligma_image_flush (image);
}

void
vectors_delete_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  ligma_image_remove_vectors (image, vectors, TRUE, NULL);
  ligma_image_flush (image);
}

void
vectors_merge_visible_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  GtkWidget   *widget;
  GError      *error = NULL;
  return_if_no_vectors (image, vectors, data);
  return_if_no_widget (widget, data);

  if (! ligma_image_merge_visible_vectors (image, &error))
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
      return;
    }

  ligma_image_flush (image);
}

void
vectors_to_selection_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaImage      *image;
  LigmaVectors    *vectors;
  LigmaChannelOps  operation;
  return_if_no_vectors (image, vectors, data);

  operation = (LigmaChannelOps) g_variant_get_int32 (value);

  ligma_item_to_selection (LIGMA_ITEM (vectors), operation,
                          TRUE, FALSE, 0, 0);
  ligma_image_flush (image);
}

void
vectors_selection_to_vectors_cmd_callback (LigmaAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  LigmaImage      *image;
  GtkWidget      *widget;
  LigmaProcedure  *procedure;
  LigmaValueArray *args;
  LigmaDisplay    *display;
  gboolean        advanced;
  GError         *error = NULL;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  advanced = (gboolean) g_variant_get_int32 (value);

  procedure = ligma_pdb_lookup_procedure (image->ligma->pdb,
                                         "plug-in-sel2path");

  if (! procedure)
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_ERROR,
                            "Selection to path procedure lookup failed.");
      return;
    }

  display = ligma_context_get_display (action_data_get_context (data));

  args = ligma_procedure_get_arguments (procedure);

  g_value_set_enum   (ligma_value_array_index (args, 0),
                      advanced ?
                      LIGMA_RUN_INTERACTIVE : LIGMA_RUN_NONINTERACTIVE);
  g_value_set_object (ligma_value_array_index (args, 1),
                      image);

  ligma_procedure_execute_async (procedure, image->ligma,
                                action_data_get_context (data),
                                LIGMA_PROGRESS (display), args,
                                display, &error);

  ligma_value_array_unref (args);

  if (error)
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_ERROR,
                            error->message);
      g_error_free (error);
    }
}

void
vectors_fill_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_fill_cmd_callback (action,
                           image, LIGMA_ITEM (vectors),
                           "ligma-vectors-fill-dialog",
                           _("Fill Path"),
                           LIGMA_ICON_TOOL_BUCKET_FILL,
                           LIGMA_HELP_PATH_FILL,
                           data);
}

void
vectors_fill_last_vals_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_fill_last_vals_cmd_callback (action,
                                     image, LIGMA_ITEM (vectors),
                                     data);
}

void
vectors_stroke_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_stroke_cmd_callback (action,
                             image, LIGMA_ITEM (vectors),
                             "ligma-vectors-stroke-dialog",
                             _("Stroke Path"),
                             LIGMA_ICON_PATH_STROKE,
                             LIGMA_HELP_PATH_STROKE,
                             data);
}

void
vectors_stroke_last_vals_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_stroke_last_vals_cmd_callback (action,
                                       image, LIGMA_ITEM (vectors),
                                       data);
}

void
vectors_copy_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaImage   *image;
  GList       *vectors;
  gchar       *svg;
  return_if_no_vectors_list (image, vectors, data);

  svg = ligma_vectors_export_string (image, vectors);

  if (svg)
    {
      ligma_clipboard_set_svg (image->ligma, svg);
      g_free (svg);
    }
}

void
vectors_paste_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  gchar     *svg;
  gsize      svg_size;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  svg = ligma_clipboard_get_svg (image->ligma, &svg_size);

  if (svg)
    {
      GError *error = NULL;

      if (! ligma_vectors_import_buffer (image, svg, svg_size,
                                        TRUE, FALSE,
                                        LIGMA_IMAGE_ACTIVE_PARENT, -1,
                                        NULL, &error))
        {
          ligma_message (image->ligma, G_OBJECT (widget), LIGMA_MESSAGE_ERROR,
                        "%s", error->message);
          g_clear_error (&error);
        }
      else
        {
          ligma_image_flush (image);
        }

      g_free (svg);
    }
}

void
vectors_export_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaImage   *image;
  GList       *vectors;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_vectors_list (image, vectors, data);
  return_if_no_widget (widget, data);

#define EXPORT_DIALOG_KEY "ligma-vectors-export-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), EXPORT_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
      GFile            *folder = NULL;

      if (config->vectors_export_path)
        folder = ligma_file_new_for_config_path (config->vectors_export_path,
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
vectors_import_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define IMPORT_DIALOG_KEY "ligma-vectors-import-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), IMPORT_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
      GFile            *folder = NULL;

      if (config->vectors_import_path)
        folder = ligma_file_new_for_config_path (config->vectors_import_path,
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
vectors_visible_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_visible_cmd_callback (action, value, image, LIGMA_ITEM (vectors));
}

void
vectors_lock_content_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_lock_content_cmd_callback (action, value, image, LIGMA_ITEM (vectors));
}

void
vectors_lock_position_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaImage   *image;
  LigmaVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  items_lock_position_cmd_callback (action, value, image, LIGMA_ITEM (vectors));
}

void
vectors_color_tag_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaImage    *image;
  LigmaVectors  *vectors;
  LigmaColorTag  color_tag;
  return_if_no_vectors (image, vectors, data);

  color_tag = (LigmaColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, LIGMA_ITEM (vectors),
                                color_tag);
}


/*  private functions  */

static void
vectors_new_callback (GtkWidget    *dialog,
                      LigmaImage    *image,
                      LigmaVectors  *vectors,
                      LigmaContext  *context,
                      const gchar  *vectors_name,
                      gboolean      vectors_visible,
                      LigmaColorTag  vectors_color_tag,
                      gboolean      vectors_lock_content,
                      gboolean      vectors_lock_position,
                      gpointer      user_data)
{
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  g_object_set (config,
                "path-new-name", vectors_name,
                NULL);

  vectors = ligma_vectors_new (image, config->vectors_new_name);
  ligma_item_set_visible (LIGMA_ITEM (vectors), vectors_visible, FALSE);
  ligma_item_set_color_tag (LIGMA_ITEM (vectors), vectors_color_tag, FALSE);
  ligma_item_set_lock_content (LIGMA_ITEM (vectors), vectors_lock_content, FALSE);
  ligma_item_set_lock_position (LIGMA_ITEM (vectors), vectors_lock_position, FALSE);

  ligma_image_add_vectors (image, vectors,
                          LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);
  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
vectors_edit_attributes_callback (GtkWidget    *dialog,
                                  LigmaImage    *image,
                                  LigmaVectors  *vectors,
                                  LigmaContext  *context,
                                  const gchar  *vectors_name,
                                  gboolean      vectors_visible,
                                  LigmaColorTag  vectors_color_tag,
                                  gboolean      vectors_lock_content,
                                  gboolean      vectors_lock_position,
                                  gpointer      user_data)
{
  LigmaItem *item = LIGMA_ITEM (vectors);

  if (strcmp (vectors_name, ligma_object_get_name (vectors))      ||
      vectors_visible       != ligma_item_get_visible (item)      ||
      vectors_color_tag     != ligma_item_get_color_tag (item)    ||
      vectors_lock_content  != ligma_item_get_lock_content (item) ||
      vectors_lock_position != ligma_item_get_lock_position (item))
    {
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Path Attributes"));

      if (strcmp (vectors_name, ligma_object_get_name (vectors)))
        ligma_item_rename (LIGMA_ITEM (vectors), vectors_name, NULL);

      if (vectors_visible != ligma_item_get_visible (item))
        ligma_item_set_visible (item, vectors_visible, TRUE);

      if (vectors_color_tag != ligma_item_get_color_tag (item))
        ligma_item_set_color_tag (item, vectors_color_tag, TRUE);

      if (vectors_lock_content != ligma_item_get_lock_content (item))
        ligma_item_set_lock_content (item, vectors_lock_content, TRUE);

      if (vectors_lock_position != ligma_item_get_lock_position (item))
        ligma_item_set_lock_position (item, vectors_lock_position, TRUE);

      ligma_image_undo_group_end (image);

      ligma_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}

static void
vectors_import_callback (GtkWidget *dialog,
                         LigmaImage *image,
                         GFile     *file,
                         GFile     *import_folder,
                         gboolean   merge_vectors,
                         gboolean   scale_vectors,
                         gpointer   user_data)
{
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
  gchar            *path   = NULL;
  GError           *error  = NULL;

  if (import_folder)
    path = ligma_file_get_config_path (import_folder, NULL);

  g_object_set (config,
                "path-import-path",  path,
                "path-import-merge", merge_vectors,
                "path-import-scale", scale_vectors,
                NULL);

  if (path)
    g_free (path);

  if (ligma_vectors_import_file (image, file,
                                config->vectors_import_merge,
                                config->vectors_import_scale,
                                LIGMA_IMAGE_ACTIVE_PARENT, -1,
                                NULL, &error))
    {
      ligma_image_flush (image);
    }
  else
    {
      ligma_message (image->ligma, G_OBJECT (dialog),
                    LIGMA_MESSAGE_ERROR,
                    "%s", error->message);
      g_error_free (error);
      return;
    }

  gtk_widget_destroy (dialog);
}

static void
vectors_export_callback (GtkWidget *dialog,
                         LigmaImage *image,
                         GFile     *file,
                         GFile     *export_folder,
                         gboolean   active_only,
                         gpointer   user_data)
{
  LigmaDialogConfig *config  = LIGMA_DIALOG_CONFIG (image->ligma->config);
  GList            *vectors = NULL;
  gchar            *path    = NULL;
  GError           *error   = NULL;

  if (export_folder)
    path = ligma_file_get_config_path (export_folder, NULL);

  g_object_set (config,
                "path-export-path",        path,
                "path-export-active-only", active_only,
                NULL);

  if (path)
    g_free (path);

  if (config->vectors_export_active_only)
    vectors = ligma_image_get_selected_vectors (image);

  if (! ligma_vectors_export_file (image, vectors, file, &error))
    {
      ligma_message (image->ligma, G_OBJECT (dialog),
                    LIGMA_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);
      return;
    }

  gtk_widget_destroy (dialog);
}

void
vectors_select_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaImage            *image;
  GList                *new_vectors = NULL;
  GList                *vectors;
  GList                *iter;
  LigmaActionSelectType  select_type;
  gboolean              run_once;
  return_if_no_image (image, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  vectors = ligma_image_get_selected_vectors (image);
  run_once = (g_list_length (vectors) == 0);

  for (iter = vectors; iter || run_once; iter = iter ? iter->next : NULL)
    {
      LigmaVectors   *new_vec;
      LigmaContainer *container;

      if (iter)
        {
          container = ligma_item_get_container (LIGMA_ITEM (iter->data));
        }
      else /* run_once */
        {
          container = ligma_image_get_vectors (image);
          run_once  = FALSE;
        }
      new_vec = (LigmaVectors *) action_select_object (select_type,
                                                      container,
                                                      iter ? iter->data : NULL);
      if (new_vec)
        new_vectors = g_list_prepend (new_vectors, new_vec);
    }

  if (new_vectors)
    {
      ligma_image_set_selected_vectors (image, new_vectors);
      ligma_image_flush (image);
    }

  g_list_free (new_vectors);
}
