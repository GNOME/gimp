/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectorstreeview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "vectors/gimpvectors.h"

#include "gimpcontainerview.h"
#include "gimpvectorstreeview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_vectors_tree_view_class_init (GimpVectorsTreeViewClass *klass);
static void   gimp_vectors_tree_view_init       (GimpVectorsTreeView      *view);

static void  gimp_vectors_tree_view_view_iface_init (GimpContainerViewInterface *view_iface);

static gboolean gimp_vectors_tree_view_select_item  (GimpContainerView   *view,
						     GimpViewable        *item,
						     gpointer             insert_data);
static void     gimp_vectors_tree_view_set_image    (GimpItemTreeView    *view,
                                                     GimpImage           *gimage);
static void   gimp_vectors_tree_view_toselection_clicked
                                                    (GtkWidget           *widget,
						     GimpVectorsTreeView *view);
static void   gimp_vectors_tree_view_toselection_extended_clicked
                                                    (GtkWidget           *widget,
						     guint                state,
						     GimpVectorsTreeView *view);

static void   gimp_vectors_tree_view_tovectors_clicked
                                                    (GtkWidget           *widget,
                                                     GimpVectorsTreeView *view);
static void   gimp_vectors_tree_view_tovectors_extended_clicked
                                                    (GtkWidget           *widget,
                                                     guint                state,
                                                     GimpVectorsTreeView *view);

static void   gimp_vectors_tree_view_stroke_clicked (GtkWidget           *widget,
						     GimpVectorsTreeView *view);


static GimpItemTreeViewClass      *parent_class      = NULL;
static GimpContainerViewInterface *parent_view_iface = NULL;


GType
gimp_vectors_tree_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpVectorsTreeViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_vectors_tree_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpVectorsTreeView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_vectors_tree_view_init,
      };

      static const GInterfaceInfo view_iface_info =
      {
        (GInterfaceInitFunc) gimp_vectors_tree_view_view_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      view_type = g_type_register_static (GIMP_TYPE_ITEM_TREE_VIEW,
                                          "GimpVectorsTreeView",
                                          &view_info, 0);

      g_type_add_interface_static (view_type, GIMP_TYPE_CONTAINER_VIEW,
                                   &view_iface_info);
    }

  return view_type;
}

static void
gimp_vectors_tree_view_class_init (GimpVectorsTreeViewClass *klass)
{
  GimpItemTreeViewClass *item_view_class = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  item_view_class->set_image       = gimp_vectors_tree_view_set_image;

  item_view_class->get_container   = gimp_image_get_vectors;
  item_view_class->get_active_item = (GimpGetItemFunc) gimp_image_get_active_vectors;
  item_view_class->set_active_item = (GimpSetItemFunc) gimp_image_set_active_vectors;
  item_view_class->reorder_item    = (GimpReorderItemFunc) gimp_image_position_vectors;
  item_view_class->add_item        = (GimpAddItemFunc) gimp_image_add_vectors;
  item_view_class->remove_item     = (GimpRemoveItemFunc) gimp_image_remove_vectors;

  item_view_class->edit_desc               = _("Edit Path Attributes");
  item_view_class->edit_help_id            = GIMP_HELP_PATH_EDIT;
  item_view_class->new_desc                = _("New Path\n%s New Path Dialog");
  item_view_class->new_help_id             = GIMP_HELP_PATH_NEW;
  item_view_class->duplicate_desc          = _("Duplicate Path");
  item_view_class->duplicate_help_id       = GIMP_HELP_PATH_DUPLICATE;
  item_view_class->delete_desc             = _("Delete Path");
  item_view_class->delete_help_id          = GIMP_HELP_PATH_DELETE;
  item_view_class->raise_desc              = _("Raise Path");
  item_view_class->raise_help_id           = GIMP_HELP_PATH_RAISE;
  item_view_class->raise_to_top_desc       = _("Raise Path to Top");
  item_view_class->raise_to_top_help_id    = GIMP_HELP_PATH_RAISE_TO_TOP;
  item_view_class->lower_desc              = _("Lower Path");
  item_view_class->lower_help_id           = GIMP_HELP_PATH_LOWER;
  item_view_class->lower_to_bottom_desc    = _("Lower Path to Bottom");
  item_view_class->lower_to_bottom_help_id = GIMP_HELP_PATH_LOWER_TO_BOTTOM;
  item_view_class->reorder_desc            = _("Reorder Path");
}

static void
gimp_vectors_tree_view_init (GimpVectorsTreeView *view)
{
  GimpEditor *editor = GIMP_EDITOR (view);
  gchar      *str;

  /*  Hide basically useless Edit button  */

  gtk_widget_hide (GIMP_ITEM_TREE_VIEW (view)->edit_button);

  str = g_strdup_printf (_("Path to Selection\n"
                           "%s  Add\n"
                           "%s  Subtract\n"
                           "%s%s%s  Intersect"),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_name_control (),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_separator (),
                         gimp_get_mod_name_control ());

  view->toselection_button =
    gimp_editor_add_button (editor,
                            GIMP_STOCK_SELECTION_REPLACE, str,
                            GIMP_HELP_PATH_SELECTION_REPLACE,
                            G_CALLBACK (gimp_vectors_tree_view_toselection_clicked),
                            G_CALLBACK (gimp_vectors_tree_view_toselection_extended_clicked),
                            view);

  g_free (str);

  str = g_strdup_printf (_("Selection to Path\n"
                           "%s  Advanced Options"),
                         gimp_get_mod_name_shift ());

  view->tovectors_button =
    gimp_editor_add_button (editor,
                            GIMP_STOCK_SELECTION_TO_PATH, str,
                            GIMP_HELP_SELECTION_TO_PATH,
                            G_CALLBACK (gimp_vectors_tree_view_tovectors_clicked),
                            G_CALLBACK (gimp_vectors_tree_view_tovectors_extended_clicked),
                            view);

  g_free (str);

  view->stroke_button =
    gimp_editor_add_button (editor,
                            GIMP_STOCK_PATH_STROKE, _("Stroke Path"),
                            GIMP_HELP_PATH_STROKE,
                            G_CALLBACK (gimp_vectors_tree_view_stroke_clicked),
                            NULL,
                            view);

  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->toselection_button, 5);
  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->tovectors_button, 6);
  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->stroke_button, 7);

  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
				  GTK_BUTTON (view->toselection_button),
				  GIMP_TYPE_VECTORS);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
				  GTK_BUTTON (view->stroke_button),
				  GIMP_TYPE_VECTORS);

  gtk_widget_set_sensitive (view->toselection_button, FALSE);
  gtk_widget_set_sensitive (view->tovectors_button,   FALSE);
  gtk_widget_set_sensitive (view->stroke_button,      FALSE);
}

static void
gimp_vectors_tree_view_view_iface_init (GimpContainerViewInterface *view_iface)
{
  parent_view_iface = g_type_interface_peek_parent (view_iface);

  view_iface->select_item = gimp_vectors_tree_view_select_item;
}


/*  GimpContainerView methods  */

static gboolean
gimp_vectors_tree_view_select_item (GimpContainerView *view,
				    GimpViewable      *item,
				    gpointer           insert_data)
{
  GimpVectorsTreeView *tree_view = GIMP_VECTORS_TREE_VIEW (view);
  gboolean             success;

  success = parent_view_iface->select_item (view, item, insert_data);

  gtk_widget_set_sensitive (tree_view->toselection_button, item != NULL);
  gtk_widget_set_sensitive (tree_view->stroke_button,      item != NULL);

  return success;
}

static void
gimp_vectors_tree_view_set_image (GimpItemTreeView *view,
                                  GimpImage        *gimage)
{
  GIMP_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (view, gimage);

  gtk_widget_set_sensitive (GIMP_VECTORS_TREE_VIEW (view)->tovectors_button,
                            gimage != NULL);
}

static void
gimp_vectors_tree_view_toselection_clicked (GtkWidget           *widget,
					    GimpVectorsTreeView *view)
{
  gimp_vectors_tree_view_toselection_extended_clicked (widget, 0, view);
}

static void
gimp_vectors_tree_view_toselection_extended_clicked (GtkWidget           *widget,
						     guint                state,
						     GimpVectorsTreeView *view)
{
  GimpImage *gimage;
  GimpItem  *item;

  gimage = GIMP_ITEM_TREE_VIEW (view)->gimage;

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (gimage);

  if (item)
    {
      GimpChannelOps operation = GIMP_CHANNEL_OP_REPLACE;

      if (state & GDK_SHIFT_MASK)
	{
	  if (state & GDK_CONTROL_MASK)
	    operation = GIMP_CHANNEL_OP_INTERSECT;
	  else
	    operation = GIMP_CHANNEL_OP_ADD;
	}
      else if (state & GDK_CONTROL_MASK)
	{
	  operation = GIMP_CHANNEL_OP_SUBTRACT;
	}

      gimp_channel_select_vectors (gimp_image_get_mask (gimage),
                                   _("Path to Selection"),
                                   GIMP_VECTORS (item),
                                   operation,
                                   TRUE,
                                   FALSE, 0, 0);
      gimp_image_flush (gimage);
    }
}

static void
gimp_vectors_tree_view_tovectors_clicked (GtkWidget           *widget,
                                          GimpVectorsTreeView *view)
{
  gimp_vectors_tree_view_tovectors_extended_clicked (widget, 0, view);
}

static void
gimp_vectors_tree_view_tovectors_extended_clicked (GtkWidget           *widget,
                                                   guint                state,
                                                   GimpVectorsTreeView *view)
{
  if (view->selection_to_vectors_func)
    view->selection_to_vectors_func (GIMP_ITEM_TREE_VIEW (view)->gimage,
                                     (state & GDK_SHIFT_MASK) != 0);
}

static void
gimp_vectors_tree_view_stroke_clicked (GtkWidget           *widget,
                                       GimpVectorsTreeView *view)
{
  GimpImage *gimage;
  GimpItem  *item;

  gimage = GIMP_ITEM_TREE_VIEW (view)->gimage;

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (gimage);

  if (item && view->stroke_item_func)
    view->stroke_item_func (item, GTK_WIDGET (view));
}
