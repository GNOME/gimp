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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "gimpsmudgeoptions.h"

#include "gimp-intl.h"


#define SMUDGE_DEFAULT_RATE       50.0
#define SMUDGE_DEFAULT_FLOW       0.0
#define SMUDGE_DEFAULT_NO_ERASING FALSE


enum
{
  PROP_0,
  PROP_RATE,
  PROP_FLOW,
  PROP_NO_ERASING,
  PROP_SAMPLE_MERGED
};


static void   gimp_smudge_options_set_property (GObject      *object,
                                                guint         property_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   gimp_smudge_options_get_property (GObject      *object,
                                                guint         property_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);


G_DEFINE_TYPE (GimpSmudgeOptions, gimp_smudge_options,
               GIMP_TYPE_PAINT_OPTIONS)


static void
gimp_smudge_options_class_init (GimpSmudgeOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_smudge_options_set_property;
  object_class->get_property = gimp_smudge_options_get_property;

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_RATE,
                           "rate",
                           C_("smudge-tool", "Rate"),
                           _("The strength of smudging"),
                           0.0, 100.0, SMUDGE_DEFAULT_RATE,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_FLOW,
                           "flow",
                           C_("smudge-tool", "Flow"),
                           _("The amount of brush color to blend"),
                           0.0, 100.0, SMUDGE_DEFAULT_FLOW,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_NO_ERASING,
                            "no-erasing",
                            C_("smudge-tool", "No erasing effect"),
                            _("Never decrease alpha of existing pixels"),
                            SMUDGE_DEFAULT_NO_ERASING,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                            "sample-merged",
                            _("Sample merged"),
                            NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_smudge_options_init (GimpSmudgeOptions *options)
{
}

static void
gimp_smudge_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpSmudgeOptions *options = GIMP_SMUDGE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_RATE:
      options->rate = g_value_get_double (value);
      break;
    case PROP_FLOW:
      options->flow = g_value_get_double (value);
      break;
    case PROP_NO_ERASING:
      options->no_erasing = g_value_get_boolean (value);
      break;
    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_smudge_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpSmudgeOptions *options = GIMP_SMUDGE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_RATE:
      g_value_set_double (value, options->rate);
      break;
    case PROP_FLOW:
      g_value_set_double (value, options->flow);
      break;
    case PROP_NO_ERASING:
      g_value_set_boolean (value, options->no_erasing);
      break;
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
