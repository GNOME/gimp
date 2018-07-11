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
#include "item-options-dialog.h"

#include "gimp-intl.h"


typedef struct _ChannelOptionsDialog ChannelOptionsDialog;

struct _ChannelOptionsDialog
{
  GimpChannelOptionsCallback  callback;
  gpointer                    user_data;

  GtkWidget                  *color_panel;
  GtkWidget                  *save_sel_toggle;
};


/*  local function prototypes  */

static void channel_options_dialog_free     (ChannelOptionsDialog *private);
static void channel_options_dialog_callback (GtkWidget            *dialog,
                                             GimpImage            *image,
                                             GimpItem             *item,
                                             GimpContext          *context,
                                             const gchar          *item_name,
                                             gboolean              item_visible,
                                             gboolean              item_linked,
                                             GimpColorTag          item_color_tag,
                                             gboolean              item_lock_content,
                                             gboolean              item_lock_position,
                                             gpointer              user_data);
static void channel_options_opacity_changed (GtkAdjustment        *adjustment,
                                             GimpColorButton      *color_button);
static void channel_options_color_changed   (GimpColorButton      *color_button,
                                             GtkAdjustment        *adjustment);


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
                            const gchar                *color_label,
                            const gchar                *opacity_label,
                            gboolean                    show_from_sel,
                            const gchar                *channel_name,
                            const GimpRGB              *channel_color,
                            gboolean                    channel_visible,
                            gboolean                    channel_linked,
                            GimpColorTag                channel_color_tag,
                            gboolean                    channel_lock_content,
                            gboolean                    channel_lock_position,
                            GimpChannelOptionsCallback  callback,
                            gpointer                    user_data)
{
  ChannelOptionsDialog *private;
  GtkWidget            *dialog;
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

  private->callback  = callback;
  private->user_data = user_data;

  dialog = item_options_dialog_new (image, GIMP_ITEM (channel), context,
                                    parent, title, role,
                                    icon_name, desc, help_id,
                                    channel_name ? _("Channel _name:") : NULL,
                                    GIMP_ICON_TOOL_PAINTBRUSH,
                                    _("Lock _pixels"),
                                    _("Lock position and _size"),
                                    channel_name,
                                    channel_visible,
                                    channel_linked,
                                    channel_color_tag,
                                    channel_lock_content,
                                    channel_lock_position,
                                    channel_options_dialog_callback,
                                    private);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) channel_options_dialog_free, private);

  opacity_adj = (GtkAdjustment *)
                gtk_adjustment_new (channel_color->a * 100.0,
                                    0.0, 100.0, 1.0, 10.0, 0);
  scale = gimp_spin_scale_new (opacity_adj, NULL, 1);
  gtk_widget_set_size_request (scale, 200, -1);
  item_options_dialog_add_widget (dialog,
                                  opacity_label, scale);

  private->color_panel = gimp_color_panel_new (color_label,
                                               channel_color,
                                               GIMP_COLOR_AREA_LARGE_CHECKS,
                                               24, 24);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (private->color_panel),
                                context);

  g_signal_connect (opacity_adj, "value-changed",
                    G_CALLBACK (channel_options_opacity_changed),
                    private->color_panel);

  g_signal_connect (private->color_panel, "color-changed",
                    G_CALLBACK (channel_options_color_changed),
                    opacity_adj);

  item_options_dialog_add_widget (dialog,
                                  NULL, private->color_panel);

  if (show_from_sel)
    {
      private->save_sel_toggle =
        gtk_check_button_new_with_mnemonic (_("Initialize from _selection"));

      item_options_dialog_add_widget (dialog,
                                      NULL, private->save_sel_toggle);
    }

  return dialog;
}


/*  private functions  */

static void
channel_options_dialog_free (ChannelOptionsDialog *private)
{
  g_slice_free (ChannelOptionsDialog, private);
}

static void
channel_options_dialog_callback (GtkWidget    *dialog,
                                 GimpImage    *image,
                                 GimpItem     *item,
                                 GimpContext  *context,
                                 const gchar  *item_name,
                                 gboolean      item_visible,
                                 gboolean      item_linked,
                                 GimpColorTag  item_color_tag,
                                 gboolean      item_lock_content,
                                 gboolean      item_lock_position,
                                 gpointer      user_data)
{
  ChannelOptionsDialog *private = user_data;
  GimpRGB               color;
  gboolean              save_selection = FALSE;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (private->color_panel),
                               &color);

  if (private->save_sel_toggle)
    save_selection =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (private->save_sel_toggle));

  private->callback (dialog,
                     image,
                     GIMP_CHANNEL (item),
                     context,
                     item_name,
                     &color,
                     save_selection,
                     item_visible,
                     item_linked,
                     item_color_tag,
                     item_lock_content,
                     item_lock_position,
                     private->user_data);
}

static void
channel_options_opacity_changed (GtkAdjustment   *adjustment,
                                 GimpColorButton *color_button)
{
  GimpRGB color;

  gimp_color_button_get_color (color_button, &color);
  gimp_rgb_set_alpha (&color, gtk_adjustment_get_value (adjustment) / 100.0);
  gimp_color_button_set_color (color_button, &color);
}

static void
channel_options_color_changed (GimpColorButton *button,
                               GtkAdjustment   *adjustment)
{
  GimpRGB color;

  gimp_color_button_get_color (button, &color);
  gtk_adjustment_set_value (adjustment, color.a * 100.0);
}
