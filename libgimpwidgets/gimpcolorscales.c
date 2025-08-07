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
  PROP_SHOW_RGB_U8,
  PROP_SHOW_HSV
};

enum
{
  GIMP_COLOR_SELECTOR_RED_U8 = GIMP_COLOR_SELECTOR_LCH_HUE + 1,
  GIMP_COLOR_SELECTOR_GREEN_U8,
  GIMP_COLOR_SELECTOR_BLUE_U8,
  GIMP_COLOR_SELECTOR_ALPHA_U8
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


struct _GimpColorScales
{
  GimpColorSelector  parent_instance;

  const Babl        *format;

  gboolean           show_rgb_u8;
  GBinding          *show_rgb_u8_binding;
  GBinding          *show_hsv_binding;

  GtkWidget         *lch_group;
  GtkWidget         *hsv_group;
  GtkWidget         *rgb_percent_group;
  GtkWidget         *rgb_u8_group;
  GtkWidget         *alpha_percent_group;
  GtkWidget         *alpha_u8_group;

  GtkWidget         *dummy_u8_toggle;
  GtkWidget         *toggles[14];
  GtkWidget         *scales[14];

  GList             *profile_labels;
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
                                                GeglColor         *color);
static void   gimp_color_scales_set_channel    (GimpColorSelector *selector,
                                                GimpColorSelectorChannel  channel);
static void   gimp_color_scales_set_model_visible
                                               (GimpColorSelector *selector,
                                                GimpColorSelectorModel  model,
                                                gboolean           visible);
static void   gimp_color_scales_set_config     (GimpColorSelector *selector,
                                                GimpColorConfig   *config);
static void   gimp_color_scales_set_format     (GimpColorSelector *selector,
                                                const Babl        *format);

static void   gimp_color_scales_update_visible (GimpColorScales   *scales);
static void   gimp_color_scales_update_scales  (GimpColorScales   *scales,
                                                gint               skip);
static void   gimp_color_scales_toggle_changed (GtkWidget         *widget,
                                                GimpColorScales   *scales);
static void   gimp_color_scales_scale_changed  (GtkWidget         *scale,
                                                GimpColorScales   *scales);
static void   gimp_color_scales_toggle_lch_hsv (GtkToggleButton   *toggle,
                                                GimpColorScales   *scales);


G_DEFINE_TYPE (GimpColorScales, gimp_color_scales, GIMP_TYPE_COLOR_SELECTOR)

#define parent_class gimp_color_scales_parent_class


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

  { (GimpColorSelectorChannel) GIMP_COLOR_SELECTOR_RED_U8,
                                       0, 0, 255, 16, -1275, 1275 },
  { (GimpColorSelectorChannel) GIMP_COLOR_SELECTOR_GREEN_U8,
                                       0, 0, 255, 16, -1275, 1275 },
  { (GimpColorSelectorChannel) GIMP_COLOR_SELECTOR_BLUE_U8,
                                       0, 0, 255, 16, -1275, 1275 },
  { (GimpColorSelectorChannel) GIMP_COLOR_SELECTOR_ALPHA_U8,
                                       0, 0, 255, 16,     0,  255 }
};


static void
gimp_color_scales_class_init (GimpColorScalesClass *klass)
{
  GObjectClass           *object_class   = G_OBJECT_CLASS (klass);
  GtkWidgetClass         *widget_class   = GTK_WIDGET_CLASS (klass);
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
  selector_class->set_format            = gimp_color_scales_set_format;

  g_object_class_install_property (object_class, PROP_SHOW_RGB_U8,
                                   g_param_spec_boolean ("show-rgb-u8",
                                                         "Show RGB 0..255",
                                                         "Show RGB 0..255 scales",
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_SHOW_HSV,
                                   g_param_spec_boolean ("show-hsv",
                                                         "Show HSV",
                                                         "Show HSV instead of LCH",
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  gtk_widget_class_set_css_name (widget_class, "GimpColorScales");
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
  GtkWidget         *grid;
  GEnumClass        *enum_class;
  GtkWidget         *label     = NULL;
  gboolean           add_label = FALSE;
  gint               row;
  gint               i;

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 1);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 1);

  enum_class = g_type_class_ref (GIMP_TYPE_COLOR_SELECTOR_CHANNEL);

  for (i = first_channel, row = 0; i <= last_channel; i++, row++)
    {
      const GimpEnumDesc *enum_desc;
      gint                enum_value = i;
      gboolean            is_u8      = FALSE;

      if ((enum_value >= GIMP_COLOR_SELECTOR_RED_U8 &&
           enum_value <= GIMP_COLOR_SELECTOR_BLUE_U8) ||
          (enum_value >= GIMP_COLOR_SELECTOR_HUE &&
           enum_value <= GIMP_COLOR_SELECTOR_BLUE))
        add_label = TRUE;

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

      gtk_grid_attach (GTK_GRID (grid), scales->toggles[i], 0, row, 1, 1);

      if (gimp_color_selector_get_toggles_visible (selector))
        gtk_widget_show (scales->toggles[i]);

      gimp_help_set_help_data (scales->toggles[i],
                               gettext (enum_desc->value_help), NULL);

      gtk_size_group_add_widget (size_group0, scales->toggles[i]);

      scales->scales[i] =
        gimp_color_scale_entry_new (gettext (enum_desc->value_desc),
                                    scale_defs[i].default_value,
                                    scale_defs[i].spin_min_value,
                                    scale_defs[i].spin_max_value,
                                    is_u8 ? 0 : 1);
      gtk_grid_attach (GTK_GRID (grid), scales->scales[i], 1, row, 3, 1);
      gimp_label_spin_set_increments (GIMP_LABEL_SPIN (scales->scales[i]),
                                      1.0, scale_defs[i].scale_inc);
      gimp_help_set_help_data (scales->scales[i],
                               gettext (enum_desc->value_help),
                               NULL);
      gtk_widget_show (scales->scales[i]);

      gimp_scale_entry_set_bounds (GIMP_SCALE_ENTRY (scales->scales[i]),
                                   scale_defs[i].scale_min_value,
                                   scale_defs[i].scale_max_value,
                                   TRUE);

      g_object_add_weak_pointer (G_OBJECT (scales->scales[i]),
                                 (gpointer) &scales->scales[i]);

      gimp_color_scale_set_channel (GIMP_COLOR_SCALE (gimp_scale_entry_get_range (GIMP_SCALE_ENTRY (scales->scales[i]))),
                                    enum_value);
      gtk_size_group_add_widget (size_group1, scales->scales[i]);

      gtk_size_group_add_widget (size_group2,
                                 gimp_label_spin_get_spin_button (GIMP_LABEL_SPIN (scales->scales[i])));

      g_signal_connect (scales->scales[i], "value-changed",
                        G_CALLBACK (gimp_color_scales_scale_changed),
                        scales);
    }

  if (add_label)
    {
      GtkWidget *scrolled_window;

      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                      GTK_POLICY_EXTERNAL, GTK_POLICY_NEVER);
      gtk_grid_attach (GTK_GRID (grid), scrolled_window, 1, row, 3, 1);
      gtk_widget_set_visible (scrolled_window, TRUE);

      label = gtk_label_new (NULL);
      gtk_widget_set_halign (label, GTK_ALIGN_START);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
      gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
      gtk_label_set_text (GTK_LABEL (label), _("Profile: sRGB"));
      gtk_container_add (GTK_CONTAINER (scrolled_window), label);
      gtk_widget_set_visible (label, TRUE);

      scales->profile_labels = g_list_prepend (scales->profile_labels, label);
    }

  g_type_class_unref (enum_class);

  return grid;
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
  GtkWidget         *grid;
  GSList            *main_group;
  GSList            *u8_group;

  gtk_box_set_spacing (GTK_BOX (scales), 5);

  scales->show_rgb_u8_binding = NULL;
  scales->show_hsv_binding    = NULL;

  /*  don't need the toggles for our own operation  */
  gimp_color_selector_set_toggles_visible (selector, FALSE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (scales), hbox, 0, 0, FALSE);
  gtk_widget_show (hbox);

  main_group = NULL;
  u8_group   = NULL;

  scales->profile_labels = NULL;

  scales->dummy_u8_toggle = gtk_radio_button_new (NULL);
  g_object_ref_sink (scales->dummy_u8_toggle);
  u8_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (scales->dummy_u8_toggle));

  size_group0 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  size_group1 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  size_group2 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  scales->rgb_percent_group =
    grid = create_group (scales, &main_group,
                         size_group0, size_group1, size_group2,
                         GIMP_COLOR_SELECTOR_RED,
                         GIMP_COLOR_SELECTOR_BLUE);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  scales->rgb_u8_group =
    grid = create_group (scales, &u8_group,
                         size_group0, size_group1, size_group2,
                         (GimpColorSelectorChannel) GIMP_COLOR_SELECTOR_RED_U8,
                         (GimpColorSelectorChannel) GIMP_COLOR_SELECTOR_BLUE_U8);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  scales->lch_group =
    grid = create_group (scales, &main_group,
                         size_group0, size_group1, size_group2,
                         GIMP_COLOR_SELECTOR_LCH_LIGHTNESS,
                         GIMP_COLOR_SELECTOR_LCH_HUE);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  scales->hsv_group =
    grid = create_group (scales, &main_group,
                         size_group0, size_group1, size_group2,
                         GIMP_COLOR_SELECTOR_HUE,
                         GIMP_COLOR_SELECTOR_VALUE);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  scales->alpha_percent_group =
    grid = create_group (scales, &main_group,
                         size_group0, size_group1, size_group2,
                         GIMP_COLOR_SELECTOR_ALPHA,
                         GIMP_COLOR_SELECTOR_ALPHA);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  scales->alpha_u8_group =
    grid = create_group (scales, &u8_group,
                         size_group0, size_group1, size_group2,
                         (GimpColorSelectorChannel) GIMP_COLOR_SELECTOR_ALPHA_U8,
                         (GimpColorSelectorChannel) GIMP_COLOR_SELECTOR_ALPHA_U8);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  g_object_unref (size_group0);
  g_object_unref (size_group1);
  g_object_unref (size_group2);

  radio1 = gtk_radio_button_new_with_label (NULL, _("0..100"));
  radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1),
                                                        _("0..255"));

  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radio1), FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radio2), FALSE);

  gtk_box_pack_start (GTK_BOX (hbox), radio1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), radio2, FALSE, FALSE, 0);

  gtk_widget_show (radio1);
  gtk_widget_show (radio2);

  if (scales->show_rgb_u8)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio2), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio1), TRUE);

  g_object_bind_property (G_OBJECT (radio2), "active",
                          G_OBJECT (scales), "show-rgb-u8",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  radio1 = gtk_radio_button_new_with_label (NULL, _("LCh"));
  radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1),
                                                        _("HSV"));

  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radio1), FALSE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (radio2), FALSE);

  gtk_box_pack_end (GTK_BOX (hbox), radio2, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), radio1, FALSE, FALSE, 0);

  gtk_widget_show (radio1);
  gtk_widget_show (radio2);

  if (gimp_color_selector_get_model_visible (selector,
                                             GIMP_COLOR_SELECTOR_MODEL_HSV))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio2), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio1), TRUE);

  g_object_bind_property (G_OBJECT (radio2), "active",
                          G_OBJECT (scales), "show-hsv",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  g_signal_connect (radio1, "toggled",
                    G_CALLBACK (gimp_color_scales_toggle_lch_hsv),
                    scales);

  gimp_color_scales_update_visible (scales);
}

static void
gimp_color_scales_dispose (GObject *object)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (object);

  g_clear_object (&scales->dummy_u8_toggle);

  g_clear_pointer (&scales->show_rgb_u8_binding, g_binding_unbind);
  g_clear_pointer (&scales->show_hsv_binding, g_binding_unbind);
  g_clear_pointer (&scales->profile_labels, g_list_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_scales_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (object);
  gboolean         hsv;

  switch (property_id)
    {
    case PROP_SHOW_RGB_U8:
      g_value_set_boolean (value, scales->show_rgb_u8);
      break;
    case PROP_SHOW_HSV:
      hsv = gimp_color_selector_get_model_visible (GIMP_COLOR_SELECTOR (object),
                                                   GIMP_COLOR_SELECTOR_MODEL_HSV);
      g_value_set_boolean (value, hsv);
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
  gboolean         show_hsv;

  switch (property_id)
    {
    case PROP_SHOW_RGB_U8:
      gimp_color_scales_set_show_rgb_u8 (scales, g_value_get_boolean (value));
      break;
    case PROP_SHOW_HSV:
      show_hsv = g_value_get_boolean (value);

      gimp_color_selector_set_model_visible (GIMP_COLOR_SELECTOR (object),
                                             GIMP_COLOR_SELECTOR_MODEL_LCH,
                                             ! show_hsv);
      gimp_color_selector_set_model_visible (GIMP_COLOR_SELECTOR (object),
                                             GIMP_COLOR_SELECTOR_MODEL_HSV,
                                             show_hsv);
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
                             GeglColor         *color)
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

  g_clear_pointer (&scales->show_rgb_u8_binding, g_binding_unbind);
  g_clear_pointer (&scales->show_hsv_binding, g_binding_unbind);

  if (config)
    {
      scales->show_rgb_u8_binding = g_object_bind_property (config, "show-rgb-u8",
                                                            scales, "show-rgb-u8",
                                                            G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
      scales->show_hsv_binding = g_object_bind_property (config, "show-hsv",
                                                         scales, "show-hsv",
                                                         G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
    }


  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    {
      if (scales->scales[i])
        gimp_color_scale_set_color_config (GIMP_COLOR_SCALE (gimp_scale_entry_get_range (GIMP_SCALE_ENTRY (scales->scales[i]))),
                                           config);
    }
}

static void
gimp_color_scales_set_format (GimpColorSelector *selector,
                              const Babl        *format)
{
  GimpColorScales *scales = GIMP_COLOR_SCALES (selector);

  scales->format = format;

  if (format == NULL || babl_format_get_space (format) == babl_space ("sRGB"))
    {
      for (GList *iter = scales->profile_labels; iter; iter = iter->next)
        {
          gtk_label_set_text (GTK_LABEL (iter->data), _("Profile: sRGB"));
          gimp_help_set_help_data (iter->data, NULL, NULL);
        }
    }
  else
    {
      GimpColorProfile *profile = NULL;
      const gchar      *icc;
      gint              icc_len;

      icc = babl_space_get_icc (babl_format_get_space (format), &icc_len);
      profile = gimp_color_profile_new_from_icc_profile ((const guint8 *) icc, icc_len, NULL);

      if (profile != NULL)
        {
          gchar *text;

          text = g_strdup_printf (_("Profile: %s"), gimp_color_profile_get_label (profile));
          for (GList *iter = scales->profile_labels; iter; iter = iter->next)
            {
              gtk_label_set_text (GTK_LABEL (iter->data), text);
              gimp_help_set_help_data (iter->data,
                                       gimp_color_profile_get_summary (profile),
                                       NULL);
            }
          g_free (text);
        }
      else
        {
          for (GList *iter = scales->profile_labels; iter; iter = iter->next)
            {
              gtk_label_set_markup (GTK_LABEL (iter->data), _("Profile: <i>unknown</i>"));
              gimp_help_set_help_data (iter->data, NULL, NULL);
            }
        }

      g_clear_object (&profile);
    }

  gimp_color_scales_update_scales (scales, -1);
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

  gtk_widget_set_visible (scales->rgb_u8_group,
                          rgb_visible &&   scales->show_rgb_u8);
  gtk_widget_set_visible (scales->rgb_percent_group,
                          rgb_visible && ! scales->show_rgb_u8);

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
  GeglColor         *color    = gimp_color_selector_get_color (selector);
  gdouble            pixel[4];
  gfloat             pixel_f[4];
  gdouble            values[G_N_ELEMENTS (scale_defs)];
  gint               i;

  gegl_color_get_pixel (color, babl_format_with_space ("HSV float", scales->format), pixel_f);
  values[GIMP_COLOR_SELECTOR_HUE]           = pixel_f[0] * 360.0;
  values[GIMP_COLOR_SELECTOR_SATURATION]    = pixel_f[1] * 100.0;
  values[GIMP_COLOR_SELECTOR_VALUE]         = pixel_f[2] * 100.0;

  gegl_color_get_pixel (color, babl_format_with_space ("R'G'B'A double", scales->format), pixel);
  values[GIMP_COLOR_SELECTOR_RED]           = pixel[0] * 100.0;
  values[GIMP_COLOR_SELECTOR_GREEN]         = pixel[1] * 100.0;
  values[GIMP_COLOR_SELECTOR_BLUE]          = pixel[2] * 100.0;
  values[GIMP_COLOR_SELECTOR_ALPHA]         = pixel[3] * 100.0;

  values[GIMP_COLOR_SELECTOR_RED_U8]        = pixel[0] * 255.0;
  values[GIMP_COLOR_SELECTOR_GREEN_U8]      = pixel[1] * 255.0;
  values[GIMP_COLOR_SELECTOR_BLUE_U8]       = pixel[2] * 255.0;
  values[GIMP_COLOR_SELECTOR_ALPHA_U8]      = pixel[3] * 255.0;

  gegl_color_get_pixel (color, babl_format ("CIE LCH(ab) float"), pixel_f);
  values[GIMP_COLOR_SELECTOR_LCH_LIGHTNESS] = pixel_f[0];
  values[GIMP_COLOR_SELECTOR_LCH_CHROMA]    = pixel_f[1];
  values[GIMP_COLOR_SELECTOR_LCH_HUE]       = pixel_f[2];

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    {
      if (i != skip)
        {
          g_signal_handlers_block_by_func (scales->scales[i],
                                           gimp_color_scales_scale_changed,
                                           scales);

          gimp_label_spin_set_value (GIMP_LABEL_SPIN (scales->scales[i]), values[i]);

          g_signal_handlers_unblock_by_func (scales->scales[i],
                                             gimp_color_scales_scale_changed,
                                             scales);
        }

      gimp_color_scale_set_format (GIMP_COLOR_SCALE (gimp_scale_entry_get_range (GIMP_SCALE_ENTRY (scales->scales[i]))),
                                   scales->format);
      gimp_color_scale_set_color (GIMP_COLOR_SCALE (gimp_scale_entry_get_range (GIMP_SCALE_ENTRY (scales->scales[i]))), color);
    }

  g_object_unref (color);
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
gimp_color_scales_scale_changed (GtkWidget       *scale,
                                 GimpColorScales *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);
  GeglColor         *color    = gimp_color_selector_get_color (selector);
  gdouble            value    = gimp_label_spin_get_value (GIMP_LABEL_SPIN (scale));
  gfloat             lch[4];
  gfloat             hsv[4];
  gdouble            rgb[4];
  gint               i;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    if (scales->scales[i] == scale)
      break;

  gegl_color_get_pixel (color, babl_format_with_space ("R'G'B'A double", scales->format), rgb);
  gegl_color_get_pixel (color, babl_format_with_space ("HSVA float", scales->format), hsv);
  gegl_color_get_pixel (color, babl_format ("CIE LCH(ab) alpha float"), lch);

  switch (i)
    {
    case GIMP_COLOR_SELECTOR_HUE:
      hsv[0] = value / 360.0f;
      break;

    case GIMP_COLOR_SELECTOR_SATURATION:
      hsv[1] = value / 100.0f;
      break;

    case GIMP_COLOR_SELECTOR_VALUE:
      hsv[2] = value / 100.0f;
      break;

    case GIMP_COLOR_SELECTOR_RED:
      rgb[0] = value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_GREEN:
      rgb[1] = value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_BLUE:
      rgb[2] = value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_ALPHA:
      gimp_color_set_alpha (color, value / 100.0);
      break;

    case GIMP_COLOR_SELECTOR_LCH_LIGHTNESS:
      lch[0] = (gfloat) value;
      break;

    case GIMP_COLOR_SELECTOR_LCH_CHROMA:
      lch[1] = (gfloat) value;
      break;

    case GIMP_COLOR_SELECTOR_LCH_HUE:
      lch[2] = (gfloat) value;
      break;

    case GIMP_COLOR_SELECTOR_RED_U8:
      rgb[0] = value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_GREEN_U8:
      rgb[1] = value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_BLUE_U8:
      rgb[2] = value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_ALPHA_U8:
      gimp_color_set_alpha (color, value / 255.0);
      break;
    }


  if ((i >= GIMP_COLOR_SELECTOR_HUE) &&
      (i <= GIMP_COLOR_SELECTOR_VALUE))
    {
      gegl_color_set_pixel (color, babl_format_with_space ("HSVA float", scales->format), hsv);
    }
  else if ((i >= GIMP_COLOR_SELECTOR_LCH_LIGHTNESS) &&
           (i <= GIMP_COLOR_SELECTOR_LCH_HUE))
    {
      gegl_color_set_pixel (color, babl_format ("CIE LCH(ab) alpha float"), lch);
    }
  else if (((i >= GIMP_COLOR_SELECTOR_RED) &&
            (i <= GIMP_COLOR_SELECTOR_BLUE)) ||
           ((i >= GIMP_COLOR_SELECTOR_RED_U8) &&
            (i <= GIMP_COLOR_SELECTOR_BLUE_U8)))
    {
      gegl_color_set_pixel (color, babl_format_with_space ("R'G'B'A double", scales->format), rgb);
    }

  gimp_color_selector_set_color (selector, color);

  g_object_unref (color);
}

static void
gimp_color_scales_toggle_lch_hsv (GtkToggleButton *toggle,
                                  GimpColorScales *scales)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (scales);
  gboolean           show_hsv = ! gtk_toggle_button_get_active (toggle);

  gimp_color_selector_set_model_visible (selector,
                                         GIMP_COLOR_SELECTOR_MODEL_LCH,
                                         ! show_hsv);
  gimp_color_selector_set_model_visible (selector,
                                         GIMP_COLOR_SELECTOR_MODEL_HSV,
                                         show_hsv);
  g_object_set (scales, "show-hsv", show_hsv, NULL);
}
