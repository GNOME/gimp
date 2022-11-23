/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacolorbalanceconfig.c
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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmaconfig/ligmaconfig.h"

#include "operations-types.h"

#include "ligmacolorbalanceconfig.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_RANGE,
  PROP_CYAN_RED,
  PROP_MAGENTA_GREEN,
  PROP_YELLOW_BLUE,
  PROP_PRESERVE_LUMINOSITY
};


static void     ligma_color_balance_config_iface_init   (LigmaConfigInterface *iface);

static void     ligma_color_balance_config_get_property (GObject          *object,
                                                        guint             property_id,
                                                        GValue           *value,
                                                        GParamSpec       *pspec);
static void     ligma_color_balance_config_set_property (GObject          *object,
                                                        guint             property_id,
                                                        const GValue     *value,
                                                        GParamSpec       *pspec);

static gboolean ligma_color_balance_config_serialize    (LigmaConfig       *config,
                                                        LigmaConfigWriter *writer,
                                                        gpointer          data);
static gboolean ligma_color_balance_config_deserialize  (LigmaConfig       *config,
                                                        GScanner         *scanner,
                                                        gint              nest_level,
                                                        gpointer          data);
static gboolean ligma_color_balance_config_equal        (LigmaConfig       *a,
                                                        LigmaConfig       *b);
static void     ligma_color_balance_config_reset        (LigmaConfig       *config);
static gboolean ligma_color_balance_config_copy         (LigmaConfig       *src,
                                                        LigmaConfig       *dest,
                                                        GParamFlags       flags);


G_DEFINE_TYPE_WITH_CODE (LigmaColorBalanceConfig, ligma_color_balance_config,
                         LIGMA_TYPE_OPERATION_SETTINGS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_color_balance_config_iface_init))

#define parent_class ligma_color_balance_config_parent_class


static void
ligma_color_balance_config_class_init (LigmaColorBalanceConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class = LIGMA_VIEWABLE_CLASS (klass);

  object_class->set_property        = ligma_color_balance_config_set_property;
  object_class->get_property        = ligma_color_balance_config_get_property;

  viewable_class->default_icon_name = "ligma-tool-color-balance";

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_RANGE,
                         "range",
                         _("Range"),
                         _("The affected range"),
                         LIGMA_TYPE_TRANSFER_MODE,
                         LIGMA_TRANSFER_MIDTONES, 0);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_CYAN_RED,
                           "cyan-red",
                           _("Cyan-Red"),
                           _("Cyan-Red"),
                           -1.0, 1.0, 0.0, 0);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_MAGENTA_GREEN,
                           "magenta-green",
                           _("Magenta-Green"),
                           _("Magenta-Green"),
                           -1.0, 1.0, 0.0, 0);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_YELLOW_BLUE,
                           "yellow-blue",
                           _("Yellow-Blue"),
                           _("Yellow-Blue"),
                           -1.0, 1.0, 0.0, 0);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_PRESERVE_LUMINOSITY,
                            "preserve-luminosity",
                            _("Preserve Luminosity"),
                            _("Preserve Luminosity"),
                            TRUE, 0);
}

static void
ligma_color_balance_config_iface_init (LigmaConfigInterface *iface)
{
  iface->serialize   = ligma_color_balance_config_serialize;
  iface->deserialize = ligma_color_balance_config_deserialize;
  iface->equal       = ligma_color_balance_config_equal;
  iface->reset       = ligma_color_balance_config_reset;
  iface->copy        = ligma_color_balance_config_copy;
}

static void
ligma_color_balance_config_init (LigmaColorBalanceConfig *self)
{
  ligma_config_reset (LIGMA_CONFIG (self));
}

static void
ligma_color_balance_config_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  LigmaColorBalanceConfig *self = LIGMA_COLOR_BALANCE_CONFIG (object);

  switch (property_id)
    {
    case PROP_RANGE:
      g_value_set_enum (value, self->range);
      break;

    case PROP_CYAN_RED:
      g_value_set_double (value, self->cyan_red[self->range]);
      break;

    case PROP_MAGENTA_GREEN:
      g_value_set_double (value, self->magenta_green[self->range]);
      break;

    case PROP_YELLOW_BLUE:
      g_value_set_double (value, self->yellow_blue[self->range]);
      break;

    case PROP_PRESERVE_LUMINOSITY:
      g_value_set_boolean (value, self->preserve_luminosity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_balance_config_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  LigmaColorBalanceConfig *self = LIGMA_COLOR_BALANCE_CONFIG (object);

  switch (property_id)
    {
    case PROP_RANGE:
      self->range = g_value_get_enum (value);
      g_object_notify (object, "cyan-red");
      g_object_notify (object, "magenta-green");
      g_object_notify (object, "yellow-blue");
      break;

    case PROP_CYAN_RED:
      self->cyan_red[self->range] = g_value_get_double (value);
      break;

    case PROP_MAGENTA_GREEN:
      self->magenta_green[self->range] = g_value_get_double (value);
      break;

    case PROP_YELLOW_BLUE:
      self->yellow_blue[self->range] = g_value_get_double (value);
      break;

    case PROP_PRESERVE_LUMINOSITY:
      self->preserve_luminosity = g_value_get_boolean (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_color_balance_config_serialize (LigmaConfig       *config,
                                     LigmaConfigWriter *writer,
                                     gpointer          data)
{
  LigmaColorBalanceConfig *bc_config = LIGMA_COLOR_BALANCE_CONFIG (config);
  LigmaTransferMode        range;
  LigmaTransferMode        old_range;
  gboolean                success = TRUE;

  if (! ligma_operation_settings_config_serialize_base (config, writer, data))
    return FALSE;

  old_range = bc_config->range;

  for (range = LIGMA_TRANSFER_SHADOWS;
       range <= LIGMA_TRANSFER_HIGHLIGHTS;
       range++)
    {
      bc_config->range = range;

      success = (ligma_config_serialize_property_by_name (config,
                                                         "range",
                                                         writer) &&
                 ligma_config_serialize_property_by_name (config,
                                                         "cyan-red",
                                                         writer) &&
                 ligma_config_serialize_property_by_name (config,
                                                         "magenta-green",
                                                         writer) &&
                 ligma_config_serialize_property_by_name (config,
                                                         "yellow-blue",
                                                         writer));

      if (! success)
        break;
    }

  if (success)
    success = ligma_config_serialize_property_by_name (config,
                                                      "preserve-luminosity",
                                                      writer);

  bc_config->range = old_range;

  return success;
}

static gboolean
ligma_color_balance_config_deserialize (LigmaConfig *config,
                                       GScanner   *scanner,
                                       gint        nest_level,
                                       gpointer    data)
{
  LigmaColorBalanceConfig *cb_config = LIGMA_COLOR_BALANCE_CONFIG (config);
  LigmaTransferMode        old_range;
  gboolean                success = TRUE;

  old_range = cb_config->range;

  success = ligma_config_deserialize_properties (config, scanner, nest_level);

  g_object_set (config, "range", old_range, NULL);

  return success;
}

static gboolean
ligma_color_balance_config_equal (LigmaConfig *a,
                                 LigmaConfig *b)
{
  LigmaColorBalanceConfig *config_a = LIGMA_COLOR_BALANCE_CONFIG (a);
  LigmaColorBalanceConfig *config_b = LIGMA_COLOR_BALANCE_CONFIG (b);
  LigmaTransferMode        range;

  if (! ligma_operation_settings_config_equal_base (a, b))
    return FALSE;

  for (range = LIGMA_TRANSFER_SHADOWS;
       range <= LIGMA_TRANSFER_HIGHLIGHTS;
       range++)
    {
      if (config_a->cyan_red[range]      != config_b->cyan_red[range]      ||
          config_a->magenta_green[range] != config_b->magenta_green[range] ||
          config_a->yellow_blue[range]   != config_b->yellow_blue[range])
        return FALSE;
    }

  /* don't compare "range" */

  if (config_a->preserve_luminosity != config_b->preserve_luminosity)
    return FALSE;

  return TRUE;
}

static void
ligma_color_balance_config_reset (LigmaConfig *config)
{
  LigmaColorBalanceConfig *cb_config = LIGMA_COLOR_BALANCE_CONFIG (config);
  LigmaTransferMode        range;

  ligma_operation_settings_config_reset_base (config);

  for (range = LIGMA_TRANSFER_SHADOWS;
       range <= LIGMA_TRANSFER_HIGHLIGHTS;
       range++)
    {
      cb_config->range = range;
      ligma_color_balance_config_reset_range (cb_config);
    }

  ligma_config_reset_property (G_OBJECT (config), "range");
  ligma_config_reset_property (G_OBJECT (config), "preserve-luminosity");
}

static gboolean
ligma_color_balance_config_copy (LigmaConfig  *src,
                                LigmaConfig  *dest,
                                GParamFlags  flags)
{
  LigmaColorBalanceConfig *src_config  = LIGMA_COLOR_BALANCE_CONFIG (src);
  LigmaColorBalanceConfig *dest_config = LIGMA_COLOR_BALANCE_CONFIG (dest);
  LigmaTransferMode        range;

  if (! ligma_operation_settings_config_copy_base (src, dest, flags))
    return FALSE;

  for (range = LIGMA_TRANSFER_SHADOWS;
       range <= LIGMA_TRANSFER_HIGHLIGHTS;
       range++)
    {
      dest_config->cyan_red[range]      = src_config->cyan_red[range];
      dest_config->magenta_green[range] = src_config->magenta_green[range];
      dest_config->yellow_blue[range]   = src_config->yellow_blue[range];
    }

  g_object_notify (G_OBJECT (dest), "cyan-red");
  g_object_notify (G_OBJECT (dest), "magenta-green");
  g_object_notify (G_OBJECT (dest), "yellow-blue");

  dest_config->range               = src_config->range;
  dest_config->preserve_luminosity = src_config->preserve_luminosity;

  g_object_notify (G_OBJECT (dest), "range");
  g_object_notify (G_OBJECT (dest), "preserve-luminosity");

  return TRUE;
}


/*  public functions  */

void
ligma_color_balance_config_reset_range (LigmaColorBalanceConfig *config)
{
  g_return_if_fail (LIGMA_IS_COLOR_BALANCE_CONFIG (config));

  g_object_freeze_notify (G_OBJECT (config));

  ligma_config_reset_property (G_OBJECT (config), "cyan-red");
  ligma_config_reset_property (G_OBJECT (config), "magenta-green");
  ligma_config_reset_property (G_OBJECT (config), "yellow-blue");

  g_object_thaw_notify (G_OBJECT (config));
}
