/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorscales.c
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"

#include "ligmawidgetstypes.h"

#include "ligmacolorscale.h"
#include "ligmacolorscales.h"
#include "ligmawidgets.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmacolorscales
 * @title: LigmaColorScales
 * @short_description: A #LigmaColorSelector implementation.
 *
 * The #LigmaColorScales widget is an implementation of a
 * #LigmaColorSelector. It shows a group of #LigmaColorScale widgets
 * that allow to adjust the HSV, LCH, and RGB color channels.
 **/


enum
{
  PROP_0,
  PROP_SHOW_RGB_U8
};

enum
{
  LIGMA_COLOR_SELECTOR_RED_U8 = LIGMA_COLOR_SELECTOR_LCH_HUE + 1,
  LIGMA_COLOR_SELECTOR_GREEN_U8,
  LIGMA_COLOR_SELECTOR_BLUE_U8,
  LIGMA_COLOR_SELECTOR_ALPHA_U8
};


typedef struct _LigmaLCH  LigmaLCH;

struct _LigmaLCH
{
  gdouble l, c, h, a;
};


typedef struct _ColorScale ColorScale;

struct _ColorScale
{
  LigmaColorSelectorChannel  channel;

  gdouble                   default_value;
  gdouble                   scale_min_value;
  gdouble                   scale_max_value;
  gdouble                   scale_inc;
  gdouble                   spin_min_value;
  gdouble                   spin_max_value;
};


#define LIGMA_COLOR_SCALES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_SCALES, LigmaColorScalesClass))
#define LIGMA_IS_COLOR_SCALES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_SCALES))
#define LIGMA_COLOR_SCALES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_SCALES, LigmaColorScalesClass))


typedef struct _LigmaColorScalesClass LigmaColorScalesClass;

struct _LigmaColorScales
{
  LigmaColorSelector  parent_instance;

  gboolean           show_rgb_u8;

  GtkWidget         *lch_group;
  GtkWidget         *hsv_group;
  GtkWidget         *rgb_percent_group;
  GtkWidget         *rgb_u8_group;
  GtkWidget         *alpha_percent_group;
  GtkWidget         *alpha_u8_group;

  GtkWidget         *dummy_u8_toggle;
  GtkWidget         *toggles[14];
  GtkWidget         *scales[14];
};

struct _LigmaColorScalesClass
{
  LigmaColorSelectorClass  parent_class;
};


static void   ligma_color_scales_dispose        (GObject           *object);
static void   ligma_color_scales_get_property   (GObject           *object,
                                                guint              property_id,
                                                GValue            *value,
                                                GParamSpec        *pspec);
static void   ligma_color_scales_set_property   (GObject           *object,
                                                guint              property_id,
                                                const GValue      *value,
                                                GParamSpec        *pspec);

static void   ligma_color_scales_togg_sensitive (LigmaColorSelector *selector,
                                                gboolean           sensitive);
static void   ligma_color_scales_togg_visible   (LigmaColorSelector *selector,
                                                gboolean           visible);

static void   ligma_color_scales_set_show_alpha (LigmaColorSelector *selector,
                                                gboolean           show_alpha);
static void   ligma_color_scales_set_color      (LigmaColorSelector *selector,
                                                const LigmaRGB     *rgb,
                                                const LigmaHSV     *hsv);
static void   ligma_color_scales_set_channel    (LigmaColorSelector *selector,
                                                LigmaColorSelectorChannel  channel);
static void   ligma_color_scales_set_model_visible
                                               (LigmaColorSelector *selector,
                                                LigmaColorSelectorModel  model,
                                                gboolean           visible);
static void   ligma_color_scales_set_config     (LigmaColorSelector *selector,
                                                LigmaColorConfig   *config);

static void   ligma_color_scales_update_visible (LigmaColorScales   *scales);
static void   ligma_color_scales_update_scales  (LigmaColorScales   *scales,
                                                gint               skip);
static void   ligma_color_scales_toggle_changed (GtkWidget         *widget,
                                                LigmaColorScales   *scales);
static void   ligma_color_scales_scale_changed  (GtkWidget         *scale,
                                                LigmaColorScales   *scales);
static void   ligma_color_scales_toggle_lch_hsv (GtkToggleButton   *toggle,
                                                LigmaColorScales   *scales);


G_DEFINE_TYPE (LigmaColorScales, ligma_color_scales, LIGMA_TYPE_COLOR_SELECTOR)

#define parent_class ligma_color_scales_parent_class

static const Babl *fish_rgb_to_lch = NULL;
static const Babl *fish_lch_to_rgb = NULL;

static const ColorScale scale_defs[] =
{
  { LIGMA_COLOR_SELECTOR_HUE,           0, 0, 360, 30,     0,  360 },
  { LIGMA_COLOR_SELECTOR_SATURATION,    0, 0, 100, 10,     0,  500 },
  { LIGMA_COLOR_SELECTOR_VALUE,         0, 0, 100, 10,     0,  500 },

  { LIGMA_COLOR_SELECTOR_RED,           0, 0, 100, 10,  -500,  500 },
  { LIGMA_COLOR_SELECTOR_GREEN,         0, 0, 100, 10,  -500,  500 },
  { LIGMA_COLOR_SELECTOR_BLUE,          0, 0, 100, 10,  -500,  500 },
  { LIGMA_COLOR_SELECTOR_ALPHA,         0, 0, 100, 10,     0,  100 },

  { LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS, 0, 0, 100, 10,     0,  300 },
  { LIGMA_COLOR_SELECTOR_LCH_CHROMA,    0, 0, 200, 10,     0,  300 },
  { LIGMA_COLOR_SELECTOR_LCH_HUE,       0, 0, 360, 30,     0,  360 },

  { LIGMA_COLOR_SELECTOR_RED_U8,        0, 0, 255, 16, -1275, 1275 },
  { LIGMA_COLOR_SELECTOR_GREEN_U8,      0, 0, 255, 16, -1275, 1275 },
  { LIGMA_COLOR_SELECTOR_BLUE_U8,       0, 0, 255, 16, -1275, 1275 },
  { LIGMA_COLOR_SELECTOR_ALPHA_U8,      0, 0, 255, 16,     0,  255 }
};


static void
ligma_color_scales_class_init (LigmaColorScalesClass *klass)
{
  GObjectClass           *object_class   = G_OBJECT_CLASS (klass);
  GtkWidgetClass         *widget_class   = GTK_WIDGET_CLASS (klass);
  LigmaColorSelectorClass *selector_class = LIGMA_COLOR_SELECTOR_CLASS (klass);

  object_class->dispose                 = ligma_color_scales_dispose;
  object_class->get_property            = ligma_color_scales_get_property;
  object_class->set_property            = ligma_color_scales_set_property;

  selector_class->name                  = _("Scales");
  selector_class->help_id               = "ligma-colorselector-scales";
  selector_class->icon_name             = LIGMA_ICON_DIALOG_TOOL_OPTIONS;
  selector_class->set_toggles_visible   = ligma_color_scales_togg_visible;
  selector_class->set_toggles_sensitive = ligma_color_scales_togg_sensitive;
  selector_class->set_show_alpha        = ligma_color_scales_set_show_alpha;
  selector_class->set_color             = ligma_color_scales_set_color;
  selector_class->set_channel           = ligma_color_scales_set_channel;
  selector_class->set_model_visible     = ligma_color_scales_set_model_visible;
  selector_class->set_config            = ligma_color_scales_set_config;

  g_object_class_install_property (object_class, PROP_SHOW_RGB_U8,
                                   g_param_spec_boolean ("show-rgb-u8",
                                                         "Show RGB 0..255",
                                                         "Show RGB 0..255 scales",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  gtk_widget_class_set_css_name (widget_class, "LigmaColorScales");

  fish_rgb_to_lch = babl_fish (babl_format ("R'G'B'A double"),
                               babl_format ("CIE LCH(ab) alpha double"));
  fish_lch_to_rgb = babl_fish (babl_format ("CIE LCH(ab) alpha double"),
                               babl_format ("R'G'B'A double"));
}

static GtkWidget *
create_group (LigmaColorScales           *scales,
              GSList                   **radio_group,
              GtkSizeGroup              *size_group0,
              GtkSizeGroup              *size_group1,
              GtkSizeGroup              *size_group2,
              LigmaColorSelectorChannel   first_channel,
              LigmaColorSelectorChannel   last_channel)
{
  LigmaColorSelector *selector = LIGMA_COLOR_SELECTOR (scales);
  GtkWidget         *grid;
  GEnumClass        *enum_class;
  gint               row;
  gint               i;

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 1);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 1);

  enum_class = g_type_class_ref (LIGMA_TYPE_COLOR_SELECTOR_CHANNEL);

  for (i = first_channel, row = 0; i <= last_channel; i++, row++)
    {
      const LigmaEnumDesc *enum_desc;
      gint                enum_value = i;
      gboolean            is_u8      = FALSE;

      if (enum_value >= LIGMA_COLOR_SELECTOR_RED_U8 &&
          enum_value <= LIGMA_COLOR_SELECTOR_ALPHA_U8)
        {
          enum_value -= 7;
          is_u8 = TRUE;
        }

      enum_desc = ligma_enum_get_desc (enum_class, enum_value);

      if (i == LIGMA_COLOR_SELECTOR_ALPHA ||
          i == LIGMA_COLOR_SELECTOR_ALPHA_U8)
        {
          /*  just to allocate the space via the size group  */
          scales->toggles[i] = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        }
      else
        {
          scales->toggles[i] = gtk_radio_button_new (*radio_group);
          *radio_group =
            gtk_radio_button_get_group (GTK_RADIO_BUTTON (scales->toggles[i]));

          if (enum_value == ligma_color_selector_get_channel (selector))
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
                                G_CALLBACK (ligma_color_scales_toggle_changed),
                                scales);
            }
        }

      gtk_grid_attach (GTK_GRID (grid), scales->toggles[i], 0, row, 1, 1);

      if (ligma_color_selector_get_toggles_visible (selector))
        gtk_widget_show (scales->toggles[i]);

      ligma_help_set_help_data (scales->toggles[i],
                               gettext (enum_desc->value_help), NULL);

      gtk_size_group_add_widget (size_group0, scales->toggles[i]);

      scales->scales[i] =
        ligma_color_scale_entry_new (gettext (enum_desc->value_desc),
                                    scale_defs[i].default_value,
                                    scale_defs[i].spin_min_value,
                                    scale_defs[i].spin_max_value,
                                    1);
      gtk_grid_attach (GTK_GRID (grid), scales->scales[i], 1, row, 3, 1);
      ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scales->scales[i]),
                                      1.0, scale_defs[i].scale_inc);
      ligma_help_set_help_data (scales->scales[i],
                               gettext (enum_desc->value_help),
                               NULL);
      gtk_widget_show (scales->scales[i]);

      ligma_scale_entry_set_bounds (LIGMA_SCALE_ENTRY (scales->scales[i]),
                                   scale_defs[i].scale_min_value,
                                   scale_defs[i].scale_max_value,
                                   TRUE);

      g_object_add_weak_pointer (G_OBJECT (scales->scales[i]),
                                 (gpointer) &scales->scales[i]);

      ligma_color_scale_set_channel (LIGMA_COLOR_SCALE (ligma_scale_entry_get_range (LIGMA_SCALE_ENTRY (scales->scales[i]))),
                                    enum_value);
      gtk_size_group_add_widget (size_group1, scales->scales[i]);

      gtk_size_group_add_widget (size_group2,
                                 ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scales->scales[i])));

      g_signal_connect (scales->scales[i], "value-changed",
                        G_CALLBACK (ligma_color_scales_scale_changed),
                        scales);
    }

  g_type_class_unref (enum_class);

  return grid;
}

static void
ligma_color_scales_init (LigmaColorScales *scales)
{
  LigmaColorSelector *selector = LIGMA_COLOR_SELECTOR (scales);
  GtkSizeGroup      *size_group0;
  GtkSizeGroup      *size_group1;
  GtkSizeGroup      *size_group2;
  GtkWidget         *hbox;
  GtkWidget         *radio1;
  GtkWidget         *radio2;
  GtkWidget         *grid;
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
    grid = create_group (scales, &main_group,
                         size_group0, size_group1, size_group2,
                         LIGMA_COLOR_SELECTOR_RED,
                         LIGMA_COLOR_SELECTOR_BLUE);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  scales->rgb_u8_group =
    grid = create_group (scales, &u8_group,
                         size_group0, size_group1, size_group2,
                         LIGMA_COLOR_SELECTOR_RED_U8,
                         LIGMA_COLOR_SELECTOR_BLUE_U8);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  scales->lch_group =
    grid = create_group (scales, &main_group,
                         size_group0, size_group1, size_group2,
                         LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS,
                         LIGMA_COLOR_SELECTOR_LCH_HUE);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

 scales->hsv_group =
    grid = create_group (scales, &main_group,
                         size_group0, size_group1, size_group2,
                         LIGMA_COLOR_SELECTOR_HUE,
                         LIGMA_COLOR_SELECTOR_VALUE);
 gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  scales->alpha_percent_group =
    grid = create_group (scales, &main_group,
                         size_group0, size_group1, size_group2,
                         LIGMA_COLOR_SELECTOR_ALPHA,
                         LIGMA_COLOR_SELECTOR_ALPHA);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  scales->alpha_u8_group =
    grid = create_group (scales, &u8_group,
                         size_group0, size_group1, size_group2,
                         LIGMA_COLOR_SELECTOR_ALPHA_U8,
                         LIGMA_COLOR_SELECTOR_ALPHA_U8);
  gtk_box_pack_start (GTK_BOX (scales), grid, FALSE, FALSE, 0);

  g_object_unref (size_group0);
  g_object_unref (size_group1);
  g_object_unref (size_group2);

  ligma_color_scales_update_visible (scales);

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

  if (ligma_color_selector_get_model_visible (selector,
                                             LIGMA_COLOR_SELECTOR_MODEL_HSV))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio2), TRUE);

  g_signal_connect (radio1, "toggled",
                    G_CALLBACK (ligma_color_scales_toggle_lch_hsv),
                    scales);
}

static void
ligma_color_scales_dispose (GObject *object)
{
  LigmaColorScales *scales = LIGMA_COLOR_SCALES (object);

  g_clear_object (&scales->dummy_u8_toggle);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_color_scales_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaColorScales *scales = LIGMA_COLOR_SCALES (object);

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
ligma_color_scales_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaColorScales *scales = LIGMA_COLOR_SCALES (object);

  switch (property_id)
    {
    case PROP_SHOW_RGB_U8:
      ligma_color_scales_set_show_rgb_u8 (scales, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_scales_togg_sensitive (LigmaColorSelector *selector,
                                  gboolean           sensitive)
{
  LigmaColorScales *scales = LIGMA_COLOR_SCALES (selector);
  gint             i;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    if (scales->toggles[i])
      gtk_widget_set_sensitive (scales->toggles[i], sensitive);
}

static void
ligma_color_scales_togg_visible (LigmaColorSelector *selector,
                                gboolean           visible)
{
  LigmaColorScales *scales = LIGMA_COLOR_SCALES (selector);
  gint             i;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    if (scales->toggles[i])
      gtk_widget_set_visible (scales->toggles[i], visible);
}

static void
ligma_color_scales_set_show_alpha (LigmaColorSelector *selector,
                                  gboolean           show_alpha)
{
  ligma_color_scales_update_visible (LIGMA_COLOR_SCALES (selector));
}

static void
ligma_color_scales_set_color (LigmaColorSelector *selector,
                             const LigmaRGB     *rgb,
                             const LigmaHSV     *hsv)
{
  LigmaColorScales *scales = LIGMA_COLOR_SCALES (selector);

  ligma_color_scales_update_scales (scales, -1);
}

static void
ligma_color_scales_set_channel (LigmaColorSelector        *selector,
                               LigmaColorSelectorChannel  channel)
{
  LigmaColorScales *scales = LIGMA_COLOR_SCALES (selector);

  if (GTK_IS_RADIO_BUTTON (scales->toggles[channel]))
    {
      g_signal_handlers_block_by_func (scales->toggles[channel],
                                       ligma_color_scales_toggle_changed,
                                       scales);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scales->toggles[channel]),
                                    TRUE);

      g_signal_handlers_unblock_by_func (scales->toggles[channel],
                                         ligma_color_scales_toggle_changed,
                                         scales);
    }
}

static void
ligma_color_scales_set_model_visible (LigmaColorSelector      *selector,
                                     LigmaColorSelectorModel  model,
                                     gboolean                visible)
{
  ligma_color_scales_update_visible (LIGMA_COLOR_SCALES (selector));
}

static void
ligma_color_scales_set_config (LigmaColorSelector *selector,
                              LigmaColorConfig   *config)
{
  LigmaColorScales *scales = LIGMA_COLOR_SCALES (selector);
  gint             i;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    {
      if (scales->scales[i])
        ligma_color_scale_set_color_config (LIGMA_COLOR_SCALE (ligma_scale_entry_get_range (LIGMA_SCALE_ENTRY (scales->scales[i]))),
                                           config);
    }
}


/*  public functions  */

void
ligma_color_scales_set_show_rgb_u8 (LigmaColorScales *scales,
                                   gboolean         show_rgb_u8)
{
  g_return_if_fail (LIGMA_IS_COLOR_SCALES (scales));

  show_rgb_u8 = show_rgb_u8 ? TRUE : FALSE;

  if (show_rgb_u8 != scales->show_rgb_u8)
    {
      scales->show_rgb_u8 = show_rgb_u8;

      g_object_notify (G_OBJECT (scales), "show-rgb-u8");

      ligma_color_scales_update_visible (scales);
    }
}

gboolean
ligma_color_scales_get_show_rgb_u8 (LigmaColorScales *scales)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_SCALES (scales), FALSE);

  return scales->show_rgb_u8;
}


/*  private functions  */

static void
ligma_color_scales_update_visible (LigmaColorScales *scales)
{
  LigmaColorSelector *selector = LIGMA_COLOR_SELECTOR (scales);
  gboolean           show_alpha;
  gboolean           rgb_visible;
  gboolean           lch_visible;
  gboolean           hsv_visible;

  show_alpha  = ligma_color_selector_get_show_alpha (selector);
  rgb_visible = ligma_color_selector_get_model_visible (selector,
                                                       LIGMA_COLOR_SELECTOR_MODEL_RGB);
  lch_visible = ligma_color_selector_get_model_visible (selector,
                                                       LIGMA_COLOR_SELECTOR_MODEL_LCH);
  hsv_visible = ligma_color_selector_get_model_visible (selector,
                                                       LIGMA_COLOR_SELECTOR_MODEL_HSV);

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
ligma_color_scales_update_scales (LigmaColorScales *scales,
                                 gint             skip)
{
  LigmaColorSelector *selector = LIGMA_COLOR_SELECTOR (scales);
  LigmaLCH            lch;
  gdouble            values[G_N_ELEMENTS (scale_defs)];
  gint               i;

  babl_process (fish_rgb_to_lch, &selector->rgb, &lch, 1);

  values[LIGMA_COLOR_SELECTOR_HUE]           = selector->hsv.h * 360.0;
  values[LIGMA_COLOR_SELECTOR_SATURATION]    = selector->hsv.s * 100.0;
  values[LIGMA_COLOR_SELECTOR_VALUE]         = selector->hsv.v * 100.0;

  values[LIGMA_COLOR_SELECTOR_RED]           = selector->rgb.r * 100.0;
  values[LIGMA_COLOR_SELECTOR_GREEN]         = selector->rgb.g * 100.0;
  values[LIGMA_COLOR_SELECTOR_BLUE]          = selector->rgb.b * 100.0;
  values[LIGMA_COLOR_SELECTOR_ALPHA]         = selector->rgb.a * 100.0;

  values[LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS] = lch.l;
  values[LIGMA_COLOR_SELECTOR_LCH_CHROMA]    = lch.c;
  values[LIGMA_COLOR_SELECTOR_LCH_HUE]       = lch.h;

  values[LIGMA_COLOR_SELECTOR_RED_U8]        = selector->rgb.r * 255.0;
  values[LIGMA_COLOR_SELECTOR_GREEN_U8]      = selector->rgb.g * 255.0;
  values[LIGMA_COLOR_SELECTOR_BLUE_U8]       = selector->rgb.b * 255.0;
  values[LIGMA_COLOR_SELECTOR_ALPHA_U8]      = selector->rgb.a * 255.0;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    {
      if (i != skip)
        {
          g_signal_handlers_block_by_func (scales->scales[i],
                                           ligma_color_scales_scale_changed,
                                           scales);

          ligma_label_spin_set_value (LIGMA_LABEL_SPIN (scales->scales[i]), values[i]);

          g_signal_handlers_unblock_by_func (scales->scales[i],
                                             ligma_color_scales_scale_changed,
                                             scales);
        }

      ligma_color_scale_set_color (LIGMA_COLOR_SCALE (ligma_scale_entry_get_range (LIGMA_SCALE_ENTRY (scales->scales[i]))),
                                  &selector->rgb, &selector->hsv);
    }
}

static void
ligma_color_scales_toggle_changed (GtkWidget       *widget,
                                  LigmaColorScales *scales)
{
  LigmaColorSelector *selector = LIGMA_COLOR_SELECTOR (scales);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
        {
          if (widget == scales->toggles[i])
            {
              ligma_color_selector_set_channel (selector, i);

              if (i < LIGMA_COLOR_SELECTOR_RED ||
                  i > LIGMA_COLOR_SELECTOR_BLUE)
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
ligma_color_scales_scale_changed (GtkWidget       *scale,
                                 LigmaColorScales *scales)
{
  LigmaColorSelector *selector = LIGMA_COLOR_SELECTOR (scales);
  gdouble            value    = ligma_label_spin_get_value (LIGMA_LABEL_SPIN (scale));
  LigmaLCH            lch;
  gint               i;

  for (i = 0; i < G_N_ELEMENTS (scale_defs); i++)
    if (scales->scales[i] == scale)
      break;

  switch (i)
    {
    case LIGMA_COLOR_SELECTOR_HUE:
      selector->hsv.h = value / 360.0;
      break;

    case LIGMA_COLOR_SELECTOR_SATURATION:
      selector->hsv.s = value / 100.0;
      break;

    case LIGMA_COLOR_SELECTOR_VALUE:
      selector->hsv.v = value / 100.0;
      break;

    case LIGMA_COLOR_SELECTOR_RED:
      selector->rgb.r = value / 100.0;
      break;

    case LIGMA_COLOR_SELECTOR_GREEN:
      selector->rgb.g = value / 100.0;
      break;

    case LIGMA_COLOR_SELECTOR_BLUE:
      selector->rgb.b = value / 100.0;
      break;

    case LIGMA_COLOR_SELECTOR_ALPHA:
      selector->hsv.a = selector->rgb.a = value / 100.0;
      break;

    case LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS:
      babl_process (fish_rgb_to_lch, &selector->rgb, &lch, 1);
      lch.l = value;
      break;

    case LIGMA_COLOR_SELECTOR_LCH_CHROMA:
      babl_process (fish_rgb_to_lch, &selector->rgb, &lch, 1);
      lch.c = value;
      break;

    case LIGMA_COLOR_SELECTOR_LCH_HUE:
      babl_process (fish_rgb_to_lch, &selector->rgb, &lch, 1);
      lch.h = value;
      break;

    case LIGMA_COLOR_SELECTOR_RED_U8:
      selector->rgb.r = value / 255.0;
      break;

    case LIGMA_COLOR_SELECTOR_GREEN_U8:
      selector->rgb.g = value / 255.0;
      break;

    case LIGMA_COLOR_SELECTOR_BLUE_U8:
      selector->rgb.b = value / 255.0;
      break;

    case LIGMA_COLOR_SELECTOR_ALPHA_U8:
      selector->hsv.a = selector->rgb.a = value / 255.0;
      break;
    }

  if ((i >= LIGMA_COLOR_SELECTOR_HUE) &&
      (i <= LIGMA_COLOR_SELECTOR_VALUE))
    {
      ligma_hsv_to_rgb (&selector->hsv, &selector->rgb);
    }
  else if ((i >= LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS) &&
           (i <= LIGMA_COLOR_SELECTOR_LCH_HUE))
    {
      babl_process (fish_lch_to_rgb, &lch, &selector->rgb, 1);
      ligma_rgb_to_hsv (&selector->rgb, &selector->hsv);
    }
  else if ((i >= LIGMA_COLOR_SELECTOR_RED) &&
           (i <= LIGMA_COLOR_SELECTOR_BLUE))
    {
      ligma_rgb_to_hsv (&selector->rgb, &selector->hsv);
    }
  else if ((i >= LIGMA_COLOR_SELECTOR_RED_U8) &&
           (i <= LIGMA_COLOR_SELECTOR_BLUE_U8))
    {
      ligma_rgb_to_hsv (&selector->rgb, &selector->hsv);
    }

  ligma_color_scales_update_scales (scales, i);

  ligma_color_selector_emit_color_changed (selector);
}

static void
ligma_color_scales_toggle_lch_hsv (GtkToggleButton *toggle,
                                  LigmaColorScales *scales)
{
  LigmaColorSelector *selector = LIGMA_COLOR_SELECTOR (scales);

  if (gtk_toggle_button_get_active (toggle))
    {
      ligma_color_selector_set_model_visible (selector,
                                             LIGMA_COLOR_SELECTOR_MODEL_LCH,
                                             TRUE);
      ligma_color_selector_set_model_visible (selector,
                                             LIGMA_COLOR_SELECTOR_MODEL_HSV,
                                             FALSE);
    }
  else
    {
      ligma_color_selector_set_model_visible (selector,
                                             LIGMA_COLOR_SELECTOR_MODEL_LCH,
                                             FALSE);
      ligma_color_selector_set_model_visible (selector,
                                             LIGMA_COLOR_SELECTOR_MODEL_HSV,
                                             TRUE);
    }
}
