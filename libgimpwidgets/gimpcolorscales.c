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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorscale.h"
#include "gimpcolorscales.h"
#include "gimpcolorhexentry.h"
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
  GtkWidget         *hex_entry;
};

struct _GimpColorScalesClass
{
  GimpColorSelectorClass  parent_class;
};


static void   gimp_color_scales_class_init (GimpColorScalesClass  *klass);
static void   gimp_color_scales_init       (GimpColorScales       *scales);

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

static void   gimp_color_scales_entry_changed  (GimpColorHexEntry *entry,
                                                GimpColorScales   *scales);


static GimpColorSelectorClass *parent_class = NULL;


GType
gimp_color_scales_get_type (void)
{
  static GType scales_type = 0;

  if (! scales_type)
    {
      static const GTypeInfo scales_info =
      {
        sizeof (GimpColorScalesClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_scales_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorScales),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_scales_init,
      };

      scales_type = g_type_register_static (GIMP_TYPE_COLOR_SELECTOR,
                                            "GimpColorScales",
                                            &scales_info, 0);
    }

  return scales_type;
}

static void
gimp_color_scales_class_init (GimpColorScalesClass *klass)
{
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

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
  GtkWidget         *hbox;
  GtkWidget         *label;
  GSList            *group;
  gint               i;

  static const gchar *toggle_titles[] =
  {
    N_("_H"),
    N_("_S"),
    N_("_V"),
    N_("_R"),
    N_("_G"),
    N_("_B"),
    N_("_A")
  };
  static const gchar *slider_tips[7] =
  {
    N_("Hue"),
    N_("Saturation"),
    N_("Value"),
    N_("Red"),
    N_("Green"),
    N_("Blue"),
    N_("Alpha")
  };
  static gdouble slider_initial_vals[] = {   0,   0,   0,   0,   0,   0,   0 };
  static gdouble slider_max_vals[]     = { 360, 100, 100, 255, 255, 255, 100 };
  static gdouble slider_incs[]         = {  30,  10,  10,  16,  16,  16,  10 };

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

  group = NULL;

  for (i = 0; i < 7; i++)
    {
      if (i == 6)
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
				   gettext (slider_tips[i]), NULL);

	  g_signal_connect (scales->toggles[i], "toggled",
			    G_CALLBACK (gimp_color_scales_toggle_update),
			    scales);
	}

      scales->slider_data[i] =
        gimp_color_scale_entry_new (GTK_TABLE (table), 1, i,
                                    gettext (toggle_titles[i]),
                                    -1, -1,
                                    slider_initial_vals[i],
                                    0.0, slider_max_vals[i],
                                    1.0, slider_incs[i],
                                    0,
                                    gettext (slider_tips[i]),
                                    NULL);

      scales->sliders[i] = GIMP_SCALE_ENTRY_SCALE (scales->slider_data[i]);

      gimp_color_scale_set_channel (GIMP_COLOR_SCALE (scales->sliders[i]), i);

      g_signal_connect (scales->slider_data[i], "value_changed",
			G_CALLBACK (gimp_color_scales_scale_update),
			scales);
    }

  /* The hex triplet entry */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_end (GTK_BOX (scales), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  scales->hex_entry = gimp_color_hex_entry_new ();
  gimp_help_set_help_data (scales->hex_entry,
                           _("Hexadecimal color notation "
                             "as used in HTML and CSS"), NULL);
  gtk_box_pack_end (GTK_BOX (hbox), scales->hex_entry, TRUE, TRUE, 0);
  gtk_widget_show (scales->hex_entry);

  label = gtk_label_new_with_mnemonic (_("HTML _Notation:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), scales->hex_entry);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_signal_connect (scales->hex_entry, "color_changed",
                    G_CALLBACK (gimp_color_scales_entry_changed),
                    scales);
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

  label = GIMP_SCALE_ENTRY_LABEL (scales->slider_data[6]);
  scale = GIMP_SCALE_ENTRY_SCALE (scales->slider_data[6]);
  spin  = GIMP_SCALE_ENTRY_SPINBUTTON (scales->slider_data[6]);

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

  if (channel >= 0 && channel <= 7)
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

  gimp_color_hex_entry_set_color (GIMP_COLOR_HEX_ENTRY (scales->hex_entry),
                                                        &selector->rgb);
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

static void
gimp_color_scales_entry_changed (GimpColorHexEntry *entry,
                                 GimpColorScales   *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);

  gimp_color_hex_entry_get_color (entry, &selector->rgb);

  gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);
  gimp_color_scales_update_scales (scales, -1);

  gimp_color_selector_color_changed (selector);
}
