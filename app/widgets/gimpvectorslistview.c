/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectorslistview.c
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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask-select.h"

#include "vectors/gimpvectors.h"

#include "display/gimpdisplay-foreach.h"

#include "gimpvectorslistview.h"
#include "gimpcomponentlistitem.h"
#include "gimpdnd.h"
#include "gimpimagepreview.h"
#include "gimplistitem.h"

#include "libgimp/gimpintl.h"


static void   gimp_vectors_list_view_class_init (GimpVectorsListViewClass *klass);
static void   gimp_vectors_list_view_init       (GimpVectorsListView      *view);

static void   gimp_vectors_list_view_select_item    (GimpContainerView   *view,
						     GimpViewable        *item,
						     gpointer             insert_data);
static void   gimp_vectors_list_view_to_selection   (GimpVectorsListView *view,
						     GimpVectors         *vectors,
						     GimpChannelOps       operation);
static void   gimp_vectors_list_view_toselection_clicked
                                                    (GtkWidget           *widget,
						     GimpVectorsListView *view);
static void   gimp_vectors_list_view_toselection_extended_clicked
                                                    (GtkWidget           *widget,
						     guint                state,
						     GimpVectorsListView *view);

static void   gimp_vectors_list_view_stroke         (GimpVectorsListView *view,
						     GimpVectors         *vectors);
static void   gimp_vectors_list_view_stroke_clicked (GtkWidget           *widget,
						     GimpVectorsListView *view);


static GimpItemListViewClass *parent_class = NULL;


GType
gimp_vectors_list_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpVectorsListViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_vectors_list_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpVectorsListView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_vectors_list_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_ITEM_LIST_VIEW,
                                          "GimpVectorsListView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_vectors_list_view_class_init (GimpVectorsListViewClass *klass)
{
  GimpContainerViewClass *container_view_class;

  container_view_class = GIMP_CONTAINER_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  container_view_class->select_item = gimp_vectors_list_view_select_item;
}

static void
gimp_vectors_list_view_init (GimpVectorsListView *view)
{
  GimpEditor *editor;

  editor = GIMP_EDITOR (view);

  /*  To Selection button  */

  view->toselection_button =
    gimp_editor_add_button (editor,
                            GIMP_STOCK_SELECTION_REPLACE,
                            _("Path to Selection\n"
                              "<Shift> Add\n"
                              "<Ctrl> Subtract\n"
                              "<Shift><Ctrl> Intersect"), NULL,
                            G_CALLBACK (gimp_vectors_list_view_toselection_clicked),
                            G_CALLBACK (gimp_vectors_list_view_toselection_extended_clicked),
                            view);

  view->stroke_button =
    gimp_editor_add_button (editor,
                            GIMP_STOCK_PATH_STROKE,
                            _("Stroke Path"), NULL,
                            G_CALLBACK (gimp_vectors_list_view_stroke_clicked),
                            NULL,
                            view);

  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->toselection_button, 5);
  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->stroke_button, 6);

  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
				  GTK_BUTTON (view->toselection_button),
				  GIMP_TYPE_VECTORS);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
				  GTK_BUTTON (view->stroke_button),
				  GIMP_TYPE_VECTORS);

  gtk_widget_set_sensitive (view->toselection_button, FALSE);
  gtk_widget_set_sensitive (view->stroke_button,      FALSE);
}


/*  GimpContainerView methods  */

static void
gimp_vectors_list_view_select_item (GimpContainerView *view,
				    GimpViewable      *item,
				    gpointer           insert_data)
{
  GimpVectorsListView *list_view;

  list_view = GIMP_VECTORS_LIST_VIEW (view);

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item (view,
							   item,
							   insert_data);

  gtk_widget_set_sensitive (list_view->toselection_button, item != NULL);
  gtk_widget_set_sensitive (list_view->stroke_button,      item != NULL);
}


/*  "To Selection" functions  */

static void
gimp_vectors_list_view_to_selection (GimpVectorsListView *view,
				     GimpVectors         *vectors,
				     GimpChannelOps       operation)
{
  if (vectors)
    {
      gimp_image_mask_select_vectors (GIMP_ITEM (vectors)->gimage,
                                      vectors,
                                      operation,
                                      TRUE,
                                      FALSE, 0, 0);

      gdisplays_flush ();
    }
}

static void
gimp_vectors_list_view_toselection_clicked (GtkWidget           *widget,
					    GimpVectorsListView *view)
{
  gimp_vectors_list_view_toselection_extended_clicked (widget, 0, view);
}

static void
gimp_vectors_list_view_toselection_extended_clicked (GtkWidget           *widget,
						     guint                state,
						     GimpVectorsListView *view)
{
  GimpItemListView *item_view;
  GimpViewable     *viewable;

  item_view = GIMP_ITEM_LIST_VIEW (view);

  viewable = item_view->get_item_func (item_view->gimage);

  if (viewable)
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

      gimp_vectors_list_view_to_selection (view, GIMP_VECTORS (viewable),
					   operation);
    }
}

static void
gimp_vectors_list_view_stroke (GimpVectorsListView *view,
                               GimpVectors         *vectors)
{
  if (view->stroke_item_func)
    {
      view->stroke_item_func (vectors);
    }
}

static void
gimp_vectors_list_view_stroke_clicked (GtkWidget           *widget,
                                       GimpVectorsListView *view)
{
  GimpItemListView *item_view;
  GimpViewable     *viewable;

  item_view = GIMP_ITEM_LIST_VIEW (view);

  viewable = item_view->get_item_func (item_view->gimage);

  if (viewable)
    gimp_vectors_list_view_stroke (view, GIMP_VECTORS (viewable));
}
