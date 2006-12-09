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

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-quick-mask.h"

#include "widgets/gimphelp-ids.h"

#include "dialogs/channel-options-dialog.h"

#include "actions.h"
#include "quick-mask-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   quick_mask_configure_response (GtkWidget            *widget,
                                             gint                  response_id,
                                             ChannelOptionsDialog *options);


/*  public functionss */

void
quick_mask_toggle_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpImage *image;
  gboolean   active;
  return_if_no_image (image, data);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != gimp_image_get_quick_mask_state (image))
    {
      gimp_image_set_quick_mask_state (image, active);
      gimp_image_flush (image);
    }
}

void
quick_mask_invert_cmd_callback (GtkAction *action,
                                GtkAction *current,
                                gpointer   data)
{
  GimpImage *image;
  gint       value;
  return_if_no_image (image, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value != image->quick_mask_inverted)
    {
      gimp_image_quick_mask_invert (image);
      gimp_image_flush (image);
    }
}

void
quick_mask_configure_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  ChannelOptionsDialog *options;
  GimpImage            *image;
  GtkWidget            *widget;
  GimpRGB               color;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  gimp_image_get_quick_mask_color (image, &color);

  options = channel_options_dialog_new (image, NULL,
                                        action_data_get_context (data),
                                        widget,
                                        &color,
                                        NULL,
                                        _("Quick Mask Attributes"),
                                        "gimp-quick-mask-edit",
                                        GIMP_STOCK_QUICK_MASK_ON,
                                        _("Edit Quick Mask Attributes"),
                                        GIMP_HELP_QUICK_MASK_EDIT,
                                        _("Edit Quick Mask Color"),
                                        _("_Mask opacity:"),
                                        FALSE);

  g_signal_connect (options->dialog, "response",
                    G_CALLBACK (quick_mask_configure_response),
                    options);

  gtk_widget_show (options->dialog);
}


/*  private functions  */

static void
quick_mask_configure_response (GtkWidget            *widget,
                               gint                  response_id,
                               ChannelOptionsDialog *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpRGB old_color;
      GimpRGB new_color;

      gimp_image_get_quick_mask_color (options->image, &old_color);
      gimp_color_button_get_color (GIMP_COLOR_BUTTON (options->color_panel),
                                   &new_color);

      if (gimp_rgba_distance (&old_color, &new_color) > 0.0001)
        {
          gimp_image_set_quick_mask_color (options->image, &new_color);

          gimp_image_flush (options->image);
        }
    }

  gtk_widget_destroy (options->dialog);
}
