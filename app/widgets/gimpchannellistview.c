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

#include "apptypes.h"

#include "drawable.h"
#include "gdisplay.h"
#include "gimpcontainer.h"
#include "gimpchannel.h"
#include "gimpdnd.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimprc.h"
#include "gimpviewable.h"

#include "gimpchannellistview.h"
#include "gimpcomponentlistitem.h"
#include "gimpimagepreview.h"
#include "gimplistitem.h"

#include "tools/paint_options.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/toselection.xpm"


static void   gimp_channel_list_view_class_init (GimpChannelListViewClass *klass);
static void   gimp_channel_list_view_init       (GimpChannelListView      *view);

static void   gimp_channel_list_view_destroy    (GtkObject                *object);

static void   gimp_channel_list_view_set_image      (GimpDrawableListView *view,
						     GimpImage            *gimage);

static void   gimp_channel_list_view_select_item    (GimpContainerView *view,
						     GimpViewable      *item,
						     gpointer           insert_data);

static void   gimp_channel_list_view_to_selection   (GimpChannelListView *view,
						     GimpChannel         *channel);
static void   gimp_channel_list_view_toselection_clicked
                                                    (GtkWidget           *widget,
						     GimpChannelListView *view);
static void   gimp_channel_list_view_toselection_dropped
                                                    (GtkWidget           *widget,
						     GimpViewable        *viewable,
						     gpointer             data);

static void   gimp_channel_list_view_create_components (GimpChannelListView *view);


static GimpDrawableListViewClass *parent_class = NULL;


GtkType
gimp_channel_list_view_get_type (void)
{
  static guint view_type = 0;

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

  parent_class = gtk_type_class (GIMP_TYPE_DRAWABLE_LIST_VIEW);

  object_class->destroy               = gimp_channel_list_view_destroy;

  container_view_class->select_item   = gimp_channel_list_view_select_item;

  drawable_view_class->set_image      = gimp_channel_list_view_set_image;
}

static void
gimp_channel_list_view_init (GimpChannelListView *view)
{
  GimpDrawableListView *drawable_view;
  GtkWidget            *pixmap;

  drawable_view = GIMP_DRAWABLE_LIST_VIEW (view);

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

  /*  To Selection button  */

  view->toselection_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (drawable_view->button_box),
		      view->toselection_button, TRUE, TRUE, 0);
  gtk_box_reorder_child (GTK_BOX (drawable_view->button_box),
			 view->toselection_button, 5);
  gtk_widget_show (view->toselection_button);

  gimp_help_set_help_data (view->toselection_button,
			   _("Channel to selection"), NULL);

  gtk_signal_connect (GTK_OBJECT (view->toselection_button), "clicked",
		      GTK_SIGNAL_FUNC (gimp_channel_list_view_toselection_clicked),
		      view);

  pixmap = gimp_pixmap_new (toselection_xpm);
  gtk_container_add (GTK_CONTAINER (view->toselection_button), pixmap);
  gtk_widget_show (pixmap);

  gimp_gtk_drag_dest_set_by_type (GTK_WIDGET (view->toselection_button),
				  GTK_DEST_DEFAULT_ALL,
				  GIMP_TYPE_CHANNEL,
				  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (view->toselection_button),
			      GIMP_TYPE_CHANNEL,
			      gimp_channel_list_view_toselection_dropped,
			      view);

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

      gtk_list_clear_items (GTK_LIST (channel_view->component_list), 0, -1);
    }

  if (GIMP_DRAWABLE_LIST_VIEW_CLASS (parent_class)->set_image)
    GIMP_DRAWABLE_LIST_VIEW_CLASS (parent_class)->set_image (view, gimage);

  if (view->gimage)
    {
      if (! GTK_WIDGET_VISIBLE (channel_view->component_frame))
	gtk_widget_show (channel_view->component_frame);

      gimp_channel_list_view_create_components (channel_view);
    }
}


/*  GimpContainerView methods  */

static void
gimp_channel_list_view_select_item (GimpContainerView *view,
				    GimpViewable      *item,
				    gpointer           insert_data)
{
  GimpChannelListView *list_view;
  gboolean             toselection_sensitive  = FALSE;

  list_view = GIMP_CHANNEL_LIST_VIEW (view);

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item (view,
							   item,
							   insert_data);

  if (item)
    {
      toselection_sensitive = TRUE;
    }

  gtk_widget_set_sensitive (list_view->toselection_button, toselection_sensitive);
}


/*  "To Selection" functions  */

static void
gimp_channel_list_view_to_selection (GimpChannelListView *view,
				     GimpChannel         *channel)
{
  if (channel)
    g_print ("to selection \"%s\"\n", GIMP_OBJECT (channel)->name);
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
    gimp_channel_list_view_to_selection (view, GIMP_CHANNEL (drawable));
}

static void
gimp_channel_list_view_toselection_dropped (GtkWidget    *widget,
					    GimpViewable *viewable,
					    gpointer      data)
{
  GimpChannelListView *view;

  view = (GimpChannelListView *) data;

  if (viewable && gimp_container_have (GIMP_CONTAINER_VIEW (view)->container,
				       GIMP_OBJECT (viewable)))
    {
      gimp_channel_list_view_to_selection (view, GIMP_CHANNEL (viewable));
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
