/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpdialogconfig.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable-fill.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpcomponenteditor.h"
#include "widgets/gimpdock.h"
#include "widgets/gimphelp-ids.h"

#include "dialogs/dialogs.h"
#include "dialogs/channel-options-dialog.h"

#include "actions.h"
#include "channels-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   channels_new_callback             (GtkWidget     *dialog,
                                                 GimpImage     *image,
                                                 GimpChannel   *channel,
                                                 GimpContext   *context,
                                                 const gchar   *channel_name,
                                                 const GimpRGB *channel_color,
                                                 gboolean       save_selection,
                                                 gpointer       user_data);
static void   channels_edit_attributes_callback (GtkWidget     *dialog,
                                                 GimpImage     *image,
                                                 GimpChannel   *channel,
                                                 GimpContext   *context,
                                                 const gchar   *channel_name,
                                                 const GimpRGB *channel_color,
                                                 gboolean       save_selection,
                                                 gpointer       user_data);


/*  public functions  */

void
channels_edit_attributes_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  GimpImage   *image;
  GimpChannel *channel;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_channel (image, channel, data);
  return_if_no_widget (widget, data);

#define EDIT_DIALOG_KEY "gimp-channel-edit-attributes-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (channel), EDIT_DIALOG_KEY);

  if (! dialog)
    {
      dialog = channel_options_dialog_new (image, channel,
                                           action_data_get_context (data),
                                           widget,
                                           _("Channel Attributes"),
                                           "gimp-channel-edit",
                                           "gtk-edit",
                                           _("Edit Channel Attributes"),
                                           GIMP_HELP_CHANNEL_EDIT,
                                           gimp_object_get_name (channel),
                                           &channel->color,
                                           _("Edit Channel Color"),
                                           _("_Fill opacity"),
                                           FALSE,
                                           channels_edit_attributes_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (channel), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
channels_new_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GimpImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define NEW_DIALOG_KEY "gimp-channel-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), NEW_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

      dialog = channel_options_dialog_new (image, NULL,
                                           action_data_get_context (data),
                                           widget,
                                           _("New Channel"),
                                           "gimp-channel-new",
                                           GIMP_STOCK_CHANNEL,
                                           _("Create a New Channel"),
                                           GIMP_HELP_CHANNEL_NEW,
                                           config->channel_new_name,
                                           &config->channel_new_color,
                                           _("New Channel Color"),
                                           _("_Fill opacity"),
                                           TRUE,
                                           channels_new_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
channels_new_last_vals_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpImage        *image;
  GimpChannel      *new_channel;
  gint              width, height;
  GimpRGB           color;
  GimpDialogConfig *config;
  return_if_no_image (image, data);

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  if (GIMP_IS_CHANNEL (GIMP_ACTION (action)->viewable))
    {
      GimpChannel *template = GIMP_CHANNEL (GIMP_ACTION (action)->viewable);

      width  = gimp_item_get_width  (GIMP_ITEM (template));
      height = gimp_item_get_height (GIMP_ITEM (template));
      color  = template->color;
    }
  else
    {
      width  = gimp_image_get_width (image);
      height = gimp_image_get_height (image);
      color  = config->channel_new_color;
    }

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                               _("New Channel"));

  new_channel = gimp_channel_new (image, width, height,
                                  config->channel_new_name, &color);

  gimp_drawable_fill (GIMP_DRAWABLE (new_channel),
                      action_data_get_context (data),
                      GIMP_FILL_TRANSPARENT);

  gimp_image_add_channel (image, new_channel,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);
}

void
channels_raise_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  gimp_image_raise_item (image, GIMP_ITEM (channel), NULL);
  gimp_image_flush (image);
}

void
channels_raise_to_top_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  gimp_image_raise_item_to_top (image, GIMP_ITEM (channel));
  gimp_image_flush (image);
}

void
channels_lower_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  gimp_image_lower_item (image, GIMP_ITEM (channel), NULL);
  gimp_image_flush (image);
}

void
channels_lower_to_bottom_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  gimp_image_lower_item_to_bottom (image, GIMP_ITEM (channel));
  gimp_image_flush (image);
}

void
channels_duplicate_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpImage   *image;
  GimpChannel *new_channel;
  GimpChannel *parent = GIMP_IMAGE_ACTIVE_PARENT;

  if (GIMP_IS_COMPONENT_EDITOR (data))
    {
      GimpChannelType  component;
      const gchar     *desc;
      gchar           *name;
      return_if_no_image (image, data);

      component = GIMP_COMPONENT_EDITOR (data)->clicked_component;

      gimp_enum_get_value (GIMP_TYPE_CHANNEL_TYPE, component,
                           NULL, NULL, &desc, NULL);

      name = g_strdup_printf (_("%s Channel Copy"), desc);

      new_channel = gimp_channel_new_from_component (image, component,
                                                     name, NULL);

      /*  copied components are invisible by default so subsequent copies
       *  of components don't affect each other
       */
      gimp_item_set_visible (GIMP_ITEM (new_channel), FALSE, FALSE);

      g_free (name);
    }
  else
    {
      GimpChannel *channel;
      return_if_no_channel (image, channel, data);

      new_channel =
        GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (channel),
                                           G_TYPE_FROM_INSTANCE (channel)));

      /*  use the actual parent here, not GIMP_IMAGE_ACTIVE_PARENT because
       *  the latter would add a duplicated group inside itself instead of
       *  above it
       */
      parent = gimp_channel_get_parent (channel);
    }

  gimp_image_add_channel (image, new_channel, parent, -1, TRUE);
  gimp_image_flush (image);
}

void
channels_delete_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  gimp_image_remove_channel (image, channel, TRUE, NULL);
  gimp_image_flush (image);
}

void
channels_to_selection_cmd_callback (GtkAction *action,
                                    gint       value,
                                    gpointer   data)
{
  GimpChannelOps  op;
  GimpImage      *image;

  op = (GimpChannelOps) value;

  if (GIMP_IS_COMPONENT_EDITOR (data))
    {
      GimpChannelType component;
      return_if_no_image (image, data);

      component = GIMP_COMPONENT_EDITOR (data)->clicked_component;

      gimp_channel_select_component (gimp_image_get_mask (image), component,
                                     op, FALSE, 0.0, 0.0);
    }
  else
    {
      GimpChannel *channel;
      return_if_no_channel (image, channel, data);

      gimp_item_to_selection (GIMP_ITEM (channel),
                              op, TRUE, FALSE, 0.0, 0.0);
    }

  gimp_image_flush (image);
}


/*  private functions  */

static void
channels_new_callback (GtkWidget     *dialog,
                       GimpImage     *image,
                       GimpChannel   *channel,
                       GimpContext   *context,
                       const gchar   *channel_name,
                       const GimpRGB *channel_color,
                       gboolean       save_selection,
                       gpointer       user_data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);

  g_object_set (config,
                "channel-new-name",  channel_name,
                "channel-new-color", channel_color,
                NULL);

  if (save_selection)
    {
      GimpChannel *selection = gimp_image_get_mask (image);

      channel = GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (selection),
                                                   GIMP_TYPE_CHANNEL));

      gimp_object_set_name (GIMP_OBJECT (channel),
                            config->channel_new_name);
      gimp_channel_set_color (channel, &config->channel_new_color, FALSE);
    }
  else
    {
      channel = gimp_channel_new (image,
                                  gimp_image_get_width  (image),
                                  gimp_image_get_height (image),
                                  config->channel_new_name,
                                  &config->channel_new_color);

      gimp_drawable_fill (GIMP_DRAWABLE (channel), context,
                          GIMP_FILL_TRANSPARENT);
    }

  gimp_image_add_channel (image, channel,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
channels_edit_attributes_callback (GtkWidget     *dialog,
                                   GimpImage     *image,
                                   GimpChannel   *channel,
                                   GimpContext   *context,
                                   const gchar   *channel_name,
                                   const GimpRGB *channel_color,
                                   gboolean       save_selection,
                                   gpointer       user_data)
{
  gboolean name_changed  = FALSE;
  gboolean color_changed = FALSE;

  if (strcmp (channel_name, gimp_object_get_name (channel)))
    name_changed = TRUE;

  if (gimp_rgba_distance (channel_color, &channel->color) > 0.0001)
    color_changed = TRUE;

  if (name_changed || color_changed)
    {
      if (name_changed && color_changed)
        gimp_image_undo_group_start (image,
                                     GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                     _("Channel Attributes"));

      if (name_changed)
        gimp_item_rename (GIMP_ITEM (channel), channel_name, NULL);

      if (color_changed)
        gimp_channel_set_color (channel, channel_color, TRUE);

      if (name_changed && color_changed)
        gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}
