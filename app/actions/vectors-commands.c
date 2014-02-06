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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitemundo.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"
#include "core/gimpstrokeoptions.h"
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

#include "dialogs/stroke-dialog.h"
#include "dialogs/vectors-export-dialog.h"
#include "dialogs/vectors-import-dialog.h"
#include "dialogs/vectors-options-dialog.h"

#include "actions.h"
#include "vectors-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   vectors_new_vectors_response  (GtkWidget            *widget,
                                             gint                  response_id,
                                             VectorsOptionsDialog *options);
static void   vectors_edit_vectors_response (GtkWidget            *widget,
                                             gint                  response_id,
                                             VectorsOptionsDialog *options);
static void   vectors_import_response       (GtkWidget            *widget,
                                             gint                  response_id,
                                             VectorsImportDialog  *dialog);
static void   vectors_export_response       (GtkWidget            *widget,
                                             gint                  response_id,
                                             VectorsExportDialog  *dialog);


/*  private variables  */

static gchar    *vectors_name               = NULL;
static gboolean  vectors_import_merge       = FALSE;
static gboolean  vectors_import_scale       = FALSE;
static gboolean  vectors_export_active_only = TRUE;


/*  public functions  */

void
vectors_vectors_tool_cmd_callback (GtkAction *action,
                                   gpointer   data)
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
vectors_edit_attributes_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  VectorsOptionsDialog *options;
  GimpImage            *image;
  GimpVectors          *vectors;
  GtkWidget            *widget;
  return_if_no_vectors (image, vectors, data);
  return_if_no_widget (widget, data);

  options = vectors_options_dialog_new (image, vectors,
                                        action_data_get_context (data),
                                        widget,
                                        gimp_object_get_name (vectors),
                                        _("Path Attributes"),
                                        "gimp-vectors-edit",
                                        GTK_STOCK_EDIT,
                                        _("Edit Path Attributes"),
                                        GIMP_HELP_PATH_EDIT);

  g_signal_connect (options->dialog, "response",
                    G_CALLBACK (vectors_edit_vectors_response),
                    options);

  gtk_widget_show (options->dialog);
}

void
vectors_new_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  VectorsOptionsDialog *options;
  GimpImage            *image;
  GtkWidget            *widget;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  options = vectors_options_dialog_new (image, NULL,
                                        action_data_get_context (data),
                                        widget,
                                        vectors_name ? vectors_name :
                                        _("Path"),
                                        _("New Path"),
                                        "gimp-vectors-new",
                                        GIMP_STOCK_PATH,
                                        _("New Path Options"),
                                        GIMP_HELP_PATH_NEW);

  g_signal_connect (options->dialog, "response",
                    G_CALLBACK (vectors_new_vectors_response),
                    options);

  gtk_widget_show (options->dialog);
}

void
vectors_new_last_vals_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpImage   *image;
  GimpVectors *new_vectors;
  return_if_no_image (image, data);

  new_vectors = gimp_vectors_new (image, vectors_name);

  gimp_image_add_vectors (image, new_vectors,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  gimp_image_flush (image);
}

void
vectors_raise_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_image_raise_item (image, GIMP_ITEM (vectors), NULL);
  gimp_image_flush (image);
}

void
vectors_raise_to_top_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_image_raise_item_to_top (image, GIMP_ITEM (vectors));
  gimp_image_flush (image);
}

void
vectors_lower_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_image_lower_item (image, GIMP_ITEM (vectors), NULL);
  gimp_image_flush (image);
}

void
vectors_lower_to_bottom_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_image_lower_item_to_bottom (image, GIMP_ITEM (vectors));
  gimp_image_flush (image);
}

void
vectors_duplicate_cmd_callback (GtkAction *action,
                                gpointer   data)
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
vectors_delete_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_image_remove_vectors (image, vectors, TRUE, NULL);
  gimp_image_flush (image);
}

void
vectors_merge_visible_cmd_callback (GtkAction *action,
                                    gpointer   data)
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
vectors_to_selection_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  return_if_no_vectors (image, vectors, data);

  gimp_item_to_selection (GIMP_ITEM (vectors),
                          (GimpChannelOps) value,
                          TRUE, FALSE, 0, 0);
  gimp_image_flush (image);
}

void
vectors_selection_to_vectors_cmd_callback (GtkAction *action,
                                           gint       value,
                                           gpointer   data)
{
  GimpImage     *image;
  GtkWidget     *widget;
  GimpProcedure *procedure;
  GValueArray   *args;
  GimpDisplay   *display;
  GError        *error = NULL;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  if (value)
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

  g_value_set_int      (&args->values[0], GIMP_RUN_INTERACTIVE);
  gimp_value_set_image (&args->values[1], image);

  gimp_procedure_execute_async (procedure, image->gimp,
                                action_data_get_context (data),
                                GIMP_PROGRESS (display), args,
                                GIMP_OBJECT (display), &error);

  g_value_array_free (args);

  if (error)
    {
      gimp_message_literal (image->gimp,
			    G_OBJECT (widget), GIMP_MESSAGE_ERROR,
			    error->message);
      g_error_free (error);
    }
}

void
vectors_stroke_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage    *image;
  GimpVectors  *vectors;
  GimpDrawable *drawable;
  GtkWidget    *widget;
  GtkWidget    *dialog;
  return_if_no_vectors (image, vectors, data);
  return_if_no_widget (widget, data);

  drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    {
      gimp_message_literal (image->gimp,
			    G_OBJECT (widget), GIMP_MESSAGE_WARNING,
			    _("There is no active layer or channel to stroke to."));
      return;
    }

  dialog = stroke_dialog_new (GIMP_ITEM (vectors),
                              action_data_get_context (data),
                              _("Stroke Path"),
                              GIMP_STOCK_PATH_STROKE,
                              GIMP_HELP_PATH_STROKE,
                              widget);
  gtk_widget_show (dialog);
}

void
vectors_stroke_last_vals_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  GimpImage         *image;
  GimpVectors       *vectors;
  GimpDrawable      *drawable;
  GimpContext       *context;
  GtkWidget         *widget;
  GimpStrokeOptions *options;
  GError            *error = NULL;
  return_if_no_vectors (image, vectors, data);
  return_if_no_context (context, data);
  return_if_no_widget (widget, data);

  drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    {
      gimp_message_literal (image->gimp,
			    G_OBJECT (widget), GIMP_MESSAGE_WARNING,
			    _("There is no active layer or channel to stroke to."));
      return;
    }


  options = g_object_get_data (G_OBJECT (image->gimp), "saved-stroke-options");

  if (options)
    g_object_ref (options);
  else
    options = gimp_stroke_options_new (image->gimp, context, TRUE);

  if (! gimp_item_stroke (GIMP_ITEM (vectors),
                          drawable, context, options, NULL,
                          TRUE, NULL, &error))
    {
      gimp_message_literal (image->gimp, G_OBJECT (widget),
			    GIMP_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }
  else
    {
      gimp_image_flush (image);
    }

  g_object_unref (options);
}

void
vectors_copy_cmd_callback (GtkAction *action,
                           gpointer   data)
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
vectors_paste_cmd_callback (GtkAction *action,
                            gpointer   data)
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
vectors_export_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  VectorsExportDialog *dialog;
  GimpImage           *image;
  GimpVectors         *vectors;
  GtkWidget           *widget;
  const gchar         *folder;
  return_if_no_vectors (image, vectors, data);
  return_if_no_widget (widget, data);

  dialog = vectors_export_dialog_new (image, widget,
                                      vectors_export_active_only);

  folder = g_object_get_data (G_OBJECT (image->gimp),
                              "gimp-vectors-export-folder");
  if (folder)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog->dialog),
                                         folder);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (vectors_export_response),
                    dialog);

  gtk_widget_show (dialog->dialog);
}

void
vectors_import_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  VectorsImportDialog *dialog;
  GimpImage           *image;
  GtkWidget           *widget;
  const gchar         *folder;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = vectors_import_dialog_new (image, widget,
                                      vectors_import_merge,
                                      vectors_import_scale);

  folder = g_object_get_data (G_OBJECT (image->gimp),
                              "gimp-vectors-import-folder");
  if (folder)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog->dialog),
                                         folder);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (vectors_import_response),
                    dialog);

  gtk_widget_show (dialog->dialog);
}

void
vectors_visible_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  gboolean     visible;
  return_if_no_vectors (image, vectors, data);

  visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (visible != gimp_item_get_visible (GIMP_ITEM (vectors)))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_VISIBILITY);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (vectors))
        push_undo = FALSE;

      gimp_item_set_visible (GIMP_ITEM (vectors), visible, push_undo);
      gimp_image_flush (image);
    }
}

void
vectors_linked_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  gboolean     linked;
  return_if_no_vectors (image, vectors, data);

  linked = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (linked != gimp_item_get_linked (GIMP_ITEM (vectors)))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_LINKED);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (vectors))
        push_undo = FALSE;

      gimp_item_set_linked (GIMP_ITEM (vectors), linked, push_undo);
      gimp_image_flush (image);
    }
}

void
vectors_lock_content_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpImage   *image;
  GimpVectors *vectors;
  gboolean     locked;
  return_if_no_vectors (image, vectors, data);

  locked = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (locked != gimp_item_get_lock_content (GIMP_ITEM (vectors)))
    {
#if 0
      GimpUndo *undo;
#endif
      gboolean  push_undo = TRUE;

#if 0
      undo = gimp_image_undo_can_compress (image, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_LINKED);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (vectors))
        push_undo = FALSE;
#endif

      gimp_item_set_lock_content (GIMP_ITEM (vectors), locked, push_undo);
      gimp_image_flush (image);
    }
}


/*  private functions  */

static void
vectors_new_vectors_response (GtkWidget            *widget,
                              gint                  response_id,
                              VectorsOptionsDialog *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpVectors *new_vectors;

      if (vectors_name)
        g_free (vectors_name);

      vectors_name =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

      new_vectors = gimp_vectors_new (options->image, vectors_name);

      gimp_image_add_vectors (options->image, new_vectors,
                              GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

      gimp_image_flush (options->image);
    }

  gtk_widget_destroy (options->dialog);
}

static void
vectors_edit_vectors_response (GtkWidget            *widget,
                               gint                  response_id,
                               VectorsOptionsDialog *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpVectors *vectors = options->vectors;
      const gchar *new_name;

      new_name = gtk_entry_get_text (GTK_ENTRY (options->name_entry));

      if (strcmp (new_name, gimp_object_get_name (vectors)))
        {
          gimp_item_rename (GIMP_ITEM (vectors), new_name, NULL);
          gimp_image_flush (options->image);
        }
    }

  gtk_widget_destroy (options->dialog);
}

static void
vectors_import_response (GtkWidget           *widget,
                         gint                 response_id,
                         VectorsImportDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (widget);
      gchar          *filename;
      GError         *error = NULL;

      vectors_import_merge = dialog->merge_vectors;
      vectors_import_scale = dialog->scale_vectors;

      filename = gtk_file_chooser_get_filename (chooser);

      if (gimp_vectors_import_file (dialog->image, filename,
                                    vectors_import_merge, vectors_import_scale,
                                    GIMP_IMAGE_ACTIVE_PARENT, -1,
                                    NULL, &error))
        {
          gimp_image_flush (dialog->image);
        }
      else
        {
          gimp_message (dialog->image->gimp, G_OBJECT (widget),
                        GIMP_MESSAGE_ERROR,
                        "%s", error->message);
          g_error_free (error);
          return;
        }

      g_free (filename);

      g_object_set_data_full (G_OBJECT (dialog->image->gimp),
                              "gimp-vectors-import-folder",
                              gtk_file_chooser_get_current_folder (chooser),
                              (GDestroyNotify) g_free);
    }

  gtk_widget_destroy (widget);
}

static void
vectors_export_response (GtkWidget           *widget,
                         gint                 response_id,
                         VectorsExportDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (widget);
      GimpVectors    *vectors = NULL;
      gchar          *filename;
      GError         *error   = NULL;

      vectors_export_active_only = dialog->active_only;

      filename = gtk_file_chooser_get_filename (chooser);

      if (vectors_export_active_only)
        vectors = gimp_image_get_active_vectors (dialog->image);

      if (! gimp_vectors_export_file (dialog->image, vectors, filename, &error))
        {
          gimp_message (dialog->image->gimp, G_OBJECT (widget),
                        GIMP_MESSAGE_ERROR,
                        "%s", error->message);
          g_error_free (error);
          return;
        }

      g_free (filename);

      g_object_set_data_full (G_OBJECT (dialog->image->gimp),
                              "gimp-vectors-export-folder",
                              gtk_file_chooser_get_current_folder (chooser),
                              (GDestroyNotify) g_free);
    }

  gtk_widget_destroy (widget);
}
