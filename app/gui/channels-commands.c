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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-mask-select.h"
#include "core/gimplist.h"

#include "display/gimpdisplay-foreach.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "channels-commands.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   channels_opacity_update (GtkAdjustment   *adjustment,
                                       gpointer         data);
static void   channels_color_changed  (GimpColorButton *button,
                                       gpointer         data);


#define return_if_no_image(gimage) \
  gimage = (GimpImage *) gimp_widget_get_callback_context (widget); \
  if (! GIMP_IS_IMAGE (gimage)) \
    gimage = gimp_context_get_image (gimp_get_user_context (GIMP (data))); \
  if (! gimage) \
    return

#define return_if_no_channel(gimage,channel) \
  return_if_no_image (gimage); \
  channel = gimp_image_get_active_channel (gimage); \
  if (! channel) \
    return


/*  public functions  */

void
channels_new_channel_cmd_callback (GtkWidget *widget,
				   gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage);

  channels_new_channel_query (gimage, NULL);
}

void
channels_raise_channel_cmd_callback (GtkWidget *widget,
				     gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;
  return_if_no_channel (gimage, active_channel);

  gimp_image_raise_channel (gimage, active_channel);
  gdisplays_flush ();
}

void
channels_lower_channel_cmd_callback (GtkWidget *widget,
				     gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;
  return_if_no_channel (gimage, active_channel);

  gimp_image_lower_channel (gimage, active_channel);
  gdisplays_flush ();
}

void
channels_duplicate_channel_cmd_callback (GtkWidget *widget,
					 gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;
  GimpChannel *new_channel;
  return_if_no_channel (gimage, active_channel);

  new_channel = gimp_channel_copy (active_channel,
                                   G_TYPE_FROM_INSTANCE (active_channel),
                                   TRUE);
  gimp_image_add_channel (gimage, new_channel, -1);
  gdisplays_flush ();
}

void
channels_delete_channel_cmd_callback (GtkWidget *widget,
				      gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;
  return_if_no_channel (gimage, active_channel);

  gimp_image_remove_channel (gimage, active_channel);
  gdisplays_flush ();
}

static void
channels_channel_to_sel (GtkWidget      *widget,
                         gpointer        data,
                         GimpChannelOps  op)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;
  return_if_no_channel (gimage, active_channel);

  gimp_image_mask_select_channel (gimage,
                                  active_channel,
                                  0, 0,
                                  op,
                                  FALSE, 0, 0);
  gdisplays_flush ();
}

void
channels_channel_to_sel_cmd_callback (GtkWidget *widget,
				      gpointer   data)
{
  channels_channel_to_sel (widget, data, GIMP_CHANNEL_OP_REPLACE);
}

void
channels_add_channel_to_sel_cmd_callback (GtkWidget *widget,
					  gpointer   data)
{
  channels_channel_to_sel (widget, data, GIMP_CHANNEL_OP_ADD);
}

void
channels_sub_channel_from_sel_cmd_callback (GtkWidget *widget,
					    gpointer   data)
{
  channels_channel_to_sel (widget, data, GIMP_CHANNEL_OP_SUBTRACT);
}

void
channels_intersect_channel_with_sel_cmd_callback (GtkWidget *widget,
						  gpointer   data)
{
  channels_channel_to_sel (widget, data, GIMP_CHANNEL_OP_INTERSECT);
}

void
channels_edit_channel_attributes_cmd_callback (GtkWidget *widget,
					       gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *active_channel;
  return_if_no_channel (gimage, active_channel);

  channels_edit_channel_query (active_channel);
}


/**********************************/
/*  The new channel query dialog  */
/**********************************/

typedef struct _NewChannelOptions NewChannelOptions;

struct _NewChannelOptions
{
  GtkWidget  *query_box;
  GtkWidget  *name_entry;
  GtkWidget  *color_panel;

  GimpImage  *gimage;
};

static gchar   *channel_name  = NULL;
static GimpRGB  channel_color = { 0.0, 0.0, 0.0, 0.5 };


static void
new_channel_query_ok_callback (GtkWidget *widget,
			       gpointer   data)
{
  NewChannelOptions *options;
  GimpChannel       *new_channel;
  GimpImage         *gimage;

  options = (NewChannelOptions *) data;

  if (channel_name)
    g_free (channel_name);
  channel_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

  if ((gimage = options->gimage))
    {
      gimp_color_button_get_color (GIMP_COLOR_BUTTON (options->color_panel),
				   &channel_color);
      new_channel = gimp_channel_new (gimage, gimage->width, gimage->height,
				      channel_name,
				      &channel_color);

      gimp_drawable_fill_by_type (GIMP_DRAWABLE (new_channel),
				  gimp_get_user_context (gimage->gimp),
				  GIMP_TRANSPARENT_FILL);

      gimp_image_add_channel (gimage, new_channel, -1);
      gdisplays_flush ();
    }

  gtk_widget_destroy (options->query_box);
}

void
channels_new_channel_query (GimpImage   *gimage,
                            GimpChannel *template)
{
  NewChannelOptions *options;
  GtkWidget         *hbox;
  GtkWidget         *vbox;
  GtkWidget         *table;
  GtkObject         *opacity_scale_data;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (! template || GIMP_IS_CHANNEL (template));

  if (template)
    {
      GimpChannel *new_channel;
      gint         width, height;
      gint         off_x, off_y;

      width  = gimp_drawable_width  (GIMP_DRAWABLE (template));
      height = gimp_drawable_height (GIMP_DRAWABLE (template));
      gimp_drawable_offsets (GIMP_DRAWABLE (template), &off_x, &off_y);

      undo_push_group_start (gimage, EDIT_PASTE_UNDO_GROUP);

      new_channel = gimp_channel_new (gimage,
                                      width, height,
                                      _("Empty Channel Copy"),
                                      &template->color);

      gimp_drawable_fill_by_type (GIMP_DRAWABLE (new_channel),
                                  gimp_get_user_context (gimage->gimp),
                                  GIMP_TRANSPARENT_FILL);
      gimp_channel_translate (new_channel, off_x, off_y);
      gimp_image_add_channel (gimage, new_channel, -1);

      undo_push_group_end (gimage);

      gdisplays_flush ();
      return;
    }

  /*  the new options structure  */
  options = g_new (NewChannelOptions, 1);
  options->gimage      = gimage;
  options->color_panel = gimp_color_panel_new (_("New Channel Color"),
					       &channel_color,
					       GIMP_COLOR_AREA_LARGE_CHECKS,
					       48, 64);
  
  /*  The dialog  */
  options->query_box =
    gimp_dialog_new (_("New Channel Options"), "new_channel_options",
		     gimp_standard_help_func,
		     "dialogs/channels/new_channel.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     GTK_STOCK_CANCEL, gtk_widget_destroy,
		     NULL, 1, NULL, FALSE, TRUE,

		     GTK_STOCK_OK, new_channel_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,

		     NULL);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     hbox);

  /*  The vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  /*  The table  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The name entry  */
  options->name_entry = gtk_entry_new ();
  gtk_widget_set_size_request (options->name_entry, 150, -1);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      (channel_name ? channel_name : _("New Channel")));

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Channel Name:"), 1.0, 0.5,
			     options->name_entry, 2, FALSE);

  /*  The opacity scale  */
  opacity_scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
					     _("Fill Opacity:"), -1, 50,
					     channel_color.a * 100.0,
					     0.0, 100.0, 1.0, 10.0, 1,
					     TRUE, 0.0, 0.0,
					     NULL, NULL);

  g_signal_connect (G_OBJECT (opacity_scale_data), "value_changed",
		    G_CALLBACK (channels_opacity_update),
		    options->color_panel);

  /*  The color panel  */
  gtk_box_pack_start (GTK_BOX (hbox), options->color_panel,
		      TRUE, TRUE, 0);
  gtk_widget_show (options->color_panel);

  g_signal_connect (G_OBJECT (options->color_panel), "color_changed",
		    G_CALLBACK (channels_color_changed),
		    opacity_scale_data);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (options->query_box);
}

/****************************************/
/*  The edit channel attributes dialog  */
/****************************************/

typedef struct _EditChannelOptions EditChannelOptions;

struct _EditChannelOptions
{
  GtkWidget     *query_box;
  GtkWidget     *name_entry;
  GtkWidget     *color_panel;

  GimpChannel   *channel;
  GimpImage     *gimage;
};

static void
edit_channel_query_ok_callback (GtkWidget *widget,
				gpointer   data)
{
  EditChannelOptions *options;
  GimpChannel        *channel;

  options = (EditChannelOptions *) data;
  channel = options->channel;

  if (options->gimage)
    {
      const gchar *new_name;
      GimpRGB      color;

      new_name = gtk_entry_get_text (GTK_ENTRY (options->name_entry));

      if (strcmp (new_name, gimp_object_get_name (GIMP_OBJECT (channel))))
        {
          undo_push_item_rename (options->gimage, GIMP_ITEM (channel));

          gimp_object_set_name (GIMP_OBJECT (channel), new_name);
        }

      gimp_color_button_get_color (GIMP_COLOR_BUTTON (options->color_panel),
				   &color);

      if (gimp_rgba_distance (&color, &channel->color) > 0.0001)
	{
	  gimp_channel_set_color (channel, &color);

	  gdisplays_flush ();
	}
    }

  gtk_widget_destroy (options->query_box);
}

void
channels_edit_channel_query (GimpChannel *channel)
{
  EditChannelOptions *options;
  GtkWidget          *hbox;
  GtkWidget          *vbox;
  GtkWidget          *table;
  GtkObject          *opacity_scale_data;

  options = g_new0 (EditChannelOptions, 1);

  options->channel = channel;
  options->gimage  = gimp_item_get_image (GIMP_ITEM (channel));

  channel_color = channel->color;

  options->color_panel = gimp_color_panel_new (_("Edit Channel Color"),
					       &channel_color,
					       GIMP_COLOR_AREA_LARGE_CHECKS,
					       48, 64);

  /*  The dialog  */
  options->query_box =
    gimp_dialog_new (_("Edit Channel Attributes"), "edit_channel_attributes",
		     gimp_standard_help_func,
		     "dialogs/channels/edit_channel_attributes.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     GTK_STOCK_CANCEL, gtk_widget_destroy,
		     NULL, 1, NULL, FALSE, TRUE,

		     GTK_STOCK_OK, edit_channel_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,

		     NULL);

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  g_signal_connect_object (G_OBJECT (channel), "removed",
			   G_CALLBACK (gtk_widget_destroy),
			   G_OBJECT (options->query_box),
			   G_CONNECT_SWAPPED);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     hbox);

  /*  The vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  /*  The table  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The name entry  */
  options->name_entry = gtk_entry_new ();
  gtk_widget_set_size_request (options->name_entry, 150, -1);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      gimp_object_get_name (GIMP_OBJECT (channel)));

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Channel Name:"), 1.0, 0.5,
			     options->name_entry, 2, FALSE);

  /*  The opacity scale  */
  opacity_scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
					     _("Fill Opacity:"), -1, 50,
					     channel_color.a * 100.0,
					     0.0, 100.0, 1.0, 10.0, 1,
					     TRUE, 0.0, 0.0,
					     NULL, NULL);

  g_signal_connect (G_OBJECT (opacity_scale_data), "value_changed",
		    G_CALLBACK (channels_opacity_update),
		    options->color_panel);

  /*  The color panel  */
  gtk_box_pack_start (GTK_BOX (hbox), options->color_panel,
		      TRUE, TRUE, 0);
  gtk_widget_show (options->color_panel);

  g_signal_connect (G_OBJECT (options->color_panel), "color_changed",
		    G_CALLBACK (channels_color_changed),
		    opacity_scale_data);		      

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (options->query_box);
}

void
channels_menu_update (GtkItemFactory *factory,
                      gpointer        data)
{
  GimpImage   *gimage;
  GimpChannel *channel;
  gboolean     fs;
  GList       *list;
  GList       *next = NULL;
  GList       *prev = NULL;

  gimage = GIMP_IMAGE (data);

  channel = gimp_image_get_active_channel (gimage);

  fs = (gimp_image_floating_sel (gimage) != NULL);

  for (list = GIMP_LIST (gimage->channels)->list;
       list;
       list = g_list_next (list))
    {
      if (channel == (GimpChannel *) list->data)
	{
	  prev = g_list_previous (list);
	  next = g_list_next (list);
	  break;
	}
    }

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/New Channel...",             !fs);
  SET_SENSITIVE ("/Raise Channel",              !fs && channel && prev);
  SET_SENSITIVE ("/Lower Channel",              !fs && channel && next);
  SET_SENSITIVE ("/Duplicate Channel",          !fs && channel);
  SET_SENSITIVE ("/Channel to Selection",       !fs && channel);
  SET_SENSITIVE ("/Add to Selection",           !fs && channel);
  SET_SENSITIVE ("/Subtract from Selection",    !fs && channel);
  SET_SENSITIVE ("/Intersect with Selection",   !fs && channel);
  SET_SENSITIVE ("/Delete Channel",             !fs && channel);
  SET_SENSITIVE ("/Edit Channel Attributes...", !fs && channel);

#undef SET_SENSITIVE
}


/*  private functions  */

static void
channels_opacity_update (GtkAdjustment *adjustment,
			 gpointer       data)
{
  GimpRGB  color;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (data), &color);
  gimp_rgb_set_alpha (&color, adjustment->value / 100.0);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (data), &color);
}

static void
channels_color_changed (GimpColorButton *button,
			gpointer         data)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT (data);
  GimpRGB        color;

  gimp_color_button_get_color (button, &color);
  gtk_adjustment_set_value (adj, color.a * 100.0);
}

