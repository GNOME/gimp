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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpcomponenteditor.h"
#include "widgets/gimpdock.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "actions.h"
#include "channels-commands.h"

#include "gimp-intl.h"


typedef struct _ChannelOptions ChannelOptions;

struct _ChannelOptions
{
  GtkWidget   *query_box;
  GtkWidget   *name_entry;
  GtkWidget   *color_panel;

  GimpImage   *gimage;
  GimpContext *context;
  GimpChannel *channel;
};


/*  local function prototypes  */

static ChannelOptions * channel_options_new  (GimpImage       *gimage,
                                              GimpContext     *context,
                                              GimpChannel     *channel,
                                              GtkWidget       *parent);
static void   channels_new_channel_response  (GtkWidget       *widget,
                                              gint             response_id,
                                              ChannelOptions  *options);
static void   channels_edit_channel_response (GtkWidget       *widget,
                                              gint             response_id,
                                              ChannelOptions  *options);
static void   channels_opacity_update        (GtkAdjustment   *adjustment,
                                              gpointer         data);
static void   channels_color_changed         (GimpColorButton *button,
                                              gpointer         data);


/*  private variables  */

static gchar   *channel_name  = NULL;
static GimpRGB  channel_color = { 0.0, 0.0, 0.0, 0.5 };


/*  public functions  */

void
channels_edit_attributes_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  ChannelOptions *options;
  GimpImage      *gimage;
  GimpChannel    *channel;
  GtkWidget      *widget;
  return_if_no_channel (gimage, channel, data);
  return_if_no_widget (widget, data);

  options = channel_options_new (gimp_item_get_image (GIMP_ITEM (channel)),
                                 action_data_get_context (data),
                                 channel, widget);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (channels_edit_channel_response),
                    options);

  gtk_widget_show (options->query_box);
}

void
channels_new_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  ChannelOptions *options;
  GimpImage      *gimage;
  GtkWidget      *widget;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  options = channel_options_new (gimage, action_data_get_context (data),
                                 NULL, widget);

  g_signal_connect (options->query_box, "response",
                    G_CALLBACK (channels_new_channel_response),
                    options);

  gtk_widget_show (options->query_box);
}

void
channels_new_default_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *new_channel;
  gint         width, height;
  GimpRGB      color;
  return_if_no_image (gimage, data);

  if (GIMP_IS_CHANNEL (GIMP_ACTION (action)->viewable))
    {
      GimpChannel *template = GIMP_CHANNEL (GIMP_ACTION (action)->viewable);

      width  = gimp_item_width  (GIMP_ITEM (template));
      height = gimp_item_height (GIMP_ITEM (template));
      color  = template->color;
    }
  else
    {
      width  = gimp_image_get_width (gimage);
      height = gimp_image_get_height (gimage);
      gimp_rgba_set (&color, 0.0, 0.0, 0.0, 0.5);
    }

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_EDIT_PASTE,
                               _("New Channel"));

  new_channel = gimp_channel_new (gimage, width, height,
                                  _("Empty Channel"), &color);

  gimp_drawable_fill_by_type (GIMP_DRAWABLE (new_channel),
                              action_data_get_context (data),
                              GIMP_TRANSPARENT_FILL);

  gimp_image_add_channel (gimage, new_channel, -1);

  gimp_image_undo_group_end (gimage);

  gimp_image_flush (gimage);
}

void
channels_raise_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *channel;
  return_if_no_channel (gimage, channel, data);

  gimp_image_raise_channel (gimage, channel);
  gimp_image_flush (gimage);
}

void
channels_raise_to_top_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *channel;
  return_if_no_channel (gimage, channel, data);

  gimp_image_raise_channel_to_top (gimage, channel);
  gimp_image_flush (gimage);
}

void
channels_lower_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *channel;
  return_if_no_channel (gimage, channel, data);

  gimp_image_lower_channel (gimage, channel);
  gimp_image_flush (gimage);
}

void
channels_lower_to_bottom_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *channel;
  return_if_no_channel (gimage, channel, data);

  gimp_image_lower_channel_to_bottom (gimage, channel);
  gimp_image_flush (gimage);
}

void
channels_duplicate_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *new_channel;

  if (GIMP_IS_COMPONENT_EDITOR (data))
    {
      GimpRGB          color;
      GimpChannelType  component;
      GEnumClass      *enum_class;
      GEnumValue      *enum_value;
      gchar           *name;
      return_if_no_image (gimage, data);

      gimp_rgba_set (&color, 0, 0, 0, 0.5);

      component = GIMP_COMPONENT_EDITOR (data)->clicked_component;

      enum_class = g_type_class_ref (GIMP_TYPE_CHANNEL_TYPE);
      enum_value = g_enum_get_value (enum_class, component);
      g_type_class_unref (enum_class);

      name = g_strdup_printf (_("%s Channel Copy"),
                              gettext (enum_value->value_name));

      new_channel = gimp_channel_new_from_component (gimage, component,
                                                     name, &color);

      /*  copied components are invisible by default so subsequent copies
       *  of components don't affect each other
       */
      gimp_item_set_visible (GIMP_ITEM (new_channel), FALSE, FALSE);

      g_free (name);
    }
  else
    {
      GimpChannel *channel;
      return_if_no_channel (gimage, channel, data);

      new_channel =
        GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (channel),
                                           G_TYPE_FROM_INSTANCE (channel),
                                           TRUE));
    }

  gimp_image_add_channel (gimage, new_channel, -1);
  gimp_image_flush (gimage);
}

void
channels_delete_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage   *gimage;
  GimpChannel *channel;
  return_if_no_channel (gimage, channel, data);

  gimp_image_remove_channel (gimage, channel);
  gimp_image_flush (gimage);
}

void
channels_to_selection_cmd_callback (GtkAction *action,
                                    gint       value,
                                    gpointer   data)
{
  GimpChannelOps  op;
  GimpImage      *gimage;

  op = (GimpChannelOps) value;

  if (GIMP_IS_COMPONENT_EDITOR (data))
    {
      GimpChannelType component;
      return_if_no_image (gimage, data);

      component = GIMP_COMPONENT_EDITOR (data)->clicked_component;

      gimp_channel_select_component (gimp_image_get_mask (gimage), component,
                                     op, FALSE, 0.0, 0.0);
    }
  else
    {
      GimpChannel *channel;
      return_if_no_channel (gimage, channel, data);

      gimp_channel_select_channel (gimp_image_get_mask (gimage),
                                   _("Channel to Selection"),
                                   channel, 0, 0,
                                   op, FALSE, 0.0, 0.0);
    }

  gimp_image_flush (gimage);
}


/*  private functions  */

static ChannelOptions *
channel_options_new (GimpImage   *gimage,
                     GimpContext *context,
                     GimpChannel *channel,
                     GtkWidget   *parent)
{
  ChannelOptions *options;
  GtkWidget      *hbox;
  GtkWidget      *vbox;
  GtkWidget      *table;
  GtkObject      *opacity_scale_data;

  options = g_new0 (ChannelOptions, 1);

  options->gimage  = gimage;
  options->context = context;
  options->channel = channel;

  if (channel)
    channel_color = channel->color;

  options->color_panel = gimp_color_panel_new (channel ?
                                               _("Edit Channel Color") :
                                               _("New Channel Color"),
					       &channel_color,
					       GIMP_COLOR_AREA_LARGE_CHECKS,
					       48, 64);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (options->color_panel),
                                context);

  if (channel)
    {
      options->query_box =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (channel),
                                  _("Channel Attributes"), "gimp-channel-edit",
                                  GIMP_STOCK_EDIT,
                                  _("Edit Channel Attributes"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_CHANNEL_EDIT,

                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                  GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                  NULL);
    }
  else
    {
      options->query_box =
        gimp_viewable_dialog_new (GIMP_VIEWABLE (gimage),
                                  _("New Channel"), "gimp-channel-new",
                                  GIMP_STOCK_CHANNEL,
                                  _("New Channel Options"),
                                  parent,
                                  gimp_standard_help_func,
                                  GIMP_HELP_CHANNEL_NEW,

                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                  GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                  NULL);
    }

  g_object_weak_ref (G_OBJECT (options->query_box),
		     (GWeakNotify) g_free,
		     options);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     hbox);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->name_entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (options->name_entry), TRUE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Channel Name:"), 0.0, 0.5,
			     options->name_entry, 2, FALSE);

  if (channel)
    gtk_entry_set_text (GTK_ENTRY (options->name_entry),
                        gimp_object_get_name (GIMP_OBJECT (channel)));
  else
    gtk_entry_set_text (GTK_ENTRY (options->name_entry),
                        channel_name ? channel_name : _("New Channel"));

  opacity_scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
					     _("Fill Opacity:"), 100, -1,
					     channel_color.a * 100.0,
					     0.0, 100.0, 1.0, 10.0, 1,
					     TRUE, 0.0, 0.0,
					     NULL, NULL);

  g_signal_connect (opacity_scale_data, "value_changed",
		    G_CALLBACK (channels_opacity_update),
		    options->color_panel);

  gtk_box_pack_start (GTK_BOX (hbox), options->color_panel,
		      TRUE, TRUE, 0);
  gtk_widget_show (options->color_panel);

  g_signal_connect (options->color_panel, "color_changed",
		    G_CALLBACK (channels_color_changed),
		    opacity_scale_data);

  return options;
}

static void
channels_new_channel_response (GtkWidget      *widget,
                               gint            response_id,
                               ChannelOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpChannel *new_channel;

      if (channel_name)
        g_free (channel_name);
      channel_name =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

      gimp_color_button_get_color (GIMP_COLOR_BUTTON (options->color_panel),
                                   &channel_color);

      new_channel = gimp_channel_new (options->gimage,
                                      options->gimage->width,
                                      options->gimage->height,
                                      channel_name,
                                      &channel_color);

      gimp_drawable_fill_by_type (GIMP_DRAWABLE (new_channel),
                                  options->context,
                                  GIMP_TRANSPARENT_FILL);

      gimp_image_add_channel (options->gimage, new_channel, -1);
      gimp_image_flush (options->gimage);
    }

  gtk_widget_destroy (options->query_box);
}

static void
channels_edit_channel_response (GtkWidget      *widget,
                                gint            response_id,
                                ChannelOptions *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpChannel *channel = options->channel;
      const gchar *new_name;
      GimpRGB      color;
      gboolean     name_changed  = FALSE;
      gboolean     color_changed = FALSE;

      new_name = gtk_entry_get_text (GTK_ENTRY (options->name_entry));

      gimp_color_button_get_color (GIMP_COLOR_BUTTON (options->color_panel),
                                   &color);

      if (strcmp (new_name, gimp_object_get_name (GIMP_OBJECT (channel))))
        name_changed = TRUE;

      if (gimp_rgba_distance (&color, &channel->color) > 0.0001)
        color_changed = TRUE;

      if (name_changed && color_changed)
        gimp_image_undo_group_start (options->gimage,
                                     GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                     _("Channel Attributes"));

      if (name_changed)
        gimp_item_rename (GIMP_ITEM (channel), new_name);

      if (color_changed)
        gimp_channel_set_color (channel, &color, TRUE);

      if (name_changed && color_changed)
        gimp_image_undo_group_end (options->gimage);

      if (name_changed || color_changed)
        gimp_image_flush (options->gimage);
    }

  gtk_widget_destroy (options->query_box);
}

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
