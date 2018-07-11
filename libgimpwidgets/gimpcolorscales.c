/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorscales.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorscale.h"
#include "gimpcolorscales.h"
#include "gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpcolorscales
 * @title: GimpColorScales
 * @short_description: A #GimpColorSelector implementation.
 *
 * The #GimpColorScales widget is an implementation of a
 * #GimpColorSelector. It shows a group of #GimpColorScale widgets
 * that allow to adjust the HSV, LCH, and RGB color channels.
 **/


enum
{
  PROP_0,
  PROP_SHOW_RGB_U8
};

enum
{
  GIMP_COLOR_SELECTOR_RED_U8 = GIMP_COLOR_SELECTOR_LCH_HUE + 1,
  GIMP_COLOR_SELECTOR_GREEN_U8,
  GIMP_COLOR_SELECTOR_BLUE_U8,
  GIMP_COLOR_SELECTOR_ALPHA_U8
};


typedef struct _GimpLCH  GimpLCH;

struct _GimpLCH
{
  gdouble l, c, h, a;
};


typedef struct _ColorScale ColorScale;

struct _ColorScale
{
  GimpColorSelectorChannel  channel;

  gdouble                   default_value;
  gdouble                   scale_min_value;
  gdouble                   scale_max_value;
  gdouble                   scale_inc;
  gdouble                   spin_min_value;
  gdouble                   spin_max_value;
};


#define GIMP_COLOR_SCALES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_SCALES, GimpColorScalesClass))
#define GIMP_IS_COLOR_SCALES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_SCALES))
#define GIMP_COLOR_SCALES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_SCALES, GimpColorScalesClass))


typedef struct _GimpColorScalesClass GimpColorScalesClass;

struct _GimpColorScales
{
  GimpColorSelector  parent_instance;

  gboolean           show_rgb_u8;

  GtkWidget         *lch_group;
  GtkWidget         *hsv_group;
  GtkWidget         *rgb_percent_group;
  GtkWidget         *rgb_u8_group;
  GtkWidget         *alpha_percent_group;
  GtkWidget         *alpha_u8_group;

  GtkWidget         *dummy_u8_toggle;
  GtkWidget         *toggles[14];
  GtkAdjustment     *adjustments[14];
  GtkWidget         *scales[14];
};

struct _GimpColorScalesClass
{
  GimpColorSelectorClass  parent_class;
};


static void   gimp_color_scales_dispose        (GObject           *object);
static void   gimp_color_scales_get_property   (GObject           *object,
                                                guint              property_id,
                                                GValue            *value,
                                                GParamSpec        *pspec);
static void   gimp_color_scales_set_property   (GObject           *object,
                                                guint              property_id,
                                                const GValue      *value,
                                                GParamSpec        *pspec);

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
static void   gimp_color_scales_set_model_visible
                                               (GimpColorSelector *selector,
                                                GimpColorSelectorModel  model,
                                                gboolean           visible);
static void   gimp_color_scales_set_config     (GimpColorSelector *selector,
                                                GimpColorConfig   *config);

static void   gimp_color_scales_update_visible (GimpColorScales   *scales);
static void   gimp_color_scales_update_scales  (GimpColorScales   *scales,
                                                gint               skip);
static void   gimp_color_scales_toggle_changed (GtkWidget         *widget,
                                                GimpColorScales   *scales);
static void   gimp_color_scales_scale_changed  (GtkAdjustment     *adjustment,
                                                GimpColorScales   *scales);
static void   gimp_color_scales_toggle_lch_hsv (GtkToggleButton   *toggle,
                                                GimpColorScales   *scales);


G_DEFINE_TYPE (GimpColorScales, gimp_color_scales, GIMP_TYPE_COLOR_SELECTOR)

#define parent_class gimp_color_scales_parent_class

static const Babl *fish_rgb_to_lch = NULL;
static const Babl *fish_lch_to_rgb = NULL;

static const ColorScale scale_defs[] =
{
  { GIMP_COLOR_SELECTOR_HUE,           0, 0, 360, 30,     0,  360 },
  { GIMP_COLOR_SELECTOR_SATURATION,    0, 0, 100, 10,     0,  500 },
  { GIMP_COLOR_SELECTOR_VALUE,         0, 0, 100, 10,     0,  500 },

  { GIMP_COLOR_SELECTOR_RED,           0, 0, 100, 10,  -500,  500 },
  { GIMP_COLOR_SELECTOR_GREEN,         0, 0, 100, 10,  -500,  500 },
  { GIMP_COLOR_SELECTOR_BLUE,          0, 0, 100, 10,  -500,  500 },
  { GIMP_COLOR_SELECTOR_ALPHA,         0, 0, 100, 10,     0,  100 },

  { GIMP_COLOR_SELECTOR_LCH_LIGHTNESS, 0, 0, 100, 10,     0,  300 },
  { GIMP_COLOR_SELECTOR_LCH_CHROMA,    0, 0, 200, 10,     0,  300 },
  { GIMP_COLOR_SELECTOR_LCH_HUE,       0, 0, 360, 30,     0,  360 },

  { GIMP_COLOR_SELECTOR_RED_U8,        0, 0, 255, 16, -1275, 1275 },
  { GIMP_COLOR_SELECTOR_GREEN_U8,      0, 0, 255, 16, -1275, 1275 },
  { GIMP_COLOR_SELECTOR_BLUE_U8,       0, 0, 255, 16, -1275, 1275 },
  { GIMP_COLOR_SELECTOR_ALPHA_U8,      0, 0, 255, 16,     0,  255 }
};


static void
gimp_color_scales_class_init (GimpColorScalesClass *klass)
{
  GObjectClass           *object_class   = G_OBJECT_CLASS (klass);
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  object_class->dispose                 = gimp_color_scales_dispose;
  object_class->get_property            = gimp_color_scales_get_property;
  object_class->set_property            = gimp_color_scales_set_property;

  selector_class->name                  = _("Scales");
  selector_class->help_id               = "gimp-colorselector-scales";
  selector_class->icon_name             = GIMP_ICON_DIALOG_TOOL_OPTIONS;
  selector_class->set_toggles_visible   = gimp_color_scales_togg_visible;
  selector_class->set_toggles_sensitive = gimp_color_scales_togg_sensitive;
  selector_class->set_show_alpha        = gimp_color_scales_set_show_alpha;
  selector_class->set_color             = gimp_color_scales_set_color;
  selector_class->set_channel           = gimp_color_scales_set_channel;
  selector_class->set_model_visible     = gimp_color_scales_set_model_visible;
  selector_class->set_config            = gimp_color_scales_set_config;

  g_object_class_install_property (object_class, PROP_SHOW_RGB_U8,
                                   g_param_spec_boolean ("show-rgb-u8",
                                                         "Show RGB 0..255",
                                                         "Show RGB 0..255 scales",
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  fish_rgb_to_lch = babl_fish (babl_format ("R'G'B'A double"),
                               babl_format ("CIE LCH(ab) alpha double"));
  fish_lch_to_rgb = babl_fish (babl_format ("CIE LCH(ab) alpha double"),
                               babl_format ("R'G'B'A double"));
}

static GtkWidget *
create_group (GimpColorScales           *scales,
              GSList                   **radio_group,
              GtkSizeGroup              *size_group0,
              GtkSizeGroup              *size_group1,
              GtkSizeGroup              *size_group2,
              GimpColorSelectorChannel   first_channel,
              GimpColorSelectorChannel   last_channel)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);
  GtkWidget         *table;
  GEnumClass        *enum_class;
  gint               row;
  gint               i;

  table = gtk_table_new (last_channel - first_channel + 1, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 0);

  enum_class = g_type_class_ref (GIMP_TYPE_COLOR_SELECTOR_CHANNEL);

  for (i = first_channel, row = 0; i <= last_channel; i++, row++)
    {
      GimpEnumDesc *enum_desc;
      gint          enum_value = i;
      gboolean      is_u8      = FALSE;

      if (enum_value >= GIMP_COLOR_SELECTOR_RED_U8 &&
          enum_value <= GIMP_COLOR_SELECTOR_ALPHA_U8)
        {
          enum_value -= 7;
          is_u8 = TRUE;
        }

      enum_desc = gimp_enum_get_desc (enum_class, enum_value);

      if (i == GIMP_COLOR_SELECTOR_ALPHA ||
          i == GIMP_COLOR_SELECTOR_ALPHA_U8)
        {
          /*  just to allocate the space via the size group  */
          scales->toggles[i] = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        }
      else
        {
          scales->toggles[i] = gtk_radio_button_new (*radio_group);
          *radio_group =
            gtk_radio_button_get_group (GTK_RADIO_BUTTON (scales->toggles[i]));

          if (enum_value == gimp_color_selector_get_channel (selector))
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scales->toggles[i]),
                                          TRUE);

          if (is_u8)
            {
              /*  bind the RGB U8 toggles to the RGB percent toggles  */
              g_object_bind_property (scales->toggles[i - 7], "active",
                                      scales->toggles[i],     "active",
                                      G_BINDING_SYNC_CREATE |
                                      G_BINDING_BIDIRECTIONAL);
            }
          else
            {
              g_signal_connect (scales->toggles[i], "toggled",
                                G_CALLBACK (gimp_color_scales_toggle_changed),
                                scales);
            }
        }

      gtk_table_attach (GTK_TABLE (table), scales->toggles[i],
                        0, 1, row, row + 1,
                        GTK_SHRINK, GTK_EXPAND, 0, 0);

      if (gimp_color_selector_get_toggles_visible (selector))
        gtk_widget_show (scales->toggles[i]);

      gimp_help_set_help_data (scales->toggles[i],
                               gettext (enum_desc->value_help), NULL);

      gtk_size_group_add_widget (size_group0, scales->toggles[i]);

      scales->adjustments[i] = (GtkAdjustment *)
        gimp_color_scale_entry_new (GTK_TABLE (table), 1, row,
                                    gettext (enum_desc->value_desc),
                                    -1, -1,
                                    scale_defs[i].default_value,
                                    scale_defs[i].scale_min_value,
                                    scale_defs[i].scale_max_value,
                                    1.0,
                                    scale_defs[i].scale_inc,
                                    1,
                                    gettext (enum_desc->value_help),
                                    NULL);

      gtk_adjustment_configure (scales->adjustments[i],
                                scale_defs[i].default_value,
                                scale_defs[i].spin_min_value,
                                scale_defs[i].spin_max_value,
                                1.0,
                                scale_defs[i].scale_inc,
                                0);

      scales->scales[i] = GIMP_SCALE_ENTRY_SCALE (scales->adjustments[i]);
      g_object_add_weak_pointer (G_OBJECT (scales->scales[i]),
                                 (gpointer) &scales->scales[i]);

      gimp_color_scale_set_channel (GIMP_COLOR_SCALE (scales->scales[i]),
                                    enum_value);
      gtk_size_group_add_widget (size_group1, scales->scales[i]);

      gtk_size_group_add_widget (size_group2,
                                 GIMP_SCALE_ENTRY_SPINBUTTON (scales->adjustments[i]));

      g_signal_connect (scales->adjustments[i], "value-changed",
                        G_CALLBACK (gimp_color_scales_scale_changed),
                        scales);
    }

  g_type_class_unref (enum_class);

  return table;
}

static void
gimp_color_scales_init (GimpColorScales *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);
  GtkSizeGroup      *size_group0;
  GtkSizeGroup      *size_group1;
  GtkSizeGroup      *size_group2;
  GtkWidget         *hbox;
  GtkWidget         *radio1;
  GtkWidget         *radio2;
  GtkWidget         *table;
  GSList            *main_group;
  GSList            *u8_group;
  GSList            *radio_group;

  gtk_box_set_spacing (GTK_BOX (scales), 5);

  /*  don't need the toggles for our own operation  */
  selector->toggles_visible = FALSE;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (scales), hbox, 0, 0, FALSE);
  gtk_widget_show (hbox);

  main_group = NULL;
  u8_group   = NULL;

  scales->dummy_u8_toggle = gtk_radio_button_new (NULL);
  g_object_ref_sink (scales->dummy_u8_toggle);
  u8_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (scales->dummy_u8_toggle));

  size_group0 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  size_group1 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  size_group2 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  scales->rgb_percent_group =
    table = create_group (scales, &main_group,
                          size_group0, size_group1, size_group2,
                          GIMP_COLOR_SELECTOR_RED,
                          GIMP_COLOR_SELECTOR_BLUE);
  gtk_box_pack_start (GTK_BOX (scales), table, FALSE, FALSE, 0);

  scales->rgb_u8_group =
    table = create_group (scales, &u8_group,
                          size_group0, size_group1, size_group2,
                          GIMP_COLOR_SELECTOR_RED_U8,
                          GIMP_COLOR_SELECTOR_BLUE_U8);
  gtk_box_pack_start (GTK_BOX (scales), table, FALSE, FALSE, 0);

  scales->lch_group =
    table = create_group (scales, &main_group,
                          size_group0, size_group1, size_group2,
                          GIMP_COLOR_SELECTOR_LCH_LIGHTNESS,
                          GIMP_COLOR_SELECTOR_LCH_HUE);
  gtk_box_pack_start (GTK_BOX (scales), table, FALSE, FALSE, 0);

 scales->hsv_group =
    table = create_group (scales, &main_group,
                          size_group0, size_group1, size_group2,
                          GIMP_COLOR_SELECTOR_HUE,
                          GIMP_COLOR_SELECTOR_VALUE);
  gtk_box_pack_start (GTK_BOX (scales), table, FALSE, FALSE, 0);

  scales->alpha_percent_group =
    table = create_group (scales, &main_group,
                          size_group0, size_group1, size_group2,
                          GIMP_COLOR_SELECTOR_ALPHA,
                          GIMP_COLOR_SELECTOR_ALPHA);
  gtk_box_pack_start (GTK_BOX (scales), table, FALSE, FALSE, 0);

  scales->alpha_u8_group =
    table = create_group (scales, &u8_group,
                          size_group0, size_group1, size_group2,
                          GIMP_COLOR_SELECTOR_ALPHA_U8,
                          GIMP_COLOR_SELECTOR_ALPHA_U8);
  gtk_box_pack_start (GTK_BOX (scales), table, FALSE, FALSE, 0);

  g_object_unref (size_group0);
  g_object_unref (size_group1);
  g_object_unref (size_group2);

  gimp_color_scales_update_visible (scales);

  radio_group = NULL;

  radio1      = gtk_radio_button_new_with_label (NULL,  _("0..100"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio1));
  radio2      = gtk_radio_button_new_with_label (radio_group, _("0..255"));

  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radio1), FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radio2), FALSE);

  gtk_box_pack_start (GTK_BOX (hbox), radio1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), radio2, FALSE, FALSE, 0);

  gtk_widget_show (radio1);
  gtk_widget_show (radio2);

  if (scales->show_rgb_u8)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio2), TRUE);

  g_object_bind_property (G_OBJECT (radio2), "active",
                          G_OBJECT (scales), "show-rgb-u8",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  radio_group = NULL;

  radio1      = gtk_radio_button_new_with_label (NULL,  _("LCh"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio1));
  radio2      = gtk_radio_button_new_with_label (radio_group, _("HSV"));

  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radio1), FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radio2), FALSE);

  gtk_box_pack_end (GTK_BOX (hbox), radio2, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), radio1, FALSE, FALSE, 0);

  gtk_widget_show (radio1);
  gtk_widget_show (radio2);

  if (gimp_color_selector_get_model_visible (selector,
                                             GIMP_COLOR_SELECTOR_MODEL_HSV))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio2), TRUE);

  g_signal_connect (radio1, "toggled",
                    G_CALLBACK (gimp_color_scales_toggle_lch_hsv),
                    scales);
}

static void
gimp_color_scales_dispose (GObject *object)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (object);

  g_clear_object (&scales->dummy_u8_toggle);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_scales_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (object);

  switch (property_id)
    {
    case PROP_SHOW_RGB_U8:
      g_value_set_boolean (value, scales->show_rgb_u8);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_scales_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (object);

  switch (property_id)
    {
    case PROP_SHOW_RGB_U8:
      gimp_color_scales_set_show_rgb_u8 (scales, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_scales_togg_sensitive (GimpColorSelector *selector,
                                  gboolean           sensitive)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (selector);
  gint             i;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    if (scales->toggles[i])
      gtk_widget_set_sensitive (scales->toggles[i], sensitive);
}

static void
gimp_color_scales_togg_visible (GimpColorSelector *selector,
                                gboolean           visible)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (selector);
  gint             i;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    if (scales->toggles[i])
      gtk_widget_set_visible (scales->toggles[i], visible);
}

static void
gimp_color_scales_set_show_alpha (GimpColorSelector *selector,
                                  gboolean           show_alpha)
{
  gimp_color_scales_update_visible (GIMP_COLOR_SCALES (selector));
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

  if (GTK_IS_RADIO_BUTTON (scales->toggles[channel]))
    {
      g_signal_handlers_block_by_func (scales->toggles[channel],
                                       gimp_color_scales_toggle_changed,
                                       scales);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scales->toggles[channel]),
                                    TRUE);

      g_signal_handlers_unblock_by_func (scales->toggles[channel],
                                         gimp_color_scales_toggle_changed,
                                         scales);
    }
}

static void
gimp_color_scales_set_model_visible (GimpColorSelector      *selector,
                                     GimpColorSelectorModel  model,
                                     gboolean                visible)
{
  gimp_color_scales_update_visible (GIMP_COLOR_SCALES (selector));
}

static void
gimp_color_scales_set_config (GimpColorSelector *selector,
                              GimpColorConfig   *config)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (selector);
  gint             i;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    {
      if (scales->scales[i])
        gimp_color_scale_set_color_config (GIMP_COLOR_SCALE (scales->scales[i]),
                                           config);
    }
}


/*  public functions  */

void
gimp_color_scales_set_show_rgb_u8 (GimpColorScales *scales,
                                   gboolean         show_rgb_u8)
{
  g_return_if_fail (GIMP_IS_COLOR_SCALES (scales));

  show_rgb_u8 = show_rgb_u8 ? TRUE : FALSE;

  if (show_rgb_u8 != scales->show_rgb_u8)
    {
      scales->show_rgb_u8 = show_rgb_u8;

      g_object_notify (G_OBJECT (scales), "show-rgb-u8");

      gimp_color_scales_update_visible (scales);
    }
}

gboolean
gimp_color_scales_get_show_rgb_u8 (GimpColorScales *scales)
{
  g_return_val_if_fail (GIMP_IS_COLOR_SCALES (scales), FALSE);

  return scales->show_rgb_u8;
}


/*  private functions  */

static void
gimp_color_scales_update_visible (GimpColorScales *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);
  gboolean           show_alpha;
  gboolean           rgb_visible;
  gboolean           lch_visible;
  gboolean           hsv_visible;

  show_alpha  = gimp_color_selector_get_show_alpha (selector);
  rgb_visible = gimp_color_selector_get_model_visible (selector,
                                                       GIMP_COLOR_SELECTOR_MODEL_RGB);
  lch_visible = gimp_color_selector_get_model_visible (selector,
                                                       GIMP_COLOR_SELECTOR_MODEL_LCH);
  hsv_visible = gimp_color_selector_get_model_visible (selector,
                                                       GIMP_COLOR_SELECTOR_MODEL_HSV);

  gtk_widget_set_visible (scales->rgb_percent_group,
                          rgb_visible && ! scales->show_rgb_u8);
  gtk_widget_set_visible (scales->rgb_u8_group,
                          rgb_visible &&   scales->show_rgb_u8);

  gtk_widget_set_visible (scales->lch_group, lch_visible);
  gtk_widget_set_visible (scales->hsv_group, hsv_visible);

  gtk_widget_set_visible (scales->alpha_percent_group,
                          show_alpha && ! scales->show_rgb_u8);
  gtk_widget_set_visible (scales->alpha_u8_group,
                          show_alpha &&   scales->show_rgb_u8);
}

static void
gimp_color_scales_update_scales (GimpColorScales *scales,
                                 gint             skip)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);
  GimpLCH            lch;
  gdouble            values[G_N_ELEMENTS (scale_defs)];
  gint               i;

  babl_process (fish_rgb_to_lch, &selector->rgb, &lch, 1);

  values[GIMP_COLOR_SELECTOR_HUE]           = selector->hsv.h * 360.0;
  values[GIMP_COLOR_SELECTOR_SATURATION]    = selector->hsv.s * 100.0;
  values[GIMP_COLOR_SELECTOR_VALUE]         = selector->hsv.v * 100.0;

  values[GIMP_COLOR_SELECTOR_RED]           = selector->rgb.r * 100.0;
  values[GIMP_COLOR_SELECTOR_GREEN]         = selector->rgb.g * 100.0;
  values[GIMP_COLOR_SELECTOR_BLUE]          = selector->rgb.b * 100.0;
  values[GIMP_COLOR_SELECTOR_ALPHA]         = selector->rgb.a * 100.0;

  values[GIMP_COLOR_SELECTOR_LCH_LIGHTNESS] = lch.l;
  values[GIMP_COLOR_SELECTOR_LCH_CHROMA]    = lch.c;
  values[GIMP_COLOR_SELECTOR_LCH_HUE]       = lch.h;

  values[GIMP_COLOR_SELECTOR_RED_U8]        = selector->rgb.r * 255.0;
  values[GIMP_COLOR_SELECTOR_GREEN_U8]      = selector->rgb.g * 255.0;
  values[GIMP_COLOR_SELECTOR_BLUE_U8]       = selector->rgb.b * 255.0;
  values[GIMP_COLOR_SELECTOR_ALPHA_U8]      = selector->rgb.a * 255.0;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    {
      if (i != skip)
        {
          g_signal_handlers_block_by_func (scales->adjustments[i],
                                           gimp_color_scales_scale_changed,
                                           scales);

          gtk_adjustment_set_value (scales->adjustments[i], values[i]);

          g_signal_handlers_unblock_by_func (scales->adjustments[i],
                                             gimp_color_scales_scale_changed,
                                             scales);
        }

      gimp_color_scale_set_color (GIMP_COLOR_SCALE (scales->scales[i]),
                                  &selector->rgb, &selector->hsv);
    }
}

static void
gimp_color_scales_toggle_changed (GtkWidget       *widget,
                                  GimpColorScales *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
        {
          if (widget == scales->toggles[i])
            {
              gimp_color_selector_set_channel (selector, i);

              if (i < GIMP_COLOR_SELECTOR_RED ||
                  i > GIMP_COLOR_SELECTOR_BLUE)
                {
                  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scales->dummy_u8_toggle),
                                                TRUE);
                }

              break;
            }
        }
    }
}

static void
gimp_color_scales_scale_changed (GtkAdjustment   *adjustment,
                                 GimpColorScales *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);
  gdouble            value    = gtk_adjustment_get_value (adjustment);
  GimpLCH            lch;
  gint               i;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    if (scales->adjustments[i] == adjustment)
      break;

  switch (i)
    {
    case GIMP_COLOR_SELECTOR_HUE:
      selector->hsv.h = value / 360.0;
      break;

    case GIMP_COLOR_SELECTOR_SATURATION:
      selector->hsv.s = value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_VALUE:
      selector->hsv.v = value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_RED:
      selector->rgb.r = value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_GREEN:
      selector->rgb.g = value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_BLUE:
      selector->rgb.b = value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_ALPHA:
      selector->hsv.a = selector->rgb.a = value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_LCH_LIGHTNESS:
      babl_process (fish_rgb_to_lch, &selector->rgb, &lch, 1);
      lch.l = value;
      break;

    case GIMP_COLOR_SELECTOR_LCH_CHROMA:
      babl_process (fish_rgb_to_lch, &selector->rgb, &lch, 1);
      lch.c = value;
      break;

    case GIMP_COLOR_SELECTOR_LCH_HUE:
      babl_process (fish_rgb_to_lch, &selector->rgb, &lch, 1);
      lch.h = value;
      break;

    case GIMP_COLOR_SELECTOR_RED_U8:
      selector->rgb.r = value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_GREEN_U8:
      selector->rgb.g = value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_BLUE_U8:
      selector->rgb.b = value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_ALPHA_U8:
      selector->hsv.a = selector->rgb.a = value / 255.0;
      break;
    }

  if ((i >= GIMP_COLOR_SELECTOR_HUE) &&
      (i <= GIMP_COLOR_SELECTOR_VALUE))
    {
      gimp_hsv_to_rgb (&selector->hsv, &selector->rgb);
    }
  else if ((i >= GIMP_COLOR_SELECTOR_LCH_LIGHTNESS) &&
           (i <= GIMP_COLOR_SELECTOR_LCH_HUE))
    {
      babl_process (fish_lch_to_rgb, &lch, &selector->rgb, 1);
      gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);
    }
  else if ((i >= GIMP_COLOR_SELECTOR_RED) &&
           (i <= GIMP_COLOR_SELECTOR_BLUE))
    {
      gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);
    }
  else if ((i >= GIMP_COLOR_SELECTOR_RED_U8) &&
           (i <= GIMP_COLOR_SELECTOR_BLUE_U8))
    {
      gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);
    }

  gimp_color_scales_update_scales (scales, i);

  gimp_color_selector_color_changed (selector);
}

static void
gimp_color_scales_toggle_lch_hsv (GtkToggleButton *toggle,
                                  GimpColorScales *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);

  if (gtk_toggle_button_get_active (toggle))
    {
      gimp_color_selector_set_model_visible (selector,
                                             GIMP_COLOR_SELECTOR_MODEL_LCH,
                                             TRUE);
      gimp_color_selector_set_model_visible (selector,
                                             GIMP_COLOR_SELECTOR_MODEL_HSV,
                                             FALSE);
    }
  else
    {
      gimp_color_selector_set_model_visible (selector,
                                             GIMP_COLOR_SELECTOR_MODEL_LCH,
                                             FALSE);
      gimp_color_selector_set_model_visible (selector,
                                             GIMP_COLOR_SELECTOR_MODEL_HSV,
                                             TRUE);
    }
}
