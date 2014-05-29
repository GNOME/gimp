/* GIMP - The GNU Image Manipulation Program
 *
 * gimpwarpoptions.c
 * Copyright (C) 2011 Michael Muré <batolettre@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"

#include "gimpwarpoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_BEHAVIOR,
  PROP_EFFECT_STRENGTH,
  PROP_EFFECT_SIZE,
  PROP_EFFECT_HARDNESS
};


static void   gimp_warp_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   gimp_warp_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (GimpWarpOptions, gimp_warp_options,
               GIMP_TYPE_TOOL_OPTIONS)

#define parent_class gimp_warp_options_parent_class


static void
gimp_warp_options_class_init (GimpWarpOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_warp_options_set_property;
  object_class->get_property = gimp_warp_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_BEHAVIOR,
                                 "behavior",
                                 _("Behavior"),
                                 GIMP_TYPE_WARP_BEHAVIOR,
                                 GIMP_WARP_BEHAVIOR_MOVE,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_EFFECT_STRENGTH,
                                   "effect-strength",
                                   _("Effect Strength"),
                                   1.0, 100.0, 50.0,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_EFFECT_SIZE,
                                   "effect-size",
                                   _("Effect Size"),
                                   1.0, 10000.0, 40.0,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_EFFECT_HARDNESS,
                                   "effect-hardness",
                                   _("Effect Hardness"),
                                   0.0, 1.0, 0.5,
                                   GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_warp_options_init (GimpWarpOptions *options)
{
}

static void
gimp_warp_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpWarpOptions *options = GIMP_WARP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_BEHAVIOR:
      options->behavior = g_value_get_enum (value);
      break;
    case PROP_EFFECT_STRENGTH:
      options->effect_strength = g_value_get_double (value);
      break;
    case PROP_EFFECT_SIZE:
      options->effect_size = g_value_get_double (value);
      break;
    case PROP_EFFECT_HARDNESS:
      options->effect_hardness = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_warp_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpWarpOptions *options = GIMP_WARP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_BEHAVIOR:
      g_value_set_enum (value, options->behavior);
      break;
    case PROP_EFFECT_STRENGTH:
      g_value_set_double (value, options->effect_strength);
      break;
    case PROP_EFFECT_SIZE:
      g_value_set_double (value, options->effect_size);
      break;
    case PROP_EFFECT_HARDNESS:
      g_value_set_double (value, options->effect_hardness);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_warp_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget *combo;
  GtkWidget *scale;

  combo = gimp_prop_enum_combo_box_new (config, "behavior", 0, 0);
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  scale = gimp_prop_spin_scale_new (config, "effect-strength",
                                    _("Strength"),
                                    1, 10, 1);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 100.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, "effect-size",
                                    _("Size"),
                                    0.01, 1.0, 2);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 1000.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, "effect-hardness",
                                    _("Hardness"),
                                    0.01, 1.0, 2);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 0.0, 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  return vbox;
}
