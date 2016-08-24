/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDialogConfig class
 * Copyright (C) 2016  Michael Natterer <mitch@gimp.org>
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "config-types.h"

#include "gimprc-blurbs.h"
#include "gimpdialogconfig.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_COLOR_PROFILE_POLICY,

  PROP_LAYER_NEW_NAME,
  PROP_LAYER_NEW_FILL_TYPE,

  PROP_LAYER_ADD_MASK_TYPE,
  PROP_LAYER_ADD_MASK_INVERT,

  PROP_CHANNEL_NEW_NAME,
  PROP_CHANNEL_NEW_COLOR,

  PROP_VECTORS_NEW_NAME
};


static void  gimp_dialog_config_finalize     (GObject      *object);
static void  gimp_dialog_config_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void  gimp_dialog_config_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (GimpDialogConfig, gimp_dialog_config, GIMP_TYPE_GUI_CONFIG)

#define parent_class gimp_dialog_config_parent_class


static void
gimp_dialog_config_class_init (GimpDialogConfigClass *klass)
{
  GObjectClass *object_class     = G_OBJECT_CLASS (klass);
  GimpRGB       half_transparent = { 0.0, 0.0, 0.0, 0.5 };

  object_class->finalize     = gimp_dialog_config_finalize;
  object_class->set_property = gimp_dialog_config_set_property;
  object_class->get_property = gimp_dialog_config_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_COLOR_PROFILE_POLICY,
                         "color-profile-policy",
                         "Color profile policy",
                         COLOR_PROFILE_POLICY_BLURB,
                         GIMP_TYPE_COLOR_PROFILE_POLICY,
                         GIMP_COLOR_PROFILE_POLICY_ASK,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_LAYER_NEW_NAME,
                           "layer-new-name",
                           "Default new layer name",
                           LAYER_NEW_NAME_BLURB,
                           _("Layer"),
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_NEW_FILL_TYPE,
                         "layer-new-fill-type",
                         "Default new layer fill type",
                         LAYER_NEW_FILL_TYPE_BLURB,
                         GIMP_TYPE_FILL_TYPE,
                         GIMP_FILL_TRANSPARENT,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_ADD_MASK_TYPE,
                         "layer-add-mask-type",
                         "Default layer mask type",
                         LAYER_ADD_MASK_TYPE_BLURB,
                         GIMP_TYPE_ADD_MASK_TYPE,
                         GIMP_ADD_MASK_WHITE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LAYER_ADD_MASK_INVERT,
                            "layer-add-mask-invert",
                            "Default layer mask invert",
                            LAYER_ADD_MASK_INVERT_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_CHANNEL_NEW_NAME,
                           "channel-new-name",
                           "Default new channel name",
                           CHANNEL_NEW_NAME_BLURB,
                           _("Channel"),
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_RGB (object_class, PROP_CHANNEL_NEW_COLOR,
                        "channel-new-color",
                        "Default new channel color and opacity",
                        CHANNEL_NEW_COLOR_BLURB,
                        TRUE,
                        &half_transparent,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_VECTORS_NEW_NAME,
                           "path-new-name",
                           "Default new path name",
                           VECTORS_NEW_NAME_BLURB,
                           _("Path"),
                           GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_dialog_config_init (GimpDialogConfig *config)
{
}

static void
gimp_dialog_config_finalize (GObject *object)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (object);

  if (config->layer_new_name)
    {
      g_free (config->layer_new_name);
      config->layer_new_name = NULL;
    }

  if (config->channel_new_name)
    {
      g_free (config->channel_new_name);
      config->channel_new_name = NULL;
    }

  if (config->vectors_new_name)
    {
      g_free (config->vectors_new_name);
      config->vectors_new_name = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dialog_config_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (object);

  switch (property_id)
    {
    case PROP_COLOR_PROFILE_POLICY:
      config->color_profile_policy = g_value_get_enum (value);
      break;

    case PROP_LAYER_NEW_NAME:
      if (config->layer_new_name)
        g_free (config->layer_new_name);
      config->layer_new_name = g_value_dup_string (value);
      break;
    case PROP_LAYER_NEW_FILL_TYPE:
      config->layer_new_fill_type = g_value_get_enum (value);
      break;

    case PROP_LAYER_ADD_MASK_TYPE:
      config->layer_add_mask_type = g_value_get_enum (value);
      break;
    case PROP_LAYER_ADD_MASK_INVERT:
      config->layer_add_mask_invert = g_value_get_boolean (value);
      break;

    case PROP_CHANNEL_NEW_NAME:
      if (config->channel_new_name)
        g_free (config->channel_new_name);
      config->channel_new_name = g_value_dup_string (value);
      break;
    case PROP_CHANNEL_NEW_COLOR:
      gimp_value_get_rgb (value, &config->channel_new_color);
      break;

    case PROP_VECTORS_NEW_NAME:
      if (config->vectors_new_name)
        g_free (config->vectors_new_name);
      config->vectors_new_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dialog_config_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (object);

  switch (property_id)
    {
    case PROP_COLOR_PROFILE_POLICY:
      g_value_set_enum (value, config->color_profile_policy);
      break;

    case PROP_LAYER_NEW_NAME:
      g_value_set_string (value, config->layer_new_name);
      break;
    case PROP_LAYER_NEW_FILL_TYPE:
      g_value_set_enum (value, config->layer_new_fill_type);
      break;

    case PROP_LAYER_ADD_MASK_TYPE:
      g_value_set_enum (value, config->layer_add_mask_type);
      break;
    case PROP_LAYER_ADD_MASK_INVERT:
      g_value_set_boolean (value, config->layer_add_mask_invert);
      break;

    case PROP_CHANNEL_NEW_NAME:
      g_value_set_string (value, config->channel_new_name);
      break;
    case PROP_CHANNEL_NEW_COLOR:
      gimp_value_set_rgb (value, &config->channel_new_color);
      break;

    case PROP_VECTORS_NEW_NAME:
      g_value_set_string (value, config->vectors_new_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
