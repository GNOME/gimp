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

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
#include "core/gimptoolinfo.h"

#include "pdb/procedural_db.h"

#include "plug-in/plug-in-run.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpvectors-export.h"
#include "vectors/gimpvectors-import.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemtreeview.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"

#include "tools/gimppainttool.h"
#include "tools/gimpvectortool.h"
#include "tools/tool_manager.h"

#include "stroke-dialog.h"
#include "vectors-commands.h"

#include "gimp-intl.h"


#define return_if_no_image(gimage,data) \
  if (GIMP_IS_DISPLAY (data)) \
    gimage = ((GimpDisplay *) data)->gimage; \
  else if (GIMP_IS_GIMP (data)) \
    gimage = gimp_context_get_image (gimp_get_user_context (GIMP (data))); \
  else if (GIMP_IS_ITEM_TREE_VIEW (data)) \
    gimage = ((GimpItemTreeView *) data)->gimage; \
  else \
    gimage = NULL; \
  \
  if (! gimage) \
    return

#define return_if_no_vectors(gimage,vectors,data) \
  return_if_no_image (gimage,data); \
  vectors = gimp_image_get_active_vectors (gimage); \
  if (! vectors) \
    return


static void  vectors_import_query  (GimpImage   *gimage,
                                    GtkWidget   *parent);
static void  vectors_export_query  (GimpImage   *gimage,
                                    GimpVectors *vectors,
                                    GtkWidget   *parent);


/*  public functions  */

void
vectors_new_cmd_callback (GtkWidget *widget,
                          gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  vectors_new_vectors_query (gimage, NULL, TRUE, widget);
}

void
vectors_raise_cmd_callback (GtkWidget *widget,
                            gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  gimp_image_raise_vectors (gimage, active_vectors);
  gimp_image_flush (gimage);
}

void
vectors_lower_cmd_callback (GtkWidget *widget,
                            gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  gimp_image_lower_vectors (gimage, active_vectors);
  gimp_image_flush (gimage);
}

void
vectors_duplicate_cmd_callback (GtkWidget *widget,
                                gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  GimpVectors *new_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  new_vectors = GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (active_vectors),
                                                   G_TYPE_FROM_INSTANCE (active_vectors),
                                                   TRUE));
  gimp_image_add_vectors (gimage, new_vectors, -1);
  gimp_image_flush (gimage);
}

void
vectors_delete_cmd_callback (GtkWidget *widget,
                             gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  gimp_image_remove_vectors (gimage, active_vectors);
  gimp_image_flush (gimage);
}

void
vectors_merge_visible_cmd_callback (GtkWidget *widget,
                                    gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  gimp_image_merge_visible_vectors (gimage);
  gimp_image_flush (gimage);
}

void
vectors_to_selection_cmd_callback (GtkWidget *widget,
                                   gpointer   data,
                                   guint      action)
{
  GimpChannelOps  op;
  GimpImage      *gimage;
  GimpVectors    *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  op = (GimpChannelOps) action;

  gimp_channel_select_vectors (gimp_image_get_mask (gimage),
                               _("Path to Selection"),
                               active_vectors,
                               op, TRUE, FALSE, 0, 0);
  gimp_image_flush (gimage);
}

void
vectors_selection_to_vectors_cmd_callback (GtkWidget *widget,
                                           gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  vectors_selection_to_vectors (gimage, FALSE);
}

void
vectors_stroke_cmd_callback (GtkWidget *widget,
                             gpointer   data)
{
  GimpImage    *gimage;
  GimpVectors  *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  vectors_stroke_vectors (GIMP_ITEM (active_vectors), widget);
}

void
vectors_copy_cmd_callback (GtkWidget *widget,
                           gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

#ifdef __GNUC__
#warning FIXME: need vectors clipboard
#endif
}

void
vectors_paste_cmd_callback (GtkWidget *widget,
                            gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

#ifdef __GNUC__
#warning FIXME: need vectors clipboard
#endif
}

void
vectors_import_cmd_callback (GtkWidget *widget,
                             gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  vectors_import_query (gimage, widget);
}

void
vectors_export_cmd_callback (GtkWidget *widget,
                             gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  vectors_export_query (gimage, active_vectors, widget);
}

void
vectors_vectors_tool_cmd_callback (GtkWidget   *widget,
                                   gpointer     data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  vectors_vectors_tool (active_vectors);
}

void
vectors_edit_attributes_cmd_callback (GtkWidget *widget,
                                      gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  vectors_edit_vectors_query (active_vectors, widget);
}

void
vectors_stroke_vectors (GimpItem  *item,
                        GtkWidget *parent)
{
  GimpImage    *gimage;
  GimpDrawable *active_drawable;
  GtkWidget    *dialog;

  g_return_if_fail (GIMP_IS_ITEM (item));

  gimage = gimp_item_get_image (item);

  active_drawable = gimp_image_active_drawable (gimage);

  if (! active_drawable)
    {
      g_message (_("There is no active layer or channel to stroke to."));
      return;
    }

  dialog = stroke_dialog_new (item, GIMP_STOCK_PATH_STROKE,
                              GIMP_HELP_PATH_STROKE,
                              parent);
  gtk_widget_show (dialog);
}

void
vectors_selection_to_vectors (GimpImage *gimage,
                              gboolean   advanced)
{
  ProcRecord   *proc_rec;
  Argument     *args;
  GimpDrawable *drawable;
  GimpDisplay  *gdisp;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  drawable = gimp_image_active_drawable (gimage);
  gdisp    = gimp_context_get_display (gimp_get_user_context (gimage->gimp));

  if (advanced)
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

  /*  plug-in arguments as if called by <Image>/Filters/...  */
  args = g_new (Argument, 3);

  args[0].arg_type      = GIMP_PDB_INT32;
  args[0].value.pdb_int = GIMP_RUN_INTERACTIVE;
  args[1].arg_type      = GIMP_PDB_IMAGE;
  args[1].value.pdb_int = (gint32) gimp_image_get_ID (gimage);
  args[2].arg_type      = GIMP_PDB_DRAWABLE;
  args[2].value.pdb_int = (gint32) gimp_item_get_ID (GIMP_ITEM (drawable));

  plug_in_run (gimage->gimp,
               proc_rec, args, 3, FALSE, TRUE,
	       gdisp ? gdisp->ID : 0);

  g_free (args);
}

void
vectors_vectors_tool (GimpVectors *vectors)
{
  GimpImage *gimage;
  GimpTool  *active_tool;

  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  gimage = gimp_item_get_image (GIMP_ITEM (vectors));

  active_tool = tool_manager_get_active (gimage->gimp);

  if (! GIMP_IS_VECTOR_TOOL (active_tool))
    {
      GimpContainer *tool_info_list;
      GimpToolInfo  *tool_info;

      tool_info_list = gimage->gimp->tool_info_list;

      tool_info = (GimpToolInfo *)
        gimp_container_get_child_by_name (tool_info_list,
                                          "gimp-vector-tool");

      if (GIMP_IS_TOOL_INFO (tool_info))
        {
          gimp_context_set_tool (gimp_get_current_context (gimage->gimp),
                                 tool_info);

          active_tool = tool_manager_get_active (gimage->gimp);
        }
    }

  if (GIMP_IS_VECTOR_TOOL (active_tool))
    gimp_vector_tool_set_vectors (GIMP_VECTOR_TOOL (active_tool), vectors);
}

/**********************************/
/*  The new vectors query dialog  */
/**********************************/

typedef struct _NewVectorsOptions NewVectorsOptions;

struct _NewVectorsOptions
{
  GtkWidget  *query_box;
  GtkWidget  *name_entry;

  GimpImage  *gimage;
};

static gchar *vectors_name = NULL;


static void
new_vectors_query_response (GtkWidget         *widget,
                            gint               response_id,
                            NewVectorsOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpVectors *new_vectors;
      GimpImage   *gimage;

      if (vectors_name)
        g_free (vectors_name);
      vectors_name =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

      if ((gimage = options->gimage))
        {
          new_vectors = gimp_vectors_new (gimage, vectors_name);

          gimp_image_add_vectors (gimage, new_vectors, -1);

          gimp_image_flush (gimage);
        }
    }

  gtk_widget_destroy (options->query_box);
}

void
vectors_new_vectors_query (GimpImage   *gimage,
                           GimpVectors *template,
                           gboolean     interactive,
                           GtkWidget   *parent)
{
  NewVectorsOptions *options;
  GtkWidget         *hbox;
  GtkWidget         *vbox;
  GtkWidget         *table;
  GtkWidget         *label;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (! template || GIMP_IS_VECTORS (template));

  if (template || ! interactive)
    {
      GimpVectors *new_vectors;

      new_vectors = gimp_vectors_new (gimage, _("Empty Vectors Copy"));

      gimp_image_add_vectors (gimage, new_vectors, -1);

      return;
    }

  /*  the new options structure  */
  options = g_new (NewVectorsOptions, 1);
  options->gimage = gimage;

  /*  The dialog  */
  options->query_box =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (gimage),
                              _("New Path"), "gimp-vectors-new",
                              GIMP_STOCK_TOOL_PATH,
                              _("New Path Options"),
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_PATH_NEW,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (new_vectors_query_response),
                    options);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     hbox);

  /*  The vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  /*  The table  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The name entry hbox, label and entry  */
  label = gtk_label_new (_("Path name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_widget_set_size_request (options->name_entry, 150, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), options->name_entry,
			     1, 2, 0, 1);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      (vectors_name ? vectors_name : _("New Path")));
  gtk_widget_show (options->name_entry);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (options->query_box);
}

/****************************************/
/*  The edit vectors attributes dialog  */
/****************************************/

typedef struct _EditVectorsOptions EditVectorsOptions;

struct _EditVectorsOptions
{
  GtkWidget     *query_box;
  GtkWidget     *name_entry;

  GimpVectors   *vectors;
  GimpImage     *gimage;
};

static void
edit_vectors_query_response (GtkWidget          *widget,
                             gint                response_id,
                             EditVectorsOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpVectors *vectors = options->vectors;

      if (options->gimage)
        {
          const gchar *new_name;

          new_name = gtk_entry_get_text (GTK_ENTRY (options->name_entry));

          if (strcmp (new_name, gimp_object_get_name (GIMP_OBJECT (vectors))))
            {
              gimp_item_rename (GIMP_ITEM (vectors), new_name);
              gimp_image_flush (options->gimage);
            }
        }
    }

  gtk_widget_destroy (options->query_box);
}

void
vectors_edit_vectors_query (GimpVectors *vectors,
                            GtkWidget   *parent)
{
  EditVectorsOptions *options;
  GtkWidget          *hbox;
  GtkWidget          *vbox;
  GtkWidget          *table;
  GtkWidget          *label;

  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  options = g_new0 (EditVectorsOptions, 1);

  options->vectors = vectors;
  options->gimage  = gimp_item_get_image (GIMP_ITEM (vectors));

  /*  The dialog  */
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

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (edit_vectors_query_response),
                    options);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     hbox);

  /*  The vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  /*  The table  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The name entry  */
  label = gtk_label_new (_("Path name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_widget_set_size_request (options->name_entry, 150, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), options->name_entry,
			     1, 2, 0, 1);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      gimp_object_get_name (GIMP_OBJECT (vectors)));
  gtk_widget_show (options->name_entry);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (options->query_box);
}


/*******************************/
/*  The vectors import dialog  */
/*******************************/

static void
vectors_import_response (GtkWidget *dialog,
                         gint       response_id,
                         GimpImage *gimage)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *filename;
      GError      *error = NULL;

      filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog));

      if (gimp_vectors_import (gimage, filename, FALSE, FALSE, &error))
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
vectors_import_query (GimpImage *gimage,
                      GtkWidget *parent)
{
  GtkFileSelection *filesel;

  filesel =
    GTK_FILE_SELECTION (gtk_file_selection_new (_("Import Paths from SVG")));

  g_object_weak_ref (G_OBJECT (gimage),
                     (GWeakNotify) gtk_widget_destroy, filesel);

  gtk_window_set_screen (GTK_WINDOW (filesel),
                         gtk_widget_get_screen (parent));

  gtk_window_set_role (GTK_WINDOW (filesel), "gimp-vectors-import");
  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (filesel), 6);
  gtk_container_set_border_width (GTK_CONTAINER (filesel->button_area), 4);

  g_signal_connect (filesel, "response",
                    G_CALLBACK (vectors_import_response),
                    gimage);
  g_signal_connect (filesel, "delete_event",
                    G_CALLBACK (gtk_true),
                    NULL);

  /*  FIXME: add a proper file selector
      and controls for merge and scale options  */

  gtk_widget_show (GTK_WIDGET (filesel));
}


/*******************************/
/*  The vectors export dialog  */
/*******************************/

static void
vectors_export_response (GtkWidget *dialog,
                         gint       response_id,
                         GimpImage *gimage)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *filename;
      GError      *error = NULL;

      filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog));

      if (! gimp_vectors_export (gimage, NULL, filename, &error))
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
vectors_export_query (GimpImage   *gimage,
                      GimpVectors *vectors,
                      GtkWidget   *parent)
{
  GtkFileSelection *filesel;

  filesel =
    GTK_FILE_SELECTION (gtk_file_selection_new (_("Export Path to SVG")));

  g_object_weak_ref (G_OBJECT (gimage),
                     (GWeakNotify) gtk_widget_destroy, filesel);

  gtk_window_set_screen (GTK_WINDOW (filesel),
                         gtk_widget_get_screen (parent));

  gtk_window_set_role (GTK_WINDOW (filesel), "gimp-vectors-export");
  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (filesel), 6);
  gtk_container_set_border_width (GTK_CONTAINER (filesel->button_area), 4);

  g_signal_connect (filesel, "response",
                    G_CALLBACK (vectors_export_response),
                    gimage);
  g_signal_connect (filesel, "delete_event",
                    G_CALLBACK (gtk_true),
                    NULL);

  gtk_widget_show (GTK_WIDGET (filesel));
}
