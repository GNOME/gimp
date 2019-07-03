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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include "items-commands.h"

#include "gimp-intl.h"


#define RGBA_EPSILON 1e-6


/*  local function prototypes  */

static void   channels_new_callback             (GtkWidget     *dialog,
                                                 GimpImage     *image,
                                                 GimpChannel   *channel,
                                                 GimpContext   *context,
                                                 const gchar   *channel_name,
                                                 const GimpRGB *channel_color,
                                                 gboolean       save_selection,
                                                 gboolean       channel_visible,
                                                 gboolean       channel_linked,
                                                 GimpColorTag   channel_color_tag,
                                                 gboolean       channel_lock_content,
                                                 gboolean       channel_lock_position,
                                                 gpointer       user_data);
static void   channels_edit_attributes_callback (GtkWidget     *dialog,
                                                 GimpImage     *image,
                                                 GimpChannel   *channel,
                                                 GimpContext   *context,
                                                 const gchar   *channel_name,
                                                 const GimpRGB *channel_color,
                                                 gboolean       save_selection,
                                                 gboolean       channel_visible,
                                                 gboolean       channel_linked,
                                                 GimpColorTag   channel_color_tag,
                                                 gboolean       channel_lock_content,
                                                 gboolean       channel_lock_position,
                                                 gpointer       user_data);


/*  public functions  */

void
channels_edit_attributes_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
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
      GimpItem *item = GIMP_ITEM (channel);

      dialog = channel_options_dialog_new (image, channel,
                                           action_data_get_context (data),
                                           widget,
                                           _("Channel Attributes"),
                                           "gimp-channel-edit",
                                           GIMP_ICON_EDIT,
                                           _("Edit Channel Attributes"),
                                           GIMP_HELP_CHANNEL_EDIT,
                                           _("Edit Channel Color"),
                                           _("_Fill opacity:"),
                                           FALSE,
                                           gimp_object_get_name (channel),
                                           &channel->color,
                                           gimp_item_get_visible (item),
                                           gimp_item_get_linked (item),
                                           gimp_item_get_color_tag (item),
                                           gimp_item_get_lock_content (item),
                                           gimp_item_get_lock_position (item),
                                           channels_edit_attributes_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (channel), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
channels_new_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
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
                                           GIMP_ICON_CHANNEL,
                                           _("Create a New Channel"),
                                           GIMP_HELP_CHANNEL_NEW,
                                           _("New Channel Color"),
                                           _("_Fill opacity:"),
                                           TRUE,
                                           config->channel_new_name,
                                           &config->channel_new_color,
                                           TRUE,
                                           FALSE,
                                           GIMP_COLOR_TAG_NONE,
                                           FALSE,
                                           FALSE,
                                           channels_new_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
channels_new_last_vals_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage        *image;
  GimpChannel      *channel;
  GimpDialogConfig *config;
  return_if_no_image (image, data);

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  channel = gimp_channel_new (image,
                              gimp_image_get_width (image),
                              gimp_image_get_height (image),
                              config->channel_new_name,
                              &config->channel_new_color);

  gimp_drawable_fill (GIMP_DRAWABLE (channel),
                      action_data_get_context (data),
                      GIMP_FILL_TRANSPARENT);

  gimp_image_add_channel (image, channel,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_image_flush (image);
}

void
channels_raise_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  gimp_image_raise_item (image, GIMP_ITEM (channel), NULL);
  gimp_image_flush (image);
}

void
channels_raise_to_top_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  gimp_image_raise_item_to_top (image, GIMP_ITEM (channel));
  gimp_image_flush (image);
}

void
channels_lower_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  gimp_image_lower_item (image, GIMP_ITEM (channel), NULL);
  gimp_image_flush (image);
}

void
channels_lower_to_bottom_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  gimp_image_lower_item_to_bottom (image, GIMP_ITEM (channel));
  gimp_image_flush (image);
}

void
channels_duplicate_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
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
channels_delete_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  gimp_image_remove_channel (image, channel, TRUE, NULL);
  gimp_image_flush (image);
}

void
channels_to_selection_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpChannelOps  op;
  GimpImage      *image;

  op = (GimpChannelOps) g_variant_get_int32 (value);

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

void
channels_visible_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  items_visible_cmd_callback (action, value, image, GIMP_ITEM (channel));
}

void
channels_linked_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  items_linked_cmd_callback (action, value, image, GIMP_ITEM (channel));
}

void
channels_lock_content_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  items_lock_content_cmd_callback (action, value, image, GIMP_ITEM (channel));
}

void
channels_lock_position_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  return_if_no_channel (image, channel, data);

  items_lock_position_cmd_callback (action, value, image, GIMP_ITEM (channel));
}

void
channels_color_tag_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage    *image;
  GimpChannel  *channel;
  GimpColorTag  color_tag;
  return_if_no_channel (image, channel, data);

  color_tag = (GimpColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, GIMP_ITEM (channel),
                                color_tag);
}

void
channels_select_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpImage            *image;
  GimpChannel          *channel;
  GimpChannel          *channel2;
  GimpContainer        *container;
  GimpActionSelectType  type;
  return_if_no_channel (image, channel, data);

  type = (GimpActionSelectType) g_variant_get_int32 (value);

  container = gimp_image_get_channels (image);
  channel2 = (GimpChannel *) action_select_object (type, container,
                                                   (GimpObject *) channel);

  if (channel2 && channel2 != channel)
    {
      gimp_image_set_active_channel (image, channel2);
      gimp_image_flush (image);
    }
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
                       gboolean       channel_visible,
                       gboolean       channel_linked,
                       GimpColorTag   channel_color_tag,
                       gboolean       channel_lock_content,
                       gboolean       channel_lock_position,
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

  gimp_item_set_visible (GIMP_ITEM (channel), channel_visible, FALSE);
  gimp_item_set_linked (GIMP_ITEM (channel), channel_linked, FALSE);
  gimp_item_set_color_tag (GIMP_ITEM (channel), channel_color_tag, FALSE);
  gimp_item_set_lock_content (GIMP_ITEM (channel), channel_lock_content, FALSE);
  gimp_item_set_lock_position (GIMP_ITEM (channel), channel_lock_position, FALSE);

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
                                   gboolean       channel_visible,
                                   gboolean       channel_linked,
                                   GimpColorTag   channel_color_tag,
                                   gboolean       channel_lock_content,
                                   gboolean       channel_lock_position,
                                   gpointer       user_data)
{
  GimpItem *item = GIMP_ITEM (channel);

  if (strcmp (channel_name, gimp_object_get_name (channel))              ||
      gimp_rgba_distance (channel_color, &channel->color) > RGBA_EPSILON ||
      channel_visible       != gimp_item_get_visible (item)              ||
      channel_linked        != gimp_item_get_linked (item)               ||
      channel_color_tag     != gimp_item_get_color_tag (item)            ||
      channel_lock_content  != gimp_item_get_lock_content (item)         ||
      channel_lock_position != gimp_item_get_lock_position (item))
    {
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Channel Attributes"));

      if (strcmp (channel_name, gimp_object_get_name (channel)))
        gimp_item_rename (GIMP_ITEM (channel), channel_name, NULL);

      if (gimp_rgba_distance (channel_color, &channel->color) > RGBA_EPSILON)
        gimp_channel_set_color (channel, channel_color, TRUE);

      if (channel_visible != gimp_item_get_visible (item))
        gimp_item_set_visible (item, channel_visible, TRUE);

      if (channel_linked != gimp_item_get_linked (item))
        gimp_item_set_linked (item, channel_linked, TRUE);

      if (channel_color_tag != gimp_item_get_color_tag (item))
        gimp_item_set_color_tag (item, channel_color_tag, TRUE);

      if (channel_lock_content != gimp_item_get_lock_content (item))
        gimp_item_set_lock_content (item, channel_lock_content, TRUE);

      if (channel_lock_position != gimp_item_get_lock_position (item))
        gimp_item_set_lock_position (item, channel_lock_position, TRUE);

      gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}
