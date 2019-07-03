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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpdialogconfig.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpselection.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpwidgets-utils.h"
#include "widgets/gimpwindowstrategy.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "items-commands.h"
#include "select-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   select_feather_callback (GtkWidget *widget,
                                       gdouble    size,
                                       GimpUnit   unit,
                                       gpointer   data);
static void   select_border_callback  (GtkWidget *widget,
                                       gdouble    size,
                                       GimpUnit   unit,
                                       gpointer   data);
static void   select_grow_callback    (GtkWidget *widget,
                                       gdouble    size,
                                       GimpUnit   unit,
                                       gpointer   data);
static void   select_shrink_callback  (GtkWidget *widget,
                                       gdouble    size,
                                       GimpUnit   unit,
                                       gpointer   data);


/*  public functions  */

void
select_all_cmd_callback (GimpAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  gimp_channel_all (gimp_image_get_mask (image), TRUE);
  gimp_image_flush (image);
}

void
select_none_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  gimp_channel_clear (gimp_image_get_mask (image), NULL, TRUE);
  gimp_image_flush (image);
}

void
select_invert_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  gimp_channel_invert (gimp_image_get_mask (image), TRUE);
  gimp_image_flush (image);
}

void
select_float_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  GtkWidget *widget;
  GError    *error = NULL;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  if (gimp_selection_float (GIMP_SELECTION (gimp_image_get_mask (image)),
                            gimp_image_get_active_drawable (image),
                            action_data_get_context (data),
                            TRUE, 0, 0, &error))
    {
      gimp_image_flush (image);
    }
  else
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (widget), GIMP_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}

void
select_feather_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpDisplay *display;
  GimpImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

#define FEATHER_DIALOG_KEY "gimp-selection-feather-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), FEATHER_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
      GtkWidget        *button;
      gdouble           xres;
      gdouble           yres;

      gimp_image_get_resolution (image, &xres, &yres);

      dialog = gimp_query_size_box (_("Feather Selection"),
                                    GTK_WIDGET (gimp_display_get_shell (display)),
                                    gimp_standard_help_func,
                                    GIMP_HELP_SELECTION_FEATHER,
                                    _("Feather selection by"),
                                    config->selection_feather_radius, 0, 32767, 3,
                                    gimp_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_feather_callback, image);

      /* Edge lock button */
      button = gtk_check_button_new_with_mnemonic (_("_Selected areas continue outside the image"));
      g_object_set_data (G_OBJECT (dialog), "edge-lock-toggle", button);
      gimp_help_set_help_data (button,
                               _("When feathering, act as if selected areas "
                                 "continued outside the image."),
                               NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    config->selection_feather_edge_lock);
      gtk_box_pack_start (GTK_BOX (GIMP_QUERY_BOX_VBOX (dialog)), button,
                          FALSE, FALSE, 0);
      gtk_widget_show (button);

      dialogs_attach_dialog (G_OBJECT (image), FEATHER_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_sharpen_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  gimp_channel_sharpen (gimp_image_get_mask (image), TRUE);
  gimp_image_flush (image);
}

void
select_shrink_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpDisplay *display;
  GimpImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

#define SHRINK_DIALOG_KEY "gimp-selection-shrink-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), SHRINK_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
      GtkWidget        *button;
      gint              width;
      gint              height;
      gint              max_value;
      gdouble           xres;
      gdouble           yres;

      gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                        NULL, NULL, &width, &height);
      max_value = MIN (width, height) / 2;

      gimp_image_get_resolution (image, &xres, &yres);

      dialog = gimp_query_size_box (_("Shrink Selection"),
                                    GTK_WIDGET (gimp_display_get_shell (display)),
                                    gimp_standard_help_func,
                                    GIMP_HELP_SELECTION_SHRINK,
                                    _("Shrink selection by"),
                                    config->selection_shrink_radius,
                                    1, max_value, 0,
                                    gimp_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_shrink_callback, image);

      /* Edge lock button */
      button = gtk_check_button_new_with_mnemonic (_("_Selected areas continue outside the image"));
      g_object_set_data (G_OBJECT (dialog), "edge-lock-toggle", button);
      gimp_help_set_help_data (button,
                               _("When shrinking, act as if selected areas "
                                 "continued outside the image."),
                               NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    config->selection_shrink_edge_lock);
      gtk_box_pack_start (GTK_BOX (GIMP_QUERY_BOX_VBOX (dialog)), button,
                          FALSE, FALSE, 0);
      gtk_widget_show (button);

      dialogs_attach_dialog (G_OBJECT (image), SHRINK_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_grow_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpDisplay *display;
  GimpImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

#define GROW_DIALOG_KEY "gimp-selection-grow-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), GROW_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
      gint              width;
      gint              height;
      gint              max_value;
      gdouble           xres;
      gdouble           yres;

      width  = gimp_image_get_width  (image);
      height = gimp_image_get_height (image);
      max_value = MAX (width, height);

      gimp_image_get_resolution (image, &xres, &yres);

      dialog = gimp_query_size_box (_("Grow Selection"),
                                    GTK_WIDGET (gimp_display_get_shell (display)),
                                    gimp_standard_help_func,
                                    GIMP_HELP_SELECTION_GROW,
                                    _("Grow selection by"),
                                    config->selection_grow_radius,
                                    1, max_value, 0,
                                    gimp_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_grow_callback, image);

      dialogs_attach_dialog (G_OBJECT (image), GROW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_border_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpDisplay *display;
  GimpImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = gimp_display_get_image (display);

#define BORDER_DIALOG_KEY "gimp-selection-border-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), BORDER_DIALOG_KEY);

  if (! dialog)
    {
      GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
      GtkWidget        *combo;
      GtkWidget        *button;
      gint              width;
      gint              height;
      gint              max_value;
      gdouble           xres;
      gdouble           yres;

      gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                        NULL, NULL, &width, &height);
      max_value = MIN (width, height) / 2;

      gimp_image_get_resolution (image, &xres, &yres);

      dialog = gimp_query_size_box (_("Border Selection"),
                                    GTK_WIDGET (gimp_display_get_shell (display)),
                                    gimp_standard_help_func,
                                    GIMP_HELP_SELECTION_BORDER,
                                    _("Border selection by"),
                                    config->selection_border_radius,
                                    1, max_value, 0,
                                    gimp_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_border_callback, image);

      /* Border style combo */
      combo = gimp_enum_combo_box_new (GIMP_TYPE_CHANNEL_BORDER_STYLE);
      gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo),
                                    _("Border style"));

      gtk_box_pack_start (GTK_BOX (GIMP_QUERY_BOX_VBOX (dialog)), combo,
                          FALSE, FALSE, 0);

      g_object_set_data (G_OBJECT (dialog), "border-style-combo", combo);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                     config->selection_border_style);
      gtk_widget_show (combo);

      /* Edge lock button */
      button = gtk_check_button_new_with_mnemonic (_("_Selected areas continue outside the image"));
      g_object_set_data (G_OBJECT (dialog), "edge-lock-toggle", button);
      gimp_help_set_help_data (button,
                               _("When bordering, act as if selected areas "
                                 "continued outside the image."),
                               NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    config->selection_border_edge_lock);
      gtk_box_pack_start (GTK_BOX (GIMP_QUERY_BOX_VBOX (dialog)), button,
                          FALSE, FALSE, 0);
      gtk_widget_show (button);

      dialogs_attach_dialog (G_OBJECT (image), BORDER_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_flood_cmd_callback (GimpAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  gimp_channel_flood (gimp_image_get_mask (image), TRUE);
  gimp_image_flush (image);
}

void
select_save_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpImage   *image;
  GimpChannel *channel;
  GtkWidget   *widget;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  channel = GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (gimp_image_get_mask (image)),
                                               GIMP_TYPE_CHANNEL));

  /*  saved selections are not visible by default  */
  gimp_item_set_visible (GIMP_ITEM (channel), FALSE, FALSE);

  gimp_image_add_channel (image, channel,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_image_flush (image);

  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (image->gimp)),
                                             image->gimp,
                                             gimp_dialog_factory_get_singleton (),
                                             gtk_widget_get_screen (widget),
                                             gimp_widget_get_monitor (widget),
                                             "gimp-channel-list");
}

void
select_fill_cmd_callback (GimpAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  items_fill_cmd_callback (action,
                           image, GIMP_ITEM (gimp_image_get_mask (image)),
                           "gimp-selection-fill-dialog",
                           _("Fill Selection Outline"),
                           GIMP_ICON_TOOL_BUCKET_FILL,
                           GIMP_HELP_SELECTION_FILL,
                           data);
}

void
select_fill_last_vals_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  items_fill_last_vals_cmd_callback (action,
                                     image,
                                     GIMP_ITEM (gimp_image_get_mask (image)),
                                     data);
}

void
select_stroke_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  items_stroke_cmd_callback (action,
                             image, GIMP_ITEM (gimp_image_get_mask (image)),
                             "gimp-selection-stroke-dialog",
                             _("Stroke Selection"),
                             GIMP_ICON_SELECTION_STROKE,
                             GIMP_HELP_SELECTION_STROKE,
                             data);
}

void
select_stroke_last_vals_cmd_callback (GimpAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  items_stroke_last_vals_cmd_callback (action,
                                       image,
                                       GIMP_ITEM (gimp_image_get_mask (image)),
                                       data);
}


/*  private functions  */

static void
select_feather_callback (GtkWidget *widget,
                         gdouble    size,
                         GimpUnit   unit,
                         gpointer   data)
{
  GimpImage        *image  = GIMP_IMAGE (data);
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
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

  if (unit != GIMP_UNIT_PIXEL)
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      gimp_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  gimp_channel_feather (gimp_image_get_mask (image), radius_x, radius_y,
                        config->selection_feather_edge_lock,
                        TRUE);
  gimp_image_flush (image);
}

static void
select_border_callback (GtkWidget *widget,
                        gdouble    size,
                        GimpUnit   unit,
                        gpointer   data)
{
  GimpImage        *image  = GIMP_IMAGE (data);
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
  GtkWidget        *combo;
  GtkWidget        *button;
  gdouble           radius_x;
  gdouble           radius_y;
  gint              border_style;

  combo  = g_object_get_data (G_OBJECT (widget), "border-style-combo");
  button = g_object_get_data (G_OBJECT (widget), "edge-lock-toggle");

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &border_style);

  g_object_set (config,
                "selection-border-radius", size,
                "selection-border-style",  border_style,
                "selection-border-edge-lock",
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)),
                NULL);

  radius_x = ROUND (config->selection_border_radius);
  radius_y = ROUND (config->selection_border_radius);

  if (unit != GIMP_UNIT_PIXEL)
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      gimp_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  gimp_channel_border (gimp_image_get_mask (image), radius_x, radius_y,
                       config->selection_border_style,
                       config->selection_border_edge_lock,
                       TRUE);
  gimp_image_flush (image);
}

static void
select_grow_callback (GtkWidget *widget,
                      gdouble    size,
                      GimpUnit   unit,
                      gpointer   data)
{
  GimpImage        *image  = GIMP_IMAGE (data);
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
  gdouble           radius_x;
  gdouble           radius_y;

  g_object_set (config,
                "selection-grow-radius", size,
                NULL);

  radius_x = ROUND (config->selection_grow_radius);
  radius_y = ROUND (config->selection_grow_radius);

  if (unit != GIMP_UNIT_PIXEL)
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      gimp_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  gimp_channel_grow (gimp_image_get_mask (image), radius_x, radius_y, TRUE);
  gimp_image_flush (image);
}

static void
select_shrink_callback (GtkWidget *widget,
                        gdouble    size,
                        GimpUnit   unit,
                        gpointer   data)
{
  GimpImage        *image  = GIMP_IMAGE (data);
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (image->gimp->config);
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

  if (unit != GIMP_UNIT_PIXEL)
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      gimp_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  gimp_channel_shrink (gimp_image_get_mask (image), radius_x, radius_y,
                       config->selection_shrink_edge_lock,
                       TRUE);
  gimp_image_flush (image);
}
