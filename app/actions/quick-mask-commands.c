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
#include "core/gimpimage-qmask.h"

#include "widgets/gimphelp-ids.h"

#include "dialogs/channel-options-dialog.h"

#include "actions.h"
#include "qmask-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   qmask_configure_response (GtkWidget            *widget,
                                        gint                  response_id,
                                        ChannelOptionsDialog *options);


/*  public functionss */

void
qmask_toggle_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GimpImage *gimage;
  gboolean   active;
  return_if_no_image (gimage, data);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != gimp_image_get_qmask_state (gimage))
    {
      gimp_image_set_qmask_state (gimage, active);
      gimp_image_flush (gimage);
    }
}

void
qmask_invert_cmd_callback (GtkAction *action,
                           GtkAction *current,
                           gpointer   data)
{
  GimpImage *gimage;
  gint       value;
  return_if_no_image (gimage, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value != gimage->qmask_inverted)
    {
      gimp_image_qmask_invert (gimage);
      gimp_image_flush (gimage);
    }
}

void
qmask_configure_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  ChannelOptionsDialog *options;
  GimpImage            *gimage;
  GtkWidget            *widget;
  GimpRGB               color;
  return_if_no_image (gimage, data);
  return_if_no_widget (widget, data);

  gimp_image_get_qmask_color (gimage, &color);

  options = channel_options_dialog_new (gimage,
                                        action_data_get_context (data),
                                        NULL,
                                        widget,
                                        &color,
                                        NULL,
                                        _("Quick Mask Attributes"),
                                        "gimp-qmask-edit",
                                        GIMP_STOCK_QMASK_ON,
                                        _("Edit Quick Mask Attributes"),
                                        GIMP_HELP_QMASK_EDIT,
                                        _("Edit Quick Mask Color"),
                                        _("Mask Opacity:"));

  g_signal_connect (options->dialog, "response",
                    G_CALLBACK (qmask_configure_response),
                    options);

  gtk_widget_show (options->dialog);
}


/*  private functions  */

static void
qmask_configure_response (GtkWidget            *widget,
                          gint                  response_id,
                          ChannelOptionsDialog *options)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpRGB old_color;
      GimpRGB new_color;

      gimp_image_get_qmask_color (options->gimage, &old_color);
      gimp_color_button_get_color (GIMP_COLOR_BUTTON (options->color_panel),
                                   &new_color);

      if (gimp_rgba_distance (&old_color, &new_color) > 0.0001)
        {
          gimp_image_set_qmask_color (options->gimage, &new_color);

          gimp_image_flush (options->gimage);
        }
    }

  gtk_widget_destroy (options->dialog);
}
