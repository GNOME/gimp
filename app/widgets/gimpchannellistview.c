/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpchannellistview.c
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

#include "gimpchannellistview.h"
#include "gimpcomponentlistitem.h"
#include "gimpdnd.h"
#include "gimpimagepreview.h"
#include "gimplistitem.h"
#include "gimpwidgets-utils.h"

#include "libgimp/gimpintl.h"


static void   gimp_channel_list_view_class_init (GimpChannelListViewClass *klass);
static void   gimp_channel_list_view_init       (GimpChannelListView      *view);

static void   gimp_channel_list_view_set_image      (GimpItemListView     *item_view,
						     GimpImage            *gimage);

static void   gimp_channel_list_view_select_item    (GimpContainerView   *view,
						     GimpViewable        *item,
						     gpointer             insert_data);
static void   gimp_channel_list_view_set_preview_size (GimpContainerView *view);

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
static void   gimp_channel_list_view_alpha_changed  (GimpImage           *gimage,
						     GimpChannelListView *view);


static GimpDrawableListViewClass *parent_class = NULL;


GType
gimp_channel_list_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpChannelListViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_channel_list_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpChannelListView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_channel_list_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_DRAWABLE_LIST_VIEW,
                                          "GimpChannelListView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_channel_list_view_class_init (GimpChannelListViewClass *klass)
{
  GimpContainerViewClass *container_view_class;
  GimpItemListViewClass  *item_view_class;

  container_view_class = GIMP_CONTAINER_VIEW_CLASS (klass);
  item_view_class      = GIMP_ITEM_LIST_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  container_view_class->select_item      = gimp_channel_list_view_select_item;
  container_view_class->set_preview_size = gimp_channel_list_view_set_preview_size;

  item_view_class->set_image             = gimp_channel_list_view_set_image;
}

static void
gimp_channel_list_view_init (GimpChannelListView *view)
{
  GimpDrawableListView *drawable_view;
  GimpContainerView    *container_view;
  gchar                *str;

  drawable_view  = GIMP_DRAWABLE_LIST_VIEW (view);
  container_view = GIMP_CONTAINER_VIEW (view);

  /*  component frame  */

  view->component_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (view->component_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (view), view->component_frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (view), view->component_frame, 0);
  /* don't show */

  view->component_list = gtk_list_new ();

  /*  FIXME: disable keyboard navigation because as of gtk+-2.0.3,
   *  GtkList kb navigation with is h-o-r-r-i-b-l-y broken with
   *  GTK_SELECTION_MULTIPLE  --mitch 05/27/2002
   */
  GTK_WIDGET_UNSET_FLAGS (view->component_list, GTK_CAN_FOCUS);

  gtk_list_set_selection_mode (GTK_LIST (view->component_list),
			       GTK_SELECTION_MULTIPLE);
  gtk_container_add (GTK_CONTAINER (view->component_frame),
		     view->component_list);
  gtk_widget_show (view->component_list);

  str = g_strdup_printf (_("Channel to Selection\n"
                           "%s  Add\n"
                           "%s  Subtract\n"
                           "%s%s%s  Intersect"),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_name_control (),
                         gimp_get_mod_name_shift (),
                         gimp_get_mod_separator (),
                         gimp_get_mod_name_control ());

  /*  To Selection button  */
  view->toselection_button =
    gimp_editor_add_button (GIMP_EDITOR (container_view),
                            GIMP_STOCK_SELECTION_REPLACE, str, NULL,
                            G_CALLBACK (gimp_channel_list_view_toselection_clicked),
                            G_CALLBACK (gimp_channel_list_view_toselection_extended_clicked),
                            view);

  g_free (str);

  gtk_box_reorder_child (GTK_BOX (GIMP_EDITOR (container_view)->button_box),
			 view->toselection_button, 5);

  gimp_container_view_enable_dnd (container_view,
				  GTK_BUTTON (view->toselection_button),
				  GIMP_TYPE_CHANNEL);

  gtk_widget_set_sensitive (view->toselection_button, FALSE);
}


/*  GimpChannelListView methods  */

static void
gimp_channel_list_view_set_image (GimpItemListView *item_view,
				  GimpImage        *gimage)
{
  GimpChannelListView *channel_view;

  channel_view = GIMP_CHANNEL_LIST_VIEW (item_view);

  if (item_view->gimage)
    {
      if (! gimage)
	gtk_widget_hide (channel_view->component_frame);

      gimp_channel_list_view_clear_components (channel_view);

      g_signal_handlers_disconnect_by_func (item_view->gimage,
					    gimp_channel_list_view_mode_changed,
					    channel_view);
      g_signal_handlers_disconnect_by_func (item_view->gimage,
					    gimp_channel_list_view_alpha_changed,
					    channel_view);
    }

  if (GIMP_ITEM_LIST_VIEW_CLASS (parent_class)->set_image)
    GIMP_ITEM_LIST_VIEW_CLASS (parent_class)->set_image (item_view, gimage);

  if (item_view->gimage)
    {
      if (! GTK_WIDGET_VISIBLE (channel_view->component_frame))
	gtk_widget_show (channel_view->component_frame);

      gimp_channel_list_view_create_components (channel_view);

      g_signal_connect (item_view->gimage, "mode_changed",
			G_CALLBACK (gimp_channel_list_view_mode_changed),
			channel_view);
      g_signal_connect (item_view->gimage, "alpha_changed",
			G_CALLBACK (gimp_channel_list_view_alpha_changed),
			channel_view);
    }
}


/*  GimpContainerView methods  */

static void
gimp_channel_list_view_select_item (GimpContainerView *view,
				    GimpViewable      *item,
				    gpointer           insert_data)
{
  GimpItemListView    *item_view;
  GimpChannelListView *list_view;

  item_view = GIMP_ITEM_LIST_VIEW (view);
  list_view = GIMP_CHANNEL_LIST_VIEW (view);

  if (GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_VIEW_CLASS (parent_class)->select_item (view,
							   item,
							   insert_data);

  if (item_view->gimage)
    {
      gboolean floating_sel;

      floating_sel = (gimp_image_floating_sel (item_view->gimage) != NULL);

      gtk_widget_set_sensitive (GIMP_EDITOR (view)->button_box, ! floating_sel);
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

static void
gimp_channel_list_view_toselection_clicked (GtkWidget           *widget,
					    GimpChannelListView *view)
{
  gimp_channel_list_view_toselection_extended_clicked (widget, 0, view);
}

static void
gimp_channel_list_view_toselection_extended_clicked (GtkWidget           *widget,
						     guint                state,
						     GimpChannelListView *view)
{
  GimpItemListView *item_view;
  GimpViewable     *viewable;

  item_view = GIMP_ITEM_LIST_VIEW (view);

  viewable = item_view->get_item_func (item_view->gimage);

  if (viewable)
    {
      GimpChannelOps  operation = GIMP_CHANNEL_OP_REPLACE;
      GimpImage      *gimage;

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

      gimage = gimp_item_get_image (GIMP_ITEM (viewable));

      gimp_image_mask_select_channel (gimage,
                                      _("Channel to Selection"),
                                      GIMP_CHANNEL (viewable),
                                      0, 0,
                                      operation,
                                      FALSE, 0.0, 0.0);
      gimp_image_flush (gimage);
    }
}

static void
gimp_channel_list_view_create_components (GimpChannelListView *view)
{
  GimpImage       *gimage;
  GtkWidget       *list_item;
  gint             n_components = 0;
  GimpChannelType  components[MAX_CHANNELS];
  GList           *list = NULL;
  gint             i;

  gimage = GIMP_ITEM_LIST_VIEW (view)->gimage;

  switch (gimp_image_base_type (gimage))
    {
    case GIMP_RGB:
      n_components  = 3;
      components[0] = GIMP_RED_CHANNEL;
      components[1] = GIMP_GREEN_CHANNEL;
      components[2] = GIMP_BLUE_CHANNEL;
      break;

    case GIMP_GRAY:
      n_components  = 1;
      components[0] = GIMP_GRAY_CHANNEL;
      break;

    case GIMP_INDEXED:
      n_components  = 1;
      components[0] = GIMP_INDEXED_CHANNEL;
      break;
    }

  if (gimp_image_has_alpha (gimage))
    {
      components[n_components++] = GIMP_ALPHA_CHANNEL;
    }

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
  gtk_list_clear_items (GTK_LIST (view->component_list), 0, -1);
}

static void
gimp_channel_list_view_mode_changed (GimpImage           *gimage,
				     GimpChannelListView *view)
{
  gimp_channel_list_view_clear_components (view);
  gimp_channel_list_view_create_components (view);
}

static void
gimp_channel_list_view_alpha_changed (GimpImage           *gimage,
                                      GimpChannelListView *view)
{
  gimp_channel_list_view_clear_components (view);
  gimp_channel_list_view_create_components (view);
}
