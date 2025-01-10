/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationsettings.c
 * Copyright (C) 2020 Ell
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpconfig/gimpconfig.h"

#include "operations-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawablefilter.h"

#include "gimpoperationsettings.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CLIP,
  PROP_REGION,
  PROP_MODE,
  PROP_OPACITY,
};


static void   gimp_operation_settings_get_property (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);
static void   gimp_operation_settings_set_property (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);


G_DEFINE_TYPE (GimpOperationSettings, gimp_operation_settings,
               GIMP_TYPE_SETTINGS)

#define parent_class gimp_operation_settings_parent_class


static void
gimp_operation_settings_class_init (GimpOperationSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_operation_settings_set_property;
  object_class->get_property = gimp_operation_settings_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_CLIP,
                         "gimp-clip",
                         _("Clipping"),
                         _("How to clip"),
                         GIMP_TYPE_TRANSFORM_RESIZE,
                         GIMP_TRANSFORM_RESIZE_ADJUST,
                         GIMP_CONFIG_PARAM_DEFAULTS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_REGION,
                         "gimp-region",
                         NULL, NULL,
                         GIMP_TYPE_FILTER_REGION,
                         GIMP_FILTER_REGION_SELECTION,
                         GIMP_CONFIG_PARAM_DEFAULTS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "gimp-mode",
                         _("Mode"),
                         NULL,
                         GIMP_TYPE_LAYER_MODE,
                         GIMP_LAYER_MODE_REPLACE,
                         GIMP_CONFIG_PARAM_DEFAULTS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_OPACITY,
                           "gimp-opacity",
                           _("Opacity"),
                           NULL,
                           0.0, 1.0, 1.0,
                           GIMP_CONFIG_PARAM_DEFAULTS);
}

static void
gimp_operation_settings_init (GimpOperationSettings *settings)
{
}

static void
gimp_operation_settings_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpOperationSettings *settings = GIMP_OPERATION_SETTINGS (object);

  switch (property_id)
    {
    case PROP_CLIP:
      g_value_set_enum (value, settings->clip);
      break;

    case PROP_REGION:
      g_value_set_enum (value, settings->region);
      break;

    case PROP_MODE:
      g_value_set_enum (value, settings->mode);
      break;

    case PROP_OPACITY:
      g_value_set_double (value, settings->opacity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_settings_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpOperationSettings *settings = GIMP_OPERATION_SETTINGS (object);

  switch (property_id)
    {
    case PROP_CLIP:
      settings->clip = g_value_get_enum (value);
      break;

    case PROP_REGION:
      settings->region = g_value_get_enum (value);
      break;

    case PROP_MODE:
      settings->mode = g_value_get_enum (value);
      break;

    case PROP_OPACITY:
      settings->opacity = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
gimp_operation_settings_sync_drawable_filter (GimpOperationSettings *settings,
                                              GimpDrawableFilter    *filter)
{
  gboolean clip;

  g_return_if_fail (GIMP_IS_OPERATION_SETTINGS (settings));
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  clip = settings->clip == GIMP_TRANSFORM_RESIZE_CLIP ||
         ! babl_format_has_alpha (gimp_drawable_filter_get_format (filter));

  gimp_drawable_filter_set_region     (filter, settings->region);
  gimp_drawable_filter_set_clip       (filter, clip);
  gimp_drawable_filter_set_mode       (filter,
                                       settings->mode,
                                       GIMP_LAYER_COLOR_SPACE_AUTO,
                                       GIMP_LAYER_COLOR_SPACE_AUTO,
                                       GIMP_LAYER_COMPOSITE_AUTO);
  gimp_drawable_filter_set_opacity    (filter, settings->opacity);
}


/*  protected functions  */

static const gchar * const base_properties[] =
{
  "time",
  "gimp-clip",
  "gimp-region",
  "gimp-mode",
  "gimp-opacity",
};

gboolean
gimp_operation_settings_config_serialize_base (GimpConfig       *config,
                                               GimpConfigWriter *writer,
                                               gpointer          data)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (base_properties); i++)
    {
      if (! gimp_config_serialize_property_by_name (config,
                                                    base_properties[i],
                                                    writer))
        {
          return FALSE;
        }
    }

  return TRUE;
}

gboolean
gimp_operation_settings_config_equal_base (GimpConfig *a,
                                           GimpConfig *b)
{
  GimpOperationSettings *settings_a = GIMP_OPERATION_SETTINGS (a);
  GimpOperationSettings *settings_b = GIMP_OPERATION_SETTINGS (b);

  return settings_a->clip       == settings_b->clip    &&
         settings_a->region     == settings_b->region  &&
         settings_a->mode       == settings_b->mode    &&
         settings_a->opacity    == settings_b->opacity;
}

void
gimp_operation_settings_config_reset_base (GimpConfig *config)
{
  gint i;

  g_object_freeze_notify (G_OBJECT (config));

  for (i = 0; i < G_N_ELEMENTS (base_properties); i++)
    gimp_config_reset_property (G_OBJECT (config), base_properties[i]);

  g_object_thaw_notify (G_OBJECT (config));
}

gboolean
gimp_operation_settings_config_copy_base (GimpConfig  *src,
                                          GimpConfig  *dest,
                                          GParamFlags  flags)
{
  gint i;

  g_object_freeze_notify (G_OBJECT (dest));

  for (i = 0; i < G_N_ELEMENTS (base_properties); i++)
    {
      g_object_unref (g_object_bind_property (src,  base_properties[i],
                                              dest, base_properties[i],
                                              G_BINDING_SYNC_CREATE));
    }

  g_object_thaw_notify (G_OBJECT (dest));

  return TRUE;
}
