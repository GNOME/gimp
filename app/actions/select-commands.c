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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmadialogconfig.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmaimage.h"
#include "core/ligmaselection.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmawidgets-utils.h"
#include "widgets/ligmawindowstrategy.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "items-commands.h"
#include "select-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   select_feather_callback (GtkWidget *widget,
                                       gdouble    size,
                                       LigmaUnit   unit,
                                       gpointer   data);
static void   select_border_callback  (GtkWidget *widget,
                                       gdouble    size,
                                       LigmaUnit   unit,
                                       gpointer   data);
static void   select_grow_callback    (GtkWidget *widget,
                                       gdouble    size,
                                       LigmaUnit   unit,
                                       gpointer   data);
static void   select_shrink_callback  (GtkWidget *widget,
                                       gdouble    size,
                                       LigmaUnit   unit,
                                       gpointer   data);


/*  public functions  */

void
select_all_cmd_callback (LigmaAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  ligma_channel_all (ligma_image_get_mask (image), TRUE);
  ligma_image_flush (image);
}

void
select_none_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  ligma_channel_clear (ligma_image_get_mask (image), NULL, TRUE);
  ligma_image_flush (image);
}

void
select_invert_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  ligma_channel_invert (ligma_image_get_mask (image), TRUE);
  ligma_image_flush (image);
}

void
select_float_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaImage *image;
  GtkWidget *widget;
  GList     *drawables;
  GError    *error = NULL;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  drawables = ligma_image_get_selected_drawables (image);
  if (ligma_selection_float (LIGMA_SELECTION (ligma_image_get_mask (image)),
                            drawables,
                            action_data_get_context (data),
                            TRUE, 0, 0, &error))
    {
      ligma_image_flush (image);
    }
  else
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (widget), LIGMA_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
  g_list_free (drawables);
}

void
select_feather_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

#define FEATHER_DIALOG_KEY "ligma-selection-feather-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), FEATHER_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
      GtkWidget        *button;
      gdouble           xres;
      gdouble           yres;

      ligma_image_get_resolution (image, &xres, &yres);

      dialog = ligma_query_size_box (_("Feather Selection"),
                                    GTK_WIDGET (ligma_display_get_shell (display)),
                                    ligma_standard_help_func,
                                    LIGMA_HELP_SELECTION_FEATHER,
                                    _("Feather selection by"),
                                    config->selection_feather_radius, 0, 32767, 3,
                                    ligma_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_feather_callback,
                                    image, NULL);

      /* Edge lock button */
      button = gtk_check_button_new_with_mnemonic (_("_Selected areas continue outside the image"));
      g_object_set_data (G_OBJECT (dialog), "edge-lock-toggle", button);
      ligma_help_set_help_data (button,
                               _("When feathering, act as if selected areas "
                                 "continued outside the image."),
                               NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    config->selection_feather_edge_lock);
      gtk_box_pack_start (GTK_BOX (LIGMA_QUERY_BOX_VBOX (dialog)), button,
                          FALSE, FALSE, 0);
      gtk_widget_show (button);

      dialogs_attach_dialog (G_OBJECT (image), FEATHER_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_sharpen_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  ligma_channel_sharpen (ligma_image_get_mask (image), TRUE);
  ligma_image_flush (image);
}

void
select_shrink_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

#define SHRINK_DIALOG_KEY "ligma-selection-shrink-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), SHRINK_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
      GtkWidget        *button;
      gint              width;
      gint              height;
      gint              max_value;
      gdouble           xres;
      gdouble           yres;

      ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                        NULL, NULL, &width, &height);
      max_value = MIN (width, height) / 2;

      ligma_image_get_resolution (image, &xres, &yres);

      dialog = ligma_query_size_box (_("Shrink Selection"),
                                    GTK_WIDGET (ligma_display_get_shell (display)),
                                    ligma_standard_help_func,
                                    LIGMA_HELP_SELECTION_SHRINK,
                                    _("Shrink selection by"),
                                    config->selection_shrink_radius,
                                    1, max_value, 0,
                                    ligma_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_shrink_callback,
                                    image, NULL);

      /* Edge lock button */
      button = gtk_check_button_new_with_mnemonic (_("_Selected areas continue outside the image"));
      g_object_set_data (G_OBJECT (dialog), "edge-lock-toggle", button);
      ligma_help_set_help_data (button,
                               _("When shrinking, act as if selected areas "
                                 "continued outside the image."),
                               NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    config->selection_shrink_edge_lock);
      gtk_box_pack_start (GTK_BOX (LIGMA_QUERY_BOX_VBOX (dialog)), button,
                          FALSE, FALSE, 0);
      gtk_widget_show (button);

      dialogs_attach_dialog (G_OBJECT (image), SHRINK_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_grow_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

#define GROW_DIALOG_KEY "ligma-selection-grow-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), GROW_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
      gint              width;
      gint              height;
      gint              max_value;
      gdouble           xres;
      gdouble           yres;

      width  = ligma_image_get_width  (image);
      height = ligma_image_get_height (image);
      max_value = MAX (width, height);

      ligma_image_get_resolution (image, &xres, &yres);

      dialog = ligma_query_size_box (_("Grow Selection"),
                                    GTK_WIDGET (ligma_display_get_shell (display)),
                                    ligma_standard_help_func,
                                    LIGMA_HELP_SELECTION_GROW,
                                    _("Grow selection by"),
                                    config->selection_grow_radius,
                                    1, max_value, 0,
                                    ligma_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_grow_callback,
                                    image, NULL);

      dialogs_attach_dialog (G_OBJECT (image), GROW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_border_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = ligma_display_get_image (display);

#define BORDER_DIALOG_KEY "ligma-selection-border-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), BORDER_DIALOG_KEY);

  if (! dialog)
    {
      LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
      GtkWidget        *combo;
      GtkWidget        *button;
      gint              width;
      gint              height;
      gint              max_value;
      gdouble           xres;
      gdouble           yres;

      ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                        NULL, NULL, &width, &height);
      max_value = MIN (width, height) / 2;

      ligma_image_get_resolution (image, &xres, &yres);

      dialog = ligma_query_size_box (_("Border Selection"),
                                    GTK_WIDGET (ligma_display_get_shell (display)),
                                    ligma_standard_help_func,
                                    LIGMA_HELP_SELECTION_BORDER,
                                    _("Border selection by"),
                                    config->selection_border_radius,
                                    1, max_value, 0,
                                    ligma_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_border_callback,
                                    image, NULL);

      /* Border style combo */
      combo = ligma_enum_combo_box_new (LIGMA_TYPE_CHANNEL_BORDER_STYLE);
      ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo),
                                    _("Border style"));

      gtk_box_pack_start (GTK_BOX (LIGMA_QUERY_BOX_VBOX (dialog)), combo,
                          FALSE, FALSE, 0);

      g_object_set_data (G_OBJECT (dialog), "border-style-combo", combo);
      ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (combo),
                                     config->selection_border_style);
      gtk_widget_show (combo);

      /* Edge lock button */
      button = gtk_check_button_new_with_mnemonic (_("_Selected areas continue outside the image"));
      g_object_set_data (G_OBJECT (dialog), "edge-lock-toggle", button);
      ligma_help_set_help_data (button,
                               _("When bordering, act as if selected areas "
                                 "continued outside the image."),
                               NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    config->selection_border_edge_lock);
      gtk_box_pack_start (GTK_BOX (LIGMA_QUERY_BOX_VBOX (dialog)), button,
                          FALSE, FALSE, 0);
      gtk_widget_show (button);

      dialogs_attach_dialog (G_OBJECT (image), BORDER_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_flood_cmd_callback (LigmaAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  ligma_channel_flood (ligma_image_get_mask (image), TRUE);
  ligma_image_flush (image);
}

void
select_save_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  LigmaImage   *image;
  LigmaChannel *channel;
  GtkWidget   *widget;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  channel = LIGMA_CHANNEL (ligma_item_duplicate (LIGMA_ITEM (ligma_image_get_mask (image)),
                                               LIGMA_TYPE_CHANNEL));

  /*  saved selections are not visible by default  */
  ligma_item_set_visible (LIGMA_ITEM (channel), FALSE, FALSE);

  ligma_image_add_channel (image, channel,
                          LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);
  ligma_image_flush (image);

  ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (image->ligma)),
                                             image->ligma,
                                             ligma_dialog_factory_get_singleton (),
                                             ligma_widget_get_monitor (widget),
                                             "ligma-channel-list");
}

void
select_fill_cmd_callback (LigmaAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  items_fill_cmd_callback (action,
                           image, LIGMA_ITEM (ligma_image_get_mask (image)),
                           "ligma-selection-fill-dialog",
                           _("Fill Selection Outline"),
                           LIGMA_ICON_TOOL_BUCKET_FILL,
                           LIGMA_HELP_SELECTION_FILL,
                           data);
}

void
select_fill_last_vals_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  items_fill_last_vals_cmd_callback (action,
                                     image,
                                     LIGMA_ITEM (ligma_image_get_mask (image)),
                                     data);
}

void
select_stroke_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  items_stroke_cmd_callback (action,
                             image, LIGMA_ITEM (ligma_image_get_mask (image)),
                             "ligma-selection-stroke-dialog",
                             _("Stroke Selection"),
                             LIGMA_ICON_SELECTION_STROKE,
                             LIGMA_HELP_SELECTION_STROKE,
                             data);
}

void
select_stroke_last_vals_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaImage *image;
  return_if_no_image (image, data);

  items_stroke_last_vals_cmd_callback (action,
                                       image,
                                       LIGMA_ITEM (ligma_image_get_mask (image)),
                                       data);
}


/*  private functions  */

static void
select_feather_callback (GtkWidget *widget,
                         gdouble    size,
                         LigmaUnit   unit,
                         gpointer   data)
{
  LigmaImage        *image  = LIGMA_IMAGE (data);
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
  GtkWidget        *button;
  gdouble           radius_x;
  gdouble           radius_y;

  button = g_object_get_data (G_OBJECT (widget), "edge-lock-toggle");

  g_object_set (config,
                "selection-feather-radius", size,
                "selection-feather-edge-lock",
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)),
                NULL);

  radius_x = config->selection_feather_radius;
  radius_y = config->selection_feather_radius;

  if (unit != LIGMA_UNIT_PIXEL)
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      ligma_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  ligma_channel_feather (ligma_image_get_mask (image), radius_x, radius_y,
                        config->selection_feather_edge_lock,
                        TRUE);
  ligma_image_flush (image);
}

static void
select_border_callback (GtkWidget *widget,
                        gdouble    size,
                        LigmaUnit   unit,
                        gpointer   data)
{
  LigmaImage        *image  = LIGMA_IMAGE (data);
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
  GtkWidget        *combo;
  GtkWidget        *button;
  gdouble           radius_x;
  gdouble           radius_y;
  gint              border_style;

  combo  = g_object_get_data (G_OBJECT (widget), "border-style-combo");
  button = g_object_get_data (G_OBJECT (widget), "edge-lock-toggle");

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (combo), &border_style);

  g_object_set (config,
                "selection-border-radius", size,
                "selection-border-style",  border_style,
                "selection-border-edge-lock",
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)),
                NULL);

  radius_x = ROUND (config->selection_border_radius);
  radius_y = ROUND (config->selection_border_radius);

  if (unit != LIGMA_UNIT_PIXEL)
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      ligma_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  ligma_channel_border (ligma_image_get_mask (image), radius_x, radius_y,
                       config->selection_border_style,
                       config->selection_border_edge_lock,
                       TRUE);
  ligma_image_flush (image);
}

static void
select_grow_callback (GtkWidget *widget,
                      gdouble    size,
                      LigmaUnit   unit,
                      gpointer   data)
{
  LigmaImage        *image  = LIGMA_IMAGE (data);
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
  gdouble           radius_x;
  gdouble           radius_y;

  g_object_set (config,
                "selection-grow-radius", size,
                NULL);

  radius_x = ROUND (config->selection_grow_radius);
  radius_y = ROUND (config->selection_grow_radius);

  if (unit != LIGMA_UNIT_PIXEL)
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      ligma_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  ligma_channel_grow (ligma_image_get_mask (image), radius_x, radius_y, TRUE);
  ligma_image_flush (image);
}

static void
select_shrink_callback (GtkWidget *widget,
                        gdouble    size,
                        LigmaUnit   unit,
                        gpointer   data)
{
  LigmaImage        *image  = LIGMA_IMAGE (data);
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (image->ligma->config);
  GtkWidget        *button;
  gint              radius_x;
  gint              radius_y;

  button = g_object_get_data (G_OBJECT (widget), "edge-lock-toggle");

  g_object_set (config,
                "selection-shrink-radius", size,
                "selection-shrink-edge-lock",
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)),
                NULL);

  radius_x = ROUND (config->selection_shrink_radius);
  radius_y = ROUND (config->selection_shrink_radius);

  if (unit != LIGMA_UNIT_PIXEL)
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      ligma_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  ligma_channel_shrink (ligma_image_get_mask (image), radius_x, radius_y,
                       config->selection_shrink_edge_lock,
                       TRUE);
  ligma_image_flush (image);
}
