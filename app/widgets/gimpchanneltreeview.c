/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpchannellistview.c
 * Copyright (C) 2001 Michael Natterer
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

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpmarshal.h"

#include "gimpchannellistview.h"
#include "gimpcomponentlistitem.h"
#include "gimpdnd.h"
#include "gimpimagepreview.h"
#include "gimplistitem.h"

#include "gdisplay.h"

#include "libgimp/gimpintl.h"


static void   gimp_channel_list_view_class_init (GimpChannelListViewClass *klass);
static void   gimp_channel_list_view_init       (GimpChannelListView      *view);

static void   gimp_channel_list_view_destroy    (GtkObject                *object);

static void   gimp_channel_list_view_set_image      (GimpDrawableListView *view,
						     GimpImage            *gimage);

static void   gimp_channel_list_view_select_item    (GimpContainerView   *view,
						     GimpViewable        *item,
						     gpointer             insert_data);
static void   gimp_channel_list_view_set_preview_size (GimpContainerView *view);

static void   gimp_channel_list_view_to_selection   (GimpChannelListView *view,
						     GimpChannel         *channel,
						     ChannelOps           operation);
static void   gimp_channel_list_view_toselection_clicked
                                                    (GtkWidget           *widget,
						     GimpChannelListView *view);
static void   gimp_channel_list_view_toselection_extended_clicked
                                                    (GtkWidget           *widget,
						     guint                state,
						     GimpChannelListView *view);

static void   gimp_channel_list_view_create_components
                                                    (GimpChannelListView *view);
static void   gimp_channel_list_view_clear_components
                                                    (GimpChannelListView *view);

static void   gimp_channel_list_view_mode_changed   (GimpImage           *gimage,
						     GimpChannelListView *view);

static void   gimp_channel_list_view_component_toggle
                                                    (GtkList             *list,
						     GtkWidget           *child,
						     GimpChannelListView *view);


static GimpDrawableListViewClass *parent_class = NULL;


GtkType
gimp_channel_list_view_get_type (void)
{
  static GtkType view_type = 0;

  if (! view_type)
    {
      GtkTypeInfo view_info =
      {
	"GimpChannelListView",
	sizeof (GimpChannelListView),
	sizeof (GimpChannelListViewClass),
	(GtkClassInitFunc) gimp_channel_list_view_class_init,
	(GtkObjectInitFunc) gimp_channel_list_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      view_type = gtk_type_unique (GIMP_TYPE_DRAWABLE_LIST_VIEW, &view_info);
    }

  return view_type;
}

static void
gimp_channel_list_view_class_init (GimpChannelListViewClass *klass)
{
  GtkObjectClass            *object_class;
  GimpContainerViewClass    *container_view_class;
  GimpDrawableListViewClass *drawable_view_class;

  object_class         = (GtkObjectClass *) klass;
  container_view_class = (GimpContainerViewClass *) klass;
  drawable_view_class  = (GimpDrawableListViewClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy                  = gimp_channel_list_view_destroy;

  container_view_class->select_item      = gimp_channel_list_view_select_item;
  container_view_class->set_preview_size = gimp_channel_list_view_set_preview_size;

  drawable_view_class->set_image         = gimp_channel_list_view_set_image;
}

static void
gimp_channel_list_view_init (GimpChannelListView *view)
{
  GimpDrawableListView *drawable_view;
  GimpContainerView    *container_view;

  drawable_view  = GIMP_DRAWABLE_LIST_VIEW (view);
  container_view = GIMP_CONTAINER_VIEW (view);

  /*  component frame  */

  view->component_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (view->component_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (view), view->component_frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (view), view->component_frame, 0);
  /* don't show */

  view->component_list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (view->component_list),
			       GTK_SELECTION_MULTIPLE);
  gtk_container_add (GTK_CONTAINER (view->component_frame),
		     view->component_list);
  gtk_widget_show (view->component_list);

  g_signal_connect (G_OBJECT (view->component_list), "select_child",
		    G_CALLBACK (gimp_channel_list_view_component_toggle),
		    view);
  g_signal_connect (G_OBJECT (view->component_list), "unselect_child",
		    G_CALLBACK (gimp_channel_list_view_component_toggle),
		    view);

  /*  To Selection button  */

  view->toselection_button =
    gimp_container_view_add_button (container_view,
				    GIMP_STOCK_TO_SELECTION,
				    _("Channel to Selection\n"
				      "<Shift> Add\n"
				      "<Ctrl> Subtract\n"
				      "<Shift><Ctrl> Intersect"), NULL,
				    G_CALLBACK (gimp_channel_list_view_toselection_clicked),
				    G_CALLBACK (gimp_channel_list_view_toselection_extended_clicked),
				    view);

  gimp_container_view_enable_dnd (container_view,
				  GTK_BUTTON (view->toselection_button),
				  GIMP_TYPE_CHANNEL);

  gtk_widget_set_sensitive (view->toselection_button, FALSE);
}

static void
gimp_channel_list_view_destroy (GtkObject *object)
{
  GimpChannelListView *view;

  view = GIMP_CHANNEL_LIST_VIEW (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


/*  GimpChannelListView methods  */

static void
gimp_channel_list_view_set_image (GimpDrawableListView *view,
				  GimpImage            *gimage)
{
  GimpChannelListView *channel_view;

  channel_view = GIMP_CHANNEL_LIST_VIEW (view);

  if (view->gimage)
    {
      if (! gimage)
	gtk_widget_hide (channel_view->component_frame);

      gimp_channel_list_view_clear_components (channel_view);

      g_signal_handlers_disconnect_by_func (G_OBJECT (view->gimage),
					    gimp_channel_list_view_mode_changed,
					    channel_view);
    }

  if (GIMP_DRAWABLE_LIST_VIEW_CLASS (parent_class)->set_image)
    GIMP_DRAWABLE_LIST_VIEW_CLASS (parent_class)->set_image (view, gimage);

  if (view->gimage)
    {
      if (! GTK_WIDGET_VISIBLE (channel_view->component_frame))
	gtk_widget_show (channel_view->component_frame);

      gimp_channel_list_view_create_components (channel_view);

      g_signal_connect (G_OBJECT (view->gimage), "mode_changed",
			G_CALLBACK (gimp_channel_list_view_mode_changed),
			channel_view);
    }
}


/*  GimpContainerView methods  */

static void
gimp_channel_list_view_select_item (GimpContainerView *view,
				    GimpViewable      *item,
				    gpointer           insert_data)
{
  GimpDrawableListView *drawable_view;
  GimpChannelListView  *list_view;

  drawable_view = GIMP_DRAWABLE_LIST_VIEW (view);
  list_view     = GIMP_CHANNEL_LIST_VIEW (view);

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item (view,
							   item,
							   insert_data);

  if (drawable_view->gimage)
    {
      gboolean floating_sel;

      floating_sel = (gimp_image_floating_sel (drawable_view->gimage) != NULL);

      gtk_widget_set_sensitive (view->button_box, !floating_sel);
    }

  gtk_widget_set_sensitive (list_view->toselection_button, item != NULL);
}

static void
gimp_channel_list_view_set_preview_size (GimpContainerView *view)
{
  GimpChannelListView *channel_view;
  GList               *list;

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->set_preview_size)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->set_preview_size (view);

  channel_view = GIMP_CHANNEL_LIST_VIEW (view);

  for (list = GTK_LIST (channel_view->component_list)->children;
       list;
       list = g_list_next (list))
    {
      gimp_list_item_set_preview_size (GIMP_LIST_ITEM (list->data),
				       view->preview_size);
    }

  gtk_widget_queue_resize (channel_view->component_frame);
}


/*  "To Selection" functions  */

static void
gimp_channel_list_view_to_selection (GimpChannelListView *view,
				     GimpChannel         *channel,
				     ChannelOps           operation)
{
  if (channel)
    {
      GimpImage   *gimage;
      GimpChannel *new_channel;

      gimage = gimp_drawable_gimage (GIMP_DRAWABLE (channel));

      if (operation == CHANNEL_OP_REPLACE)
	{
	  new_channel = channel;

	  g_object_ref (G_OBJECT (channel));
	}
      else
	{
	  new_channel = gimp_channel_copy (gimp_image_get_mask (gimage), TRUE);

	  gimp_channel_combine_mask (new_channel,
				     channel,
				     operation,
				     0, 0);
	}

      gimage_mask_load (gimage, new_channel);

      g_object_unref (G_OBJECT (new_channel));

      gdisplays_flush ();
    }
}

static void
gimp_channel_list_view_toselection_clicked (GtkWidget           *widget,
					    GimpChannelListView *view)
{
  GimpDrawableListView *drawable_view;
  GimpDrawable         *drawable;

  drawable_view = GIMP_DRAWABLE_LIST_VIEW (view);

  drawable = drawable_view->get_drawable_func (drawable_view->gimage);

  if (drawable)
    gimp_channel_list_view_to_selection (view, GIMP_CHANNEL (drawable),
					 CHANNEL_OP_REPLACE);
}

static void
gimp_channel_list_view_toselection_extended_clicked (GtkWidget           *widget,
						     guint                state,
						     GimpChannelListView *view)
{
  GimpDrawableListView *drawable_view;
  GimpDrawable         *drawable;

  drawable_view = GIMP_DRAWABLE_LIST_VIEW (view);

  drawable = drawable_view->get_drawable_func (drawable_view->gimage);

  if (drawable)
    {
      ChannelOps operation = CHANNEL_OP_REPLACE;

      if (state & GDK_SHIFT_MASK)
	{
	  if (state & GDK_CONTROL_MASK)
	    operation = CHANNEL_OP_INTERSECT;
	  else
	    operation = CHANNEL_OP_ADD;
	}
      else if (state & GDK_CONTROL_MASK)
	{
	  operation = CHANNEL_OP_SUB;
	}

      gimp_channel_list_view_to_selection (view, GIMP_CHANNEL (drawable),
					   operation);
    }
}

static void
gimp_channel_list_view_create_components (GimpChannelListView *view)
{
  GimpImage   *gimage;
  GtkWidget   *list_item;
  gint         n_components = 0;
  ChannelType  components[MAX_CHANNELS];
  GList       *list = NULL;
  gint         i;

  gimage = GIMP_DRAWABLE_LIST_VIEW (view)->gimage;

  switch (gimp_image_base_type (gimage))
    {
    case RGB:
      n_components  = 3;
      components[0] = RED_CHANNEL;
      components[1] = GREEN_CHANNEL;
      components[2] = BLUE_CHANNEL;
      break;

    case GRAY:
      n_components  = 1;
      components[0] = GRAY_CHANNEL;
      break;

    case INDEXED:
      n_components  = 1;
      components[0] = INDEXED_CHANNEL;
      break;
    }

  components[n_components++] = ALPHA_CHANNEL;

  for (i = 0; i < n_components; i++)
    {
      list_item =
	gimp_component_list_item_new (gimage,
				      GIMP_CONTAINER_VIEW (view)->preview_size,
				      components[i]);

      gtk_widget_show (list_item);

      list = g_list_append (list, list_item);
    }

  gtk_list_insert_items (GTK_LIST (view->component_list), list, 0);

  gtk_widget_queue_resize (GTK_WIDGET (view->component_frame));
}

static void
gimp_channel_list_view_clear_components (GimpChannelListView *view)
{
  g_signal_handlers_block_by_func (G_OBJECT (view->component_list),
				   gimp_channel_list_view_component_toggle,
				   view);

  gtk_list_clear_items (GTK_LIST (view->component_list), 0, -1);

  g_signal_handlers_unblock_by_func (G_OBJECT (view->component_list),
				     gimp_channel_list_view_component_toggle,
				     view);
}

static void
gimp_channel_list_view_mode_changed (GimpImage           *gimage,
				     GimpChannelListView *view)
{
  gimp_channel_list_view_clear_components (view);
  gimp_channel_list_view_create_components (view);
}

static void
gimp_channel_list_view_component_toggle (GtkList             *list,
					 GtkWidget           *child,
					 GimpChannelListView *view)
{
  GimpComponentListItem *component_item;

  component_item = GIMP_COMPONENT_LIST_ITEM (child);

  g_signal_handlers_block_by_func (G_OBJECT (view->component_list),
				   gimp_channel_list_view_component_toggle,
				   view);

  gimp_image_set_component_active (GIMP_DRAWABLE_LIST_VIEW (view)->gimage,
				   component_item->channel,
				   child->state == GTK_STATE_SELECTED);

  g_signal_handlers_unblock_by_func (G_OBJECT (view->component_list),
				     gimp_channel_list_view_component_toggle,
				     view);
}
