/* GIMP - The GNU Image Manipulation Program
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

#include "dialogs-types.h"

#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpviewabledialog.h"

#include "channel-options-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void channel_options_opacity_update (GtkAdjustment        *adjustment,
                                            gpointer              data);
static void channel_options_color_changed  (GimpColorButton      *button,
                                            gpointer              data);
static void channel_options_dialog_free    (ChannelOptionsDialog *options);


/*  public functions  */

ChannelOptionsDialog *
channel_options_dialog_new (GimpImage     *image,
                            GimpChannel   *channel,
                            GimpContext   *context,
                            GtkWidget     *parent,
                            const GimpRGB *channel_color,
                            const gchar   *channel_name,
                            const gchar   *title,
                            const gchar   *role,
                            const gchar   *stock_id,
                            const gchar   *desc,
                            const gchar   *help_id,
                            const gchar   *color_label,
                            const gchar   *opacity_label,
                            gboolean       show_from_sel)
{
  ChannelOptionsDialog *options;
  GimpViewable         *viewable;
  GtkWidget            *hbox;
  GtkWidget            *vbox;
  GtkWidget            *table;
  GtkObject            *opacity_adj;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (channel == NULL || GIMP_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (channel_color != NULL, NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (color_label != NULL, NULL);
  g_return_val_if_fail (opacity_label != NULL, NULL);

  options = g_slice_new0 (ChannelOptionsDialog);

  options->image   = image;
  options->context = context;
  options->channel = channel;

  options->color_panel = gimp_color_panel_new (color_label,
                                               channel_color,
                                               GIMP_COLOR_AREA_LARGE_CHECKS,
                                               48, 64);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (options->color_panel),
                                context);

  if (channel)
    viewable = GIMP_VIEWABLE (channel);
  else
    viewable = GIMP_VIEWABLE (image);

  options->dialog =
    gimp_viewable_dialog_new (viewable, context,
                              title, role, stock_id, desc,
                              parent,
                              gimp_standard_help_func, help_id,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_object_weak_ref (G_OBJECT (options->dialog),
                     (GWeakNotify) channel_options_dialog_free,
                     options);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (options->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->dialog)->vbox), hbox);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (channel_name ? 2 : 1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  if (channel_name)
    {
      options->name_entry = gtk_entry_new ();
      gtk_entry_set_activates_default (GTK_ENTRY (options->name_entry), TRUE);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("Channel _name:"), 0.0, 0.5,
                                 options->name_entry, 2, FALSE);

      gtk_entry_set_text (GTK_ENTRY (options->name_entry), channel_name);
    }

  opacity_adj = gimp_scale_entry_new (GTK_TABLE (table),
                                      0, channel_name ? 1 : 0,
                                      opacity_label, 100, -1,
                                      channel_color->a * 100.0,
                                      0.0, 100.0, 1.0, 10.0, 1,
                                      TRUE, 0.0, 0.0,
                                      NULL, NULL);

  g_signal_connect (opacity_adj, "value-changed",
                    G_CALLBACK (channel_options_opacity_update),
                    options->color_panel);

  gtk_box_pack_start (GTK_BOX (hbox), options->color_panel,
                      TRUE, TRUE, 0);
  gtk_widget_show (options->color_panel);

  g_signal_connect (options->color_panel, "color-changed",
                    G_CALLBACK (channel_options_color_changed),
                    opacity_adj);

  if (show_from_sel)
    {
      options->save_sel_checkbutton =
        gtk_check_button_new_with_mnemonic (_("Initialize from _selection"));

      gtk_box_pack_start (GTK_BOX (vbox), options->save_sel_checkbutton,
                          FALSE, FALSE, 0);
      gtk_widget_show (options->save_sel_checkbutton);
    }

  return options;
}


/*  private functions  */

static void
channel_options_opacity_update (GtkAdjustment *adjustment,
                                gpointer       data)
{
  GimpRGB  color;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (data), &color);
  gimp_rgb_set_alpha (&color, adjustment->value / 100.0);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (data), &color);
}

static void
channel_options_color_changed (GimpColorButton *button,
                               gpointer         data)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT (data);
  GimpRGB        color;

  gimp_color_button_get_color (button, &color);
  gtk_adjustment_set_value (adj, color.a * 100.0);
}

static void
channel_options_dialog_free (ChannelOptionsDialog *options)
{
  g_slice_free (ChannelOptionsDialog, options);
}
