/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitemundo.h"
#include "core/gimpprogress.h"
#include "core/gimptoolinfo.h"

#include "pdb/procedural_db.h"

#include "plug-in/plug-in-run.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpvectors-export.h"
#include "vectors/gimpvectors-import.h"

#include "widgets/gimpaction.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"

#include "tools/gimpvectortool.h"
#include "tools/tool_manager.h"

#include "dialogs/stroke-dialog.h"

#include "actions.h"
#include "vectors-commands.h"

#include "gimp-intl.h"


typedef struct _VectorsOptions VectorsOptions;

struct _VectorsOptions
{
  GtkWidget   *query_box;
  GtkWidget   *name_entry;

  GimpImage   *gimage;
  GimpVectors *vectors;
};


/*  local function prototypes  */

static VectorsOptions * vectors_options_new (GimpImage      *gimage,
                                             GimpVectors    *vectors,
                                             GtkWidget      *parent);
static void   vectors_new_vectors_response  (GtkWidget      *widget,
                                             gint            response_id,
                                             VectorsOptions *options);
static void   vectors_edit_vectors_response (GtkWidget      *widget,
                                             gint            response_id,
                                             VectorsOptions *options);
static void   vectors_import_dialog         (GimpImage      *gimage,
                                             GtkWidget      *parent);
static void   vectors_import_response       (GtkWidget      *dialog,
                                             gint            response_id,
                                             GimpImage      *gimage);
static void   vectors_export_dialog         (GimpImage      *gimage,
                                             GimpVectors    *vectors,
                                             GtkWidget      *parent);
static void   vectors_export_response       (GtkWidget      *dialog,
                                             gint            response_id,
                                             GimpImage      *gimage);


/*  private variables  */

static gchar *vectors_name = NULL;


/*  public functions  */

void
vectors_vectors_tool_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  GimpTool    *active_tool;
  return_if_no_vectors (gimage, vectors, data);

  active_tool = tool_manager_get_active (gimage->gimp);

  if (! GIMP_IS_VECTOR_TOOL (active_tool))
    {
      GimpToolInfo  *tool_info;

      tool_info = (GimpToolInfo *)
        gimp_container_get_child_by_name (gimage->gimp->tool_info_list,
                                          "gimp-vector-tool");

      if (GIMP_IS_TOOL_INFO (tool_info))
        {
          gimp_context_set_tool (action_data_get_context (data), tool_info);
          active_tool = tool_manager_get_active (gimage->gimp);
        }
    }

  if (GIMP_IS_VECTOR_TOOL (active_tool))
    gimp_vector_tool_set_vectors (GIMP_VECTOR_TOOL (active_tool), vectors);
}

void
vectors_edit_attributes_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  VectorsOptions *options;
  GimpImage      *gimage;
  GimpVectors    *vectors;
  GtkWidget      *widget;
  return_if_no_vectors (gimage, vectors, data);
  return_if_no_widget (widget, data);

  options = vectors_options_new (gimp_item_get_image (GIMP_ITEM (vectors)),
                                 vectors, widget);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (vectors_edit_vectors_response),
                    options);

  gtk_widget_show (options->query_box);
}

void
vectors_new_cmd_callback (GtkAction *action,
                          gpointer   data)
{
  VectorsOptions *options;
  GimpImage      *gimage;
  GtkWidget      *widget;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  options = vectors_options_new (gimage, NULL, widget);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (vectors_new_vectors_response),
                    options);

  gtk_widget_show (options->query_box);
}

void
vectors_new_default_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *new_vectors;
  return_if_no_image (gimage, data);

  new_vectors = gimp_vectors_new (gimage, _("Empty Path"));

  gimp_image_add_vectors (gimage, new_vectors, -1);

  gimp_image_flush (gimage);
}

void
vectors_raise_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  return_if_no_vectors (gimage, vectors, data);

  gimp_image_raise_vectors (gimage, vectors);
  gimp_image_flush (gimage);
}

void
vectors_raise_to_top_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  return_if_no_vectors (gimage, vectors, data);

  gimp_image_raise_vectors_to_top (gimage, vectors);
  gimp_image_flush (gimage);
}

void
vectors_lower_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  return_if_no_vectors (gimage, vectors, data);

  gimp_image_lower_vectors (gimage, vectors);
  gimp_image_flush (gimage);
}

void
vectors_lower_to_bottom_cmd_callback (GtkAction *action,
                                      gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  return_if_no_vectors (gimage, vectors, data);

  gimp_image_lower_vectors_to_bottom (gimage, vectors);
  gimp_image_flush (gimage);
}

void
vectors_duplicate_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  GimpVectors *new_vectors;
  return_if_no_vectors (gimage, vectors, data);

  new_vectors =
    GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                       G_TYPE_FROM_INSTANCE (vectors),
                                       TRUE));
  gimp_image_add_vectors (gimage, new_vectors, -1);
  gimp_image_flush (gimage);
}

void
vectors_delete_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  return_if_no_vectors (gimage, vectors, data);

  gimp_image_remove_vectors (gimage, vectors);
  gimp_image_flush (gimage);
}

void
vectors_merge_visible_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  return_if_no_vectors (gimage, vectors, data);

  gimp_image_merge_visible_vectors (gimage);
  gimp_image_flush (gimage);
}

void
vectors_to_selection_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpChannelOps  op;
  GimpImage      *gimage;
  GimpVectors    *vectors;
  return_if_no_vectors (gimage, vectors, data);

  op = (GimpChannelOps) value;

  gimp_channel_select_vectors (gimp_image_get_mask (gimage),
                               _("Path to Selection"),
                               vectors,
                               op, TRUE, FALSE, 0, 0);
  gimp_image_flush (gimage);
}

void
vectors_selection_to_vectors_cmd_callback (GtkAction *action,
                                           gint       value,
                                           gpointer   data)
{
  GimpImage   *gimage;
  ProcRecord  *proc_rec;
  Argument    *args;
  GimpDisplay *gdisp;
  return_if_no_image (gimage, data);

  if (value)
    proc_rec = procedural_db_lookup (gimage->gimp,
                                     "plug_in_sel2path_advanced");
  else
    proc_rec = procedural_db_lookup (gimage->gimp,
                                     "plug_in_sel2path");

  if (! proc_rec)
    {
      g_message ("Selection to path procedure lookup failed.");
      return;
    }

  gdisp = gimp_context_get_display (action_data_get_context (data));

  /*  plug-in arguments as if called by <Image>/Filters/...  */
  args = g_new (Argument, 3);

  args[0].arg_type      = GIMP_PDB_INT32;
  args[0].value.pdb_int = GIMP_RUN_INTERACTIVE;
  args[1].arg_type      = GIMP_PDB_IMAGE;
  args[1].value.pdb_int = (gint32) gimp_image_get_ID (gimage);
  args[2].arg_type      = GIMP_PDB_DRAWABLE;
  args[2].value.pdb_int = -1;  /*  unused  */

  plug_in_run (gimage->gimp, action_data_get_context (data),
               gdisp ? GIMP_PROGRESS (gdisp) : NULL,
               proc_rec, args, 3, FALSE, TRUE,
	       gdisp ? gimp_display_get_ID (gdisp) : 0);

  g_free (args);
}

void
vectors_stroke_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage    *gimage;
  GimpVectors  *vectors;
  GimpDrawable *drawable;
  GtkWidget    *widget;
  GtkWidget    *dialog;
  return_if_no_vectors (gimage, vectors, data);
  return_if_no_widget (widget, data);

  drawable = gimp_image_active_drawable (gimage);

  if (! drawable)
    {
      g_message (_("There is no active layer or channel to stroke to."));
      return;
    }

  dialog = stroke_dialog_new (GIMP_ITEM (vectors),
                              GIMP_STOCK_PATH_STROKE,
                              GIMP_HELP_PATH_STROKE,
                              widget);
  gtk_widget_show (dialog);
}

void
vectors_copy_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  return_if_no_vectors (gimage, vectors, data);

#ifdef __GNUC__
#warning FIXME: need vectors clipboard
#endif
}

void
vectors_paste_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

#ifdef __GNUC__
#warning FIXME: need vectors clipboard
#endif
}

void
vectors_import_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage *gimage;
  GtkWidget *widget;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  vectors_import_dialog (gimage, widget);
}

void
vectors_export_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  GtkWidget   *widget;
  return_if_no_vectors (gimage, vectors, data);
  return_if_no_widget (widget, data);

  vectors_export_dialog (gimage, vectors, widget);
}

void
vectors_visible_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  gboolean     visible;
  return_if_no_vectors (gimage, vectors, data);

  visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (visible != gimp_item_get_visible (GIMP_ITEM (vectors)))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (gimage, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_VISIBILITY);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (vectors))
        push_undo = FALSE;

      gimp_item_set_visible (GIMP_ITEM (vectors), visible, push_undo);
      gimp_image_flush (gimage);
    }
}

void
vectors_linked_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  gboolean     linked;
  return_if_no_vectors (gimage, vectors, data);

  linked = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (linked != gimp_item_get_linked (GIMP_ITEM (vectors)))
    {
      GimpUndo *undo;
      gboolean  push_undo = TRUE;

      undo = gimp_image_undo_can_compress (gimage, GIMP_TYPE_ITEM_UNDO,
                                           GIMP_UNDO_ITEM_LINKED);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (vectors))
        push_undo = FALSE;

      gimp_item_set_linked (GIMP_ITEM (vectors), linked, push_undo);
      gimp_image_flush (gimage);
    }
}


/*  private functions  */

static VectorsOptions *
vectors_options_new (GimpImage   *gimage,
                     GimpVectors *vectors,
                     GtkWidget   *parent)
{
  VectorsOptions *options;
  GtkWidget      *hbox;
  GtkWidget      *vbox;
  GtkWidget      *table;

  options = g_new0 (VectorsOptions, 1);

  options->gimage  = gimage;
  options->vectors = vectors;

  if (vectors)
    {
      options->query_box =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (vectors),
                                  _("Path Attributes"), "gimp-vectors-edit",
                                  GIMP_STOCK_EDIT,
                                  _("Edit Path Attributes"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_PATH_EDIT,

                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                  GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                  NULL);
    }
  else
    {
      options->query_box =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (gimage),
                                  _("New Path"), "gimp-vectors-new",
                                  GIMP_STOCK_PATH,
                                  _("New Path Options"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_PATH_NEW,

                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                  GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                  NULL);
    }

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     hbox);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->name_entry = gtk_entry_new ();
  gtk_widget_set_size_request (options->name_entry, 150, -1);
  gtk_entry_set_activates_default (GTK_ENTRY (options->name_entry), TRUE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Path Name:"), 0.0, 0.5,
                             options->name_entry, 1, FALSE);

  if (vectors)
    gtk_entry_set_text (GTK_ENTRY (options->name_entry),
                        gimp_object_get_name (GIMP_OBJECT (vectors)));
  else
    gtk_entry_set_text (GTK_ENTRY (options->name_entry),
                        vectors_name ? vectors_name : _("New Path"));

  return options;
}

static void
vectors_new_vectors_response (GtkWidget      *widget,
                              gint            response_id,
                              VectorsOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpVectors *new_vectors;

      if (vectors_name)
        g_free (vectors_name);

      vectors_name =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

      new_vectors = gimp_vectors_new (options->gimage, vectors_name);

      gimp_image_add_vectors (options->gimage, new_vectors, -1);

      gimp_image_flush (options->gimage);
    }

  gtk_widget_destroy (options->query_box);
}

static void
vectors_edit_vectors_response (GtkWidget      *widget,
                               gint            response_id,
                               VectorsOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpVectors *vectors = options->vectors;
      const gchar *new_name;

      new_name = gtk_entry_get_text (GTK_ENTRY (options->name_entry));

      if (strcmp (new_name, gimp_object_get_name (GIMP_OBJECT (vectors))))
        {
          gimp_item_rename (GIMP_ITEM (vectors), new_name);
          gimp_image_flush (options->gimage);
        }
    }

  gtk_widget_destroy (options->query_box);
}

static void
vectors_import_dialog (GimpImage *gimage,
                       GtkWidget *parent)
{
  GtkWidget     *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new (_("Import Paths from SVG"), NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,

                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

                                        NULL);

  g_object_weak_ref (G_OBJECT (gimage),
                     (GWeakNotify) gtk_widget_destroy, dialog);

  gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (parent));

  gtk_window_set_role (GTK_WINDOW (dialog), "gimp-vectors-import");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (vectors_import_response),
                    gimage);
  g_signal_connect (dialog, "delete_event",
                    G_CALLBACK (gtk_true),
                    NULL);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All Files (*.*)"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Scalable SVG image (*.svg)"));
  gtk_file_filter_add_pattern (filter, "*.[Ss][Vv][Gg]");
  gtk_file_filter_add_mime_type (filter, "image/svg+xml");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  /*  FIXME: add controls for merge and scale options  */

  gtk_widget_show (dialog);
}

static void
vectors_import_response (GtkWidget *dialog,
                         gint       response_id,
                         GimpImage *gimage)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *filename;
      GError      *error = NULL;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (gimp_vectors_import_file (gimage, filename, FALSE, FALSE, -1, &error))
        {
          gimp_image_flush (gimage);
        }
      else
        {
          g_message (error->message);
          g_error_free (error);
        }
    }

  g_object_weak_unref (G_OBJECT (gimage),
                       (GWeakNotify) gtk_widget_destroy, dialog);
  gtk_widget_destroy (dialog);
}

static void
vectors_export_dialog (GimpImage   *gimage,
                       GimpVectors *vectors,
                       GtkWidget   *parent)
{
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new (_("Export Path to SVG"), NULL,
                                        GTK_FILE_CHOOSER_ACTION_SAVE,

                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                        NULL);

  g_object_weak_ref (G_OBJECT (gimage),
                     (GWeakNotify) gtk_widget_destroy, dialog);

  gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (parent));

  gtk_window_set_role (GTK_WINDOW (dialog), "gimp-vectors-export");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (vectors_export_response),
                    gimage);
  g_signal_connect (dialog, "delete_event",
                    G_CALLBACK (gtk_true),
                    NULL);

  /*  FIXME: add control for saving all or just the active vectors  */

  gtk_widget_show (dialog);
}

static void
vectors_export_response (GtkWidget *dialog,
                         gint       response_id,
                         GimpImage *gimage)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *filename;
      GError      *error = NULL;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (! gimp_vectors_export_file (gimage, NULL, filename, &error))
        {
          g_message (error->message);
          g_error_free (error);
        }
    }

  g_object_weak_unref (G_OBJECT (gimage),
                       (GWeakNotify) gtk_widget_destroy, dialog);
  gtk_widget_destroy (dialog);
}
