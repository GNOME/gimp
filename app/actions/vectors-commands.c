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
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-mask-select.h"
#include "core/gimplist.h"
#include "core/gimppaintinfo.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintcore.h"
#include "paint/gimppaintcore-stroke.h"

#include "vectors/gimpvectors.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "tools/gimppainttool.h"
#include "tools/gimpvectortool.h"
#include "tools/tool_manager.h"

#include "vectors-commands.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


#define return_if_no_image(gimage,data) \
  gimage = (GimpImage *) gimp_widget_get_callback_context (widget); \
  if (! GIMP_IS_IMAGE (gimage)) { \
    if (GIMP_IS_DISPLAY (data)) \
      gimage = ((GimpDisplay *) data)->gimage; \
    else if (GIMP_IS_GIMP (data)) \
      gimage = gimp_context_get_image (gimp_get_user_context (GIMP (data))); \
    else \
      gimage = NULL; \
  } \
  if (! gimage) \
    return

#define return_if_no_vectors(gimage,vectors,data) \
  return_if_no_image (gimage,data); \
  vectors = gimp_image_get_active_vectors (gimage); \
  if (! vectors) \
    return


/*  public functions  */

void
vectors_new_vectors_cmd_callback (GtkWidget *widget,
                                  gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  vectors_new_vectors_query (gimage, NULL, TRUE);
}

void
vectors_raise_vectors_cmd_callback (GtkWidget *widget,
                                    gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  gimp_image_raise_vectors (gimage, active_vectors);
  gimp_image_flush (gimage);
}

void
vectors_lower_vectors_cmd_callback (GtkWidget *widget,
                                    gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  gimp_image_lower_vectors (gimage, active_vectors);
  gimp_image_flush (gimage);
}

void
vectors_duplicate_vectors_cmd_callback (GtkWidget *widget,
                                        gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  GimpVectors *new_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  new_vectors = NULL;

#ifdef __GNUC__
#warning FIXME: need gimp_vectors_copy()
#endif
#if 0
  new_vectors = gimp_vectors_copy (active_vectors,
                                   G_TYPE_FROM_INSTANCE (active_vectors),
                                   TRUE);
  gimp_image_add_vectors (gimage, new_vectors, -1);
  gimp_image_flush (gimage);
#endif
}

void
vectors_delete_vectors_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  gimp_image_remove_vectors (gimage, active_vectors);
  gimp_image_flush (gimage);
}

static void
vectors_vectors_to_sel (GtkWidget      *widget,
                        gpointer        data,
                        GimpChannelOps  op)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  gimp_image_mask_select_vectors (gimage,
                                  active_vectors,
                                  op,
                                  TRUE,
                                  FALSE, 0, 0);
  gimp_image_flush (gimage);
}

void
vectors_vectors_to_sel_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  vectors_vectors_to_sel (widget, data, GIMP_CHANNEL_OP_REPLACE);
}

void
vectors_add_vectors_to_sel_cmd_callback (GtkWidget *widget,
                                         gpointer   data)
{
  vectors_vectors_to_sel (widget, data, GIMP_CHANNEL_OP_ADD);
}

void
vectors_sub_vectors_from_sel_cmd_callback (GtkWidget *widget,
                                           gpointer   data)
{
  vectors_vectors_to_sel (widget, data, GIMP_CHANNEL_OP_SUBTRACT);
}

void
vectors_intersect_vectors_with_sel_cmd_callback (GtkWidget *widget,
                                                 gpointer   data)
{
  vectors_vectors_to_sel (widget, data, GIMP_CHANNEL_OP_INTERSECT);
}

void
vectors_sel_to_vectors_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

#ifdef __GNUC__
#warning FIXME: need gimp_vectors_from_mask(or something)
#endif
}

void
vectors_stroke_vectors_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  vectors_stroke_vectors (active_vectors);
}

void
vectors_copy_vectors_cmd_callback (GtkWidget *widget,
                                   gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

#ifdef __GNUC__
#warning FIXME: need vectors clipoard
#endif
}

void
vectors_paste_vectors_cmd_callback (GtkWidget *widget,
                                    gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

#ifdef __GNUC__
#warning FIXME: need vectors clipoard
#endif
}

void
vectors_import_vectors_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

#ifdef __GNUC__
#warning FIXME: need vectors import/export
#endif
}

void
vectors_export_vectors_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

#ifdef __GNUC__
#warning FIXME: need vectors import/export
#endif
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
vectors_edit_vectors_attributes_cmd_callback (GtkWidget *widget,
                                              gpointer   data)
{
  GimpImage   *gimage;
  GimpVectors *active_vectors;
  return_if_no_vectors (gimage, active_vectors, data);

  vectors_edit_vectors_query (active_vectors);
}

void
vectors_stroke_vectors (GimpVectors *vectors)
{
  GimpImage    *gimage;
  GimpDrawable *active_drawable;

  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  gimage = gimp_item_get_image (GIMP_ITEM (vectors));

  active_drawable = gimp_image_active_drawable (gimage);

  if (! active_drawable)
    {
      g_message (_("There is no active Layer or Channel to stroke to"));
      return;
    }

  if (vectors && vectors->strokes)
    {
      GimpTool         *active_tool;
      GimpPaintCore    *core;
      GimpToolInfo     *tool_info;
      GimpPaintOptions *paint_options;
      GimpDisplay      *gdisp;

      active_tool = tool_manager_get_active (gimage->gimp);

      if (GIMP_IS_PAINT_TOOL (active_tool))
        {
          tool_info = active_tool->tool_info;
        }
      else
        {
          tool_info = (GimpToolInfo *)
            gimp_container_get_child_by_name (gimage->gimp->tool_info_list,
                                              "gimp-paintbrush-tool");
        }

      paint_options = (GimpPaintOptions *) tool_info->tool_options;

      core = g_object_new (tool_info->paint_info->paint_type, NULL);

      gdisp = gimp_context_get_display (gimp_get_current_context (gimage->gimp));

      tool_manager_control_active (gimage->gimp, PAUSE, gdisp);

      gimp_paint_core_stroke_vectors (core,
                                      active_drawable,
                                      paint_options,
                                      vectors);

      tool_manager_control_active (gimage->gimp, RESUME, gdisp);

      g_object_unref (core);

      gimp_image_flush (gimage);
    }
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
      GimpToolInfo *tool_info;

      tool_info = tool_manager_get_info_by_type (gimage->gimp,
                                                 GIMP_TYPE_VECTOR_TOOL);

      gimp_context_set_tool (gimp_get_current_context (gimage->gimp),
                             tool_info);

      active_tool = tool_manager_get_active (gimage->gimp);
    }

  gimp_vector_tool_set_vectors (GIMP_VECTOR_TOOL (active_tool),
                                vectors);
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
new_vectors_query_ok_callback (GtkWidget *widget,
			       gpointer   data)
{
  NewVectorsOptions *options;
  GimpVectors       *new_vectors;
  GimpImage         *gimage;

  options = (NewVectorsOptions *) data;

  if (vectors_name)
    g_free (vectors_name);
  vectors_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

  if ((gimage = options->gimage))
    {
      new_vectors = g_object_new (GIMP_TYPE_VECTORS, NULL);

      gimp_image_add_vectors (gimage, new_vectors, -1);

      gimp_object_set_name (GIMP_OBJECT (new_vectors), vectors_name);

      gimp_image_flush (gimage);
    }

  gtk_widget_destroy (options->query_box);
}

void
vectors_new_vectors_query (GimpImage   *gimage,
                           GimpVectors *template,
                           gboolean     interactive)
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

      /* undo_push_group_start (gimage, EDIT_PASTE_UNDO_GROUP); */

      new_vectors = NULL; /*gimp_vectors_new (gimage, _("Empty Vectors Copy"));*/

      gimp_image_add_vectors (gimage, new_vectors, -1);

      /* undo_push_group_end (gimage); */

      return;
    }

  /*  the new options structure  */
  options = g_new (NewVectorsOptions, 1);
  options->gimage = gimage;
  
  /*  The dialog  */
  options->query_box =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (gimage),
                              _("New Path"), "new_path_options",
                              GTK_STOCK_NEW,
                              _("New Path Options"),
                              gimp_standard_help_func,
                              "dialogs/vectors/new_vectors.html",

                              GTK_STOCK_CANCEL, gtk_widget_destroy,
                              NULL, 1, NULL, FALSE, TRUE,

                              GTK_STOCK_OK, new_vectors_query_ok_callback,
                              options, NULL, NULL, TRUE, FALSE,

                              NULL);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
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
edit_vectors_query_ok_callback (GtkWidget *widget,
				gpointer   data)
{
  EditVectorsOptions *options;
  GimpVectors        *vectors;

  options = (EditVectorsOptions *) data;
  vectors = options->vectors;

  if (options->gimage)
    {
      const gchar *new_name;

      new_name = gtk_entry_get_text (GTK_ENTRY (options->name_entry));

      if (strcmp (new_name, gimp_object_get_name (GIMP_OBJECT (vectors))))
        {
          undo_push_item_rename (options->gimage, GIMP_ITEM (vectors));

          gimp_object_set_name (GIMP_OBJECT (vectors), new_name);
        }
    }

  gtk_widget_destroy (options->query_box);
}

void
vectors_edit_vectors_query (GimpVectors *vectors)
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
                              _("Path Attributes"), "edit_path_attributes",
                              GIMP_STOCK_EDIT,
                              _("Edit Path Attributes"),
                              gimp_standard_help_func,
                              "dialogs/paths/edit_path_attributes.html",

                              GTK_STOCK_CANCEL, gtk_widget_destroy,
                              NULL, 1, NULL, FALSE, TRUE,

                              GTK_STOCK_OK, edit_vectors_query_ok_callback,
                              options, NULL, NULL, TRUE, FALSE,

                              NULL);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
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

void
vectors_menu_update (GtkItemFactory *factory,
                     gpointer        data)
{
  GimpImage   *gimage;
  GimpVectors *vectors;
  gboolean     mask_empty;
  gboolean     global_buf;
  GList       *list;
  GList       *next = NULL;
  GList       *prev = NULL;

  gimage = GIMP_IMAGE (data);

  vectors = gimp_image_get_active_vectors (gimage);

  mask_empty = gimp_image_mask_is_empty (gimage);

  global_buf = FALSE;

  for (list = GIMP_LIST (gimage->vectors)->list;
       list;
       list = g_list_next (list))
    {
      if (vectors == (GimpVectors *) list->data)
	{
	  prev = g_list_previous (list);
	  next = g_list_next (list);
	  break;
	}
    }

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/New Path...",              TRUE);
  SET_SENSITIVE ("/Raise Path",               vectors && prev);
  SET_SENSITIVE ("/Lower Path",               vectors && next);
  SET_SENSITIVE ("/Duplicate Path",           vectors);
  SET_SENSITIVE ("/Path to Selection",        vectors);
  SET_SENSITIVE ("/Add to Selection",         vectors);
  SET_SENSITIVE ("/Subtract from Selection",  vectors);
  SET_SENSITIVE ("/Intersect with Selection", vectors);
  SET_SENSITIVE ("/Selection to Path",        ! mask_empty);
  SET_SENSITIVE ("/Stroke Path",              vectors);
  SET_SENSITIVE ("/Delete Path",              vectors);
  SET_SENSITIVE ("/Copy Path",                vectors);
  SET_SENSITIVE ("/Paste Path",               global_buf);
  SET_SENSITIVE ("/Import Path...",           TRUE);
  SET_SENSITIVE ("/Export Path...",           vectors);
  SET_SENSITIVE ("/Path Tool",                vectors);
  SET_SENSITIVE ("/Edit Path Attributes...",  vectors);

#undef SET_SENSITIVE
}
