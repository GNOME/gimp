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
#include <stdio.h>
#include <stdlib.h>
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "channels_dialog.h"
#include "colormaps.h"
#include "color_panel.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "general.h"
#include "interface.h"
#include "layers_dialogP.h"
#include "ops_buttons.h"
#include "paint_funcs.h"
#include "palette.h"
#include "resize.h"

#include "tools/eye.xbm"
#include "tools/channel.xbm"
#include "tools/new.xpm"
#include "tools/new_is.xpm"
#include "tools/raise.xpm"
#include "tools/raise_is.xpm"
#include "tools/lower.xpm"
#include "tools/lower_is.xpm"
#include "tools/duplicate.xpm"
#include "tools/duplicate_is.xpm"
#include "tools/delete.xpm"
#include "tools/delete_is.xpm"

#include "channel_pvt.h"


#define PREVIEW_EVENT_MASK GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK
#define BUTTON_EVENT_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | \
                           GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK

#define CHANNEL_LIST_WIDTH 200
#define CHANNEL_LIST_HEIGHT 150

#define NORMAL 0
#define SELECTED 1
#define INSENSITIVE 2

#define COMPONENT_BASE_ID 0x10000000

typedef struct _ChannelWidget ChannelWidget;

struct _ChannelWidget {
  GtkWidget *eye_widget;
  GtkWidget *clip_widget;
  GtkWidget *channel_preview;
  GtkWidget *list_item;
  GtkWidget *label;

  GImage *gimage;
  Channel *channel;
  GdkPixmap *channel_pixmap;
  ChannelType type;
  int ID;
  int width, height;
  int visited;
};

typedef struct _ChannelsDialog ChannelsDialog;

struct _ChannelsDialog {
  GtkWidget *vbox;
  GtkWidget *channel_list;
  GtkWidget *preview;
  GtkWidget *ops_menu;
  GtkAcceleratorTable *accel_table;

  int num_components;
  int base_type;
  ChannelType components[3];
  double ratio;
  int image_width, image_height;
  int gimage_width, gimage_height;

  /*  state information  */
  int gimage_id;
  Channel * active_channel;
  Layer *floating_sel;
  GSList *channel_widgets;
};

/*  channels dialog widget routines  */
static void channels_dialog_preview_extents (void);
static void channels_dialog_set_menu_sensitivity (void);
static void channels_dialog_set_channel (ChannelWidget *);
static void channels_dialog_unset_channel (ChannelWidget *);
static void channels_dialog_position_channel (ChannelWidget *, int);
static void channels_dialog_add_channel (Channel *);
static void channels_dialog_remove_channel (ChannelWidget *);
static gint channel_list_events (GtkWidget *, GdkEvent *);

/*  channels dialog menu callbacks  */
static void channels_dialog_map_callback (GtkWidget *, gpointer);
static void channels_dialog_unmap_callback (GtkWidget *, gpointer);
static void channels_dialog_new_channel_callback (GtkWidget *, gpointer);
static void channels_dialog_raise_channel_callback (GtkWidget *, gpointer);
static void channels_dialog_lower_channel_callback (GtkWidget *, gpointer);
static void channels_dialog_duplicate_channel_callback (GtkWidget *, gpointer);
static void channels_dialog_delete_channel_callback (GtkWidget *, gpointer);
static void channels_dialog_channel_to_sel_callback (GtkWidget *, gpointer);

/*  channel widget function prototypes  */
static ChannelWidget *channel_widget_get_ID (Channel *);
static ChannelWidget *create_channel_widget (GImage *, Channel *, ChannelType);
static void channel_widget_delete (ChannelWidget *);
static void channel_widget_select_update (GtkWidget *, gpointer);
static gint channel_widget_button_events (GtkWidget *, GdkEvent *);
static gint channel_widget_preview_events (GtkWidget *, GdkEvent *);
static void channel_widget_preview_redraw (ChannelWidget *);
static void channel_widget_no_preview_redraw (ChannelWidget *);
static void channel_widget_eye_redraw (ChannelWidget *);
static void channel_widget_exclusive_visible (ChannelWidget *);
static void channel_widget_channel_flush (GtkWidget *, gpointer);

/*  assorted query dialogs  */
static void channels_dialog_new_channel_query (int);
static void channels_dialog_edit_channel_query (ChannelWidget *);


/*  Only one channels dialog  */
static ChannelsDialog *channelsD = NULL;

static GdkPixmap *eye_pixmap[3] = { NULL, NULL, NULL };
static GdkPixmap *channel_pixmap[3] = { NULL, NULL, NULL };

static int suspend_gimage_notify = 0;

static MenuItem channels_ops[] =
{
  { "New Channel", 'N', GDK_CONTROL_MASK,
    channels_dialog_new_channel_callback, NULL, NULL, NULL },
  { "Raise Channel", 'F', GDK_CONTROL_MASK,
    channels_dialog_raise_channel_callback, NULL, NULL, NULL },
  { "Lower Channel", 'B', GDK_CONTROL_MASK,
    channels_dialog_lower_channel_callback, NULL, NULL, NULL },
  { "Duplicate Channel", 'C', GDK_CONTROL_MASK,
    channels_dialog_duplicate_channel_callback, NULL, NULL, NULL },
  { "Delete Channel", 'X', GDK_CONTROL_MASK,
    channels_dialog_delete_channel_callback, NULL, NULL, NULL },
  { "Channel To Selection", 'S', GDK_CONTROL_MASK,
    channels_dialog_channel_to_sel_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL },
};

/* the ops buttons */
static OpsButton channels_ops_buttons[] =
{
  { new_xpm, new_is_xpm, channels_dialog_new_channel_callback, "New Channel", NULL, NULL, NULL, NULL, NULL, NULL },
  { raise_xpm, raise_is_xpm, channels_dialog_raise_channel_callback, "Raise Channel", NULL, NULL, NULL, NULL, NULL, NULL },
  { lower_xpm, lower_is_xpm, channels_dialog_lower_channel_callback, "Lower Channel", NULL, NULL, NULL, NULL, NULL, NULL },
  { duplicate_xpm, duplicate_is_xpm, channels_dialog_duplicate_channel_callback, "Duplicate Channel", NULL, NULL, NULL, NULL, NULL, NULL },
  { delete_xpm, delete_is_xpm, channels_dialog_delete_channel_callback, "Delete Channel", NULL, NULL, NULL, NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

/**************************************/
/*  Public channels dialog functions  */
/**************************************/

GtkWidget *
channels_dialog_create ()
{
  GtkWidget *vbox;
  GtkWidget *listbox;
  GtkWidget *button_box;

  if (!channelsD)
    {
      channelsD = g_malloc (sizeof (ChannelsDialog));
      channelsD->preview = NULL;
      channelsD->gimage_id = -1;
      channelsD->active_channel = NULL;
      channelsD->floating_sel = NULL;
      channelsD->channel_widgets = NULL;
      channelsD->accel_table = gtk_accelerator_table_new ();

      if (preview_size)
	{
	  channelsD->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
	  gtk_preview_size (GTK_PREVIEW (channelsD->preview), preview_size, preview_size);
	}

      /*  The main vbox  */
      channelsD->vbox = vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 2);

      /*  The layers commands pulldown menu  */
      channelsD->ops_menu = build_menu (channels_ops, channelsD->accel_table);

      /*  The channels listbox  */
      listbox = gtk_scrolled_window_new (NULL, NULL);
      gtk_widget_set_usize (listbox, CHANNEL_LIST_WIDTH, CHANNEL_LIST_HEIGHT);
      gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 2);

      channelsD->channel_list = gtk_list_new ();
      gtk_container_add (GTK_CONTAINER (listbox), channelsD->channel_list);
      gtk_list_set_selection_mode (GTK_LIST (channelsD->channel_list), GTK_SELECTION_MULTIPLE);
      gtk_signal_connect (GTK_OBJECT (channelsD->channel_list), "event",
			  (GtkSignalFunc) channel_list_events,
			  channelsD);

      gtk_widget_show (channelsD->channel_list);
      gtk_widget_show (listbox);


      /* The ops buttons */

      button_box = ops_button_box_new (lc_shell, tool_tips, channels_ops_buttons);

      gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
      gtk_widget_show (button_box);

      /*  Set up signals for map/unmap for the accelerators  */
      gtk_signal_connect (GTK_OBJECT (channelsD->vbox), "map",
			  (GtkSignalFunc) channels_dialog_map_callback,
			  NULL);
      gtk_signal_connect (GTK_OBJECT (channelsD->vbox), "unmap",
			  (GtkSignalFunc) channels_dialog_unmap_callback,
			  NULL);

      gtk_widget_show (vbox);
    }

  return channelsD->vbox;
}


void
channels_dialog_flush ()
{
  GImage *gimage;
  Channel *channel;
  ChannelWidget *cw;
  GSList *list;
  int gimage_pos;
  int pos;

  if (!channelsD)
    return;

  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  /*  Check if the gimage extents have changed  */
  if ((gimage->width != channelsD->gimage_width) ||
      (gimage->height != channelsD->gimage_height) ||
      (gimage_base_type (gimage) != channelsD->base_type))
    {
      channelsD->gimage_id = -1;
      channels_dialog_update (gimage->ID);
    }

  /*  Set all current channel widgets to visited = FALSE  */
  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      cw->visited = FALSE;
      list = g_slist_next (list);
    }

  /*  Add any missing channels  */
  list = gimage->channels;
  while (list)
    {
      channel = (Channel *) list->data;
      cw = channel_widget_get_ID (channel);

      /*  If the channel isn't in the channel widget list, add it  */
      if (cw == NULL)
	channels_dialog_add_channel (channel);
      else
	cw->visited = TRUE;

      list = g_slist_next (list);
    }

  /*  Remove any extraneous auxillary channels  */
  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      list = g_slist_next (list);

      if (cw->visited == FALSE && cw->type == Auxillary)
	/*  will only be true for auxillary channels  */
	channels_dialog_remove_channel (cw);
    }

  /*  Switch positions of items if necessary  */
  list = channelsD->channel_widgets;
  pos = -channelsD->num_components;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      list = g_slist_next (list);

      if (cw->type == Auxillary)
	if ((gimage_pos = gimage_get_channel_index (gimage, cw->channel)) != pos)
	  channels_dialog_position_channel (cw, gimage_pos);

      pos++;
    }

  /*  Set the active channel  */
  if (channelsD->active_channel != gimage->active_channel)
    channelsD->active_channel = gimage->active_channel;

  /*  set the menus if floating sel status has changed  */
  if (channelsD->floating_sel != gimage->floating_sel)
    channelsD->floating_sel = gimage->floating_sel;

  channels_dialog_set_menu_sensitivity ();

  gtk_container_foreach (GTK_CONTAINER (channelsD->channel_list),
			 channel_widget_channel_flush, NULL);
}


/*************************************/
/*  channels dialog widget routines  */
/*************************************/

void
channels_dialog_update (int gimage_id)
{
  ChannelWidget *cw;
  GImage *gimage;
  Channel *channel;
  GSList *list;
  GList *item_list;

  if (!channelsD)
    return;
  if (channelsD->gimage_id == gimage_id)
    return;

  channelsD->gimage_id = gimage_id;

  suspend_gimage_notify++;
  /*  Free all elements in the channels listbox  */
  gtk_list_clear_items (GTK_LIST (channelsD->channel_list), 0, -1);
  suspend_gimage_notify--;

  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      list = g_slist_next (list);
      channel_widget_delete (cw);
    }
  channelsD->channel_widgets = NULL;

  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  /*  Find the preview extents  */
  channels_dialog_preview_extents ();

  channelsD->active_channel = NULL;
  channelsD->floating_sel = NULL;

  /*  The image components  */
  item_list = NULL;
  switch ((channelsD->base_type = gimage_base_type (gimage)))
    {
    case RGB:
      cw = create_channel_widget (gimage, NULL, Red);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[0] = Red;

      cw = create_channel_widget (gimage, NULL, Green);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[1] = Green;

      cw = create_channel_widget (gimage, NULL, Blue);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[2] = Blue;

      channelsD->num_components = 3;
      break;

    case GRAY:
      cw = create_channel_widget (gimage, NULL, Gray);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[0] = Gray;

      channelsD->num_components = 1;
      break;

    case INDEXED:
      cw = create_channel_widget (gimage, NULL, Indexed);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[0] = Indexed;

      channelsD->num_components = 1;
      break;
    }

  /*  The auxillary image channels  */
  list = gimage->channels;
  while (list)
    {
      /*  create a channel list item  */
      channel = (Channel *) list->data;
      cw = create_channel_widget (gimage, channel, Auxillary);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);

      list = g_slist_next (list);
    }

  /*  get the index of the active channel  */
  if (item_list)
    gtk_list_insert_items (GTK_LIST (channelsD->channel_list), item_list, 0);
}


void
channels_dialog_clear ()
{
  ops_button_box_set_insensitive (channels_ops_buttons);

  suspend_gimage_notify++;
  gtk_list_clear_items (GTK_LIST (channelsD->channel_list), 0, -1);
  suspend_gimage_notify--;
}

void
channels_dialog_free ()
{
  GSList *list;
  ChannelWidget *cw;

  if (channelsD == NULL)
    return;

  suspend_gimage_notify++;
  /*  Free all elements in the channels listbox  */
  gtk_list_clear_items (GTK_LIST (channelsD->channel_list), 0, -1);
  suspend_gimage_notify--;

  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      list = g_slist_next (list);
      channel_widget_delete (cw);
    }
  channelsD->channel_widgets = NULL;
  channelsD->active_channel = NULL;
  channelsD->floating_sel = NULL;

  if (channelsD->preview)
    gtk_object_sink (GTK_OBJECT (channelsD->preview));

  if (channelsD->ops_menu)
    gtk_object_sink (GTK_OBJECT (channelsD->ops_menu));

  g_free (channelsD);
  channelsD = NULL;
}

static void
channels_dialog_preview_extents ()
{
  GImage *gimage;

  if (! channelsD)
    return;

  gimage = gimage_get_ID (channelsD->gimage_id);
  channelsD->gimage_width = gimage->width;
  channelsD->gimage_height = gimage->height;

  /*  Get the image width and height variables, based on the gimage  */
  if (gimage->width > gimage->height)
    channelsD->ratio = (double) preview_size / (double) gimage->width;
  else
    channelsD->ratio = (double) preview_size / (double) gimage->height;

  if (preview_size)
    {
      channelsD->image_width = (int) (channelsD->ratio * gimage->width);
      channelsD->image_height = (int) (channelsD->ratio * gimage->height);
      if (channelsD->image_width < 1) channelsD->image_width = 1;
      if (channelsD->image_height < 1) channelsD->image_height = 1;
    }
  else
    {
      channelsD->image_width = channel_width;
      channelsD->image_height = channel_height;
    }
}


static void
channels_dialog_set_menu_sensitivity ()
{
  ChannelWidget *cw;
  gint fs_sensitive;
  gint aux_sensitive;

  cw = channel_widget_get_ID (channelsD->active_channel);
  fs_sensitive = (channelsD->floating_sel != NULL);

  if (cw)
    aux_sensitive = (cw->type == Auxillary);
  else
    aux_sensitive = FALSE;

  /* new channel */
  gtk_widget_set_sensitive (channels_ops[0].widget, !fs_sensitive);
  ops_button_set_sensitive (channels_ops_buttons[0], !fs_sensitive);
  /* raise channel */
  gtk_widget_set_sensitive (channels_ops[1].widget, !fs_sensitive && aux_sensitive);
  ops_button_set_sensitive (channels_ops_buttons[1], !fs_sensitive && aux_sensitive);
  /* lower channel */
  gtk_widget_set_sensitive (channels_ops[2].widget, !fs_sensitive && aux_sensitive);
  ops_button_set_sensitive (channels_ops_buttons[2], !fs_sensitive && aux_sensitive);
  /* duplicate channel */
  gtk_widget_set_sensitive (channels_ops[3].widget, !fs_sensitive && aux_sensitive);
  ops_button_set_sensitive (channels_ops_buttons[3], !fs_sensitive && aux_sensitive);
  /* delete channel */
  gtk_widget_set_sensitive (channels_ops[4].widget, !fs_sensitive && aux_sensitive);
  ops_button_set_sensitive (channels_ops_buttons[4], !fs_sensitive && aux_sensitive);
  /* channel to selection */
  gtk_widget_set_sensitive (channels_ops[5].widget, aux_sensitive);
}


static void
channels_dialog_set_channel (ChannelWidget *channel_widget)
{
  GtkStateType state;
  int index;

  if (!channelsD || !channel_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  get the list item data  */
  state = channel_widget->list_item->state;

  if (channel_widget->type == Auxillary) 
    {
      /*  turn on the specified auxillary channel  */
      index = gimage_get_channel_index (channel_widget->gimage, channel_widget->channel);
      if ((index >= 0) && (state != GTK_STATE_SELECTED))
	{
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), NULL);
	  gtk_list_select_item (GTK_LIST (channelsD->channel_list), index + channelsD->num_components);
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), channel_widget);
	}
    } 
  else 
    {
      if (state != GTK_STATE_SELECTED)
	{
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), NULL);
	  switch (channel_widget->type)
	    {
	    case Red: case Gray: case Indexed:
	      gtk_list_select_item (GTK_LIST (channelsD->channel_list), 0);
	      break;
	    case Green:
	      gtk_list_select_item (GTK_LIST (channelsD->channel_list), 1);
	      break;
	    case Blue:
	      gtk_list_select_item (GTK_LIST (channelsD->channel_list), 2);
	      break;
	    case Auxillary:
	      g_error ("error in %s at %d: this shouldn't happen.",
		       __FILE__, __LINE__);
	      break;
	    }
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), channel_widget);
	}
    }
  suspend_gimage_notify--;
}


static void
channels_dialog_unset_channel (ChannelWidget * channel_widget)
{
  GtkStateType state;
  int index;

  if (!channelsD || !channel_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  get the list item data  */
  state = channel_widget->list_item->state;

  if (channel_widget->type == Auxillary) 
    {
      /*  turn off the specified auxillary channel  */
      index = gimage_get_channel_index (channel_widget->gimage, channel_widget->channel);
      if ((index >= 0) && (state == GTK_STATE_SELECTED))
	{
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), NULL);
	  gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), index + channelsD->num_components);
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), channel_widget);
	}
    }
  else
    {
      if (state == GTK_STATE_SELECTED)
	{
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), NULL);
	  switch (channel_widget->type)
	    {
	    case Red: case Gray: case Indexed:
	      gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), 0);
	      break;
	    case Green:
	      gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), 1);
	      break;
	    case Blue:
	      gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), 2);
	      break;
	    case Auxillary:
	      g_error ("error in %s at %d: this shouldn't happen.",
		       __FILE__, __LINE__);
	      break;
	    }
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item), channel_widget);
	}
    }
  
  suspend_gimage_notify--;
}


static void
channels_dialog_position_channel (ChannelWidget *channel_widget,
				  int new_index)
{
  GList *list = NULL;

  if (!channelsD || !channel_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Remove the channel from the dialog  */
  list = g_list_append (list, channel_widget->list_item);
  gtk_list_remove_items (GTK_LIST (channelsD->channel_list), list);

  suspend_gimage_notify--;

  /*  Add it back at the proper index  */
  gtk_list_insert_items (GTK_LIST (channelsD->channel_list), list, new_index + channelsD->num_components);

}


static void
channels_dialog_add_channel (Channel *channel)
{
  GImage *gimage;
  GList *item_list;
  ChannelWidget *channel_widget;
  int position;

  if (!channelsD || !channel)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  item_list = NULL;

  channel_widget = create_channel_widget (gimage, channel, Auxillary);
  item_list = g_list_append (item_list, channel_widget->list_item);

  position = gimage_get_channel_index (gimage, channel);
  channelsD->channel_widgets = g_slist_insert (channelsD->channel_widgets, channel_widget,
					       position + channelsD->num_components);
  gtk_list_insert_items (GTK_LIST (channelsD->channel_list), item_list,
			 position + channelsD->num_components);
}


static void
channels_dialog_remove_channel (ChannelWidget *channel_widget)
{
  GList *list = NULL;

  if (!channelsD || !channel_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Remove the requested channel from the dialog  */
  list = g_list_append (list, channel_widget->list_item);
  gtk_list_remove_items (GTK_LIST (channelsD->channel_list), list);

  /*  Delete the channel_widget  */
  channel_widget_delete (channel_widget);

  suspend_gimage_notify--;
}


static gint
channel_list_events (GtkWidget *widget,
		     GdkEvent  *event)
{
  GdkEventKey *kevent;
  GdkEventButton *bevent;
  GtkWidget *event_widget;
  ChannelWidget *channel_widget;

  event_widget = gtk_get_event_widget (event);

  if (GTK_IS_LIST_ITEM (event_widget))
    {
      channel_widget = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (event_widget));

      switch (event->type)
	{
	case GDK_BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;

	  if (bevent->button == 3)
	    {
	      gtk_menu_popup (GTK_MENU (channelsD->ops_menu), NULL, NULL, NULL, NULL, 3, bevent->time);
	      return TRUE;
	    }
	  break;

	case GDK_2BUTTON_PRESS:
	  if (channel_widget->type == Auxillary)
	    channels_dialog_edit_channel_query (channel_widget);
	  return TRUE;
	  break;

	case GDK_KEY_PRESS:
	  kevent = (GdkEventKey *) event;
	  switch (kevent->keyval)
	    {
	    case GDK_Up:
	      printf ("up arrow\n");
	      break;
	    case GDK_Down:
	      printf ("down arrow\n");
	      break;
	    default:
	      return FALSE;
	      break;
	    }
	  return TRUE;
	  break;

	default:
	  break;
	}
    }

  return FALSE;
}


/*******************************/
/*  channels dialog callbacks  */
/*******************************/

static void
channels_dialog_map_callback (GtkWidget *w,
			    gpointer   client_data)
{
  if (!channelsD)
    return;

  gtk_window_add_accelerator_table (GTK_WINDOW (lc_shell),
				    channelsD->accel_table);
}

static void
channels_dialog_unmap_callback (GtkWidget *w,
			      gpointer   client_data)
{
  if (!channelsD)
    return;

  gtk_window_remove_accelerator_table (GTK_WINDOW (lc_shell),
				       channelsD->accel_table);
}

static void
channels_dialog_new_channel_callback (GtkWidget *w,
				      gpointer   client_data)
{
  /*  if there is a currently selected gimage, request a new channel
   */
  if (!channelsD)
    return;
  if (channelsD->gimage_id == -1)
    return;

  channels_dialog_new_channel_query (channelsD->gimage_id);
}


static void
channels_dialog_raise_channel_callback (GtkWidget *w,
					gpointer   client_data)
{
  GImage *gimage;

  if (!channelsD)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  if (gimage->active_channel != NULL)
    {
      gimage_raise_channel (gimage, gimage->active_channel);
      gdisplays_flush ();
    }
}


static void
channels_dialog_lower_channel_callback (GtkWidget *w,
					gpointer   client_data)
{
  GImage *gimage;

  if (!channelsD)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  if (gimage->active_channel != NULL)
    {
      gimage_lower_channel (gimage, gimage->active_channel);
      gdisplays_flush ();
    }
}


static void
channels_dialog_duplicate_channel_callback (GtkWidget *w,
					    gpointer   client_data)
{
  GImage *gimage;
  Channel *active_channel;
  Channel *new_channel;

  /*  if there is a currently selected gimage, request a new channel
   */
  if (!channelsD)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  if ((active_channel = gimage_get_active_channel (gimage)))
    {
      new_channel = channel_copy (active_channel);
      gimage_add_channel (gimage, new_channel, -1);
      gdisplays_flush ();
    }
}


static void
channels_dialog_delete_channel_callback (GtkWidget *w,
					 gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!channelsD)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  if (gimage->active_channel != NULL)
    {
      gimage_remove_channel (gimage, gimage->active_channel);
      gdisplays_flush ();
    }
}


static void
channels_dialog_channel_to_sel_callback (GtkWidget *w,
					 gpointer   client_data)
{
  GImage *gimage;

  /*  if there is a currently selected gimage
   */
  if (!channelsD)
    return;
  if (! (gimage = gimage_get_ID (channelsD->gimage_id)))
    return;

  if (gimage->active_channel != NULL)
    {
      gimage_mask_load (gimage, gimage->active_channel);
      gdisplays_flush ();
    }
}


/****************************/
/*  channel widget functions  */
/****************************/

static ChannelWidget *
channel_widget_get_ID (Channel *channel)
{
  ChannelWidget *lw;
  GSList *list;

  if (!channelsD)
    return NULL;

  list = channelsD->channel_widgets;

  while (list)
    {
      lw = (ChannelWidget *) list->data;
      if (lw->channel == channel)
	return lw;

      list = g_slist_next (list);
    }

  return NULL;
}


static ChannelWidget *
create_channel_widget (GImage      *gimage,
		       Channel     *channel,
		       ChannelType  type)
{
  ChannelWidget *channel_widget;
  GtkWidget *list_item;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *alignment;

  list_item = gtk_list_item_new ();

  /*  create the channel widget and add it to the list  */
  channel_widget = (ChannelWidget *) g_malloc (sizeof (ChannelWidget));
  channel_widget->gimage = gimage;
  channel_widget->channel = channel;
  channel_widget->channel_preview = NULL;
  channel_widget->channel_pixmap = NULL;
  channel_widget->type = type;
  channel_widget->ID = (type == Auxillary) ? GIMP_DRAWABLE(channel)->ID : (COMPONENT_BASE_ID + type);
  channel_widget->list_item = list_item;
  channel_widget->width = -1;
  channel_widget->height = -1;
  channel_widget->visited = TRUE;

  /*  Need to let the list item know about the channel_widget  */
  gtk_object_set_user_data (GTK_OBJECT (list_item), channel_widget);

  /*  set up the list item observer  */
  gtk_signal_connect (GTK_OBJECT (list_item), "select",
		      (GtkSignalFunc) channel_widget_select_update,
		      channel_widget);
  gtk_signal_connect (GTK_OBJECT (list_item), "deselect",
		      (GtkSignalFunc) channel_widget_select_update,
		      channel_widget);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (list_item), vbox);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

  /* Create the visibility toggle button */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  channel_widget->eye_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (channel_widget->eye_widget), eye_width, eye_height);
  gtk_widget_set_events (channel_widget->eye_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (channel_widget->eye_widget), "event",
		      (GtkSignalFunc) channel_widget_button_events,
		      channel_widget);
  gtk_object_set_user_data (GTK_OBJECT (channel_widget->eye_widget), channel_widget);
  gtk_container_add (GTK_CONTAINER (alignment), channel_widget->eye_widget);
  gtk_widget_show (channel_widget->eye_widget);
  gtk_widget_show (alignment);

  /*  The preview  */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  channel_widget->channel_preview = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (channel_widget->channel_preview),
			 channelsD->image_width, channelsD->image_height);
  gtk_widget_set_events (channel_widget->channel_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (channel_widget->channel_preview), "event",
		      (GtkSignalFunc) channel_widget_preview_events,
		      channel_widget);
  gtk_object_set_user_data (GTK_OBJECT (channel_widget->channel_preview), channel_widget);
  gtk_container_add (GTK_CONTAINER (alignment), channel_widget->channel_preview);
  gtk_widget_show (channel_widget->channel_preview);

  /*  the channel name label */
  switch (channel_widget->type)
    {
    case Red:       channel_widget->label = gtk_label_new ("Red"); break;
    case Green:     channel_widget->label = gtk_label_new ("Green"); break;
    case Blue:      channel_widget->label = gtk_label_new ("Blue"); break;
    case Gray:      channel_widget->label = gtk_label_new ("Gray"); break;
    case Indexed:   channel_widget->label = gtk_label_new ("Indexed"); break;
    case Auxillary: channel_widget->label = gtk_label_new (GIMP_DRAWABLE(channel)->name); break;
    }

  gtk_box_pack_start (GTK_BOX (hbox), channel_widget->label, FALSE, FALSE, 2);
  gtk_widget_show (channel_widget->label);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (list_item);

  gtk_widget_ref (GTK_WIDGET (channel_widget->list_item));

  return channel_widget;
}


static void
channel_widget_delete (ChannelWidget *channel_widget)
{
  if (channel_widget->channel_pixmap)
    gdk_pixmap_unref (channel_widget->channel_pixmap);

  /*  Remove the channel widget from the list  */
  channelsD->channel_widgets = g_slist_remove (channelsD->channel_widgets, channel_widget);

  /*  Free the widget  */
  gtk_widget_unref (channel_widget->list_item);
  g_free (channel_widget);
}


static void
channel_widget_select_update (GtkWidget *w,
			      gpointer   data)
{
  ChannelWidget *channel_widget;

  if ((channel_widget = (ChannelWidget *) data) == NULL)
    return;

  if (suspend_gimage_notify == 0)
    {
      if (channel_widget->type == Auxillary)
	{
	  if (w->state == GTK_STATE_SELECTED)
	    /*  set the gimage's active channel to be this channel  */
	    gimage_set_active_channel (channel_widget->gimage, channel_widget->channel);
	  else
	    /*  unset the gimage's active channel  */
	    gimage_unset_active_channel (channel_widget->gimage);

	  gdisplays_flush ();
	}
      else if (channel_widget->type != Auxillary)
	{
	  if (w->state == GTK_STATE_SELECTED)
	    gimage_set_component_active (channel_widget->gimage, channel_widget->type, TRUE);
	  else
	    gimage_set_component_active (channel_widget->gimage, channel_widget->type, FALSE);
	}
    }
}


static gint
channel_widget_button_events (GtkWidget *widget,
			      GdkEvent  *event)
{
  static int button_down = 0;
  static GtkWidget *click_widget = NULL;
  static int old_state;
  static int exclusive;
  ChannelWidget *channel_widget;
  GtkWidget *event_widget;
  gint return_val;
  int visible;
  int width, height;

  channel_widget = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));
  switch (channel_widget->type)
    {
    case Auxillary:
      visible = GIMP_DRAWABLE(channel_widget->channel)->visible;
      width = GIMP_DRAWABLE(channel_widget->channel)->width;
      height = GIMP_DRAWABLE(channel_widget->channel)->height;
      break;
    default:
      visible = gimage_get_component_visible (channel_widget->gimage, channel_widget->type);
      width = channel_widget->gimage->width;
      height = channel_widget->gimage->height;
      break;
    }

  return_val = FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == channel_widget->eye_widget)
	channel_widget_eye_redraw (channel_widget);
      break;

    case GDK_BUTTON_PRESS:
      return_val = TRUE;

      button_down = 1;
      click_widget = widget;
      gtk_grab_add (click_widget);

      if (widget == channel_widget->eye_widget)
	{
	  old_state = visible;

	  /*  If this was a shift-click, make all/none visible  */
	  if (event->button.state & GDK_SHIFT_MASK)
	    {
	      exclusive = TRUE;
	      channel_widget_exclusive_visible (channel_widget);
	    }
	  else
	    {
	      exclusive = FALSE;
	      if (channel_widget->type == Auxillary)
		GIMP_DRAWABLE(channel_widget->channel)->visible = !visible;
	      else
		gimage_set_component_visible (channel_widget->gimage, channel_widget->type, !visible);
	      channel_widget_eye_redraw (channel_widget);
	    }
	}
      break;

    case GDK_BUTTON_RELEASE:
      return_val = TRUE;

      button_down = 0;
      gtk_grab_remove (click_widget);

      if (widget == channel_widget->eye_widget)
	{
	  if (exclusive)
	    {
	      gdisplays_update_area (channel_widget->gimage->ID, 0, 0, width, height);
	      gdisplays_flush ();
	    }
	  else if (old_state != visible)
	    {
	      gdisplays_update_area (channel_widget->gimage->ID, 0, 0, width, height);
	      gdisplays_flush ();
	    }
	}
      break;

    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      event_widget = gtk_get_event_widget (event);

      if (button_down && (event_widget == click_widget))
	{
	  if (widget == channel_widget->eye_widget)
	    {
	      if (exclusive)
		{
		  channel_widget_exclusive_visible (channel_widget);
		}
	      else
		{
		  if (channel_widget->type == Auxillary)
		    GIMP_DRAWABLE(channel_widget->channel)->visible = !visible;
		  else
		    gimage_set_component_visible (channel_widget->gimage, channel_widget->type, !visible);
		  channel_widget_eye_redraw (channel_widget);
		}
	    }
	}
      break;

    default:
      break;
    }

  return return_val;
}


static gint
channel_widget_preview_events (GtkWidget *widget,
			       GdkEvent  *event)
{
  GdkEventExpose *eevent;
  ChannelWidget *channel_widget;
  int valid;

  channel_widget = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (!preview_size)
	channel_widget_no_preview_redraw (channel_widget);
      else
	{
	  switch (channel_widget->type)
	    {
	    case Auxillary:
	      valid = GIMP_DRAWABLE(channel_widget->channel)->preview_valid;
	      break;
	    default:
	      valid = gimage_preview_valid (channel_widget->gimage, channel_widget->type);
	      break;
	    }

	  if (!valid || !channel_widget->channel_pixmap)
	    {
	      channel_widget_preview_redraw (channel_widget);

	      gdk_draw_pixmap (widget->window,
			       widget->style->black_gc,
			       channel_widget->channel_pixmap,
			       0, 0, 0, 0,
			       channelsD->image_width,
			       channelsD->image_height);
	    }
	  else
	    {
	      eevent = (GdkEventExpose *) event;

	      gdk_draw_pixmap (widget->window,
			       widget->style->black_gc,
			       channel_widget->channel_pixmap,
			       eevent->area.x, eevent->area.y,
			       eevent->area.x, eevent->area.y,
			       eevent->area.width, eevent->area.height);
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}


static void
channel_widget_preview_redraw (ChannelWidget *channel_widget)
{
  TempBuf * preview_buf;
  int width, height;
  int channel;

  /*  allocate the channel widget pixmap  */
  if (! channel_widget->channel_pixmap)
    channel_widget->channel_pixmap = gdk_pixmap_new (channel_widget->channel_preview->window,
						     channelsD->image_width,
						     channelsD->image_height,
						     -1);

  /*  determine width and height  */
  switch (channel_widget->type)
    {
    case Auxillary:
      width = GIMP_DRAWABLE(channel_widget->channel)->width;
      height = GIMP_DRAWABLE(channel_widget->channel)->height;
      channel_widget->width = (int) (channelsD->ratio * width);
      channel_widget->height = (int) (channelsD->ratio * height);
      preview_buf = channel_preview (channel_widget->channel,
				     channel_widget->width,
				     channel_widget->height);
      break;
    default:
      width = channel_widget->gimage->width;
      height = channel_widget->gimage->height;
      channel_widget->width = (int) (channelsD->ratio * width);
      channel_widget->height = (int) (channelsD->ratio * height);
      preview_buf = gimage_composite_preview (channel_widget->gimage,
					      channel_widget->type,
					      channel_widget->width,
					      channel_widget->height);
      break;
    }

  switch (channel_widget->type)
    {
    case Red:       channel = RED_PIX; break;
    case Green:     channel = GREEN_PIX; break;
    case Blue:      channel = BLUE_PIX; break;
    case Gray:      channel = GRAY_PIX; break;
    case Indexed:   channel = INDEXED_PIX; break;
    case Auxillary: channel = -1; break;
    default:        channel = -1; break;
    }

  render_preview (preview_buf,
		  channelsD->preview,
		  channelsD->image_width,
		  channelsD->image_height,
		  channel);

  gtk_preview_put (GTK_PREVIEW (channelsD->preview),
		   channel_widget->channel_pixmap,
		   channel_widget->channel_preview->style->black_gc,
		   0, 0, 0, 0, channelsD->image_width, channelsD->image_height);

  /*  make sure the image has been transfered completely to the pixmap before
   *  we use it again...
   */
  gdk_flush ();
}


static void
channel_widget_no_preview_redraw (ChannelWidget *channel_widget)
{
  GdkPixmap *pixmap;
  GdkPixmap **pixmap_normal;
  GdkPixmap **pixmap_selected;
  GdkPixmap **pixmap_insensitive;
  GdkColor *color;
  GtkWidget *widget;
  GtkStateType state;
  gchar *bits;
  int width, height;

  state = channel_widget->list_item->state;

  widget = channel_widget->channel_preview;
  pixmap_normal = &channel_pixmap[NORMAL];
  pixmap_selected = &channel_pixmap[SELECTED];
  pixmap_insensitive = &channel_pixmap[INSENSITIVE];
  bits = (gchar *) channel_bits;
  width = channel_width;
  height = channel_height;

  if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &widget->style->bg[GTK_STATE_SELECTED];
      else
	color = &widget->style->white;
    }
  else
    color = &widget->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (widget->window, color);

  if (!*pixmap_normal)
    {
      *pixmap_normal =
	gdk_pixmap_create_from_data (widget->window,
				     bits, width, height, -1,
				     &widget->style->fg[GTK_STATE_SELECTED],
				     &widget->style->bg[GTK_STATE_SELECTED]);
      *pixmap_selected =
	gdk_pixmap_create_from_data (widget->window,
				     bits, width, height, -1,
				     &widget->style->fg[GTK_STATE_NORMAL],
				     &widget->style->white);
      *pixmap_insensitive =
	gdk_pixmap_create_from_data (widget->window,
				     bits, width, height, -1,
				     &widget->style->fg[GTK_STATE_INSENSITIVE],
				     &widget->style->bg[GTK_STATE_INSENSITIVE]);
    }

  if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	pixmap = *pixmap_selected;
      else
	pixmap = *pixmap_normal;
    }
  else
    pixmap = *pixmap_insensitive;

  gdk_draw_pixmap (widget->window,
		   widget->style->black_gc,
		   pixmap, 0, 0, 0, 0, width, height);
}


static void
channel_widget_eye_redraw (ChannelWidget *channel_widget)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;
  int visible;

  state = channel_widget->list_item->state;

  if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
    {
      if (state == GTK_STATE_SELECTED)
	color = &channel_widget->eye_widget->style->bg[GTK_STATE_SELECTED];
      else
	color = &channel_widget->eye_widget->style->white;
    }
  else
    color = &channel_widget->eye_widget->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (channel_widget->eye_widget->window, color);

  switch (channel_widget->type)
    {
    case Auxillary:
      visible = GIMP_DRAWABLE(channel_widget->channel)->visible;
      break;
    default:
      visible = gimage_get_component_visible (channel_widget->gimage, channel_widget->type);
      break;
    }

  if (visible)
    {
      if (!eye_pixmap[NORMAL])
	{
	  eye_pixmap[NORMAL] =
	    gdk_pixmap_create_from_data (channel_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &channel_widget->eye_widget->style->fg[GTK_STATE_NORMAL],
					 &channel_widget->eye_widget->style->white);
	  eye_pixmap[SELECTED] =
	    gdk_pixmap_create_from_data (channel_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &channel_widget->eye_widget->style->fg[GTK_STATE_SELECTED],
					 &channel_widget->eye_widget->style->bg[GTK_STATE_SELECTED]);
	  eye_pixmap[INSENSITIVE] =
	    gdk_pixmap_create_from_data (channel_widget->eye_widget->window,
					 (gchar*) eye_bits, eye_width, eye_height, -1,
					 &channel_widget->eye_widget->style->fg[GTK_STATE_INSENSITIVE],
					 &channel_widget->eye_widget->style->bg[GTK_STATE_INSENSITIVE]);
	}

      if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
	{
	  if (state == GTK_STATE_SELECTED)
	    pixmap = eye_pixmap[SELECTED];
	  else
	    pixmap = eye_pixmap[NORMAL];
	}
      else
	pixmap = eye_pixmap[INSENSITIVE];

      gdk_draw_pixmap (channel_widget->eye_widget->window,
		       channel_widget->eye_widget->style->black_gc,
		       pixmap, 0, 0, 0, 0, eye_width, eye_height);
    }
  else
    {
      gdk_window_clear (channel_widget->eye_widget->window);
    }
}


static void
channel_widget_exclusive_visible (ChannelWidget *channel_widget)
{
  GSList *list;
  ChannelWidget *cw;
  int visible = FALSE;

  if (!channelsD)
    return;

  /*  First determine if _any_ other channel widgets are set to visible  */
  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      if (cw != channel_widget)
	{
	  switch (cw->type)
	    {
	    case Auxillary:
	      visible |= GIMP_DRAWABLE(cw->channel)->visible;
	      break;
	    default:
	      visible |= gimage_get_component_visible (cw->gimage, cw->type);
	      break;
	    }
	}

      list = g_slist_next (list);
    }

  /*  Now, toggle the visibility for all channels except the specified one  */
  list = channelsD->channel_widgets;
  while (list)
    {
      cw = (ChannelWidget *) list->data;
      if (cw != channel_widget)
	switch (cw->type)
	  {
	  case Auxillary:
	    GIMP_DRAWABLE(cw->channel)->visible = !visible;
	    break;
	  default:
	    gimage_set_component_visible (cw->gimage, cw->type, !visible);
	    break;
	  }
      else
	switch (cw->type)
	  {
	  case Auxillary:
	    GIMP_DRAWABLE(cw->channel)->visible = TRUE;
	    break;
	  default:
	    gimage_set_component_visible (cw->gimage, cw->type, TRUE);
	    break;
	  }

      channel_widget_eye_redraw (cw);

      list = g_slist_next (list);
    }
}


static void
channel_widget_channel_flush (GtkWidget *widget,
			      gpointer   client_data)
{
  ChannelWidget *channel_widget;
  int update_preview;

  channel_widget = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  /***  Sensitivity  ***/

  /*  If there is a floating selection...  */
  if (channelsD->floating_sel != NULL)
    {
      /*  to insensitive if this is an auxillary channel  */
      if (channel_widget->type == Auxillary)
	{
	  if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
	    gtk_widget_set_sensitive (channel_widget->list_item, FALSE);
	}
      /*  to sensitive otherwise  */
      else
	{
	  if (! GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
	    gtk_widget_set_sensitive (channel_widget->list_item, TRUE);
	}
    }
  else
    {
      /*  to insensitive if there is an active channel, and this is a component channel  */
      if (channel_widget->type != Auxillary && channelsD->active_channel != NULL)
	{
	  if (GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
	    gtk_widget_set_sensitive (channel_widget->list_item, FALSE);
	}
      /*  to sensitive otherwise  */
      else
	{
	  if (! GTK_WIDGET_IS_SENSITIVE (channel_widget->list_item))
	    gtk_widget_set_sensitive (channel_widget->list_item, TRUE);
	}
    }

  /***  Selection  ***/

  /*  If this is an auxillary channel  */
  if (channel_widget->type == Auxillary)
    {
      /*  select if this is the active channel  */
      if (channelsD->active_channel == (channel_widget->channel))
	channels_dialog_set_channel (channel_widget); 
      /*  unselect if this is not the active channel  */
      else
	channels_dialog_unset_channel (channel_widget);
    }
  else
    {
      /*  If the component is active, select. otherwise, deselect  */
      if (gimage_get_component_active (channel_widget->gimage, channel_widget->type))
	channels_dialog_set_channel (channel_widget);
      else
	channels_dialog_unset_channel (channel_widget);
    }

  switch (channel_widget->type)
    {
    case Auxillary:
      update_preview = !GIMP_DRAWABLE(channel_widget->channel)->preview_valid;
      break;
    default:
      update_preview = !gimage_preview_valid (channel_widget->gimage, channel_widget->type);
      break;
    }

  if (update_preview)
    gtk_widget_draw (channel_widget->channel_preview, NULL);
}


/*
 *  The new channel query dialog
 */

typedef struct _NewChannelOptions NewChannelOptions;

struct _NewChannelOptions {
  GtkWidget *query_box;
  GtkWidget *name_entry;
  ColorPanel *color_panel;

  int gimage_id;
  double opacity;
};

static char *channel_name = NULL;
static unsigned char channel_color[3] = {0, 0, 0};

static void
new_channel_query_ok_callback (GtkWidget *w,
			       gpointer   client_data)
{
  NewChannelOptions *options;
  Channel *new_channel;
  GImage *gimage;
  int i;

  options = (NewChannelOptions *) client_data;
  if (channel_name)
    g_free (channel_name);
  channel_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

  if ((gimage = gimage_get_ID (options->gimage_id)))
    {
      new_channel = channel_new (gimage->ID, gimage->width, gimage->height,
				 channel_name, (int) (255 * options->opacity) / 100,
				 options->color_panel->color);
      drawable_fill (GIMP_DRAWABLE(new_channel), TRANSPARENT_FILL);

      for (i = 0; i < 3; i++)
	channel_color[i] = options->color_panel->color[i];

      gimage_add_channel (gimage, new_channel, -1);
      gdisplays_flush ();
    }

  color_panel_free (options->color_panel);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
new_channel_query_cancel_callback (GtkWidget *w,
				   gpointer   client_data)
{
  NewChannelOptions *options;

  options = (NewChannelOptions *) client_data;
  color_panel_free (options->color_panel);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
new_channel_query_delete_callback (GtkWidget *w,
				   GdkEvent  *e,
				   gpointer client_data)
{
  new_channel_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
new_channel_query_scale_update (GtkAdjustment *adjustment,
				double        *scale_val)
{
  *scale_val = adjustment->value;
}

static void
channels_dialog_new_channel_query (int gimage_id)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", new_channel_query_ok_callback, NULL, NULL },
    { "Cancel", new_channel_query_cancel_callback, NULL, NULL }
  };
  GImage *gimage;
  NewChannelOptions *options;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *opacity_scale;
  GtkObject *opacity_scale_data;

  gimage = gimage_get_ID (gimage_id);

  /*  the new options structure  */
  options = (NewChannelOptions *) g_malloc (sizeof (NewChannelOptions));
  options->gimage_id = gimage_id;
  options->opacity = 50.0;
  options->color_panel = color_panel_new (channel_color, 48, 64);

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "new_channel_options", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options->query_box), "New Channel Options");
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (new_channel_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  /*  The table  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  label = gtk_label_new ("Channel name: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->name_entry, 75, 0);
  gtk_table_attach (GTK_TABLE (table), options->name_entry, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry), (channel_name ? channel_name : "New Channel"));
  gtk_widget_show (options->name_entry);

  /*  the opacity scale  */
  label = gtk_label_new ("Fill Opacity: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  gtk_widget_show (label);

  opacity_scale_data = gtk_adjustment_new (options->opacity, 0.0, 100.0, 1.0, 1.0, 0.0);
  opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (opacity_scale_data));
  gtk_table_attach (GTK_TABLE (table), opacity_scale, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 1, 1);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (opacity_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (opacity_scale_data), "value_changed",
		      (GtkSignalFunc) new_channel_query_scale_update,
		      &options->opacity);
  gtk_widget_show (opacity_scale);

  /*  the color panel  */
  gtk_table_attach (GTK_TABLE (table), options->color_panel->color_panel_widget,
		    2, 3, 0, 2, GTK_EXPAND, GTK_EXPAND, 4, 2);
  gtk_widget_show (options->color_panel->color_panel_widget);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


/*
 *  The edit channel attributes dialog
 */

typedef struct _EditChannelOptions EditChannelOptions;

struct _EditChannelOptions {
  GtkWidget *query_box;
  GtkWidget *name_entry;

  ChannelWidget *channel_widget;
  ColorPanel *color_panel;
  double opacity;
};

static void
edit_channel_query_ok_callback (GtkWidget *w,
				gpointer   client_data)
{
  EditChannelOptions *options;
  Channel *channel;
  int opacity;
  int update = FALSE;
  int i;

  options = (EditChannelOptions *) client_data;
  channel = options->channel_widget->channel;
  opacity = (int) (255 * options->opacity) / 100;

  /*  Set the new channel name  */
  if (GIMP_DRAWABLE(channel)->name)
    g_free (GIMP_DRAWABLE(channel)->name);
  GIMP_DRAWABLE(channel)->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));
  gtk_label_set (GTK_LABEL (options->channel_widget->label), GIMP_DRAWABLE(channel)->name);

  if (channel->opacity != opacity)
    {
      channel->opacity = opacity;
      update = TRUE;
    }
  for (i = 0; i < 3; i++)
    if (options->color_panel->color[i] != channel->col[i])
      {
	channel->col[i] = options->color_panel->color[i];
	update = TRUE;
      }

  if (update)
    {
      drawable_update (GIMP_DRAWABLE(channel), 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height);
      gdisplays_flush ();
    }

  color_panel_free (options->color_panel);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
edit_channel_query_cancel_callback (GtkWidget *w,
				    gpointer   client_data)
{
  EditChannelOptions *options;

  options = (EditChannelOptions *) client_data;

  color_panel_free (options->color_panel);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static gint
edit_channel_query_delete_callback (GtkWidget *w,
				    GdkEvent  *e,
				    gpointer client_data)
{
  edit_channel_query_cancel_callback (w, client_data);

  return TRUE;
}

static void
channels_dialog_edit_channel_query (ChannelWidget *channel_widget)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", edit_channel_query_ok_callback, NULL, NULL },
    { "Cancel", edit_channel_query_cancel_callback, NULL, NULL }
  };
  EditChannelOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *opacity_scale;
  GtkObject *opacity_scale_data;
  int i;

  /*  the new options structure  */
  options = (EditChannelOptions *) g_malloc (sizeof (EditChannelOptions));
  options->channel_widget = channel_widget;
  options->opacity = (double) channel_widget->channel->opacity / 2.55;
  for (i = 0; i < 3; i++) 
    channel_color[i] =  channel_widget->channel->col[i];

  options->color_panel = color_panel_new (channel_color, 48, 64);

  /*  the dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box), "edit_channel_atributes", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options->query_box), "Edit Channel Attributes");
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);

  /* deal with the wm close signal */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
		      GTK_SIGNAL_FUNC (edit_channel_query_delete_callback),
		      options);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options->query_box)->vbox), vbox, TRUE, TRUE, 0);

  /*  The table  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL, 0, 2, 2);
  label = gtk_label_new ("Channel name:");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  options->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), options->name_entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry), GIMP_DRAWABLE(channel_widget->channel)->name);
  gtk_widget_show (options->name_entry);
  gtk_widget_show (hbox);

  /*  the opacity scale  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL, 0, 2, 2);

  label = gtk_label_new ("Fill Opacity");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  opacity_scale_data = gtk_adjustment_new (options->opacity, 0.0, 100.0, 1.0, 1.0, 1.0);
  opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (opacity_scale_data));
  gtk_box_pack_start (GTK_BOX (hbox), opacity_scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_TOP);
  gtk_signal_connect (GTK_OBJECT (opacity_scale_data), "value_changed",
		      (GtkSignalFunc) new_channel_query_scale_update,
		      &options->opacity);
  gtk_widget_show (opacity_scale);
  gtk_widget_show (hbox);

  /*  the color panel  */
  gtk_table_attach (GTK_TABLE (table), options->color_panel->color_panel_widget,
		    1, 2, 0, 2, GTK_EXPAND, GTK_EXPAND, 4, 2);
  gtk_widget_show (options->color_panel->color_panel_widget);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}


