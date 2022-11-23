/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationsettings.c
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

#include "libligmaconfig/ligmaconfig.h"

#include "operations-types.h"

#include "gegl/ligma-gegl-utils.h"

#include "core/ligmadrawable.h"
#include "core/ligmadrawablefilter.h"

#include "ligmaoperationsettings.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_CLIP,
  PROP_REGION,
  PROP_MODE,
  PROP_OPACITY,
  PROP_GAMMA_HACK
};


static void   ligma_operation_settings_get_property (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);
static void   ligma_operation_settings_set_property (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaOperationSettings, ligma_operation_settings,
               LIGMA_TYPE_SETTINGS)

#define parent_class ligma_operation_settings_parent_class


static void
ligma_operation_settings_class_init (LigmaOperationSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_operation_settings_set_property;
  object_class->get_property = ligma_operation_settings_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_CLIP,
                         "ligma-clip",
                         _("Clipping"),
                         _("How to clip"),
                         LIGMA_TYPE_TRANSFORM_RESIZE,
                         LIGMA_TRANSFORM_RESIZE_ADJUST,
                         LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_REGION,
                         "ligma-region",
                         NULL, NULL,
                         LIGMA_TYPE_FILTER_REGION,
                         LIGMA_FILTER_REGION_SELECTION,
                         LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "ligma-mode",
                         _("Mode"),
                         NULL,
                         LIGMA_TYPE_LAYER_MODE,
                         LIGMA_LAYER_MODE_REPLACE,
                         LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OPACITY,
                           "ligma-opacity",
                           _("Opacity"),
                           NULL,
                           0.0, 1.0, 1.0,
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_GAMMA_HACK,
                            "ligma-gamma-hack",
                            "Gamma hack (temp hack, please ignore)",
                            NULL,
                            FALSE,
                            LIGMA_CONFIG_PARAM_DEFAULTS);
}

static void
ligma_operation_settings_init (LigmaOperationSettings *settings)
{
}

static void
ligma_operation_settings_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  LigmaOperationSettings *settings = LIGMA_OPERATION_SETTINGS (object);

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

    case PROP_GAMMA_HACK:
      g_value_set_boolean (value, settings->gamma_hack);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_operation_settings_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  LigmaOperationSettings *settings = LIGMA_OPERATION_SETTINGS (object);

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

    case PROP_GAMMA_HACK:
      settings->gamma_hack = g_value_get_boolean (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
ligma_operation_settings_sync_drawable_filter (LigmaOperationSettings *settings,
                                              LigmaDrawableFilter    *filter)
{
  gboolean clip;

  g_return_if_fail (LIGMA_IS_OPERATION_SETTINGS (settings));
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  clip = settings->clip == LIGMA_TRANSFORM_RESIZE_CLIP ||
         ! babl_format_has_alpha (ligma_drawable_filter_get_format (filter));

  ligma_drawable_filter_set_region     (filter, settings->region);
  ligma_drawable_filter_set_clip       (filter, clip);
  ligma_drawable_filter_set_mode       (filter,
                                       settings->mode,
                                       LIGMA_LAYER_COLOR_SPACE_AUTO,
                                       LIGMA_LAYER_COLOR_SPACE_AUTO,
                                       LIGMA_LAYER_COMPOSITE_AUTO);
  ligma_drawable_filter_set_opacity    (filter, settings->opacity);
  ligma_drawable_filter_set_gamma_hack (filter, settings->gamma_hack);
}


/*  protected functions  */

static const gchar * const base_properties[] =
{
  "time",
  "ligma-clip",
  "ligma-region",
  "ligma-mode",
  "ligma-opacity",
  "ligma-gamma-hack"
};

gboolean
ligma_operation_settings_config_serialize_base (LigmaConfig       *config,
                                               LigmaConfigWriter *writer,
                                               gpointer          data)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (base_properties); i++)
    {
      if (! ligma_config_serialize_property_by_name (config,
                                                    base_properties[i],
                                                    writer))
        {
          return FALSE;
        }
    }

  return TRUE;
}

gboolean
ligma_operation_settings_config_equal_base (LigmaConfig *a,
                                           LigmaConfig *b)
{
  LigmaOperationSettings *settings_a = LIGMA_OPERATION_SETTINGS (a);
  LigmaOperationSettings *settings_b = LIGMA_OPERATION_SETTINGS (b);

  return settings_a->clip       == settings_b->clip    &&
         settings_a->region     == settings_b->region  &&
         settings_a->mode       == settings_b->mode    &&
         settings_a->opacity    == settings_b->opacity &&
         settings_a->gamma_hack == settings_b->gamma_hack;
}

void
ligma_operation_settings_config_reset_base (LigmaConfig *config)
{
  gint i;

  g_object_freeze_notify (G_OBJECT (config));

  for (i = 0; i < G_N_ELEMENTS (base_properties); i++)
    ligma_config_reset_property (G_OBJECT (config), base_properties[i]);

  g_object_thaw_notify (G_OBJECT (config));
}

gboolean
ligma_operation_settings_config_copy_base (LigmaConfig  *src,
                                          LigmaConfig  *dest,
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
