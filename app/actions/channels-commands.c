/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmadialogconfig.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmachannel-select.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadrawable-fill.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmacolorpanel.h"
#include "widgets/ligmacomponenteditor.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmahelp-ids.h"

#include "dialogs/dialogs.h"
#include "dialogs/channel-options-dialog.h"

#include "actions.h"
#include "channels-commands.h"
#include "items-commands.h"

#include "ligma-intl.h"


#define RGBA_EPSILON 1e-6


/*  local function prototypes  */

static void   channels_new_callback             (GtkWidget     *dialog,
                                                 LigmaImage     *image,
                                                 LigmaChannel   *channel,
                                                 LigmaContext   *context,
                                                 const gchar   *channel_name,
                                                 const LigmaRGB *channel_color,
                                                 gboolean       save_selection,
                                                 gboolean       channel_visible,
                                                 LigmaColorTag   channel_color_tag,
                                                 gboolean       channel_lock_content,
                                                 gboolean       channel_lock_position,
                                                 gpointer       user_data);
static void   channels_edit_attributes_callback (GtkWidget     *dialog,
                                                 LigmaImage     *image,
                                                 LigmaChannel   *channel,
                                                 LigmaContext   *context,
                                                 const gchar   *channel_name,
                                                 const LigmaRGB *channel_color,
                                                 gboolean       save_selection,
                                                 gboolean       channel_visible,
                                                 LigmaColorTag   channel_color_tag,
                                                 gboolean       channel_lock_content,
                                                 gboolean       channel_lock_position,
                                                 gpointer       user_data);


/*  public functions  */

void
channels_edit_attributes_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaImage   *image;
  LigmaChannel *channel;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_channel (image, channel, data);
  return_if_no_widget (widget, data);

#define EDIT_DIALOG_KEY "ligma-channel-edit-attributes-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (channel), EDIT_DIALOG_KEY);

  if (! dialog)
    {
      LigmaItem *item = LIGMA_ITEM (channel);

      dialog = channel_options_dialog_new (image, channel,
                                           action_data_get_context (data),
                                           widget,
                                           _("Channel Attributes"),
                                           "ligma-channel-edit",
                                           LIGMA_ICON_EDIT,
                                           _("Edit Channel Attributes"),
                                           LIGMA_HELP_CHANNEL_EDIT,
                                           _("Edit Channel Color"),
                                           _("_Fill opacity:"),
                                           FALSE,
                                           ligma_object_get_name (channel),
                                           &channel->color,
                                           ligma_item_get_visible (item),
                                           ligma_item_get_color_tag (item),
                                           ligma_item_get_lock_content (item),
                                           ligma_item_get_lock_position (item),
                                           channels_edit_attributes_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (channel), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
channels_new_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define NEW_DIALOG_KEY "ligma-channel-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), NEW_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);

      dialog = channel_options_dialog_new (image, NULL,
                                           action_data_get_context (data),
                                           widget,
                                           _("New Channel"),
                                           "ligma-channel-new",
                                           LIGMA_ICON_CHANNEL,
                                           _("Create a New Channel"),
                                           LIGMA_HELP_CHANNEL_NEW,
                                           _("New Channel Color"),
                                           _("_Fill opacity:"),
                                           TRUE,
                                           config->channel_new_name,
                                           &config->channel_new_color,
                                           TRUE,
                                           LIGMA_COLOR_TAG_NONE,
                                           FALSE,
                                           FALSE,
                                           channels_new_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
channels_new_last_vals_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage        *image;
  LigmaChannel      *channel;
  LigmaDialogConfig *config;
  return_if_no_image (image, data);

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  channel = ligma_channel_new (image,
                              ligma_image_get_width (image),
                              ligma_image_get_height (image),
                              config->channel_new_name,
                              &config->channel_new_color);

  ligma_drawable_fill (LIGMA_DRAWABLE (channel),
                      action_data_get_context (data),
                      LIGMA_FILL_TRANSPARENT);

  ligma_image_add_channel (image, channel,
                          LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);
  ligma_image_flush (image);
}

void
channels_raise_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaImage *image;
  GList     *channels;
  GList     *iter;
  GList     *raised_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      gint index;

      index = ligma_item_get_index (iter->data);
      if (index > 0)
        raised_channels = g_list_prepend (raised_channels, iter->data);
    }

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Channel",
                                         "Raise Channels",
                                         g_list_length (raised_channels)));

  for (iter = raised_channels; iter; iter = iter->next)
    ligma_image_raise_item (image, iter->data, NULL);

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);

  g_list_free (raised_channels);
}

void
channels_raise_to_top_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaImage *image;
  GList     *channels;
  GList     *iter;
  GList     *raised_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      gint index;

      index = ligma_item_get_index (iter->data);
      if (index > 0)
        raised_channels = g_list_prepend (raised_channels, iter->data);
    }

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Channel to Top",
                                         "Raise Channels to Top",
                                         g_list_length (raised_channels)));

  for (iter = raised_channels; iter; iter = iter->next)
    ligma_image_raise_item_to_top (image, iter->data);

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);

  g_list_free (raised_channels);
}

void
channels_lower_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaImage *image;
  GList     *channels;
  GList     *iter;
  GList     *lowered_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = ligma_item_get_container_iter (LIGMA_ITEM (iter->data));
      index = ligma_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        lowered_channels = g_list_prepend (lowered_channels, iter->data);
    }

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Channel",
                                         "Lower Channels",
                                         g_list_length (lowered_channels)));

  for (iter = lowered_channels; iter; iter = iter->next)
    ligma_image_lower_item (image, iter->data, NULL);

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);

  g_list_free (lowered_channels);
}

void
channels_lower_to_bottom_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaImage *image;
  GList     *channels;
  GList     *iter;
  GList     *lowered_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = ligma_item_get_container_iter (LIGMA_ITEM (iter->data));
      index = ligma_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        lowered_channels = g_list_prepend (lowered_channels, iter->data);
    }

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Channel to Bottom",
                                         "Lower Channels to Bottom",
                                         g_list_length (lowered_channels)));

  for (iter = lowered_channels; iter; iter = iter->next)
    ligma_image_lower_item_to_bottom (image, iter->data);

  ligma_image_flush (image);
  ligma_image_undo_group_end (image);

  g_list_free (lowered_channels);
}

void
channels_duplicate_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaImage   *image  = NULL;
  GList       *channels;
  LigmaChannel *parent = LIGMA_IMAGE_ACTIVE_PARENT;
  return_if_no_channels (image, channels, data);

  if (LIGMA_IS_COMPONENT_EDITOR (data))
    {
      LigmaChannelType  component;
      LigmaChannel     *new_channel;
      const gchar     *desc;
      gchar           *name;

      component = LIGMA_COMPONENT_EDITOR (data)->clicked_component;

      ligma_enum_get_value (LIGMA_TYPE_CHANNEL_TYPE, component,
                           NULL, NULL, &desc, NULL);

      name = g_strdup_printf (_("%s Channel Copy"), desc);

      new_channel = ligma_channel_new_from_component (image, component,
                                                     name, NULL);

      /*  copied components are invisible by default so subsequent copies
       *  of components don't affect each other
       */
      ligma_item_set_visible (LIGMA_ITEM (new_channel), FALSE, FALSE);
      ligma_image_add_channel (image, new_channel, parent, -1, TRUE);

      g_free (name);
    }
  else
    {
      GList *new_channels = NULL;
      GList *iter;

      channels = g_list_copy (channels);
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_CHANNEL_ADD,
                                   _("Duplicate channels"));
      for (iter = channels; iter; iter = iter->next)
        {
          LigmaChannel *new_channel;

          new_channel = LIGMA_CHANNEL (ligma_item_duplicate (LIGMA_ITEM (iter->data),
                                                           G_TYPE_FROM_INSTANCE (iter->data)));

          /*  use the actual parent here, not LIGMA_IMAGE_ACTIVE_PARENT because
           *  the latter would add a duplicated group inside itself instead of
           *  above it
           */
          ligma_image_add_channel (image, new_channel,
                                  ligma_channel_get_parent (iter->data),
                                  ligma_item_get_index (iter->data),
                                  TRUE);
          new_channels = g_list_prepend (new_channels, new_channel);
        }

      ligma_image_set_selected_channels (image, new_channels);
      g_list_free (channels);
      g_list_free (new_channels);

      ligma_image_undo_group_end (image);
    }
  ligma_image_flush (image);
}

void
channels_delete_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaImage *image;
  GList     *channels;
  GList     *iter;
  return_if_no_channels (image, channels, data);

  channels = g_list_copy (channels);
  if (g_list_length (channels) > 1)
    {
      gchar *undo_name;

      undo_name = g_strdup_printf (C_("undo-type", "Remove %d Channels"),
                                   g_list_length (channels));
      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   undo_name);
    }

  for (iter = channels; iter; iter = iter->next)
    ligma_image_remove_channel (image, iter->data, TRUE, NULL);

  if (g_list_length (channels) > 1)
    ligma_image_undo_group_end (image);

  g_list_free (channels);
  ligma_image_flush (image);
}

void
channels_to_selection_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaChannelOps  op;
  LigmaImage      *image;

  op = (LigmaChannelOps) g_variant_get_int32 (value);

  if (LIGMA_IS_COMPONENT_EDITOR (data))
    {
      LigmaChannelType component;
      return_if_no_image (image, data);

      component = LIGMA_COMPONENT_EDITOR (data)->clicked_component;

      ligma_channel_select_component (ligma_image_get_mask (image), component,
                                     op, FALSE, 0.0, 0.0);
    }
  else
    {
      LigmaChannel *channel;
      return_if_no_channel (image, channel, data);

      ligma_item_to_selection (LIGMA_ITEM (channel),
                              op, TRUE, FALSE, 0.0, 0.0);
    }

  ligma_image_flush (image);
}

void
channels_visible_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaImage   *image;
  LigmaChannel *channel;
  return_if_no_channel (image, channel, data);

  items_visible_cmd_callback (action, value, image, LIGMA_ITEM (channel));
}

void
channels_lock_content_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaImage   *image;
  LigmaChannel *channel;
  return_if_no_channel (image, channel, data);

  items_lock_content_cmd_callback (action, value, image, LIGMA_ITEM (channel));
}

void
channels_lock_position_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaImage   *image;
  LigmaChannel *channel;
  return_if_no_channel (image, channel, data);

  items_lock_position_cmd_callback (action, value, image, LIGMA_ITEM (channel));
}

void
channels_color_tag_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaImage    *image;
  LigmaChannel  *channel;
  LigmaColorTag  color_tag;
  return_if_no_channel (image, channel, data);

  color_tag = (LigmaColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, LIGMA_ITEM (channel),
                                color_tag);
}

void
channels_select_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaImage            *image;
  GList                *channels;
  GList                *new_channels = NULL;
  GList                *iter;
  LigmaActionSelectType  select_type;
  gboolean              run_once;
  return_if_no_image (image, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  channels = ligma_image_get_selected_channels (image);
  run_once = (g_list_length (channels) == 0);

  for (iter = channels; iter || run_once; iter = iter ? iter->next : NULL)
    {
      LigmaChannel   *new_channel = NULL;
      LigmaContainer *container;

      if (iter)
        {
          container = ligma_item_get_container (LIGMA_ITEM (iter->data));
        }
      else /* run_once */
        {
          container = ligma_image_get_channels (image);
          run_once  = FALSE;
        }
      new_channel = (LigmaChannel *) action_select_object (select_type,
                                                          container,
                                                          iter ? iter->data : NULL);

      if (new_channel)
        new_channels = g_list_prepend (new_channels, new_channel);
    }

  if (new_channels)
    {
      ligma_image_set_selected_channels (image, new_channels);
      ligma_image_flush (image);
    }

  g_list_free (new_channels);
}

/*  private functions  */

static void
channels_new_callback (GtkWidget     *dialog,
                       LigmaImage     *image,
                       LigmaChannel   *channel,
                       LigmaContext   *context,
                       const gchar   *channel_name,
                       const LigmaRGB *channel_color,
                       gboolean       save_selection,
                       gboolean       channel_visible,
                       LigmaColorTag   channel_color_tag,
                       gboolean       channel_lock_content,
                       gboolean       channel_lock_position,
                       gpointer       user_data)
{
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  g_object_set (config,
                "channel-new-name",  channel_name,
                "channel-new-color", channel_color,
                NULL);

  if (save_selection)
    {
      LigmaChannel *selection = ligma_image_get_mask (image);

      channel = LIGMA_CHANNEL (ligma_item_duplicate (LIGMA_ITEM (selection),
                                                   LIGMA_TYPE_CHANNEL));

      ligma_object_set_name (LIGMA_OBJECT (channel),
                            config->channel_new_name);
      ligma_channel_set_color (channel, &config->channel_new_color, FALSE);
    }
  else
    {
      channel = ligma_channel_new (image,
                                  ligma_image_get_width  (image),
                                  ligma_image_get_height (image),
                                  config->channel_new_name,
                                  &config->channel_new_color);

      ligma_drawable_fill (LIGMA_DRAWABLE (channel), context,
                          LIGMA_FILL_TRANSPARENT);
    }

  ligma_item_set_visible (LIGMA_ITEM (channel), channel_visible, FALSE);
  ligma_item_set_color_tag (LIGMA_ITEM (channel), channel_color_tag, FALSE);
  ligma_item_set_lock_content (LIGMA_ITEM (channel), channel_lock_content, FALSE);
  ligma_item_set_lock_position (LIGMA_ITEM (channel), channel_lock_position, FALSE);

  ligma_image_add_channel (image, channel,
                          LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);
  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
channels_edit_attributes_callback (GtkWidget     *dialog,
                                   LigmaImage     *image,
                                   LigmaChannel   *channel,
                                   LigmaContext   *context,
                                   const gchar   *channel_name,
                                   const LigmaRGB *channel_color,
                                   gboolean       save_selection,
                                   gboolean       channel_visible,
                                   LigmaColorTag   channel_color_tag,
                                   gboolean       channel_lock_content,
                                   gboolean       channel_lock_position,
                                   gpointer       user_data)
{
  LigmaItem *item = LIGMA_ITEM (channel);

  if (strcmp (channel_name, ligma_object_get_name (channel))              ||
      ligma_rgba_distance (channel_color, &channel->color) > RGBA_EPSILON ||
      channel_visible       != ligma_item_get_visible (item)              ||
      channel_color_tag     != ligma_item_get_color_tag (item)            ||
      channel_lock_content  != ligma_item_get_lock_content (item)         ||
      channel_lock_position != ligma_item_get_lock_position (item))
    {
      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Channel Attributes"));

      if (strcmp (channel_name, ligma_object_get_name (channel)))
        ligma_item_rename (LIGMA_ITEM (channel), channel_name, NULL);

      if (ligma_rgba_distance (channel_color, &channel->color) > RGBA_EPSILON)
        ligma_channel_set_color (channel, channel_color, TRUE);

      if (channel_visible != ligma_item_get_visible (item))
        ligma_item_set_visible (item, channel_visible, TRUE);

      if (channel_color_tag != ligma_item_get_color_tag (item))
        ligma_item_set_color_tag (item, channel_color_tag, TRUE);

      if (channel_lock_content != ligma_item_get_lock_content (item))
        ligma_item_set_lock_content (item, channel_lock_content, TRUE);

      if (channel_lock_position != ligma_item_get_lock_position (item))
        ligma_item_set_lock_position (item, channel_lock_position, TRUE);

      ligma_image_undo_group_end (image);

      ligma_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}
