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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
  PROP_EFFECT_SIZE,
  PROP_EFFECT_HARDNESS,
  PROP_EFFECT_STRENGTH,
  PROP_STROKE_SPACING,
  PROP_INTERPOLATION,
  PROP_ABYSS_POLICY,
  PROP_HIGH_QUALITY_PREVIEW,
  PROP_REAL_TIME_PREVIEW,
  PROP_STROKE_DURING_MOTION,
  PROP_STROKE_PERIODICALLY,
  PROP_STROKE_PERIODICALLY_RATE,
  PROP_N_ANIMATION_FRAMES
};


static void       gimp_warp_options_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void       gimp_warp_options_get_property (GObject      *object,
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

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_BEHAVIOR,
                         "behavior",
                         _("Behavior"),
                         _("Behavior"),
                         GIMP_TYPE_WARP_BEHAVIOR,
                         GIMP_WARP_BEHAVIOR_MOVE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_EFFECT_SIZE,
                           "effect-size",
                           _("Size"),
                           _("Effect Size"),
                           1.0, 10000.0, 40.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_EFFECT_HARDNESS,
                           "effect-hardness",
                           _("Hardness"),
                           _("Effect Hardness"),
                           0.0, 100.0, 50.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_EFFECT_STRENGTH,
                           "effect-strength",
                           _("Strength"),
                           _("Effect Strength"),
                           1.0, 100.0, 50.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_SPACING,
                           "stroke-spacing",
                           _("Spacing"),
                           _("Stroke Spacing"),
                           1.0, 100.0, 10.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_INTERPOLATION,
                         "interpolation",
                         _("Interpolation"),
                         _("Interpolation method"),
                         GIMP_TYPE_INTERPOLATION_TYPE,
                         GIMP_INTERPOLATION_CUBIC,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_ABYSS_POLICY,
                         "abyss-policy",
                         _("Abyss policy"),
                         _("Out-of-bounds sampling behavior"),
                         GEGL_TYPE_ABYSS_POLICY,
                         GEGL_ABYSS_NONE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_HIGH_QUALITY_PREVIEW,
                            "high-quality-preview",
                            _("High quality preview"),
                            _("Use an accurate but slower preview"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_REAL_TIME_PREVIEW,
                            "real-time-preview",
                            _("Real-time preview"),
                            _("Render preview in real time (slower)"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_STROKE_DURING_MOTION,
                            "stroke-during-motion",
                            _("During motion"),
                            _("Apply effect during motion"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_STROKE_PERIODICALLY,
                            "stroke-periodically",
                            _("Periodically"),
                            _("Apply effect periodically"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_PERIODICALLY_RATE,
                           "stroke-periodically-rate",
                           _("Rate"),
                           _("Periodic stroke rate"),
                           0.0, 100.0, 50.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT (object_class, PROP_N_ANIMATION_FRAMES,
                        "n-animation-frames",
                        _("Frames"),
                        _("Number of animation frames"),
                        3, 1000, 10,
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
    case PROP_EFFECT_SIZE:
      options->effect_size = g_value_get_double (value);
      break;
    case PROP_EFFECT_HARDNESS:
      options->effect_hardness = g_value_get_double (value);
      break;
    case PROP_EFFECT_STRENGTH:
      options->effect_strength = g_value_get_double (value);
      break;
    case PROP_STROKE_SPACING:
      options->stroke_spacing = g_value_get_double (value);
      break;
    case PROP_INTERPOLATION:
      options->interpolation = g_value_get_enum (value);
      break;
    case PROP_ABYSS_POLICY:
      options->abyss_policy = g_value_get_enum (value);
      break;
    case PROP_HIGH_QUALITY_PREVIEW:
      options->high_quality_preview = g_value_get_boolean (value);
      break;
    case PROP_REAL_TIME_PREVIEW:
      options->real_time_preview = g_value_get_boolean (value);
      break;
    case PROP_STROKE_DURING_MOTION:
      options->stroke_during_motion = g_value_get_boolean (value);
      break;
    case PROP_STROKE_PERIODICALLY:
      options->stroke_periodically = g_value_get_boolean (value);
      break;
    case PROP_STROKE_PERIODICALLY_RATE:
      options->stroke_periodically_rate = g_value_get_double (value);
      break;
    case PROP_N_ANIMATION_FRAMES:
      options->n_animation_frames = g_value_get_int (value);
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
    case PROP_EFFECT_SIZE:
      g_value_set_double (value, options->effect_size);
      break;
    case PROP_EFFECT_HARDNESS:
      g_value_set_double (value, options->effect_hardness);
      break;
    case PROP_EFFECT_STRENGTH:
      g_value_set_double (value, options->effect_strength);
      break;
    case PROP_STROKE_SPACING:
      g_value_set_double (value, options->stroke_spacing);
      break;
    case PROP_INTERPOLATION:
      g_value_set_enum (value, options->interpolation);
      break;
    case PROP_ABYSS_POLICY:
      g_value_set_enum (value, options->abyss_policy);
      break;
    case PROP_HIGH_QUALITY_PREVIEW:
      g_value_set_boolean (value, options->high_quality_preview);
      break;
    case PROP_REAL_TIME_PREVIEW:
      g_value_set_boolean (value, options->real_time_preview);
      break;
    case PROP_STROKE_DURING_MOTION:
      g_value_set_boolean (value, options->stroke_during_motion);
      break;
    case PROP_STROKE_PERIODICALLY:
      g_value_set_boolean (value, options->stroke_periodically);
      break;
    case PROP_STROKE_PERIODICALLY_RATE:
      g_value_set_double (value, options->stroke_periodically_rate);
      break;
    case PROP_N_ANIMATION_FRAMES:
      g_value_set_int (value, options->n_animation_frames);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_warp_options_gui (GimpToolOptions *tool_options)
{
  GimpWarpOptions *options = GIMP_WARP_OPTIONS (tool_options);
  GObject         *config  = G_OBJECT (tool_options);
  GtkWidget       *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget       *frame;
  GtkWidget       *vbox2;
  GtkWidget       *button;
  GtkWidget       *combo;
  GtkWidget       *scale;

  combo = gimp_prop_enum_combo_box_new (config, "behavior", 0, 0);
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  options->behavior_combo = combo;

  scale = gimp_prop_spin_scale_new (config, "effect-size", NULL,
                                    0.01, 1.0, 2);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 1000.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, "effect-hardness", NULL,
                                    1, 10, 1);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 0.0, 100.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, "effect-strength", NULL,
                                    1, 10, 1);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 100.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, "stroke-spacing", NULL,
                                    1, 10, 1);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 100.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  combo = gimp_prop_enum_combo_box_new (config, "interpolation", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Interpolation"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  combo = gimp_prop_enum_combo_box_new (config, "abyss-policy",
                                        GEGL_ABYSS_NONE, GEGL_ABYSS_LOOP);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Abyss policy"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  button = gimp_prop_check_button_new (config, "high-quality-preview", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (config, "real-time-preview", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the stroke frame  */
  frame = gimp_frame_new (_("Stroke"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  options->stroke_frame = frame;

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  button = gimp_prop_check_button_new (config, "stroke-during-motion", NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  scale = gimp_prop_spin_scale_new (config, "stroke-periodically-rate", NULL,
                                    1, 10, 1);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 0.0, 100.0);

  frame = gimp_prop_expanding_frame_new (config, "stroke-periodically", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the animation frame  */
  frame = gimp_frame_new (_("Animate"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  scale = gimp_prop_spin_scale_new (config, "n-animation-frames", NULL,
                                    1.0, 10.0, 0);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 3.0, 100.0);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  options->animate_button = gtk_button_new_with_label (_("Create Animation"));
  gtk_widget_set_sensitive (options->animate_button, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), options->animate_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (options->animate_button);

  g_object_add_weak_pointer (G_OBJECT (options->animate_button),
                             (gpointer) &options->animate_button);

  return vbox;
}
