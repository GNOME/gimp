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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"
#include "widgets/widgets-types.h"

#include "paint-funcs/paint-funcs.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpdnd.h"

#include "channels-commands.h"
#include "channels-dialog.h"
#include "gdisplay.h"
#include "gimpui.h"
#include "layers-dialog.h"
#include "lc_dialog.h"
#include "ops_buttons.h"
#include "menus.h"

#include "appenv.h"
#include "drawable.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gimpchannel.h"
#include "gimplist.h"
#include "gimprc.h"
#include "temp_buf.h"
#include "undo.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/eye.xbm"
#include "pixmaps/channel.xbm"
#include "pixmaps/new.xpm"
#include "pixmaps/raise.xpm"
#include "pixmaps/lower.xpm"
#include "pixmaps/duplicate.xpm"
#include "pixmaps/delete.xpm"
#include "pixmaps/toselection.xpm"


#define COMPONENT_BASE_ID 0x10000000

typedef struct _ChannelsDialog ChannelsDialog;

struct _ChannelsDialog
{
  GtkWidget     *vbox;
  GtkWidget     *channel_list;
  GtkWidget     *scrolled_win;
  GtkWidget     *preview;
  GtkWidget     *ops_menu;
  GtkAccelGroup *accel_group;

  /*  state information  */
  GimpImage   *gimage;
  gint         image_width, image_height;
  gint         gimage_width, gimage_height;
  gdouble      ratio;

  gint         num_components;
  gint         base_type;
  ChannelType  components[MAX_CHANNELS];

  GimpChannel *active_channel;
  GimpLayer   *floating_sel;
  GSList      *channel_widgets;
};

typedef struct _ChannelWidget ChannelWidget;

struct _ChannelWidget
{
  GtkWidget *eye_widget;
  GtkWidget *clip_widget;
  GtkWidget *channel_preview;
  GtkWidget *list_item;
  GtkWidget *label;

  GdkPixmap *channel_pixmap;

  /*  state information  */
  GimpImage   *gimage;
  GimpChannel *channel;
  gint         width, height;

  ChannelType  type;
  gint         ID;
  gboolean     visited;

  GimpDropType drop_type;
};

/*  channels dialog widget routines  */
static void channels_dialog_preview_extents      (void);
static void channels_dialog_set_menu_sensitivity (void);
static void channels_dialog_set_channel          (ChannelWidget  *cw);
static void channels_dialog_unset_channel        (ChannelWidget  *cw);
static void channels_dialog_position_channel     (GimpChannel    *channel,
						  gint            position);
static void channels_dialog_add_channel          (GimpChannel    *channel);
static void channels_dialog_remove_channel       (ChannelWidget  *cw);

static gint channel_list_events                  (GtkWidget      *widget,
						  GdkEvent       *event);

/*  for (un)installing the menu accelarators  */
static void channels_dialog_map_callback         (GtkWidget      *widget,
						  gpointer        data);
static void channels_dialog_unmap_callback       (GtkWidget      *widget,
						  gpointer        data);

/*  ops buttons dnd callbacks  */
static gboolean channels_dialog_drag_new_channel_callback
                                                 (GtkWidget      *widget,
						  GdkDragContext *context,
						  gint            x,
						  gint            y,
						  guint           time);
static gboolean channels_dialog_drag_duplicate_channel_callback
                                                 (GtkWidget      *widget,
						  GdkDragContext *context,
						  gint            x,
						  gint            y,
						  guint           time);
static gboolean channels_dialog_drag_channel_to_sel_callback
                                                 (GtkWidget      *widget,
						  GdkDragContext *context,
						  gint            x,
						  gint            y,
						  guint           time);
static gboolean channels_dialog_drag_delete_channel_callback
                                                 (GtkWidget      *widget,
						  GdkDragContext *context,
						  gint            x,
						  gint            y,
						  guint           time);

/*  channel widget function prototypes  */
static ChannelWidget * channel_widget_get_ID     (GimpChannel    *channel);
static ChannelWidget * channel_widget_create     (GimpImage      *gimage,
						  GimpChannel    *channel,
						  ChannelType     channel_type);

static gboolean channel_widget_drag_motion_callback
                                                 (GtkWidget      *widget,
						  GdkDragContext *context,
						  gint            x,
						  gint            y,
						  guint           time);
static gboolean channel_widget_drag_drop_callback(GtkWidget      *widget,
						  GdkDragContext *context,
						  gint            x,
						  gint            y,
						  guint           time);
static void channel_widget_drag_begin_callback   (GtkWidget      *widget,
						  GdkDragContext *context);
static void channel_widget_drag_leave_callback   (GtkWidget      *widget,
						  GdkDragContext *context,
						  guint           time);
static void channel_widget_drag_indicator_callback
                                                 (GtkWidget      *widget,
						  gpointer        data);

static void channel_widget_drop_color            (GtkWidget      *widget,
						  const GimpRGB  *color,
						  gpointer        data);
static void channel_widget_draw_drop_indicator   (ChannelWidget  *cw,
						  GimpDropType    drop_type);
static void channel_widget_delete                (ChannelWidget  *cw);
static void channel_widget_select_update         (GtkWidget      *widget,
						  gpointer        data);
static gint channel_widget_button_events         (GtkWidget      *widget,
						  GdkEvent       *event,
						  gpointer        data);
static gint channel_widget_preview_events        (GtkWidget      *widget,
						  GdkEvent       *event,
						  gpointer        data);
static void channel_widget_preview_redraw        (ChannelWidget  *cw);
static void channel_widget_no_preview_redraw     (ChannelWidget  *cw);
static void channel_widget_eye_redraw            (ChannelWidget  *cw);
static void channel_widget_exclusive_visible     (ChannelWidget  *cw);
static void channel_widget_channel_flush         (GtkWidget      *widget,
						  gpointer        data);

static void   channels_dialog_new_channel_callback               (GtkWidget *widget,
								  gpointer   data);
static void   channels_dialog_raise_channel_callback             (GtkWidget *widget,
								  gpointer   data);
static void   channels_dialog_lower_channel_callback             (GtkWidget *widget,
								  gpointer   data);
static void   channels_dialog_duplicate_channel_callback         (GtkWidget *widget,
								  gpointer   data);
static void   channels_dialog_delete_channel_callback            (GtkWidget *widget,
								  gpointer   data);
static void   channels_dialog_channel_to_sel_callback            (GtkWidget *widget,
								  gpointer   data);
static void   channels_dialog_add_channel_to_sel_callback        (GtkWidget *widget,
								  gpointer   data);
static void   channels_dialog_sub_channel_from_sel_callback      (GtkWidget *widget,
								  gpointer   data);
static void   channels_dialog_intersect_channel_with_sel_callback(GtkWidget *widget,
								  gpointer   data);


/****************/
/*  Local data  */
/****************/

static ChannelsDialog *channelsD = NULL;

static GdkPixmap *eye_pixmap[]     = { NULL, NULL, NULL };
static GdkPixmap *channel_pixmap[] = { NULL, NULL, NULL };

static gint suspend_gimage_notify = 0;

/*  the ops buttons  */
static GtkSignalFunc to_selection_ext_callbacks[] = 
{ 
  channels_dialog_add_channel_to_sel_callback,          /* SHIFT */
  channels_dialog_sub_channel_from_sel_callback,        /* CTRL  */
  channels_dialog_intersect_channel_with_sel_callback,  /* MOD1  */
  channels_dialog_intersect_channel_with_sel_callback,  /* SHIFT + CTRL */
};

static OpsButton channels_ops_buttons[] =
{
  { new_xpm, channels_dialog_new_channel_callback, NULL,
    N_("New Channel"),
    "channels/dialogs/new_channel.html",
    NULL, 0 },
  { raise_xpm, channels_dialog_raise_channel_callback, NULL,
    N_("Raise Channel"),
    "channels/raise_channel.html",
    NULL, 0 },
  { lower_xpm, channels_dialog_lower_channel_callback, NULL,
    N_("Lower Channel"),
    "channels/lower_channel.html",
    NULL, 0 },
  { duplicate_xpm, channels_dialog_duplicate_channel_callback, NULL,
    N_("Duplicate Channel"),
    "channels/duplicate_channel.html",
    NULL, 0 },
  { toselection_xpm, channels_dialog_channel_to_sel_callback,
    to_selection_ext_callbacks,
    N_("Channel to Selection \n"
       "<Shift> Add          "
       "<Ctrl> Subtract      "
       "<Shift><Ctrl> Intersect"),
    "channels/channel_to_selection.html",
    NULL, 0 },
  { delete_xpm, channels_dialog_delete_channel_callback, NULL,
    N_("Delete Channel"),
    "channels/delete_channel.html",
    NULL, 0 },
  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

/*  dnd structures  */
static GtkTargetEntry channel_color_target_table[] =
{
  GIMP_TARGET_CHANNEL,
  GIMP_TARGET_COLOR
};
static guint n_channel_color_targets = (sizeof (channel_color_target_table) /
					sizeof (channel_color_target_table[0]));

static GtkTargetEntry channel_target_table[] =
{
  GIMP_TARGET_CHANNEL
};
static guint n_channel_targets = (sizeof (channel_target_table) /
				  sizeof (channel_target_table[0]));

static GtkTargetEntry component_target_table[] =
{
  GIMP_TARGET_COMPONENT
};
static guint n_component_targets = (sizeof (component_target_table) /
				    sizeof (component_target_table[0]));


/**************************************/
/*  Public channels dialog functions  */
/**************************************/

GtkWidget *
channels_dialog_create (void)
{
  GtkItemFactory *channels_factory;
  GtkWidget      *vbox;
  GtkWidget      *button_box;

  if (channelsD)
    return channelsD->vbox;

  channelsD = g_new (ChannelsDialog, 1);
  channelsD->preview         = NULL;
  channelsD->gimage          = NULL;
  channelsD->active_channel  = NULL;
  channelsD->floating_sel    = NULL;
  channelsD->channel_widgets = NULL;

  if (preview_size)
    {
      channelsD->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
      gtk_preview_size (GTK_PREVIEW (channelsD->preview),
			preview_size, preview_size);
    }

  /*  The main vbox  */
  channelsD->vbox = gtk_event_box_new ();

  gimp_help_set_help_data (channelsD->vbox, NULL,
			   "dialogs/channels/channels.html");

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (channelsD->vbox), vbox);

  /*  The channels commands pulldown menu  */
  channels_factory = menus_get_channels_factory ();

  channelsD->ops_menu    = channels_factory->widget;
  channelsD->accel_group = channels_factory->accel_group;

  /*  The channels listbox  */
  channelsD->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (channelsD->scrolled_win), 
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_set_usize (channelsD->scrolled_win, LIST_WIDTH, LIST_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), channelsD->scrolled_win, TRUE, TRUE, 2);

  channelsD->channel_list = gtk_list_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (channelsD->scrolled_win),
					 channelsD->channel_list);
  gtk_list_set_selection_mode (GTK_LIST (channelsD->channel_list),
			       GTK_SELECTION_MULTIPLE);
  gtk_signal_connect (GTK_OBJECT (channelsD->channel_list), "event",
		      (GtkSignalFunc) channel_list_events,
		      channelsD);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (channelsD->channel_list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (channelsD->scrolled_win)));
  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (channelsD->scrolled_win)->vscrollbar,
			  GTK_CAN_FOCUS);

  gtk_widget_show (channelsD->channel_list);
  gtk_widget_show (channelsD->scrolled_win);

  /*  The ops buttons  */
  button_box = ops_button_box_new (channels_ops_buttons, OPS_BUTTON_NORMAL);
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);

  /*  Drop to new  */
  gtk_drag_dest_set (channels_ops_buttons[0].widget,
                     GTK_DEST_DEFAULT_ALL,
                     channel_target_table, n_channel_targets,
                     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (channels_ops_buttons[0].widget), "drag_drop",
                      GTK_SIGNAL_FUNC (channels_dialog_drag_new_channel_callback),
                      NULL);

  /*  Drop to duplicate  */
  gtk_drag_dest_set (channels_ops_buttons[3].widget,
                     GTK_DEST_DEFAULT_ALL,
                     channel_target_table, n_channel_targets,
                     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (channels_ops_buttons[3].widget), "drag_drop",
                      GTK_SIGNAL_FUNC (channels_dialog_drag_duplicate_channel_callback),
		      NULL);

  /*  Drop to channel to selection  */
  gtk_drag_dest_set (channels_ops_buttons[4].widget,
                     GTK_DEST_DEFAULT_ALL,
                     channel_target_table, n_channel_targets,
                     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (channels_ops_buttons[4].widget), "drag_drop",
                      GTK_SIGNAL_FUNC (channels_dialog_drag_channel_to_sel_callback),
		      NULL);

  /*  Drop to trahcan  */
  gtk_drag_dest_set (channels_ops_buttons[5].widget,
                     GTK_DEST_DEFAULT_ALL,
                     channel_target_table, n_channel_targets,
                     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (channels_ops_buttons[5].widget), "drag_drop",
                      GTK_SIGNAL_FUNC (channels_dialog_drag_delete_channel_callback),
                      NULL);

  /*  Set up signals for map/unmap for the accelerators  */
  gtk_signal_connect (GTK_OBJECT (channelsD->vbox), "map",
		      (GtkSignalFunc) channels_dialog_map_callback,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (channelsD->vbox), "unmap",
		      (GtkSignalFunc) channels_dialog_unmap_callback,
		      NULL);

  gtk_widget_show (vbox);
  gtk_widget_show (channelsD->vbox);

  return channelsD->vbox;
}

void
channels_dialog_free (void)
{
  ChannelWidget *cw;

  if (!channelsD)
    return;

  suspend_gimage_notify++;
  /*  Free all elements in the channels listbox  */
  gtk_list_clear_items (GTK_LIST (channelsD->channel_list), 0, -1);
  suspend_gimage_notify--;

  while (channelsD->channel_widgets)
    {
      cw = (ChannelWidget *) channelsD->channel_widgets->data;

      channel_widget_delete (cw);
    }
  channelsD->channel_widgets = NULL;
  channelsD->active_channel  = NULL;
  channelsD->floating_sel    = NULL;

  if (channelsD->preview)
    gtk_object_sink (GTK_OBJECT (channelsD->preview));

  g_free (channelsD);
  channelsD = NULL;
}

void
channels_dialog_update (GimpImage* gimage)
{
  GimpChannel   *channel;
  ChannelWidget *cw;
  GList         *list; 
  GList         *item_list;

  if (!channelsD || channelsD->gimage == gimage)
    return;

  channelsD->gimage = gimage;

  suspend_gimage_notify++;
  /*  Free all elements in the channels listbox  */
  gtk_list_clear_items (GTK_LIST (channelsD->channel_list), 0, -1);
  suspend_gimage_notify--;

  while (channelsD->channel_widgets)
    {
      cw = (ChannelWidget *) channelsD->channel_widgets->data;

      channel_widget_delete (cw);
    }

  channelsD->channel_widgets = NULL;

  /*  Find the preview extents  */
  channels_dialog_preview_extents ();

  channelsD->active_channel = NULL;
  channelsD->floating_sel = NULL;

  /*  The image components  */
  item_list = NULL;
  switch ((channelsD->base_type = gimp_image_base_type (gimage)))
    {
    case RGB:
      cw = channel_widget_create (gimage, NULL, RED_CHANNEL);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[0] = RED_CHANNEL;

      cw = channel_widget_create (gimage, NULL, GREEN_CHANNEL);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[1] = GREEN_CHANNEL;

      cw = channel_widget_create (gimage, NULL, BLUE_CHANNEL);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[2] = BLUE_CHANNEL;

      channelsD->num_components = 3;
      break;

    case GRAY:
      cw = channel_widget_create (gimage, NULL, GRAY_CHANNEL);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[0] = GRAY_CHANNEL;

      channelsD->num_components = 1;
      break;

    case INDEXED:
      cw = channel_widget_create (gimage, NULL, INDEXED_CHANNEL);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[0] = INDEXED_CHANNEL;

      channelsD->num_components = 1;
      break;
    }

/* EEEK */
      cw = channel_widget_create (gimage, NULL, ALPHA_CHANNEL);
      channelsD->channel_widgets = g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
      channelsD->components[channelsD->num_components] = ALPHA_CHANNEL;

      channelsD->num_components++;

  /*  The auxillary image channels  */
  for (list = GIMP_LIST (gimage->channels)->list; 
       list; 
       list = g_list_next (list))
    {
      /*  create a channel list item  */
      channel = (GimpChannel *) list->data;

      cw = channel_widget_create (gimage, channel, AUXILLARY_CHANNEL);
      channelsD->channel_widgets =
	g_slist_append (channelsD->channel_widgets, cw);
      item_list = g_list_append (item_list, cw->list_item);
    }

  /*  get the index of the active channel  */
  if (item_list)
    gtk_list_insert_items (GTK_LIST (channelsD->channel_list), item_list, 0);
}

void
channels_dialog_flush (void)
{
  GimpImage     *gimage;
  GimpChannel   *channel;
  ChannelWidget *cw;
  GList         *list;
  GSList        *slist;
  gint           pos;

  if (!channelsD || !(gimage = channelsD->gimage))
    return;

  /*  Check if the gimage extents have changed  */
  if ((gimage->width != channelsD->gimage_width) ||
      (gimage->height != channelsD->gimage_height) ||
      (gimp_image_base_type (gimage) != channelsD->base_type))
    {
      channelsD->gimage = NULL;
      channels_dialog_update (gimage);
    }
  else
    {
      /*  Set all current channel widgets to visited = FALSE  */
      for (slist = channelsD->channel_widgets; 
	   slist; 
	   slist = g_slist_next (slist))
	{
	  cw = (ChannelWidget *) slist->data;

	  cw->visited = FALSE;
	}

      /*  Add any missing channels  */
      for (list = GIMP_LIST (gimage->channels)->list; 
	   list; 
	   list = g_list_next (list))
	{
	  channel = (GimpChannel *) list->data;
	  cw = channel_widget_get_ID (channel);

	  /*  If the channel isn't in the channel widget list, add it  */
	  if (cw == NULL)
	    {
	      /*  sets visited = TRUE  */
	      channels_dialog_add_channel (channel);
	    }
	  else
	    cw->visited = TRUE;
	}

      /*  Remove any extraneous auxillary channels  */
      for (slist = channelsD->channel_widgets; slist;)
	{
	  cw = (ChannelWidget *) slist->data;
	  slist = g_slist_next (slist);

	  if (cw->visited == FALSE && cw->type == AUXILLARY_CHANNEL)
	    {
	      /*  will only be true for auxillary channels  */
	      channels_dialog_remove_channel (cw);
	    }
	}

      /*  Switch positions of items if necessary  */
      pos = 0;
      for (list = GIMP_LIST (gimage->channels)->list; 
	   list; 
	   list = g_list_next (list))
	{
	  channel = (GimpChannel *) list->data;
	  channels_dialog_position_channel (channel, pos++);
	}

      /*  Set the active channel  */
      if (channelsD->active_channel != gimp_image_get_active_channel (gimage))
	channelsD->active_channel = gimp_image_get_active_channel (gimage);

      /*  set the menus if floating sel status has changed  */
      if (channelsD->floating_sel != gimage->floating_sel)
	channelsD->floating_sel = gimage->floating_sel;

      channels_dialog_set_menu_sensitivity ();

      gtk_container_foreach (GTK_CONTAINER (channelsD->channel_list),
			     channel_widget_channel_flush, NULL);
    }
}

void
channels_dialog_clear (void)
{
  if (!channelsD)
    return;

  suspend_gimage_notify++;
  gtk_list_clear_items (GTK_LIST (channelsD->channel_list), 0, -1);
  suspend_gimage_notify--;

  channelsD->gimage = NULL;
}

static void
channels_dialog_preview_extents (void)
{
  GimpImage *gimage;

  g_return_if_fail (channelsD);
  if (! (gimage = channelsD->gimage))
    return;

  channelsD->gimage_width  = gimage->width;
  channelsD->gimage_height = gimage->height;

  /*  Get the image width and height variables, based on the gimage  */
  if (gimage->width > gimage->height)
    channelsD->ratio = (gdouble) preview_size / (gdouble) gimage->width;
  else  
    channelsD->ratio = (gdouble) preview_size / (gdouble) gimage->height;

  if (preview_size)
    {
      channelsD->image_width  = RINT (channelsD->ratio * gimage->width);
      channelsD->image_height = RINT (channelsD->ratio * gimage->height);

      if (channelsD->image_width < 1)  channelsD->image_width  = 1;
      if (channelsD->image_height < 1) channelsD->image_height = 1;
    }
  else
    {
      channelsD->image_width  = channel_width;
      channelsD->image_height = channel_height;
    }
}

static void
channels_dialog_set_menu_sensitivity (void)
{
  ChannelWidget *cw;
  gint           fs_sens;
  gint           aux_sens;

  cw = channel_widget_get_ID (channelsD->active_channel);
  fs_sens = (channelsD->floating_sel != NULL);

  if (cw)
    aux_sens = (cw->type == AUXILLARY_CHANNEL);
  else
    aux_sens = FALSE;

#define SET_SENSITIVE(menu,condition) \
        menus_set_sensitive ("<Channels>/" menu, (condition) != 0)
#define SET_OPS_SENSITIVE(button,condition) \
        gtk_widget_set_sensitive (channels_ops_buttons[(button)].widget, \
                                 (condition) != 0)

  SET_SENSITIVE ("New Channel...", !fs_sens);
  SET_OPS_SENSITIVE (0, !fs_sens);

  SET_SENSITIVE ("Raise Channel", !fs_sens && aux_sens);
  SET_OPS_SENSITIVE (1, !fs_sens && aux_sens);

  SET_SENSITIVE ("Lower Channel", !fs_sens && aux_sens);
  SET_OPS_SENSITIVE (2, !fs_sens && aux_sens);

  SET_SENSITIVE ("Duplicate Channel", !fs_sens && aux_sens);
  SET_OPS_SENSITIVE (3, !fs_sens && aux_sens);

  SET_SENSITIVE ("Channel to Selection", aux_sens);
  SET_OPS_SENSITIVE (4, aux_sens);

  SET_SENSITIVE ("Add to Selection", aux_sens);
  SET_SENSITIVE ("Subtract from Selection", aux_sens);
  SET_SENSITIVE ("Intersect with Selection", aux_sens);

  SET_SENSITIVE ("Delete Channel", !fs_sens && aux_sens);
  SET_OPS_SENSITIVE (5, !fs_sens && aux_sens);

  SET_SENSITIVE ("Edit Channel Attributes...", !fs_sens && aux_sens);

#undef SET_OPS_SENSITIVE
#undef SET_SENSITIVE
}

static gint
channels_dialog_idle_set_channel_focus (gpointer data)
{
  gtk_widget_grab_focus (GTK_WIDGET (data));

  return FALSE;
}

static void
channels_dialog_set_channel (ChannelWidget *channel_widget)
{
  GtkStateType state;
  gint index;

  if (!channelsD || !channel_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  get the list item data  */
  state = channel_widget->list_item->state;

  if (channel_widget->type == AUXILLARY_CHANNEL)
    {
      /*  turn on the specified auxillary channel  */
      index = gimp_image_get_channel_index (channel_widget->gimage,
					    channel_widget->channel);
      if ((index >= 0) && (state != GTK_STATE_SELECTED))
	{
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item),
				    NULL);
	  gtk_list_select_item (GTK_LIST (channelsD->channel_list),
				index + channelsD->num_components);
	  /*  let dnd finish it's work before setting the focus  */
	  gtk_idle_add ((GtkFunction) channels_dialog_idle_set_channel_focus,
			channel_widget->list_item);
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item),
				    channel_widget);
	  /* channels_dialog_scroll_index (index + channelsD->num_components); */
	}
    }
  else
    {
      if (state != GTK_STATE_SELECTED)
	{
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item),
				    NULL);
	  switch (channel_widget->type)
	    {
	    case RED_CHANNEL:
	    case GRAY_CHANNEL:
	    case INDEXED_CHANNEL:
	      gtk_list_select_item (GTK_LIST (channelsD->channel_list), 0);
	      break;
	    case GREEN_CHANNEL:
	      gtk_list_select_item (GTK_LIST (channelsD->channel_list), 1);
	      break;
	    case BLUE_CHANNEL:
	      gtk_list_select_item (GTK_LIST (channelsD->channel_list), 2);
	      break;
	    case ALPHA_CHANNEL:
	      gtk_list_select_item (GTK_LIST (channelsD->channel_list), 3);
	      break;
	    case AUXILLARY_CHANNEL:
	      g_error ("error in %s at %d: this shouldn't happen.",
		       __FILE__, __LINE__);
	      break;
	    }

	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item),
				    channel_widget);
	  /* channels_dialog_scroll_index (0); */
	}
    }
  suspend_gimage_notify--;
}

static void
channels_dialog_unset_channel (ChannelWidget *channel_widget)
{
  GtkStateType state;
  gint index;

  if (!channelsD || !channel_widget)
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  get the list item data  */
  state = channel_widget->list_item->state;

  if (channel_widget->type == AUXILLARY_CHANNEL)
    {
      /*  turn off the specified auxillary channel  */
      index = gimp_image_get_channel_index (channel_widget->gimage,
					    channel_widget->channel);
      if ((index >= 0) && (state == GTK_STATE_SELECTED))
	{
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item),
				    NULL);
	  gtk_list_unselect_item (GTK_LIST (channelsD->channel_list),
				  index + channelsD->num_components);
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item),
				    channel_widget);
	}
    }
  else
    {
      if (state == GTK_STATE_SELECTED)
	{
	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item),
				    NULL);
	  switch (channel_widget->type)
	    {
	    case RED_CHANNEL:
	    case GRAY_CHANNEL:
	    case INDEXED_CHANNEL:
	      gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), 0);
	      break;
	    case GREEN_CHANNEL:
	      gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), 1);
	      break;
	    case BLUE_CHANNEL:
	      gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), 2);
	      break;
	    case ALPHA_CHANNEL:
	      gtk_list_unselect_item (GTK_LIST (channelsD->channel_list), 3);
	      break;
	    case AUXILLARY_CHANNEL:
	      g_error ("error in %s at %d: this shouldn't happen.",
		       __FILE__, __LINE__);
	      break;
	    }

	  gtk_object_set_user_data (GTK_OBJECT (channel_widget->list_item),
				    channel_widget);
	}
    }

  suspend_gimage_notify--;
}

static void
channels_dialog_position_channel (GimpChannel *channel,
				  gint         new_index)
{
  ChannelWidget *channel_widget;
  GList         *list = NULL;

  channel_widget = channel_widget_get_ID (channel);
  if (!channelsD || !channel_widget)
    return;

  if ((new_index + channelsD->num_components) ==
      g_slist_index (channelsD->channel_widgets, channel_widget))
    return;

  /*  Make sure the gimage is not notified of this change  */
  suspend_gimage_notify++;

  /*  Remove the channel from the dialog  */
  list = g_list_append (list, channel_widget->list_item);
  gtk_list_remove_items (GTK_LIST (channelsD->channel_list), list);
  channelsD->channel_widgets = g_slist_remove (channelsD->channel_widgets,
					       channel_widget);

  /*  Add it back at the proper index  */
  gtk_list_insert_items (GTK_LIST (channelsD->channel_list), list,
			 new_index + channelsD->num_components);
  channelsD->channel_widgets =
    g_slist_insert (channelsD->channel_widgets, channel_widget,
		    new_index + channelsD->num_components);

  /*   channels_dialog_scroll_index (new_index > 0 ?  */
  /* 				new_index + channelsD->num_components + 1 : */
  /* 				channelsD->num_components); */
  
  suspend_gimage_notify--;
}

static void
channels_dialog_add_channel (GimpChannel *channel)
{
  ChannelWidget *channel_widget;
  GimpImage     *gimage;
  GList         *item_list;
  gint           position;

  if (!channelsD || !channel || !(gimage = channelsD->gimage))
    return;

  item_list = NULL;

  channel_widget = channel_widget_create (gimage, channel, AUXILLARY_CHANNEL);
  item_list = g_list_append (item_list, channel_widget->list_item);

  position = gimp_image_get_channel_index (gimage, channel);
  channelsD->channel_widgets =
    g_slist_insert (channelsD->channel_widgets, channel_widget,
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
  ChannelWidget  *channel_widget;
  GdkEventButton *bevent;
  GtkWidget      *event_widget;

  event_widget = gtk_get_event_widget (event);

  if (GTK_IS_LIST_ITEM (event_widget))
    {
      channel_widget =
	(ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (event_widget));

      switch (event->type)
	{
	case GDK_BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;
	  if (bevent->button == 3)
	    {
	      gtk_menu_popup (GTK_MENU (channelsD->ops_menu),
			      NULL, NULL, NULL, NULL,
			      bevent->button, bevent->time);
	      return TRUE;
	    }
	  break;

	case GDK_2BUTTON_PRESS:
	  if (channel_widget->type == AUXILLARY_CHANNEL)
	    channels_edit_channel_query (channel_widget->channel);
	  return TRUE;

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
channels_dialog_map_callback (GtkWidget *widget,
			      gpointer   data)
{
  if (! channelsD)
    return;

  gtk_window_add_accel_group (GTK_WINDOW (lc_dialog->shell),
			      channelsD->accel_group);
}

static void
channels_dialog_unmap_callback (GtkWidget *widget,
				gpointer   data)
{
  if (! channelsD)
    return;

  gtk_window_remove_accel_group (GTK_WINDOW (lc_dialog->shell),
				 channelsD->accel_group);
}

/***********************************/
/*  callbacks exported to menus.c  */
/***********************************/

static void
channels_dialog_new_channel_callback (GtkWidget *widget,
				      gpointer   data)
{
  if (!channelsD || channelsD->gimage == NULL)
    return;

  channels_new_channel_query (channelsD->gimage);
}

static void
channels_dialog_raise_channel_callback (GtkWidget *widget,
					gpointer   data)
{
  GimpImage *gimage;

  if (!channelsD || !(gimage = channelsD->gimage))
    return;

  if (gimp_image_get_active_channel (gimage))
    {
      gimp_image_raise_channel (gimage, gimp_image_get_active_channel (gimage));
      gdisplays_flush ();
    }
}

static void
channels_dialog_lower_channel_callback (GtkWidget *widget,
					gpointer   data)
{
  GimpImage *gimage;

  if (!channelsD || !(gimage = channelsD->gimage))
    return;

  if (gimp_image_get_active_channel (gimage))
    {
      gimp_image_lower_channel (gimage, gimp_image_get_active_channel (gimage));
      gdisplays_flush ();
    }
}

static void
channels_dialog_duplicate_channel_callback (GtkWidget *widget,
					    gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;
  GimpChannel *new_channel;

  if (!channelsD || !(gimage = channelsD->gimage))
    return;

  if ((active_channel = gimp_image_get_active_channel (gimage)))
    {
      new_channel = gimp_channel_copy (active_channel, TRUE);
      gimp_image_add_channel (gimage, new_channel, -1);
      gdisplays_flush ();
    }
}

static void
channels_dialog_delete_channel_callback (GtkWidget *widget,
					 gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;

  if (!channelsD || !(gimage = channelsD->gimage))
    return;

  if ((active_channel = gimp_image_get_active_channel (gimage)))
    {
      gimp_image_remove_channel (gimage, active_channel);
      gdisplays_flush ();
    }
}

static void
channels_dialog_channel_to_sel_callback (GtkWidget *widget,
					 gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;

  if (!channelsD || !(gimage = channelsD->gimage))
    return;

  if ((active_channel = gimp_image_get_active_channel (gimage)))
    {
      gimage_mask_load (gimage, active_channel);
      gdisplays_flush ();
    }
}

static void
channels_dialog_add_channel_to_sel_callback (GtkWidget *widget,
					     gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;
  GimpChannel *new_channel;

  if (!channelsD || !(gimage = channelsD->gimage))
    return;

  if ((active_channel = gimp_image_get_active_channel (gimage)))
    {
      new_channel = gimp_channel_copy (gimp_image_get_mask (gimage), TRUE);
      gimp_channel_combine_mask (new_channel,
				 active_channel,
				 CHANNEL_OP_ADD, 
				 0, 0);  /* off x/y */
      gimage_mask_load (gimage, new_channel);
      gtk_object_unref (GTK_OBJECT (new_channel));
      gdisplays_flush ();
    }
}

static void
channels_dialog_sub_channel_from_sel_callback (GtkWidget *widget,
					       gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;
  GimpChannel *new_channel;

  if (!channelsD || !(gimage = channelsD->gimage))
    return;

  if ((active_channel = gimp_image_get_active_channel (gimage)))
    {
       new_channel = gimp_channel_copy (gimp_image_get_mask (gimage), TRUE);
       gimp_channel_combine_mask (new_channel,
				  active_channel,
				  CHANNEL_OP_SUB, 
				  0, 0);  /* off x/y */
      gimage_mask_load (gimage, new_channel);
      gtk_object_unref (GTK_OBJECT (new_channel));
      gdisplays_flush ();
    }
}

static void
channels_dialog_intersect_channel_with_sel_callback (GtkWidget *widget,
						     gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;
  GimpChannel *new_channel;

  if (!channelsD || !(gimage = channelsD->gimage))
    return;

  if ((active_channel = gimp_image_get_active_channel (gimage)))
    {
      new_channel = gimp_channel_copy (gimp_image_get_mask (gimage), TRUE);
      gimp_channel_combine_mask (new_channel,
				 active_channel,
				 CHANNEL_OP_INTERSECT, 
				 0, 0);  /* off x/y */
      gimage_mask_load (gimage, new_channel);
      gtk_object_unref (GTK_OBJECT (new_channel));
      gdisplays_flush ();
    }
}


/*******************************/
/*  ops buttons dnd callbacks  */
/*******************************/

static gboolean
channels_dialog_drag_new_channel_callback (GtkWidget      *widget,
					   GdkDragContext *context,
					   gint            x,
					   gint            y,
					   guint           time)
{
  GtkWidget *src_widget;
  gboolean return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      ChannelWidget *src;

      src
        = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (src_widget));

      if (src &&
          src->channel == channelsD->active_channel)
        {
          GimpChannel *channel;
          GimpImage   *gimage;
          gint         width, height;
          gint         off_x, off_y;

          gimage = channelsD->gimage;

          width  = gimp_drawable_width  (GIMP_DRAWABLE (src->channel));
          height = gimp_drawable_height (GIMP_DRAWABLE (src->channel));
          gimp_drawable_offsets (GIMP_DRAWABLE (src->channel), &off_x, &off_y);

          /*  Start a group undo  */
          undo_push_group_start (gimage, EDIT_PASTE_UNDO);

          channel = gimp_channel_new (gimage, width, height,
				      _("Empty Channel Copy"),
				      &src->channel->color);
          if (channel)
            {
              drawable_fill (GIMP_DRAWABLE (channel), TRANSPARENT_FILL);
              gimp_channel_translate (channel, off_x, off_y);
              gimp_image_add_channel (gimage, channel, -1);

              /*  End the group undo  */
              undo_push_group_end (gimage);

              gdisplays_flush ();
            } 
          else
            {
              g_message ("channels_dialog_drop_new_channel_callback():\n"
			 "could not allocate new channel");
            }

          return_val = TRUE;
        }
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static gboolean
channels_dialog_drag_duplicate_channel_callback (GtkWidget      *widget,
						 GdkDragContext *context,
						 gint            x,
						 gint            y,
						 guint           time)
{
  GtkWidget *src_widget;
  gboolean return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      ChannelWidget *src;

      src
        = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (src_widget));

      if (src &&
          src->channel == channelsD->active_channel)
        {
          channels_dialog_duplicate_channel_callback (widget, NULL);

          return_val = TRUE;
        }
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static gboolean
channels_dialog_drag_channel_to_sel_callback (GtkWidget      *widget,
					      GdkDragContext *context,
					      gint            x,
					      gint            y,
					      guint           time)
{
  GtkWidget *src_widget;
  gboolean return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      ChannelWidget *src;

      src
        = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (src_widget));

      if (src &&
          src->channel == channelsD->active_channel)
        {
          channels_dialog_channel_to_sel_callback (widget, NULL);

          return_val = TRUE;
        }
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static gboolean
channels_dialog_drag_delete_channel_callback (GtkWidget      *widget,
					      GdkDragContext *context,
					      gint            x,
					      gint            y,
					      guint           time)
{
  GtkWidget *src_widget;
  gboolean return_val = FALSE;

  if ((src_widget = gtk_drag_get_source_widget (context)))
    {
      ChannelWidget *src;

      src
        = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (src_widget));

      if (src &&
          src->channel == channelsD->active_channel)
        {
          channels_dialog_delete_channel_callback (widget, NULL);

          return_val = TRUE;
        }
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

/******************************/
/*  channel widget functions  */
/******************************/

static ChannelWidget *
channel_widget_get_ID (GimpChannel *channel)
{
  ChannelWidget *lw;
  GSList        *list;

  if (!channelsD)
    return NULL;

  for (list = channelsD->channel_widgets; list; list = g_slist_next (list))
    {
      lw = (ChannelWidget *) list->data;

      if (lw->channel == channel)
	return lw;
    }

  return NULL;
}

static ChannelWidget *
channel_widget_create (GimpImage   *gimage,
		       GimpChannel *channel,
		       ChannelType  type)
{
  ChannelWidget *channel_widget;
  GtkWidget     *list_item;
  GtkWidget     *hbox;
  GtkWidget     *vbox;
  GtkWidget     *alignment;

  list_item = gtk_list_item_new ();

  /*  Create the channel widget and add it to the list  */
  channel_widget = g_new (ChannelWidget, 1);
  channel_widget->gimage          = gimage;
  channel_widget->channel         = channel;
  channel_widget->channel_preview = NULL;
  channel_widget->channel_pixmap  = NULL;
  channel_widget->type            = type;
  channel_widget->ID              = ((type == AUXILLARY_CHANNEL) ?
				     GIMP_DRAWABLE (channel)->ID :
				     (COMPONENT_BASE_ID + type));
  channel_widget->list_item       = list_item;
  channel_widget->width           = -1;
  channel_widget->height          = -1;
  channel_widget->visited         = TRUE;
  channel_widget->drop_type       = GIMP_DROP_NONE;

  /*  Need to let the list item know about the channel_widget  */
  gtk_object_set_user_data (GTK_OBJECT (list_item), channel_widget);

  /*  Set up the list item observer  */
  gtk_signal_connect (GTK_OBJECT (list_item), "select",
		      GTK_SIGNAL_FUNC (channel_widget_select_update),
		      channel_widget);
  gtk_signal_connect (GTK_OBJECT (list_item), "deselect",
		      GTK_SIGNAL_FUNC (channel_widget_select_update),
		      channel_widget);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (list_item), vbox);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

  /*  Create the visibility toggle button  */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  channel_widget->eye_widget = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (channel_widget->eye_widget),
			 eye_width, eye_height);
  gtk_widget_set_events (channel_widget->eye_widget, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (channel_widget->eye_widget), "event",
		      GTK_SIGNAL_FUNC (channel_widget_button_events),
		      channel_widget);
  gtk_object_set_user_data (GTK_OBJECT (channel_widget->eye_widget),
			    channel_widget);
  gtk_container_add (GTK_CONTAINER (alignment), channel_widget->eye_widget);
  gtk_widget_show (channel_widget->eye_widget);
  gtk_widget_show (alignment);

  /*  The channel preview  */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  channel_widget->channel_preview = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (channel_widget->channel_preview),
			 channelsD->image_width, channelsD->image_height);
  gtk_widget_set_events (channel_widget->channel_preview, PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (channel_widget->channel_preview), "event",
		      GTK_SIGNAL_FUNC (channel_widget_preview_events),
		      channel_widget);
  gtk_object_set_user_data (GTK_OBJECT (channel_widget->channel_preview),
			    channel_widget);
  gtk_container_add (GTK_CONTAINER (alignment), channel_widget->channel_preview);
  gtk_widget_show (channel_widget->channel_preview);

  /*  The channel name label */
  switch (channel_widget->type)
    {
    case RED_CHANNEL:
      channel_widget->label = gtk_label_new (_("Red"));
      break;

    case GREEN_CHANNEL:
      channel_widget->label = gtk_label_new (_("Green"));
      break;

    case BLUE_CHANNEL:
      channel_widget->label = gtk_label_new (_("Blue"));
      break;

    case GRAY_CHANNEL:
      channel_widget->label = gtk_label_new (_("Gray"));
      break;

    case INDEXED_CHANNEL:
      channel_widget->label = gtk_label_new (_("Indexed"));
      break;

    case ALPHA_CHANNEL:
      channel_widget->label = gtk_label_new (_("Alpha"));
      break;

    case AUXILLARY_CHANNEL:
      channel_widget->label =
	gtk_label_new (gimp_object_get_name (GIMP_OBJECT (channel)));
      break;
    }

  gtk_box_pack_start (GTK_BOX (hbox), channel_widget->label, FALSE, FALSE, 2);
  gtk_widget_show (channel_widget->label);

  if (channel_widget->type == AUXILLARY_CHANNEL)
    {
      /*  dnd destination  */
      gtk_drag_dest_set (list_item,
			 GTK_DEST_DEFAULT_ALL,
			 channel_color_target_table, n_channel_color_targets,
			 GDK_ACTION_MOVE | GDK_ACTION_COPY);
      gimp_dnd_color_dest_set (list_item,
			       channel_widget_drop_color,
			       (gpointer) channel_widget);

      gtk_signal_connect (GTK_OBJECT (list_item), "drag_leave",
			  GTK_SIGNAL_FUNC (channel_widget_drag_leave_callback),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (list_item), "drag_motion",
			  GTK_SIGNAL_FUNC (channel_widget_drag_motion_callback),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (list_item), "drag_drop",
			  GTK_SIGNAL_FUNC (channel_widget_drag_drop_callback),
			  NULL);

      /*  re-paint the drop indicator after drawing the widget  */
      gtk_signal_connect_after (GTK_OBJECT (list_item), "draw",
				GTK_SIGNAL_FUNC (channel_widget_drag_indicator_callback),
				channel_widget);

      /*  dnd source  */
      gtk_drag_source_set (list_item,
			   GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
			   channel_target_table, n_channel_targets, 
			   GDK_ACTION_MOVE | GDK_ACTION_COPY);

      gtk_signal_connect (GTK_OBJECT (list_item), "drag_begin",
			  GTK_SIGNAL_FUNC (channel_widget_drag_begin_callback),
			  NULL);

      gtk_object_set_data (GTK_OBJECT (list_item), "gimp_channel",
			   (gpointer) channel);
    }
  else
    {
      /*  dnd source  */
      gtk_drag_source_set (list_item,
			   GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
			   component_target_table, n_component_targets, 
			   GDK_ACTION_MOVE | GDK_ACTION_COPY);

      gtk_object_set_data (GTK_OBJECT (list_item), "gimp_component",
			   (gpointer) gimage);
      gtk_object_set_data (GTK_OBJECT (list_item), "gimp_component_type",
			   (gpointer) channel_widget->type);
    }

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (list_item);

  gtk_widget_ref (channel_widget->list_item);

  return channel_widget;
}

static gboolean
channel_widget_drag_motion_callback (GtkWidget      *widget,
				     GdkDragContext *context,
				     gint            x,
				     gint            y,
				     guint           time)
{
  ChannelWidget *dest;
  gint           dest_index;
  GtkWidget     *src_widget;
  ChannelWidget *src = NULL;
  gint           src_index;
  gint           difference;

  GimpDropType  drop_type   = GIMP_DROP_NONE;
  GdkDragAction drag_action = GDK_ACTION_DEFAULT;
  gboolean      return_val  = FALSE;

  dest = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  if (dest)
    {
      src_widget = gtk_drag_get_source_widget (context);

      if (src_widget)
	{
	  src = (ChannelWidget *)
	    gtk_object_get_user_data (GTK_OBJECT (src_widget));

	  if (src &&
	      src->channel == channelsD->active_channel)
	    {
	      src_index  = gimp_image_get_channel_index (channelsD->gimage,
							 src->channel);
	      dest_index = gimp_image_get_channel_index (channelsD->gimage,
							 dest->channel);

	      difference = dest_index - src_index;

	      drop_type = ((y < widget->allocation.height / 2) ?
			   GIMP_DROP_ABOVE : GIMP_DROP_BELOW);

	      if (difference < 0 &&
		  drop_type == GIMP_DROP_BELOW)
		{
		  dest_index++;
		}
	      else if (difference > 0 &&
		       drop_type == GIMP_DROP_ABOVE)
		{
		  dest_index--;
		}

	      if (src_index != dest_index)
		{
		  drag_action = GDK_ACTION_MOVE;
		  return_val = TRUE;
		}
	      else
		{
		  drop_type = GIMP_DROP_NONE;
		}
	    }
	}

      if (!src)
	{
	  drag_action = GDK_ACTION_COPY;
	  return_val = TRUE;
	  drop_type = GIMP_DROP_NONE;
	}
    }

  gdk_drag_status (context, drag_action, time);

  if (dest && drop_type != dest->drop_type)
    {
      channel_widget_draw_drop_indicator (dest, dest->drop_type);
      channel_widget_draw_drop_indicator (dest, drop_type);
      dest->drop_type = drop_type;
    }

  return return_val;
}

static void
channel_widget_drag_begin_callback (GtkWidget      *widget,
				    GdkDragContext *context)
{
  ChannelWidget *channel_widget;

  channel_widget =
    (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  gimp_dnd_set_drawable_preview_icon (widget, context,
				      GIMP_DRAWABLE (channel_widget->channel));
}

typedef struct
{
  GimpImage   *gimage;
  GimpChannel *channel;
  gint         dest_index;
} ChannelDrop;

static gint
channel_widget_idle_drop_channel (gpointer data)
{
  ChannelDrop *cd;

  cd = (ChannelDrop *) data;

  gimp_image_position_channel (cd->gimage, cd->channel, cd->dest_index, TRUE);
  gdisplays_flush ();

  g_free (cd);

  return FALSE;
}

static gboolean
channel_widget_drag_drop_callback (GtkWidget      *widget,
				   GdkDragContext *context,
				   gint            x,
				   gint            y,
				   guint           time)
{
  ChannelWidget *dest;
  gint           dest_index;
  GtkWidget     *src_widget;
  ChannelWidget *src;
  gint           src_index;
  gint           difference;

  GimpDropType   drop_type  = GIMP_DROP_NONE;
  gboolean       return_val = FALSE;

  dest = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  if (dest &&
      (src_widget = gtk_drag_get_source_widget (context)))
    {
      src = (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (src_widget));

      if (src &&
          src->channel == channelsD->active_channel)
        {
          src_index  = gimp_image_get_channel_index (channelsD->gimage,
						     src->channel);
          dest_index = gimp_image_get_channel_index (channelsD->gimage,
						     dest->channel);

          difference = dest_index - src_index;

          drop_type = ((y < widget->allocation.height / 2) ?
                       GIMP_DROP_ABOVE : GIMP_DROP_BELOW);

          if (difference < 0 &&
              drop_type == GIMP_DROP_BELOW)
            {
              dest_index++;
            }
          else if (difference > 0 &&
                   drop_type == GIMP_DROP_ABOVE)
            {
              dest_index--;
            }

          if (src_index != dest_index)
            {
              ChannelDrop *cd;

              cd = g_new (ChannelDrop, 1);
              cd->gimage     = channelsD->gimage;
              cd->channel    = src->channel;
              cd->dest_index = dest_index;

              /*  let dnd finish it's work before changing the widget tree  */
              gtk_idle_add ((GtkFunction) channel_widget_idle_drop_channel, cd);
              return_val = TRUE;
            }
        }
    }

  if (dest)
    {
      if (!return_val)
	channel_widget_draw_drop_indicator (dest, dest->drop_type);

      dest->drop_type = GIMP_DROP_NONE;
    }

  gtk_drag_finish (context, return_val, FALSE, time);

  return return_val;
}

static void
channel_widget_drag_leave_callback (GtkWidget      *widget,
				    GdkDragContext *context,
				    guint           time)
{
  ChannelWidget *channel_widget;

  channel_widget =
    (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  channel_widget->drop_type = GIMP_DROP_NONE;
}

static void
channel_widget_drag_indicator_callback (GtkWidget *widget,
					gpointer   data)
{
  ChannelWidget *channel_widget;

  channel_widget =
    (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  channel_widget_draw_drop_indicator (channel_widget, channel_widget->drop_type);
}

static void
channel_widget_drop_color (GtkWidget     *widget,
			   const GimpRGB *color,
			   gpointer       data)
{
  ChannelWidget *channel_widget;
  GimpChannel   *channel;

  channel_widget = (ChannelWidget *) data;
  channel        = channel_widget->channel;

  if (gimp_rgba_distance (color, &channel->color) > 0.0001)
    {
      channel->color = *color;

      drawable_update (GIMP_DRAWABLE (channel),
		       0, 0,
		       GIMP_DRAWABLE (channel)->width,
		       GIMP_DRAWABLE (channel)->height);

      gdisplays_flush ();
    }
}

static void
channel_widget_draw_drop_indicator (ChannelWidget *channel_widget,
				    GimpDropType   drop_type)
{
  static GdkGC *gc = NULL;
  gint          y = 0;

  if (!gc)
    {
      GdkColor fg, bg;

      gc = gdk_gc_new (channel_widget->list_item->window);

      fg.pixel = 0xFFFFFFFF;
      bg.pixel = 0x00000000;

      gdk_gc_set_function (gc, GDK_INVERT);
      gdk_gc_set_foreground (gc, &fg);
      gdk_gc_set_background (gc, &bg);
      gdk_gc_set_line_attributes (gc, 5, GDK_LINE_SOLID,
                                  GDK_CAP_BUTT, GDK_JOIN_MITER);
    }

  if (drop_type != GIMP_DROP_NONE)
    {
      y = ((drop_type == GIMP_DROP_ABOVE) ?
           3 : channel_widget->list_item->allocation.height - 4);

      gdk_draw_line (channel_widget->list_item->window, gc,
                     2, y, channel_widget->list_item->allocation.width - 3, y);
    }
}

static void
channel_widget_delete (ChannelWidget *channel_widget)
{
  if (channel_widget->channel_pixmap)
    gdk_pixmap_unref (channel_widget->channel_pixmap);

  /*  Remove the channel widget from the list  */
  channelsD->channel_widgets = g_slist_remove (channelsD->channel_widgets,
					       channel_widget);

  /*  Release the widget  */
  gtk_widget_unref (channel_widget->list_item);
  g_free (channel_widget);
}

static void
channel_widget_select_update (GtkWidget *widget,
			      gpointer   data)
{
  ChannelWidget *channel_widget;

  if ((channel_widget = (ChannelWidget *) data) == NULL)
    return;

  if (suspend_gimage_notify == 0)
    {
      if (channel_widget->type == AUXILLARY_CHANNEL)
	{
	  if (widget->state == GTK_STATE_SELECTED)
	    /*  set the gimage's active channel to be this channel  */
	    gimp_image_set_active_channel (channel_widget->gimage,
					   channel_widget->channel);
	  else
	    /*  unset the gimage's active channel  */
	    gimp_image_unset_active_channel (channel_widget->gimage);

	  gdisplays_flush ();
	}
      else if (channel_widget->type != AUXILLARY_CHANNEL)
	{
	  if (widget->state == GTK_STATE_SELECTED)
	    gimp_image_set_component_active (channel_widget->gimage,
					     channel_widget->type, TRUE);
	  else
	    gimp_image_set_component_active (channel_widget->gimage,
					     channel_widget->type, FALSE);
	}
    }
}

static gint
channel_widget_button_events (GtkWidget *widget,
			      GdkEvent  *event,
			      gpointer   data)
{
  ChannelWidget  *channel_widget;
  GimpChannel    *channel;
  GtkWidget      *event_widget;
  GdkEventButton *bevent;
  gint            return_val;
  gint            visible;
  gint            width, height;

  static gboolean   button_down = FALSE;
  static GtkWidget *click_widget = NULL;
  static gint       old_state;
  static gint       exclusive;

  channel_widget =
    (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));
  return_val = FALSE;

  channel = channel_widget->channel;

  switch (channel_widget->type)
    {
    case AUXILLARY_CHANNEL:
      visible = gimp_drawable_get_visible (GIMP_DRAWABLE (channel));
      width   = GIMP_DRAWABLE (channel_widget->channel)->width;
      height  = GIMP_DRAWABLE (channel_widget->channel)->height;
      break;
    default:
      visible = gimp_image_get_component_visible (channel_widget->gimage,
						  channel_widget->type);
      width   = channel_widget->gimage->width;
      height  = channel_widget->gimage->height;
      break;
    }

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == channel_widget->eye_widget)
	channel_widget_eye_redraw (channel_widget);
      break;

    case GDK_BUTTON_PRESS:
      return_val = TRUE;
      bevent = (GdkEventButton *) event;

      if (bevent->button == 3)
	{
	  gtk_menu_popup (GTK_MENU (channelsD->ops_menu),
			  NULL, NULL, NULL, NULL,
			  3, bevent->time);
	  return TRUE;
	}

      button_down = TRUE;
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
	      if (channel_widget->type == AUXILLARY_CHANNEL)
		gimp_drawable_set_visible (GIMP_DRAWABLE (channel), ! visible);
	      else
		gimp_image_set_component_visible (channel_widget->gimage,
						  channel_widget->type,
						  !visible);
	      channel_widget_eye_redraw (channel_widget);
	    }
	}
      break;

    case GDK_BUTTON_RELEASE:
      return_val = TRUE;

      button_down = FALSE;
      gtk_grab_remove (click_widget);

      if (widget == channel_widget->eye_widget)
	{
	  if (exclusive)
	    {
	      gdisplays_update_area (channel_widget->gimage, 0, 0, width, height);
	      gdisplays_flush ();
	    }
	  else if (old_state != visible)
	    {
	      gdisplays_update_area (channel_widget->gimage, 0, 0, width, height);
	      gdisplays_flush ();
	    }
	}
      break;

    case GDK_LEAVE_NOTIFY:
      event_widget = gtk_get_event_widget (event);

      if (button_down && (event_widget == click_widget))
	{
	  /* the user moved the cursor out of the widget before
             releasing the button -> cancel the button_press */ 
	  button_down = FALSE;

	  if (widget == channel_widget->eye_widget)
	    {
	      if (exclusive)
		{
		  channel_widget_exclusive_visible (channel_widget);
		}
	      else
		{
		  if (channel_widget->type == AUXILLARY_CHANNEL)
		    gimp_drawable_set_visible (GIMP_DRAWABLE (channel),
					       ! visible);
		  else
		    gimp_image_set_component_visible (channel_widget->gimage,
						      channel_widget->type,
						      !visible);
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
			       GdkEvent  *event,
			       gpointer   data)
{
  ChannelWidget  *channel_widget;
  GdkEventExpose *eevent;
  GdkEventButton *bevent;
  gboolean        valid;

  valid = FALSE;

  channel_widget =
    (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 3)
	{
	  gtk_menu_popup (GTK_MENU (channelsD->ops_menu),
			  NULL, NULL, NULL, NULL,
			  3, bevent->time);
	  return TRUE;
	}
      break;

    case GDK_EXPOSE:
      if (!preview_size)
	channel_widget_no_preview_redraw (channel_widget);
      else
	{
	  switch (channel_widget->type)
	    {
	    case AUXILLARY_CHANNEL:
	      valid = GIMP_DRAWABLE (channel_widget->channel)->preview_valid;
	      break;
	    default:
	      valid = gimp_image_preview_valid (channel_widget->gimage);
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
  TempBuf *preview_buf;
  gint     width, height;
  gint     channel;

  /*  allocate the channel widget pixmap  */
  if (! channel_widget->channel_pixmap)
    channel_widget->channel_pixmap =
      gdk_pixmap_new (channel_widget->channel_preview->window,
		      channelsD->image_width,
		      channelsD->image_height,
		      -1);

  /*  determine width and height  */
  switch (channel_widget->type)
    {
    case AUXILLARY_CHANNEL:
      width  = GIMP_DRAWABLE (channel_widget->channel)->width;
      height = GIMP_DRAWABLE (channel_widget->channel)->height;
      channel_widget->width  = RINT (channelsD->ratio * width);
      channel_widget->height = RINT (channelsD->ratio * height);

      if (channelsD->ratio > 1.0) /*  Preview is scaling up!  */
	{
	  preview_buf =
	    gimp_viewable_get_preview (GIMP_VIEWABLE (channel_widget->channel),
				       channelsD->gimage_width,
				       channelsD->gimage_height);

	  preview_buf = temp_buf_scale (preview_buf,
					channel_widget->width,
					channel_widget->height);
	}
      else
	{
	  preview_buf =
	    gimp_viewable_get_preview (GIMP_VIEWABLE (channel_widget->channel),
				       channel_widget->width,
				       channel_widget->height);
	}
      break;

    default:
      width  = channel_widget->gimage->width;
      height = channel_widget->gimage->height;
      channel_widget->width  = RINT (channelsD->ratio * width);
      channel_widget->height = RINT (channelsD->ratio * height);

      if (channelsD->ratio > 1.0) /*  Preview is scaling up!  */
	{
	  preview_buf =
	    gimp_viewable_get_preview (GIMP_VIEWABLE (channel_widget->gimage),
				       width,
				       height);

	  preview_buf = temp_buf_scale (preview_buf,
					channel_widget->width,
					channel_widget->height);
	}
      else
	{
	  preview_buf =
	    gimp_viewable_get_preview (GIMP_VIEWABLE (channel_widget->gimage),
				       channel_widget->width,
				       channel_widget->height);
	}
      break;
    }

  switch (channel_widget->type)
    {
    case RED_CHANNEL:       channel = RED_PIX; break;
    case GREEN_CHANNEL:     channel = GREEN_PIX; break;
    case BLUE_CHANNEL:      channel = BLUE_PIX; break;
    case GRAY_CHANNEL:      channel = GRAY_PIX; break;
    case INDEXED_CHANNEL:   channel = INDEXED_PIX; break;
    case ALPHA_CHANNEL:     channel = ALPHA_PIX; break;
    case AUXILLARY_CHANNEL: channel = -1; break;
    default:                channel = -1; break;
    }

  render_preview (preview_buf,
		  channelsD->preview,
		  channelsD->image_width,
		  channelsD->image_height,
		  channel);

  gtk_preview_put (GTK_PREVIEW (channelsD->preview),
		   channel_widget->channel_pixmap,
		   channel_widget->channel_preview->style->black_gc,
		   0, 0, 0, 0, 
		   channelsD->image_width, channelsD->image_height);

  /*  make sure the image has been transfered completely to the pixmap before
   *  we use it again...
   */
  gdk_flush ();

  if (channelsD->ratio > 1.0)
    temp_buf_free (preview_buf);
}

static void
channel_widget_no_preview_redraw (ChannelWidget *channel_widget)
{
  GdkPixmap     *pixmap;
  GdkPixmap    **pixmap_normal;
  GdkPixmap    **pixmap_selected;
  GdkPixmap    **pixmap_insensitive;
  GdkColor      *color;
  GtkWidget     *widget;
  GtkStateType   state;
  gchar         *bits;
  gint           width, height;

  state = channel_widget->list_item->state;

  pixmap_normal      = &channel_pixmap[NORMAL];
  pixmap_selected    = &channel_pixmap[SELECTED];
  pixmap_insensitive = &channel_pixmap[INSENSITIVE];
  widget             = channel_widget->channel_preview;
  bits               = (gchar *) channel_bits;
  width              = channel_width;
  height             = channel_height;

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
  GdkPixmap    *pixmap;
  GdkColor     *color;
  GtkStateType  state;
  gboolean      visible;

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
    case AUXILLARY_CHANNEL:
      visible =
	gimp_drawable_get_visible (GIMP_DRAWABLE (channel_widget->channel));
      break;
    default:
      visible = gimp_image_get_component_visible (channel_widget->gimage,
						  channel_widget->type);
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
  GSList        *list;
  ChannelWidget *cw;
  gboolean       visible = FALSE;

  if (!channelsD)
    return;

  /*  First determine if _any_ other channel widgets are set to visible  */
  for (list = channelsD->channel_widgets; list;  list = g_slist_next (list))
    {
      cw = (ChannelWidget *) list->data;

      if (cw != channel_widget)
	{
	  switch (cw->type)
	    {
	    case AUXILLARY_CHANNEL:
	      visible |= gimp_drawable_get_visible (GIMP_DRAWABLE (cw->channel));
	      break;
	    default:
	      visible |= gimp_image_get_component_visible (cw->gimage, cw->type);
	      break;
	    }
	}
    }

  /*  Now, toggle the visibility for all channels except the specified one  */
  for (list = channelsD->channel_widgets; list;  list = g_slist_next (list))
    {
      cw = (ChannelWidget *) list->data;

      if (cw != channel_widget)
	switch (cw->type)
	  {
	  case AUXILLARY_CHANNEL:
	    gimp_drawable_set_visible (GIMP_DRAWABLE (cw->channel), ! visible);
	    break;
	  default:
	    gimp_image_set_component_visible (cw->gimage, cw->type, !visible);
	    break;
	  }
      else
	switch (cw->type)
	  {
	  case AUXILLARY_CHANNEL:
	    gimp_drawable_set_visible (GIMP_DRAWABLE (cw->channel), TRUE);
	    break;
	  default:
	    gimp_image_set_component_visible (cw->gimage, cw->type, TRUE);
	    break;
	  }

      channel_widget_eye_redraw (cw);
    }
}

static void
channel_widget_channel_flush (GtkWidget *widget,
			      gpointer   data)
{
  ChannelWidget *channel_widget;
  gboolean       update_preview = FALSE;

  channel_widget =
    (ChannelWidget *) gtk_object_get_user_data (GTK_OBJECT (widget));

  /*  Set sensitivity  */

  /*  If there is a floating selection...  */
  if (channelsD->floating_sel != NULL)
    {
      /*  to insensitive if this is an auxillary channel  */
      if (channel_widget->type == AUXILLARY_CHANNEL)
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
      /*  to insensitive if there is an active channel,
       *  and this is a component channel
       */
      if (channel_widget->type != AUXILLARY_CHANNEL &&
	  channelsD->active_channel != NULL)
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

  /*  Set selection  */

  /*  If this is an auxillary channel  */
  if (channel_widget->type == AUXILLARY_CHANNEL)
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
      if (gimp_image_get_component_active (channel_widget->gimage,
					   channel_widget->type))
	channels_dialog_set_channel (channel_widget);
      else
	channels_dialog_unset_channel (channel_widget);
    }

  switch (channel_widget->type)
    {
    case AUXILLARY_CHANNEL:
      update_preview = ! GIMP_DRAWABLE (channel_widget->channel)->preview_valid;
      break;
    default:
      update_preview = ! gimp_image_preview_valid (channel_widget->gimage);
      break;
    }

  if (update_preview)
    gtk_widget_queue_draw (channel_widget->channel_preview);
}
