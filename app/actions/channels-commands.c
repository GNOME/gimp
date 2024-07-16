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
#include "core/gimpcontainer.h"
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
                                                 GeglColor     *channel_color,
                                                 gboolean       save_selection,
                                                 gboolean       channel_visible,
                                                 GimpColorTag   channel_color_tag,
                                                 gboolean       channel_lock_content,
                                                 gboolean       channel_lock_position,
                                                 gboolean       channel_lock_visibility,
                                                 gpointer       user_data);
static void   channels_edit_attributes_callback (GtkWidget     *dialog,
                                                 GimpImage     *image,
                                                 GimpChannel   *channel,
                                                 GimpContext   *context,
                                                 const gchar   *channel_name,
                                                 GeglColor     *channel_color,
                                                 gboolean       save_selection,
                                                 gboolean       channel_visible,
                                                 GimpColorTag   channel_color_tag,
                                                 gboolean       channel_lock_content,
                                                 gboolean       channel_lock_position,
                                                 gboolean       channel_lock_visibility,
                                                 gpointer       user_data);


/*  public functions  */

void
channels_edit_attributes_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  GList       *channels;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_channels (image, channels, data);
  return_if_no_widget (widget, data);

#define EDIT_DIALOG_KEY "gimp-channel-edit-attributes-dialog"

  if (g_list_length (channels) != 1)
    return;

  channel = channels->data;
  dialog  = dialogs_get_dialog (G_OBJECT (channel), EDIT_DIALOG_KEY);

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
                                           channel->color,
                                           gimp_item_get_visible (item),
                                           gimp_item_get_color_tag (item),
                                           gimp_item_get_lock_content (item),
                                           gimp_item_get_lock_position (item),
                                           gimp_item_get_lock_visibility (item),
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
                                           config->channel_new_color,
                                           TRUE,
                                           GIMP_COLOR_TAG_NONE,
                                           FALSE,
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
                              config->channel_new_color);

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
  GimpImage *image;
  GList     *channels;
  GList     *iter;
  GList     *raised_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      gint index;

      index = gimp_item_get_index (iter->data);
      if (index > 0)
        {
          raised_channels = g_list_prepend (raised_channels, iter->data);
        }
      else
        {
          gimp_image_flush (image);
          g_list_free (raised_channels);
          return;
        }
    }

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Channel",
                                         "Raise Channels",
                                         g_list_length (raised_channels)));
  raised_channels = g_list_reverse (raised_channels);
  for (iter = raised_channels; iter; iter = iter->next)
    gimp_image_raise_item (image, iter->data, NULL);

  gimp_image_flush (image);
  gimp_image_undo_group_end (image);

  g_list_free (raised_channels);
}

void
channels_raise_to_top_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage *image;
  GList     *channels;
  GList     *iter;
  GList     *raised_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      gint index;

      index = gimp_item_get_index (iter->data);
      if (index > 0)
        raised_channels = g_list_prepend (raised_channels, iter->data);
    }

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Channel to Top",
                                         "Raise Channels to Top",
                                         g_list_length (raised_channels)));

  for (iter = raised_channels; iter; iter = iter->next)
    gimp_image_raise_item_to_top (image, iter->data);

  gimp_image_flush (image);
  gimp_image_undo_group_end (image);

  g_list_free (raised_channels);
}

void
channels_lower_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage *image;
  GList     *channels;
  GList     *iter;
  GList     *lowered_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = gimp_item_get_container_iter (GIMP_ITEM (iter->data));
      index = gimp_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        {
          lowered_channels = g_list_prepend (lowered_channels, iter->data);
        }
      else
        {
          gimp_image_flush (image);
          g_list_free (lowered_channels);
          return;
        }
    }

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Channel",
                                         "Lower Channels",
                                         g_list_length (lowered_channels)));

  for (iter = lowered_channels; iter; iter = iter->next)
    gimp_image_lower_item (image, iter->data, NULL);

  gimp_image_flush (image);
  gimp_image_undo_group_end (image);

  g_list_free (lowered_channels);
}

void
channels_lower_to_bottom_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpImage *image;
  GList     *channels;
  GList     *iter;
  GList     *lowered_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = gimp_item_get_container_iter (GIMP_ITEM (iter->data));
      index = gimp_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        lowered_channels = g_list_prepend (lowered_channels, iter->data);
    }

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Channel to Bottom",
                                         "Lower Channels to Bottom",
                                         g_list_length (lowered_channels)));

  for (iter = lowered_channels; iter; iter = iter->next)
    gimp_image_lower_item_to_bottom (image, iter->data);

  gimp_image_flush (image);
  gimp_image_undo_group_end (image);

  g_list_free (lowered_channels);
}

void
channels_duplicate_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage   *image  = NULL;
  GList       *channels;
  GimpChannel *parent = GIMP_IMAGE_ACTIVE_PARENT;
  return_if_no_channels (image, channels, data);

  if (GIMP_IS_COMPONENT_EDITOR (data))
    {
      GimpChannelType  component;
      GimpChannel     *new_channel;
      const gchar     *desc;
      gchar           *name;

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
      gimp_image_add_channel (image, new_channel, parent, -1, TRUE);

      g_free (name);
    }
  else
    {
      GList *new_channels = NULL;
      GList *iter;

      channels = g_list_copy (channels);
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_CHANNEL_ADD,
                                   _("Duplicate channels"));
      for (iter = channels; iter; iter = iter->next)
        {
          GimpChannel *new_channel;

          new_channel = GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (iter->data),
                                                           G_TYPE_FROM_INSTANCE (iter->data)));

          /*  use the actual parent here, not GIMP_IMAGE_ACTIVE_PARENT because
           *  the latter would add a duplicated group inside itself instead of
           *  above it
           */
          gimp_image_add_channel (image, new_channel,
                                  gimp_channel_get_parent (iter->data),
                                  gimp_item_get_index (iter->data),
                                  TRUE);
          new_channels = g_list_prepend (new_channels, new_channel);
        }

      gimp_image_set_selected_channels (image, new_channels);
      g_list_free (channels);
      g_list_free (new_channels);

      gimp_image_undo_group_end (image);
    }
  gimp_image_flush (image);
}

void
channels_delete_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpImage *image;
  GList     *channels;
  GList     *iter;
  return_if_no_channels (image, channels, data);

  channels = g_list_copy (channels);
  if (g_list_length (channels) > 1)
    {
      gchar *undo_name;

      undo_name = g_strdup_printf (C_("undo-type", "Remove %d Channels"),
                                   g_list_length (channels));
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   undo_name);
    }

  for (iter = channels; iter; iter = iter->next)
    gimp_image_remove_channel (image, iter->data, TRUE, NULL);

  if (g_list_length (channels) > 1)
    gimp_image_undo_group_end (image);

  g_list_free (channels);
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
      GList *channels;
      GList *iter;
      return_if_no_channels (image, channels, data);

      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_DRAWABLE_MOD,
                                   _("Channels to selection"));

      for (iter = channels; iter; iter = iter->next)
        {
          gimp_item_to_selection (iter->data, op, TRUE, FALSE, 0.0, 0.0);

          if (op == GIMP_CHANNEL_OP_REPLACE && iter == channels)
            op = GIMP_CHANNEL_OP_ADD;
        }

      gimp_image_undo_group_end (image);
    }

  gimp_image_flush (image);
}

void
channels_visible_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpImage *image;
  GList     *channels;
  return_if_no_channels (image, channels, data);

  items_visible_cmd_callback (action, value, image, channels);
}

void
channels_lock_content_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage *image;
  GList     *channels;
  return_if_no_channels (image, channels, data);

  items_lock_content_cmd_callback (action, value, image, channels);
}

void
channels_lock_position_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *image;
  GList     *channels;
  return_if_no_channels (image, channels, data);

  items_lock_position_cmd_callback (action, value, image, channels);
}

void
channels_color_tag_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpImage    *image;
  GList        *channels;
  GimpColorTag  color_tag;
  return_if_no_channels (image, channels, data);

  color_tag = (GimpColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, channels, color_tag);
}

void
channels_select_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpImage            *image;
  GList                *channels;
  GList                *new_channels = NULL;
  GList                *iter;
  GimpActionSelectType  select_type;
  gboolean              run_once;
  return_if_no_image (image, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  channels = gimp_image_get_selected_channels (image);
  run_once = (g_list_length (channels) == 0);

  for (iter = channels; iter || run_once; iter = iter ? iter->next : NULL)
    {
      GimpChannel   *new_channel = NULL;
      GimpContainer *container;

      if (iter)
        {
          container = gimp_item_get_container (GIMP_ITEM (iter->data));
        }
      else /* run_once */
        {
          container = gimp_image_get_channels (image);
          run_once  = FALSE;
        }
      new_channel = (GimpChannel *) action_select_object (select_type,
                                                          container,
                                                          iter ? iter->data : NULL);

      if (new_channel)
        new_channels = g_list_prepend (new_channels, new_channel);
    }

  if (new_channels)
    {
      gimp_image_set_selected_channels (image, new_channels);
      gimp_image_flush (image);
    }

  g_list_free (new_channels);
}

/*  private functions  */

static void
channels_new_callback (GtkWidget     *dialog,
                       GimpImage     *image,
                       GimpChannel   *channel,
                       GimpContext   *context,
                       const gchar   *channel_name,
                       GeglColor     *channel_color,
                       gboolean       save_selection,
                       gboolean       channel_visible,
                       GimpColorTag   channel_color_tag,
                       gboolean       channel_lock_content,
                       gboolean       channel_lock_position,
                       gboolean       channel_lock_visibility,
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
      gimp_channel_set_color (channel, config->channel_new_color, FALSE);
    }
  else
    {
      channel = gimp_channel_new (image,
                                  gimp_image_get_width  (image),
                                  gimp_image_get_height (image),
                                  config->channel_new_name,
                                  config->channel_new_color);

      gimp_drawable_fill (GIMP_DRAWABLE (channel), context,
                          GIMP_FILL_TRANSPARENT);
    }

  gimp_item_set_visible (GIMP_ITEM (channel), channel_visible, FALSE);
  gimp_item_set_color_tag (GIMP_ITEM (channel), channel_color_tag, FALSE);
  gimp_item_set_lock_content (GIMP_ITEM (channel), channel_lock_content, FALSE);
  gimp_item_set_lock_position (GIMP_ITEM (channel), channel_lock_position, FALSE);
  gimp_item_set_lock_visibility (GIMP_ITEM (channel), channel_lock_visibility, FALSE);

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
                                   GeglColor     *channel_color,
                                   gboolean       save_selection,
                                   gboolean       channel_visible,
                                   GimpColorTag   channel_color_tag,
                                   gboolean       channel_lock_content,
                                   gboolean       channel_lock_position,
                                   gboolean       channel_lock_visibility,
                                   gpointer       user_data)
{
  GimpItem *item = GIMP_ITEM (channel);

  if (strcmp (channel_name, gimp_object_get_name (channel))                   ||
      ! gimp_color_is_perceptually_identical (channel_color, channel->color) ||
      channel_visible         != gimp_item_get_visible (item)                 ||
      channel_color_tag       != gimp_item_get_color_tag (item)               ||
      channel_lock_content    != gimp_item_get_lock_content (item)            ||
      channel_lock_position   != gimp_item_get_lock_position (item)           ||
      channel_lock_visibility != gimp_item_get_lock_visibility (item))
    {
      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Channel Attributes"));

      if (strcmp (channel_name, gimp_object_get_name (channel)))
        gimp_item_rename (GIMP_ITEM (channel), channel_name, NULL);

      if (! gimp_color_is_perceptually_identical (channel_color, channel->color))
        gimp_channel_set_color (channel, channel_color, TRUE);

      if (channel_visible != gimp_item_get_visible (item))
        gimp_item_set_visible (item, channel_visible, TRUE);

      if (channel_color_tag != gimp_item_get_color_tag (item))
        gimp_item_set_color_tag (item, channel_color_tag, TRUE);

      if (channel_lock_content != gimp_item_get_lock_content (item))
        gimp_item_set_lock_content (item, channel_lock_content, TRUE);

      if (channel_lock_position != gimp_item_get_lock_position (item))
        gimp_item_set_lock_position (item, channel_lock_position, TRUE);

      if (channel_lock_visibility != gimp_item_get_lock_visibility (item))
        gimp_item_set_lock_visibility (item, channel_lock_visibility, TRUE);

      gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}
