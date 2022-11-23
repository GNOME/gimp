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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-quick-mask.h"

#include "widgets/ligmahelp-ids.h"

#include "dialogs/dialogs.h"
#include "dialogs/channel-options-dialog.h"
#include "dialogs/item-options-dialog.h"

#include "actions.h"
#include "quick-mask-commands.h"

#include "ligma-intl.h"


#define RGBA_EPSILON 1e-6


/*  local function prototypes  */

static void   quick_mask_configure_callback (GtkWidget     *dialog,
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


/*  public functions */

void
quick_mask_toggle_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaImage *image;
  gboolean   active;
  return_if_no_image (image, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_image_get_quick_mask_state (image))
    {
      ligma_image_set_quick_mask_state (image, active);
      ligma_image_flush (image);
    }
}

void
quick_mask_invert_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaImage *image;
  gboolean   inverted;
  return_if_no_image (image, data);

  inverted = (gboolean) g_variant_get_int32 (value);

  if (inverted != ligma_image_get_quick_mask_inverted (image))
    {
      ligma_image_quick_mask_invert (image);
      ligma_image_flush (image);
    }
}

void
quick_mask_configure_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define CONFIGURE_DIALOG_KEY "ligma-image-quick-mask-configure-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), CONFIGURE_DIALOG_KEY);

  if (! dialog)
    {
      LigmaRGB color;

      ligma_image_get_quick_mask_color (image, &color);

      dialog = channel_options_dialog_new (image, NULL,
                                           action_data_get_context (data),
                                           widget,
                                           _("Quick Mask Attributes"),
                                           "ligma-quick-mask-edit",
                                           LIGMA_ICON_QUICK_MASK_ON,
                                           _("Edit Quick Mask Attributes"),
                                           LIGMA_HELP_QUICK_MASK_EDIT,
                                           _("Edit Quick Mask Color"),
                                           _("_Mask opacity:"),
                                           FALSE,
                                           NULL,
                                           &color,
                                           FALSE,
                                           LIGMA_COLOR_TAG_NONE,
                                           FALSE,
                                           FALSE,
                                           quick_mask_configure_callback,
                                           NULL);

      item_options_dialog_set_switches_visible (dialog, FALSE);

      dialogs_attach_dialog (G_OBJECT (image), CONFIGURE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
quick_mask_configure_callback (GtkWidget     *dialog,
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
  LigmaRGB old_color;

  ligma_image_get_quick_mask_color (image, &old_color);

  if (ligma_rgba_distance (&old_color, channel_color) > RGBA_EPSILON)
    {
      ligma_image_set_quick_mask_color (image, channel_color);
      ligma_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}
