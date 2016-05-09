/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "tools-types.h"

#include "gimpfilteroptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_PREVIEW,
  PROP_PREVIEW_SPLIT,
  PROP_PREVIEW_ALIGNMENT,
  PROP_PREVIEW_POSITION,
  PROP_REGION,
  PROP_SETTINGS
};


static void   gimp_filter_options_finalize     (GObject      *object);
static void   gimp_filter_options_set_property (GObject      *object,
                                                guint         property_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   gimp_filter_options_get_property (GObject      *object,
                                                guint         property_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);


G_DEFINE_TYPE (GimpFilterOptions, gimp_filter_options,
               GIMP_TYPE_TOOL_OPTIONS)

#define parent_class gimp_filter_options_parent_class


static void
gimp_filter_options_class_init (GimpFilterOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_filter_options_finalize;
  object_class->set_property = gimp_filter_options_set_property;
  object_class->get_property = gimp_filter_options_get_property;

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_PREVIEW,
                            "preview",
                            _("_Preview"),
                            NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_PREVIEW_SPLIT,
                                   g_param_spec_boolean ("preview-split",
                                                         _("_Split view"),
                                                         NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PREVIEW_ALIGNMENT,
                                   g_param_spec_enum ("preview-alignment",
                                                      NULL, NULL,
                                                      GIMP_TYPE_ALIGNMENT_TYPE,
                                                      GIMP_ALIGN_LEFT,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PREVIEW_POSITION,
                                   g_param_spec_double ("preview-position",
                                                        NULL, NULL,
                                                        0.0, 1.0, 0.5,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_REGION,
                                   g_param_spec_enum ("region",
                                                      NULL, NULL,
                                                      GIMP_TYPE_IMAGE_MAP_REGION,
                                                      GIMP_IMAGE_MAP_REGION_SELECTION,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SETTINGS,
                                   g_param_spec_object ("settings",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_filter_options_init (GimpFilterOptions *options)
{
}

static void
gimp_filter_options_finalize (GObject *object)
{
  GimpFilterOptions *options = GIMP_FILTER_OPTIONS (object);

  if (options->settings)
    {
      g_object_unref (options->settings);
      options->settings = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gimp_filter_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpFilterOptions *options = GIMP_FILTER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_PREVIEW:
      options->preview = g_value_get_boolean (value);
      break;

    case PROP_PREVIEW_SPLIT:
      options->preview_split = g_value_get_boolean (value);
      break;

    case PROP_PREVIEW_ALIGNMENT:
      options->preview_alignment = g_value_get_enum (value);
      break;

    case PROP_PREVIEW_POSITION:
      options->preview_position = g_value_get_double (value);
      break;

    case PROP_REGION:
      options->region = g_value_get_enum (value);
      break;

    case PROP_SETTINGS:
      if (options->settings)
        g_object_unref (options->settings);
      options->settings = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_filter_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpFilterOptions *options = GIMP_FILTER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_PREVIEW:
      g_value_set_boolean (value, options->preview);
      break;

    case PROP_PREVIEW_SPLIT:
      g_value_set_boolean (value, options->preview_split);
      break;

    case PROP_PREVIEW_ALIGNMENT:
      g_value_set_enum (value, options->preview_alignment);
      break;

    case PROP_PREVIEW_POSITION:
      g_value_set_double (value, options->preview_position);
      break;

    case PROP_REGION:
      g_value_set_enum (value, options->region);
      break;

    case PROP_SETTINGS:
      g_value_set_object (value, options->settings);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
