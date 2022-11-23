/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmahuesaturationconfig.c
 * Copyright (C) 2007 Michael Natterer <mitch@ligma.org>
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmaconfig/ligmaconfig.h"

#include "operations-types.h"

#include "ligmahuesaturationconfig.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_RANGE,
  PROP_HUE,
  PROP_SATURATION,
  PROP_LIGHTNESS,
  PROP_OVERLAP
};


static void     ligma_hue_saturation_config_iface_init   (LigmaConfigInterface *iface);

static void     ligma_hue_saturation_config_get_property (GObject          *object,
                                                         guint             property_id,
                                                         GValue           *value,
                                                         GParamSpec       *pspec);
static void     ligma_hue_saturation_config_set_property (GObject          *object,
                                                         guint             property_id,
                                                         const GValue     *value,
                                                         GParamSpec       *pspec);

static gboolean ligma_hue_saturation_config_serialize    (LigmaConfig       *config,
                                                         LigmaConfigWriter *writer,
                                                         gpointer          data);
static gboolean ligma_hue_saturation_config_deserialize  (LigmaConfig       *config,
                                                         GScanner         *scanner,
                                                         gint              nest_level,
                                                         gpointer          data);
static gboolean ligma_hue_saturation_config_equal        (LigmaConfig       *a,
                                                         LigmaConfig       *b);
static void     ligma_hue_saturation_config_reset        (LigmaConfig       *config);
static gboolean ligma_hue_saturation_config_copy         (LigmaConfig       *src,
                                                         LigmaConfig       *dest,
                                                         GParamFlags       flags);


G_DEFINE_TYPE_WITH_CODE (LigmaHueSaturationConfig, ligma_hue_saturation_config,
                         LIGMA_TYPE_OPERATION_SETTINGS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_hue_saturation_config_iface_init))

#define parent_class ligma_hue_saturation_config_parent_class


static void
ligma_hue_saturation_config_class_init (LigmaHueSaturationConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class = LIGMA_VIEWABLE_CLASS (klass);

  object_class->set_property        = ligma_hue_saturation_config_set_property;
  object_class->get_property        = ligma_hue_saturation_config_get_property;

  viewable_class->default_icon_name = "ligma-tool-hue-saturation";

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_RANGE,
                         "range",
                         _("Range"),
                         _("The affected range"),
                         LIGMA_TYPE_HUE_RANGE,
                         LIGMA_HUE_RANGE_ALL, 0);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_HUE,
                           "hue",
                           _("Hue"),
                           _("Hue"),
                           -1.0, 1.0, 0.0, 0);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_SATURATION,
                           "saturation",
                           _("Saturation"),
                           _("Saturation"),
                           -1.0, 1.0, 0.0, 0);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_LIGHTNESS,
                           "lightness",
                           _("Lightness"),
                           _("Lightness"),
                           -1.0, 1.0, 0.0, 0);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OVERLAP,
                           "overlap",
                           _("Overlap"),
                           _("Overlap"),
                           0.0, 1.0, 0.0, 0);
}

static void
ligma_hue_saturation_config_iface_init (LigmaConfigInterface *iface)
{
  iface->serialize   = ligma_hue_saturation_config_serialize;
  iface->deserialize = ligma_hue_saturation_config_deserialize;
  iface->equal       = ligma_hue_saturation_config_equal;
  iface->reset       = ligma_hue_saturation_config_reset;
  iface->copy        = ligma_hue_saturation_config_copy;
}

static void
ligma_hue_saturation_config_init (LigmaHueSaturationConfig *self)
{
  ligma_config_reset (LIGMA_CONFIG (self));
}

static void
ligma_hue_saturation_config_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  LigmaHueSaturationConfig *self = LIGMA_HUE_SATURATION_CONFIG (object);

  switch (property_id)
    {
    case PROP_RANGE:
      g_value_set_enum (value, self->range);
      break;

    case PROP_HUE:
      g_value_set_double (value, self->hue[self->range]);
      break;

    case PROP_SATURATION:
      g_value_set_double (value, self->saturation[self->range]);
      break;

    case PROP_LIGHTNESS:
      g_value_set_double (value, self->lightness[self->range]);
      break;

    case PROP_OVERLAP:
      g_value_set_double (value, self->overlap);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_hue_saturation_config_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  LigmaHueSaturationConfig *self = LIGMA_HUE_SATURATION_CONFIG (object);

  switch (property_id)
    {
    case PROP_RANGE:
      self->range = g_value_get_enum (value);
      g_object_notify (object, "hue");
      g_object_notify (object, "saturation");
      g_object_notify (object, "lightness");
      break;

    case PROP_HUE:
      self->hue[self->range] = g_value_get_double (value);
      break;

    case PROP_SATURATION:
      self->saturation[self->range] = g_value_get_double (value);
      break;

    case PROP_LIGHTNESS:
      self->lightness[self->range] = g_value_get_double (value);
      break;

    case PROP_OVERLAP:
      self->overlap = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_hue_saturation_config_serialize (LigmaConfig       *config,
                                      LigmaConfigWriter *writer,
                                      gpointer          data)
{
  LigmaHueSaturationConfig *hs_config = LIGMA_HUE_SATURATION_CONFIG (config);
  LigmaHueRange             range;
  LigmaHueRange             old_range;
  gboolean                 success = TRUE;

  if (! ligma_operation_settings_config_serialize_base (config, writer, data))
    return FALSE;

  old_range = hs_config->range;

  for (range = LIGMA_HUE_RANGE_ALL; range <= LIGMA_HUE_RANGE_MAGENTA; range++)
    {
      hs_config->range = range;

      success = (ligma_config_serialize_property_by_name (config, "range",
                                                         writer) &&
                 ligma_config_serialize_property_by_name (config, "hue",
                                                         writer) &&
                 ligma_config_serialize_property_by_name (config, "saturation",
                                                         writer) &&
                 ligma_config_serialize_property_by_name (config, "lightness",
                                                         writer));

      if (! success)
        break;
    }

  if (success)
    success = ligma_config_serialize_property_by_name (config, "overlap",
                                                      writer);

  hs_config->range = old_range;

  return success;
}

static gboolean
ligma_hue_saturation_config_deserialize (LigmaConfig *config,
                                        GScanner   *scanner,
                                        gint        nest_level,
                                        gpointer    data)
{
  LigmaHueSaturationConfig *hs_config = LIGMA_HUE_SATURATION_CONFIG (config);
  LigmaHueRange             old_range;
  gboolean                 success = TRUE;

  old_range = hs_config->range;

  success = ligma_config_deserialize_properties (config, scanner, nest_level);

  g_object_set (config, "range", old_range, NULL);

  return success;
}

static gboolean
ligma_hue_saturation_config_equal (LigmaConfig *a,
                                  LigmaConfig *b)
{
  LigmaHueSaturationConfig *config_a = LIGMA_HUE_SATURATION_CONFIG (a);
  LigmaHueSaturationConfig *config_b = LIGMA_HUE_SATURATION_CONFIG (b);
  LigmaHueRange             range;

  if (! ligma_operation_settings_config_equal_base (a, b))
    return FALSE;

  for (range = LIGMA_HUE_RANGE_ALL; range <= LIGMA_HUE_RANGE_MAGENTA; range++)
    {
      if (config_a->hue[range]        != config_b->hue[range]        ||
          config_a->saturation[range] != config_b->saturation[range] ||
          config_a->lightness[range]  != config_b->lightness[range])
        return FALSE;
    }

  /* don't compare "range" */

  if (config_a->overlap != config_b->overlap)
    return FALSE;

  return TRUE;
}

static void
ligma_hue_saturation_config_reset (LigmaConfig *config)
{
  LigmaHueSaturationConfig *hs_config = LIGMA_HUE_SATURATION_CONFIG (config);
  LigmaHueRange             range;

  ligma_operation_settings_config_reset_base (config);

  for (range = LIGMA_HUE_RANGE_ALL; range <= LIGMA_HUE_RANGE_MAGENTA; range++)
    {
      hs_config->range = range;
      ligma_hue_saturation_config_reset_range (hs_config);
    }

  ligma_config_reset_property (G_OBJECT (config), "range");
  ligma_config_reset_property (G_OBJECT (config), "overlap");
}

static gboolean
ligma_hue_saturation_config_copy (LigmaConfig   *src,
                                 LigmaConfig   *dest,
                                 GParamFlags   flags)
{
  LigmaHueSaturationConfig *src_config  = LIGMA_HUE_SATURATION_CONFIG (src);
  LigmaHueSaturationConfig *dest_config = LIGMA_HUE_SATURATION_CONFIG (dest);
  LigmaHueRange             range;

  if (! ligma_operation_settings_config_copy_base (src, dest, flags))
    return FALSE;

  for (range = LIGMA_HUE_RANGE_ALL; range <= LIGMA_HUE_RANGE_MAGENTA; range++)
    {
      dest_config->hue[range]        = src_config->hue[range];
      dest_config->saturation[range] = src_config->saturation[range];
      dest_config->lightness[range]  = src_config->lightness[range];
    }

  g_object_notify (G_OBJECT (dest), "hue");
  g_object_notify (G_OBJECT (dest), "saturation");
  g_object_notify (G_OBJECT (dest), "lightness");

  dest_config->range   = src_config->range;
  dest_config->overlap = src_config->overlap;

  g_object_notify (G_OBJECT (dest), "range");
  g_object_notify (G_OBJECT (dest), "overlap");

  return TRUE;
}


/*  public functions  */

void
ligma_hue_saturation_config_reset_range (LigmaHueSaturationConfig *config)
{
  g_return_if_fail (LIGMA_IS_HUE_SATURATION_CONFIG (config));

  g_object_freeze_notify (G_OBJECT (config));

  ligma_config_reset_property (G_OBJECT (config), "hue");
  ligma_config_reset_property (G_OBJECT (config), "saturation");
  ligma_config_reset_property (G_OBJECT (config), "lightness");

  g_object_thaw_notify (G_OBJECT (config));
}
