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
static void     gimp_container_grid_view_clear_items  (GimpContainerView      *view);
static void gimp_container_grid_view_set_preview_size (GimpContainerView      *view);


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
  GtkWidget *scrolled_win;

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win), 
                                  GTK_POLICY_NEVER,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (grid_view), scrolled_win, TRUE, TRUE, 0);

  grid_view->wrap_box = gimp_constrained_hwrap_box_new (FALSE);
  gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (grid_view->wrap_box), 256);
  gtk_wrap_box_set_hspacing (GTK_WRAP_BOX (grid_view->wrap_box), 2);
  gtk_wrap_box_set_vspacing (GTK_WRAP_BOX (grid_view->wrap_box), 2);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
                                         grid_view->wrap_box);

  gtk_widget_set_style
    (grid_view->wrap_box->parent,
     GIMP_CONTAINER_GRID_VIEW_CLASS (GTK_OBJECT (grid_view)->klass)->white_style);

  gtk_container_set_focus_vadjustment
    (GTK_CONTAINER (grid_view->wrap_box),
    gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));

  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (scrolled_win)->vscrollbar,
                          GTK_CAN_FOCUS);

  gtk_widget_show (grid_view->wrap_box);
  gtk_widget_show (scrolled_win);
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
			      gint           preview_width,
			      gint           preview_height,
			      gint           min_items_x,
			      gint           min_items_y)
{
  GimpContainerGridView *grid_view;
  GimpContainerView     *view;

  g_return_val_if_fail (container != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (preview_width  > 0 && preview_width  <= 64, NULL);
  g_return_val_if_fail (preview_height > 0 && preview_height <= 64, NULL);
  g_return_val_if_fail (min_items_x > 0 && min_items_x <= 64, NULL);
  g_return_val_if_fail (min_items_y > 0 && min_items_y <= 64, NULL);

  grid_view = gtk_type_new (GIMP_TYPE_CONTAINER_GRID_VIEW);

  view = GIMP_CONTAINER_VIEW (grid_view);

  view->preview_width  = preview_width;
  view->preview_height = preview_height;

  gtk_widget_set_usize (grid_view->wrap_box->parent,
			(preview_width  + 2) * min_items_x - 2,
			(preview_height + 2) * min_items_y - 2);

  gimp_container_view_set_container (view, container);

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

  preview = gimp_preview_new (viewable,
			      view->preview_width,
			      view->preview_height,
			      TRUE, TRUE);
  gtk_wrap_box_pack (GTK_WRAP_BOX (grid_view->wrap_box), preview,
		     FALSE, FALSE, FALSE, FALSE);

  if (index != -1)
    gtk_wrap_box_reorder_child (GTK_WRAP_BOX (grid_view->wrap_box),
				preview, index);

  gtk_widget_show (preview);

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
      gtk_container_remove (GTK_CONTAINER (grid_view->wrap_box), preview);
    }
}

static void
gimp_container_grid_view_clear_items (GimpContainerView *view)
{
  GimpContainerGridView *grid_view;

  grid_view = GIMP_CONTAINER_GRID_VIEW (view);

  while (GTK_WRAP_BOX (grid_view->wrap_box)->children)
    gtk_container_remove (GTK_CONTAINER (grid_view->wrap_box),
			  GTK_WRAP_BOX (grid_view->wrap_box)->children->widget);
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
      gtk_preview_size (GTK_PREVIEW (child->widget),
			view->preview_width,
			view->preview_height);
    }

  gtk_widget_queue_resize (grid_view->wrap_box);
}
