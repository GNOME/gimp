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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpspinscale.h"
#include "widgets/gimpviewabledialog.h"

#include "channel-options-dialog.h"

#include "gimp-intl.h"


typedef struct _ChannelOptionsDialog ChannelOptionsDialog;

struct _ChannelOptionsDialog
{
  GimpImage                  *image;
  GimpContext                *context;
  GimpChannel                *channel;
  GimpChannelOptionsCallback  callback;
  gpointer                    user_data;

  GtkWidget                  *name_entry;
  GtkWidget                  *color_panel;
  GtkWidget                  *save_sel_checkbutton;
};


/*  local function prototypes  */

static void channel_options_dialog_response (GtkWidget            *dialog,
                                             gint                  response_id,
                                             ChannelOptionsDialog *private);
static void channel_options_opacity_update  (GtkAdjustment        *adjustment,
                                             GimpColorButton      *color_button);
static void channel_options_color_changed   (GimpColorButton      *color_button,
                                             GtkAdjustment        *adjustment);
static void channel_options_dialog_free     (ChannelOptionsDialog *private);


/*  public functions  */

GtkWidget *
channel_options_dialog_new (GimpImage                  *image,
                            GimpChannel                *channel,
                            GimpContext                *context,
                            GtkWidget                  *parent,
                            const gchar                *title,
                            const gchar                *role,
                            const gchar                *icon_name,
                            const gchar                *desc,
                            const gchar                *help_id,
                            const gchar                *channel_name,
                            const GimpRGB              *channel_color,
                            const gchar                *color_label,
                            const gchar                *opacity_label,
                            gboolean                    show_from_sel,
                            GimpChannelOptionsCallback  callback,
                            gpointer                    user_data)
{
  ChannelOptionsDialog *private;
  GtkWidget            *dialog;
  GimpViewable         *viewable;
  GtkWidget            *hbox;
  GtkWidget            *vbox;
  GtkAdjustment        *opacity_adj;
  GtkWidget            *scale;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (channel == NULL || GIMP_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (channel_color != NULL, NULL);
  g_return_val_if_fail (color_label != NULL, NULL);
  g_return_val_if_fail (opacity_label != NULL, NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (ChannelOptionsDialog);

  private->image     = image;
  private->channel   = channel;
  private->context   = context;
  private->callback  = callback;
  private->user_data = user_data;

  if (channel)
    viewable = GIMP_VIEWABLE (channel);
  else
    viewable = GIMP_VIEWABLE (image);

  dialog = gimp_viewable_dialog_new (viewable, context,
                                     title, role, icon_name, desc,
                                     parent,
                                     gimp_standard_help_func, help_id,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) channel_options_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (channel_options_dialog_response),
                    private);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  if (channel_name)
    {
      GtkWidget *vbox2;
      GtkWidget *label;

      vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
      gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
      gtk_widget_show (vbox2);

      label = gtk_label_new_with_mnemonic (_("Channel _name:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      private->name_entry = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (private->name_entry), channel_name);
      gtk_entry_set_activates_default (GTK_ENTRY (private->name_entry), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox2), private->name_entry,
                          FALSE, FALSE, 0);
      gtk_widget_show (private->name_entry);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), private->name_entry);
    }

  opacity_adj = (GtkAdjustment *)
                gtk_adjustment_new (channel_color->a * 100.0,
                                    0.0, 100.0, 1.0, 10.0, 0);
  scale = gimp_spin_scale_new (opacity_adj, opacity_label, 1);
  gtk_widget_set_size_request (scale, 200, -1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  private->color_panel = gimp_color_panel_new (color_label,
                                               channel_color,
                                               GIMP_COLOR_AREA_LARGE_CHECKS,
                                               48, 48);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (private->color_panel),
                                context);

  g_signal_connect (opacity_adj, "value-changed",
                    G_CALLBACK (channel_options_opacity_update),
                    private->color_panel);

  gtk_box_pack_start (GTK_BOX (hbox), private->color_panel,
                      FALSE, FALSE, 0);
  gtk_widget_show (private->color_panel);

  g_signal_connect (private->color_panel, "color-changed",
                    G_CALLBACK (channel_options_color_changed),
                    opacity_adj);

  if (show_from_sel)
    {
      private->save_sel_checkbutton =
        gtk_check_button_new_with_mnemonic (_("Initialize from _selection"));

      gtk_box_pack_start (GTK_BOX (vbox), private->save_sel_checkbutton,
                          FALSE, FALSE, 0);
      gtk_widget_show (private->save_sel_checkbutton);
    }

  return dialog;
}


/*  private functions  */

static void
channel_options_dialog_response (GtkWidget            *dialog,
                                 gint                  response_id,
                                 ChannelOptionsDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *name           = NULL;
      GimpRGB      color;
      gboolean     save_selection = FALSE;

      if (private->name_entry)
        name = gtk_entry_get_text (GTK_ENTRY (private->name_entry));

      gimp_color_button_get_color (GIMP_COLOR_BUTTON (private->color_panel),
                                   &color);

      if (private->save_sel_checkbutton)
        save_selection =
          gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (private->save_sel_checkbutton));

      private->callback (dialog,
                         private->image,
                         private->channel,
                         private->context,
                         name,
                         &color,
                         save_selection,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static void
channel_options_opacity_update (GtkAdjustment   *adjustment,
                                GimpColorButton *color_button)
{
  GimpRGB  color;

  gimp_color_button_get_color (color_button, &color);
  gimp_rgb_set_alpha (&color, gtk_adjustment_get_value (adjustment) / 100.0);
  gimp_color_button_set_color (color_button, &color);
}

static void
channel_options_color_changed (GimpColorButton *button,
                               GtkAdjustment   *adjustment)
{
  GimpRGB        color;

  gimp_color_button_get_color (button, &color);
  gtk_adjustment_set_value (adjustment, color.a * 100.0);
}

static void
channel_options_dialog_free (ChannelOptionsDialog *options)
{
  g_slice_free (ChannelOptionsDialog, options);
}
