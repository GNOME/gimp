/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainergridview.c
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

#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"

#include "gimpcontainergridview.h"
#include "gimppreview.h"
#include "gimpconstrainedhwrapbox.h"

#include "libgimp/gimpintl.h"


static void     gimp_container_grid_view_class_init   (GimpContainerGridViewClass *klass);
static void     gimp_container_grid_view_init         (GimpContainerGridView      *panel);

static gpointer gimp_container_grid_view_insert_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gint                    index);
static void     gimp_container_grid_view_remove_item  (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);
static void     gimp_container_grid_view_reorder_item (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gint                    new_index,
						       gpointer                insert_data);
static void     gimp_container_grid_view_select_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);
static void     gimp_container_grid_view_clear_items  (GimpContainerView      *view);
static void gimp_container_grid_view_set_preview_size (GimpContainerView      *view);
static void    gimp_container_grid_view_item_selected (GtkWidget              *widget,
						       gpointer                data);
static void   gimp_container_grid_view_item_activated (GtkWidget              *widget,
						       gpointer                data);
static void   gimp_container_grid_view_item_context   (GtkWidget              *widget,
						       gpointer                data);
static void   gimp_container_grid_view_highlight_item (GimpContainerView      *view,
						       GimpViewable           *viewable,
						       gpointer                insert_data);


static GimpContainerViewClass *parent_class = NULL;

static GimpRGB  white_color;
static GimpRGB  black_color;


GType
gimp_container_grid_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpContainerGridViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_container_grid_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpContainerGridView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_container_grid_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_VIEW,
                                          "GimpContainerGridView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_container_grid_view_class_init (GimpContainerGridViewClass *klass)
{
  GimpContainerViewClass *container_view_class;

  container_view_class = GIMP_CONTAINER_VIEW_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  container_view_class->insert_item      = gimp_container_grid_view_insert_item;
  container_view_class->remove_item      = gimp_container_grid_view_remove_item;
  container_view_class->reorder_item     = gimp_container_grid_view_reorder_item;
  container_view_class->select_item      = gimp_container_grid_view_select_item;
  container_view_class->clear_items      = gimp_container_grid_view_clear_items;
  container_view_class->set_preview_size = gimp_container_grid_view_set_preview_size;

  gimp_rgba_set (&white_color, 1.0, 1.0, 1.0, 1.0);
  gimp_rgba_set (&black_color, 0.0, 0.0, 0.0, 1.0);
}

static void
gimp_container_grid_view_init (GimpContainerGridView *grid_view)
{
  grid_view->name_label = gtk_label_new (_("(None)"));
  gtk_misc_set_alignment (GTK_MISC (grid_view->name_label), 0.0, 0.5);
  gtk_misc_set_padding (GTK_MISC (grid_view->name_label),
			grid_view->name_label->style->xthickness, 0);
  gtk_box_pack_start (GTK_BOX (grid_view), grid_view->name_label,
		      FALSE, FALSE, 0);
  gtk_widget_show (grid_view->name_label);

  grid_view->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (grid_view->scrolled_win),
                                  GTK_POLICY_NEVER,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (grid_view), grid_view->scrolled_win,
		      TRUE, TRUE, 0);
  gtk_widget_show (grid_view->scrolled_win);

  GIMP_CONTAINER_VIEW (grid_view)->dnd_widget = grid_view->scrolled_win;

  grid_view->wrap_box = gimp_constrained_hwrap_box_new (FALSE);
  gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (grid_view->wrap_box),
				 1.0 / 256.0);
  gtk_scrolled_window_add_with_viewport
    (GTK_SCROLLED_WINDOW (grid_view->scrolled_win),
     grid_view->wrap_box);
  gtk_widget_show (grid_view->wrap_box);

  gtk_container_set_focus_vadjustment
    (GTK_CONTAINER (grid_view->wrap_box->parent),
    gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
					 (grid_view->scrolled_win)));

  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW
			  (grid_view->scrolled_win)->vscrollbar,
                          GTK_CAN_FOCUS);

  GTK_WIDGET_SET_FLAGS (grid_view->wrap_box->parent, GTK_CAN_FOCUS);
}

GtkWidget *
gimp_container_grid_view_new (GimpContainer *container,
			      GimpContext   *context,
			      gint           preview_size,
                              gboolean       reorderable,
			      gint           min_items_x,
			      gint           min_items_y)
{
  GimpContainerGridView *grid_view;
  GimpContainerView     *view;
  gint                   window_border;

  g_return_val_if_fail (! container || GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (! context || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (preview_size  > 0 && preview_size  <= 64, NULL);
  g_return_val_if_fail (min_items_x > 0 && min_items_x <= 64, NULL);
  g_return_val_if_fail (min_items_y > 0 && min_items_y <= 64, NULL);

  grid_view = g_object_new (GIMP_TYPE_CONTAINER_GRID_VIEW, NULL);

  view = GIMP_CONTAINER_VIEW (grid_view);

  view->preview_size = preview_size;
  view->reorderable  = reorderable ? TRUE : FALSE;

  window_border =
    GTK_SCROLLED_WINDOW (grid_view->scrolled_win)->vscrollbar->requisition.width +
    GTK_SCROLLED_WINDOW_GET_CLASS (grid_view->scrolled_win)->scrollbar_spacing +
    grid_view->scrolled_win->style->xthickness * 4;

  gtk_widget_set_size_request (grid_view->scrolled_win,
                               (preview_size + 2) * min_items_x + window_border,
                               (preview_size + 2) * min_items_y + window_border);

  if (container)
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
				   view->preview_size,
				   view->preview_size,
				   1,
				   FALSE, TRUE, TRUE);

  GIMP_PREVIEW (preview)->border_color = white_color;

  gtk_wrap_box_pack (GTK_WRAP_BOX (grid_view->wrap_box), preview,
		     FALSE, FALSE, FALSE, FALSE);

  if (index != -1)
    gtk_wrap_box_reorder_child (GTK_WRAP_BOX (grid_view->wrap_box),
				preview, index);

  gtk_widget_show (preview);

  g_signal_connect (G_OBJECT (preview), "clicked",
                    G_CALLBACK (gimp_container_grid_view_item_selected),
                    view);

  g_signal_connect (G_OBJECT (preview), "double_clicked",
                    G_CALLBACK (gimp_container_grid_view_item_activated),
                    view);

  g_signal_connect (G_OBJECT (preview), "context",
                    G_CALLBACK (gimp_container_grid_view_item_context),
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
      if (g_object_get_data (G_OBJECT (view), "last_selected_item") == preview)
	{
	  g_object_set_data (G_OBJECT (view), "last_selected_item", NULL);
	}

      gtk_container_remove (GTK_CONTAINER (grid_view->wrap_box), preview);
    }
}

static void
gimp_container_grid_view_reorder_item (GimpContainerView *view,
				       GimpViewable      *viewable,
				       gint               new_index,
				       gpointer           insert_data)
{
  GimpContainerGridView *grid_view;
  GtkWidget             *preview;

  grid_view = GIMP_CONTAINER_GRID_VIEW (view);
  preview   = GTK_WIDGET (insert_data);

  gtk_wrap_box_reorder_child (GTK_WRAP_BOX (grid_view->wrap_box),
			      preview, new_index);
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

  g_object_set_data (G_OBJECT (view), "last_selected_item", NULL);

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

      gimp_preview_set_size (preview, view->preview_size, preview->border_width);
    }

  gtk_widget_queue_resize (grid_view->wrap_box);
}

static void
gimp_container_grid_view_item_selected (GtkWidget *widget,
					gpointer   data)
{
  gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (data),
				     GIMP_PREVIEW (widget)->viewable);
}

static void
gimp_container_grid_view_item_activated (GtkWidget *widget,
					 gpointer   data)
{
  gimp_container_view_item_activated (GIMP_CONTAINER_VIEW (data),
				      GIMP_PREVIEW (widget)->viewable);
}

static void
gimp_container_grid_view_item_context (GtkWidget *widget,
				       gpointer   data)
{
  gimp_container_view_item_selected (GIMP_CONTAINER_VIEW (data),
				     GIMP_PREVIEW (widget)->viewable);
  gimp_container_view_item_context (GIMP_CONTAINER_VIEW (data),
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

  preview = g_object_get_data (G_OBJECT (view), "last_selected_item");

  if (preview)
    gimp_preview_set_border_color (preview, &white_color);

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

      gimp_preview_set_border_color (preview, &black_color);

      if (view->get_name_func)
	{
	  gchar *name;

	  name = view->get_name_func (GTK_WIDGET (preview), NULL);

	  gtk_label_set_text (GTK_LABEL (grid_view->name_label), name);

	  g_free (name);
	}
      else
	{
	  gtk_label_set_text (GTK_LABEL (grid_view->name_label),
			      GIMP_OBJECT (viewable)->name);
	}
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (grid_view->name_label), _("(None)"));
    }

  g_object_set_data (G_OBJECT (view), "last_selected_item", preview);
}
