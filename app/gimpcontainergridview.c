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

#include <stdio.h>

#include <gtk/gtk.h>

#include "apptypes.h"

#include "appenv.h"
#include "colormaps.h"
#include "gimpcontainer.h"
#include "gimpcontainergridview.h"
#include "gimpcontext.h"
#include "gimppreview.h"
#include "gimpconstrainedhwrapbox.h"


static void     gimp_container_grid_view_class_init   (GimpContainerGridViewClass *klass);
static void     gimp_container_grid_view_init         (GimpContainerGridView      *panel);
static void     gimp_container_grid_view_destroy      (GtkObject                  *object);

static gpointer gimp_container_grid_view_insert_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gint                    index);
static void     gimp_container_grid_view_remove_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);
static void     gimp_container_grid_view_select_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);
static void     gimp_container_grid_view_clear_items  (GimpContainerView      *view);
static void gimp_container_grid_view_set_preview_size (GimpContainerView      *view);
static void    gimp_container_grid_view_item_selected (GtkWidget              *widget,
						       gpointer                data);
static void   gimp_container_grid_view_highlight_item (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);


static GimpContainerViewClass *parent_class = NULL;


GtkType
gimp_container_grid_view_get_type (void)
{
  static guint grid_view_type = 0;

  if (! grid_view_type)
    {
      GtkTypeInfo grid_view_info =
      {
	"GimpContainerGridView",
	sizeof (GimpContainerGridView),
	sizeof (GimpContainerGridViewClass),
	(GtkClassInitFunc) gimp_container_grid_view_class_init,
	(GtkObjectInitFunc) gimp_container_grid_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      grid_view_type = gtk_type_unique (GIMP_TYPE_CONTAINER_VIEW,
					&grid_view_info);
    }

  return grid_view_type;
}

static void
gimp_container_grid_view_class_init (GimpContainerGridViewClass *klass)
{
  GtkObjectClass         *object_class;
  GimpContainerViewClass *container_view_class;

  object_class         = (GtkObjectClass *) klass;
  container_view_class = (GimpContainerViewClass *) klass;
  
  parent_class = gtk_type_class (GIMP_TYPE_CONTAINER_VIEW);

  object_class->destroy = gimp_container_grid_view_destroy;

  container_view_class->insert_item      = gimp_container_grid_view_insert_item;
  container_view_class->remove_item      = gimp_container_grid_view_remove_item;
  container_view_class->select_item      = gimp_container_grid_view_select_item;
  container_view_class->clear_items      = gimp_container_grid_view_clear_items;
  container_view_class->set_preview_size = gimp_container_grid_view_set_preview_size;

  klass->white_style = gtk_style_copy (gtk_widget_get_default_style ());
  klass->white_style->bg[GTK_STATE_NORMAL].red   = 0xffff;
  klass->white_style->bg[GTK_STATE_NORMAL].green = 0xffff;
  klass->white_style->bg[GTK_STATE_NORMAL].blue  = 0xffff;
  klass->white_style->bg[GTK_STATE_NORMAL].pixel = g_white_pixel;
}

static void
gimp_container_grid_view_init (GimpContainerGridView *grid_view)
{
  grid_view->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (grid_view->scrolled_win),
                                  GTK_POLICY_NEVER,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (grid_view), grid_view->scrolled_win,
		      TRUE, TRUE, 0);

  grid_view->wrap_box = gimp_constrained_hwrap_box_new (FALSE);

  gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (grid_view->wrap_box),
				 1.0 / 256.0);

  gtk_scrolled_window_add_with_viewport
    (GTK_SCROLLED_WINDOW (grid_view->scrolled_win),
     grid_view->wrap_box);

  gtk_widget_set_style
    (grid_view->wrap_box->parent,
     GIMP_CONTAINER_GRID_VIEW_CLASS (GTK_OBJECT (grid_view)->klass)->white_style);

  gtk_container_set_focus_vadjustment
    (GTK_CONTAINER (grid_view->wrap_box->parent),
    gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
					 (grid_view->scrolled_win)));

  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW
			  (grid_view->scrolled_win)->vscrollbar,
                          GTK_CAN_FOCUS);

  GTK_WIDGET_SET_FLAGS (grid_view->wrap_box->parent, GTK_CAN_FOCUS);

  gtk_widget_show (grid_view->wrap_box);
  gtk_widget_show (grid_view->scrolled_win);
}

static void
gimp_container_grid_view_destroy (GtkObject *object)
{
  GimpContainerGridView *grid_view;

  grid_view = GIMP_CONTAINER_GRID_VIEW (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_container_grid_view_new (GimpContainer *container,
			      GimpContext   *context,
			      gint           preview_width,
			      gint           preview_height,
			      gint           min_items_x,
			      gint           min_items_y)
{
  GimpContainerGridView *grid_view;
  GimpContainerView     *view;
  gint                   window_border;

  g_return_val_if_fail (container != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (! context || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (preview_width  > 0 && preview_width  <= 64, NULL);
  g_return_val_if_fail (preview_height > 0 && preview_height <= 64, NULL);
  g_return_val_if_fail (min_items_x > 0 && min_items_x <= 64, NULL);
  g_return_val_if_fail (min_items_y > 0 && min_items_y <= 64, NULL);

  grid_view = gtk_type_new (GIMP_TYPE_CONTAINER_GRID_VIEW);

  view = GIMP_CONTAINER_VIEW (grid_view);

  view->preview_width  = preview_width;
  view->preview_height = preview_height;

  window_border =
    GTK_SCROLLED_WINDOW (grid_view->scrolled_win)->vscrollbar->requisition.width +
    GTK_SCROLLED_WINDOW_CLASS (GTK_OBJECT (grid_view->scrolled_win)->klass)->scrollbar_spacing +
    grid_view->scrolled_win->style->klass->xthickness * 4;

  gtk_widget_set_usize (grid_view->scrolled_win,
			(preview_width  + 2) * min_items_x + window_border,
			(preview_height + 2) * min_items_y + window_border);

  gimp_container_view_set_container (view, container);

  gimp_container_view_set_context (view, context);

  return GTK_WIDGET (grid_view);
}

static gpointer
gimp_container_grid_view_insert_item (GimpContainerView *view,
				      GimpViewable      *viewable,
				      gint               index)
{
  GimpContainerGridView *grid_view;
  GtkWidget             *preview;

  grid_view = GIMP_CONTAINER_GRID_VIEW (view);

  preview = gimp_preview_new_full (viewable,
				   view->preview_width,
				   view->preview_height,
				   1,
				   FALSE, TRUE, TRUE);

  GIMP_PREVIEW (preview)->border_color[0] = 255;
  GIMP_PREVIEW (preview)->border_color[1] = 255;
  GIMP_PREVIEW (preview)->border_color[2] = 255;

  gtk_wrap_box_pack (GTK_WRAP_BOX (grid_view->wrap_box), preview,
		     FALSE, FALSE, FALSE, FALSE);

  if (index != -1)
    gtk_wrap_box_reorder_child (GTK_WRAP_BOX (grid_view->wrap_box),
				preview, index);

  gtk_widget_show (preview);

  gtk_signal_connect (GTK_OBJECT (preview), "clicked",
		      GTK_SIGNAL_FUNC (gimp_container_grid_view_item_selected),
		      view);

  return (gpointer) preview;
}

static void
gimp_container_grid_view_remove_item (GimpContainerView *view,
				      GimpViewable      *viewable,
				      gpointer           insert_data)
{
  GimpContainerGridView *grid_view;
  GtkWidget             *preview;

  grid_view = GIMP_CONTAINER_GRID_VIEW (view);
  preview   = GTK_WIDGET (insert_data);

  if (preview)
    {
      if (gtk_object_get_data (GTK_OBJECT (view),
			       "last_selected_item") == preview)
	{
	  gtk_object_set_data (GTK_OBJECT (view), "last_selected_item", NULL);
	}

      gtk_container_remove (GTK_CONTAINER (grid_view->wrap_box), preview);
    }
}

static void
gimp_container_grid_view_select_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  gimp_container_grid_view_highlight_item (view, viewable, insert_data);
}

static void
gimp_container_grid_view_clear_items (GimpContainerView *view)
{
  GimpContainerGridView *grid_view;

  grid_view = GIMP_CONTAINER_GRID_VIEW (view);

  gtk_object_set_data (GTK_OBJECT (view), "last_selected_item", NULL);

  while (GTK_WRAP_BOX (grid_view->wrap_box)->children)
    gtk_container_remove (GTK_CONTAINER (grid_view->wrap_box),
			  GTK_WRAP_BOX (grid_view->wrap_box)->children->widget);

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->clear_items)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->clear_items (view);
}

static void
gimp_container_grid_view_set_preview_size (GimpContainerView *view)
{
  GimpContainerGridView *grid_view;
  GtkWrapBoxChild       *child;

  grid_view = GIMP_CONTAINER_GRID_VIEW (view);

  for (child = GTK_WRAP_BOX (grid_view->wrap_box)->children;
       child;
       child = child->next)
    {
      GimpPreview *preview;

      preview = GIMP_PREVIEW (child->widget);

      gimp_preview_set_size (preview,
			     view->preview_width,
			     view->preview_height);
    }

  gtk_widget_queue_resize (grid_view->wrap_box);
}

static void
gimp_container_grid_view_item_selected (GtkWidget *widget,
					gpointer   data)
{
  gimp_container_grid_view_highlight_item (GIMP_CONTAINER_VIEW (data),
					   GIMP_PREVIEW (widget)->viewable,
					   widget);

  gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (data),
				     GIMP_PREVIEW (widget)->viewable);
}

static void
gimp_container_grid_view_highlight_item (GimpContainerView *view,
					 GimpViewable      *viewable,
					 gpointer           insert_data)
{
  GimpContainerGridView *grid_view;
  GimpPreview           *preview;

  grid_view = GIMP_CONTAINER_GRID_VIEW (view);

  preview = gtk_object_get_data (GTK_OBJECT (view), "last_selected_item");

  if (preview)
    {
      preview->border_color[0] = 255;
      preview->border_color[1] = 255;
      preview->border_color[2] = 255;

      gtk_signal_emit_by_name (GTK_OBJECT (preview), "render");
    }

  if (insert_data)
    preview = GIMP_PREVIEW (insert_data);
  else
    preview = NULL;

  if (preview)
    {
      GtkAdjustment *adj;
      gint           item_height;
      gint           index;
      gint           row;

      adj = gtk_scrolled_window_get_vadjustment
	(GTK_SCROLLED_WINDOW (grid_view->scrolled_win));

      item_height = GTK_WIDGET (preview)->allocation.height;

      index = gimp_container_get_child_index (view->container,
                                              GIMP_OBJECT (viewable));

      row = index / GIMP_CONSTRAINED_HWRAP_BOX (grid_view->wrap_box)->columns;

      if (row * item_height < adj->value)
        {
          gtk_adjustment_set_value (adj, row * item_height);
        }
      else if ((row + 1) * item_height > adj->value + adj->page_size)
        {
          gtk_adjustment_set_value (adj,
                                    (row + 1) * item_height - adj->page_size);
        }

      preview->border_color[0] = 0;
      preview->border_color[1] = 0;
      preview->border_color[2] = 0;

      gtk_signal_emit_by_name (GTK_OBJECT (preview), "render");
    }

  gtk_object_set_data (GTK_OBJECT (view), "last_selected_item", preview);
}
