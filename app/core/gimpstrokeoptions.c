/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpstrokeoptions.c
 * Copyright (C) 2003 Simon Budig
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpdashpattern.h"
#include "gimpmarshal.h"
#include "gimpstrokeoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_STYLE,
  PROP_WIDTH,
  PROP_UNIT,
  PROP_CAP_STYLE,
  PROP_JOIN_STYLE,
  PROP_MITER_LIMIT,
  PROP_ANTIALIAS,
  PROP_DASH_UNIT,
  PROP_DASH_OFFSET,
  PROP_DASH_INFO
};

enum
{
  DASH_INFO_CHANGED,
  LAST_SIGNAL
};


static void   gimp_stroke_options_set_property  (GObject      *object,
                                                 guint         property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void   gimp_stroke_options_get_property  (GObject      *object,
                                                 guint         property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);


G_DEFINE_TYPE (GimpStrokeOptions, gimp_stroke_options, GIMP_TYPE_CONTEXT)

static guint stroke_options_signals[LAST_SIGNAL] = { 0 };


static void
gimp_stroke_options_class_init (GimpStrokeOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *array_spec;

  object_class->set_property = gimp_stroke_options_set_property;
  object_class->get_property = gimp_stroke_options_get_property;

  klass->dash_info_changed = NULL;

  stroke_options_signals[DASH_INFO_CHANGED] =
    g_signal_new ("dash-info-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpStrokeOptionsClass, dash_info_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DASH_PRESET);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_STYLE,
                                 "style", NULL,
                                 GIMP_TYPE_STROKE_STYLE,
                                 GIMP_STROKE_STYLE_SOLID,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_WIDTH,
                                   "width", NULL,
                                   0.0, 2000.0, 6.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_UNIT,
                                 "unit", NULL,
                                 TRUE, FALSE, GIMP_UNIT_PIXEL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_CAP_STYLE,
                                 "cap-style", NULL,
                                 GIMP_TYPE_CAP_STYLE, GIMP_CAP_BUTT,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_JOIN_STYLE,
                                 "join-style", NULL,
                                 GIMP_TYPE_JOIN_STYLE, GIMP_JOIN_MITER,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_MITER_LIMIT,
                                   "miter-limit",
                                   _("Convert a mitered join to a bevelled "
                                     "join if the miter would extend to a "
                                     "distance of more than miter-limit * "
                                     "line-width from the actual join point."),
                                   0.0, 100.0, 10.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                                    "antialias", NULL,
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_DASH_OFFSET,
                                   "dash-offset", NULL,
                                   0.0, 2000.0, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS);

  array_spec = g_param_spec_double ("dash-length", NULL, NULL,
                                    0.0, 2000.0, 1.0, GIMP_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DASH_INFO,
                                   g_param_spec_value_array ("dash-info",
                                                             NULL, NULL,
                                                             array_spec,
                                                             GIMP_PARAM_STATIC_STRINGS |
                                                             GIMP_CONFIG_PARAM_FLAGS));
}

static void
gimp_stroke_options_init (GimpStrokeOptions *options)
{
}

static void
gimp_stroke_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpStrokeOptions *options = GIMP_STROKE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_STYLE:
      options->style = g_value_get_enum (value);
      break;
    case PROP_WIDTH:
      options->width = g_value_get_double (value);
      break;
    case PROP_UNIT:
      options->unit = g_value_get_int (value);
      break;
    case PROP_CAP_STYLE:
      options->cap_style = g_value_get_enum (value);
      break;
    case PROP_JOIN_STYLE:
      options->join_style = g_value_get_enum (value);
      break;
    case PROP_MITER_LIMIT:
      options->miter_limit = g_value_get_double (value);
      break;
    case PROP_ANTIALIAS:
      options->antialias = g_value_get_boolean (value);
      break;
    case PROP_DASH_OFFSET:
      options->dash_offset = g_value_get_double (value);
      break;
    case PROP_DASH_INFO:
      {
        GValueArray *value_array = g_value_get_boxed (value);
        GArray      *pattern;

        pattern = gimp_dash_pattern_from_value_array (value_array);
        gimp_stroke_options_set_dash_pattern (options, GIMP_DASH_CUSTOM,
                                              pattern);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_stroke_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpStrokeOptions *options = GIMP_STROKE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_STYLE:
      g_value_set_enum (value, options->style);
      break;
    case PROP_WIDTH:
      g_value_set_double (value, options->width);
      break;
    case PROP_UNIT:
      g_value_set_int (value, options->unit);
      break;
    case PROP_CAP_STYLE:
      g_value_set_enum (value, options->cap_style);
      break;
    case PROP_JOIN_STYLE:
      g_value_set_enum (value, options->join_style);
      break;
    case PROP_MITER_LIMIT:
      g_value_set_double (value, options->miter_limit);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, options->antialias);
      break;
    case PROP_DASH_OFFSET:
      g_value_set_double (value, options->dash_offset);
      break;
    case PROP_DASH_INFO:
      {
        GValueArray *value_array;

        value_array = gimp_dash_pattern_to_value_array (options->dash_info);
        g_value_take_boxed (value, value_array);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_stroke_options_set_dash_pattern:
 * @options: a #GimpStrokeOptions object
 * @preset: a value out of the #GimpDashPreset enum
 * @pattern: a #GArray or %NULL if @preset is not %GIMP_DASH_CUSTOM
 *
 * Sets the dash pattern. Either a @preset is passed and @pattern is
 * %NULL or @preset is %GIMP_DASH_CUSTOM and @pattern is the #GArray
 * to use as the dash pattern. Note that this function takes ownership
 * of the passed pattern.
 */
void
gimp_stroke_options_set_dash_pattern (GimpStrokeOptions *options,
                                      GimpDashPreset     preset,
                                      GArray            *pattern)
{
  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (options));
  g_return_if_fail (preset == GIMP_DASH_CUSTOM || pattern == NULL);

  if (preset != GIMP_DASH_CUSTOM)
    pattern = gimp_dash_pattern_new_from_preset (preset);

  if (options->dash_info)
    gimp_dash_pattern_free (options->dash_info);

  options->dash_info = pattern;

  g_object_notify (G_OBJECT (options), "dash-info");

  g_signal_emit (options, stroke_options_signals [DASH_INFO_CHANGED], 0,
                 preset);
}
