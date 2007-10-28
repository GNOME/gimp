/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorscales.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorscale.h"
#include "gimpcolorscales.h"
#include "gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


#define GIMP_COLOR_SCALES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_SCALES, GimpColorScalesClass))
#define GIMP_IS_COLOR_SCALES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_SCALES))
#define GIMP_COLOR_SCALES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_SCALES, GimpColorScalesClass))


typedef struct _GimpColorScalesClass GimpColorScalesClass;

struct _GimpColorScales
{
  GimpColorSelector  parent_instance;

  GtkWidget         *toggles[7];
  GtkWidget         *sliders[7];
  GtkObject         *slider_data[7];
};

struct _GimpColorScalesClass
{
  GimpColorSelectorClass  parent_class;
};


static void   gimp_color_scales_togg_sensitive (GimpColorSelector *selector,
                                                gboolean           sensitive);
static void   gimp_color_scales_togg_visible   (GimpColorSelector *selector,
                                                gboolean           visible);

static void   gimp_color_scales_set_show_alpha (GimpColorSelector *selector,
                                                gboolean           show_alpha);
static void   gimp_color_scales_set_color      (GimpColorSelector *selector,
                                                const GimpRGB     *rgb,
                                                const GimpHSV     *hsv);
static void   gimp_color_scales_set_channel    (GimpColorSelector *selector,
                                                GimpColorSelectorChannel  channel);

static void   gimp_color_scales_update_scales  (GimpColorScales   *scales,
                                                gint               skip);
static void   gimp_color_scales_toggle_update  (GtkWidget         *widget,
                                                GimpColorScales   *scales);
static void   gimp_color_scales_scale_update   (GtkAdjustment     *adjustment,
                                                GimpColorScales   *scales);


G_DEFINE_TYPE (GimpColorScales, gimp_color_scales, GIMP_TYPE_COLOR_SELECTOR)

#define parent_class gimp_color_scales_parent_class


static void
gimp_color_scales_class_init (GimpColorScalesClass *klass)
{
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  selector_class->name                  = _("Scales");
  selector_class->help_id               = "gimp-colorselector-scales";
  selector_class->stock_id              = GIMP_STOCK_TOOL_OPTIONS;
  selector_class->set_toggles_visible   = gimp_color_scales_togg_visible;
  selector_class->set_toggles_sensitive = gimp_color_scales_togg_sensitive;
  selector_class->set_show_alpha        = gimp_color_scales_set_show_alpha;
  selector_class->set_color             = gimp_color_scales_set_color;
  selector_class->set_channel           = gimp_color_scales_set_channel;
}

static void
gimp_color_scales_init (GimpColorScales *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);
  GtkWidget         *table;
  GEnumClass        *enum_class;
  GSList            *group;
  gint               i;

  static const gdouble slider_initial_vals[] =
    {   0,   0,   0,   0,   0,   0,   0 };
  static const gdouble slider_max_vals[] =
    { 360, 100, 100, 255, 255, 255, 100 };
  static const gdouble slider_incs[] =
    {  30,  10,  10,  16,  16,  16,  10 };

  /*  don't needs the toggles for our own operation  */
  selector->toggles_visible = FALSE;

  table = gtk_table_new (7, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 3); /* hsv <-> rgb   */
  gtk_table_set_row_spacing (GTK_TABLE (table), 5, 3); /* rgb <-> alpha */
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 0);
  gtk_box_pack_start (GTK_BOX (scales), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  enum_class = g_type_class_ref (GIMP_TYPE_COLOR_SELECTOR_CHANNEL);

  group = NULL;

  for (i = GIMP_COLOR_SELECTOR_HUE; i <= GIMP_COLOR_SELECTOR_ALPHA; i++)
    {
      GimpEnumDesc *enum_desc;

      enum_desc = gimp_enum_get_desc (enum_class, i);

      if (i == GIMP_COLOR_SELECTOR_ALPHA)
        {
          scales->toggles[i] = NULL;
        }
      else
        {
          scales->toggles[i] = gtk_radio_button_new (group);
          group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (scales->toggles[i]));
          gtk_table_attach (GTK_TABLE (table), scales->toggles[i],
                            0, 1, i, i + 1,
                            GTK_SHRINK, GTK_EXPAND, 0, 0);

          if (selector->toggles_visible)
            gtk_widget_show (scales->toggles[i]);

          gimp_help_set_help_data (scales->toggles[i],
                                   gettext (enum_desc->value_help), NULL);

          g_signal_connect (scales->toggles[i], "toggled",
                            G_CALLBACK (gimp_color_scales_toggle_update),
                            scales);
        }

      scales->slider_data[i] =
        gimp_color_scale_entry_new (GTK_TABLE (table), 1, i,
                                    gettext (enum_desc->value_desc),
                                    -1, -1,
                                    slider_initial_vals[i],
                                    0.0, slider_max_vals[i],
                                    1.0, slider_incs[i],
                                    0,
                                    gettext (enum_desc->value_help),
                                    NULL);

      scales->sliders[i] = GIMP_SCALE_ENTRY_SCALE (scales->slider_data[i]);

      gimp_color_scale_set_channel (GIMP_COLOR_SCALE (scales->sliders[i]), i);

      g_signal_connect (scales->slider_data[i], "value-changed",
                        G_CALLBACK (gimp_color_scales_scale_update),
                        scales);
    }

  g_type_class_unref (enum_class);
}

static void
gimp_color_scales_togg_sensitive (GimpColorSelector *selector,
                                  gboolean           sensitive)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (selector);
  gint             i;

  for (i = 0; i < 6; i++)
    gtk_widget_set_sensitive (scales->toggles[i], sensitive);
}

static void
gimp_color_scales_togg_visible (GimpColorSelector *selector,
                                gboolean           visible)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (selector);
  gint             i;

  for (i = 0; i < 6; i++)
    {
      if (visible)
        gtk_widget_show (scales->toggles[i]);
      else
        gtk_widget_hide (scales->toggles[i]);
    }
}

static void
gimp_color_scales_set_show_alpha (GimpColorSelector *selector,
                                  gboolean           show_alpha)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (selector);
  GtkWidget       *label;
  GtkWidget       *scale;
  GtkWidget       *spin;
  GtkWidget       *table;

  label = GIMP_SCALE_ENTRY_LABEL (scales->slider_data[6]);
  scale = GIMP_SCALE_ENTRY_SCALE (scales->slider_data[6]);
  spin  = GIMP_SCALE_ENTRY_SPINBUTTON (scales->slider_data[6]);

  table = gtk_widget_get_parent (scale);
  if (GTK_IS_TABLE (table))
    {
      gtk_table_set_row_spacing (GTK_TABLE (table), 5, /* rgb <-> alpha */
                                 show_alpha ? 3 : 0);
    }

  if (show_alpha)
    {
      gtk_widget_show (label);
      gtk_widget_show (scale);
      gtk_widget_show (spin);
    }
  else
    {
      gtk_widget_hide (label);
      gtk_widget_hide (scale);
      gtk_widget_hide (spin);
    }
}

static void
gimp_color_scales_set_color (GimpColorSelector *selector,
                             const GimpRGB     *rgb,
                             const GimpHSV     *hsv)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (selector);

  gimp_color_scales_update_scales (scales, -1);
}

static void
gimp_color_scales_set_channel (GimpColorSelector        *selector,
                               GimpColorSelectorChannel  channel)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (selector);

  if (channel >= 0 && channel < 7)
    {
      g_signal_handlers_block_by_func (scales->toggles[channel],
                                       gimp_color_scales_toggle_update,
                                       scales);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scales->toggles[channel]),
                                    TRUE);

      g_signal_handlers_unblock_by_func (scales->toggles[channel],
                                         gimp_color_scales_toggle_update,
                                         scales);
    }
}

static void
gimp_color_scales_update_scales (GimpColorScales *scales,
                                 gint             skip)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);
  gint               values[7];
  gint               i;

  values[GIMP_COLOR_SELECTOR_HUE]        = ROUND (selector->hsv.h * 360.0);
  values[GIMP_COLOR_SELECTOR_SATURATION] = ROUND (selector->hsv.s * 100.0);
  values[GIMP_COLOR_SELECTOR_VALUE]      = ROUND (selector->hsv.v * 100.0);
  values[GIMP_COLOR_SELECTOR_RED]        = ROUND (selector->rgb.r * 255.0);
  values[GIMP_COLOR_SELECTOR_GREEN]      = ROUND (selector->rgb.g * 255.0);
  values[GIMP_COLOR_SELECTOR_BLUE]       = ROUND (selector->rgb.b * 255.0);
  values[GIMP_COLOR_SELECTOR_ALPHA]      = ROUND (selector->rgb.a * 100.0);

  for (i = 0; i < 7; i++)
    {
      if (i != skip)
        {
          g_signal_handlers_block_by_func (scales->slider_data[i],
                                           gimp_color_scales_scale_update,
                                           scales);

          gtk_adjustment_set_value (GTK_ADJUSTMENT (scales->slider_data[i]),
                                    values[i]);

          g_signal_handlers_unblock_by_func (scales->slider_data[i],
                                             gimp_color_scales_scale_update,
                                             scales);
        }

      gimp_color_scale_set_color (GIMP_COLOR_SCALE (scales->sliders[i]),
                                  &selector->rgb, &selector->hsv);
    }
}

static void
gimp_color_scales_toggle_update (GtkWidget       *widget,
                                 GimpColorScales *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      gint i;

      for (i = 0; i < 6; i++)
        if (widget == scales->toggles[i])
          {
            selector->channel = (GimpColorSelectorChannel) i;
            break;
          }

      gimp_color_selector_channel_changed (selector);
    }
}

static void
gimp_color_scales_scale_update (GtkAdjustment   *adjustment,
                                GimpColorScales *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);
  gint               i;

  for (i = 0; i < 7; i++)
    if (scales->slider_data[i] == GTK_OBJECT (adjustment))
      break;

  switch (i)
    {
    case GIMP_COLOR_SELECTOR_HUE:
      selector->hsv.h = GTK_ADJUSTMENT (adjustment)->value / 360.0;
      break;

    case GIMP_COLOR_SELECTOR_SATURATION:
      selector->hsv.s = GTK_ADJUSTMENT (adjustment)->value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_VALUE:
      selector->hsv.v = GTK_ADJUSTMENT (adjustment)->value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_RED:
      selector->rgb.r = GTK_ADJUSTMENT (adjustment)->value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_GREEN:
      selector->rgb.g = GTK_ADJUSTMENT (adjustment)->value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_BLUE:
      selector->rgb.b = GTK_ADJUSTMENT (adjustment)->value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_ALPHA:
      selector->hsv.a = selector->rgb.a =
        GTK_ADJUSTMENT (adjustment)->value / 100.0;
      break;
    }

  if ((i >= GIMP_COLOR_SELECTOR_HUE) && (i <= GIMP_COLOR_SELECTOR_VALUE))
    {
      gimp_hsv_to_rgb (&selector->hsv, &selector->rgb);
    }
  else if ((i >= GIMP_COLOR_SELECTOR_RED) && (i <= GIMP_COLOR_SELECTOR_BLUE))
    {
      gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);
    }

  gimp_color_scales_update_scales (scales, i);

  gimp_color_selector_color_changed (selector);
}
