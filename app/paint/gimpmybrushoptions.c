/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "paint-types.h"

#include "core/ligma.h"
#include "core/ligmadrawable.h"
#include "core/ligmamybrush.h"
#include "core/ligmapaintinfo.h"

#include "ligmamybrushoptions.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_RADIUS,
  PROP_OPAQUE,
  PROP_HARDNESS,
  PROP_ERASER,
  PROP_NO_ERASING
};


static void   ligma_mybrush_options_config_iface_init (LigmaConfigInterface *config_iface);

static void   ligma_mybrush_options_set_property     (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void   ligma_mybrush_options_get_property     (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static void    ligma_mybrush_options_mybrush_changed (LigmaContext  *context,
                                                     LigmaMybrush  *brush);

static void    ligma_mybrush_options_reset           (LigmaConfig   *config);


G_DEFINE_TYPE_WITH_CODE (LigmaMybrushOptions, ligma_mybrush_options,
                         LIGMA_TYPE_PAINT_OPTIONS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_mybrush_options_config_iface_init))

static LigmaConfigInterface *parent_config_iface = NULL;


static void
ligma_mybrush_options_class_init (LigmaMybrushOptionsClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  LigmaContextClass *context_class = LIGMA_CONTEXT_CLASS (klass);

  object_class->set_property     = ligma_mybrush_options_set_property;
  object_class->get_property     = ligma_mybrush_options_get_property;

  context_class->mybrush_changed = ligma_mybrush_options_mybrush_changed;

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_RADIUS,
                           "radius",
                           _("Radius"),
                           NULL,
                           -2.0, 6.0, 1.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OPAQUE,
                           "opaque",
                           _("Base Opacity"),
                           NULL,
                           0.0, 2.0, 1.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_HARDNESS,
                           "hardness",
                           _("Hardness"),
                           NULL,
                           0.0, 1.0, 1.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_ERASER,
                            "eraser",
                            _("Erase with this brush"),
                            NULL,
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_NO_ERASING,
                            "no-erasing",
                            _("No erasing effect"),
                            _("Never decrease alpha of existing pixels"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_mybrush_options_config_iface_init (LigmaConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = ligma_mybrush_options_reset;
}

static void
ligma_mybrush_options_init (LigmaMybrushOptions *options)
{
}

static void
ligma_mybrush_options_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaMybrushOptions *options = LIGMA_MYBRUSH_OPTIONS (object);

  switch (property_id)
    {
    case PROP_RADIUS:
      options->radius = g_value_get_double (value);
      break;
    case PROP_HARDNESS:
      options->hardness = g_value_get_double (value);
      break;
    case PROP_OPAQUE:
      options->opaque = g_value_get_double (value);
      break;
    case PROP_ERASER:
      options->eraser = g_value_get_boolean (value);
      break;
    case PROP_NO_ERASING:
      options->no_erasing = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_mybrush_options_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaMybrushOptions *options = LIGMA_MYBRUSH_OPTIONS (object);

  switch (property_id)
    {
    case PROP_RADIUS:
      g_value_set_double (value, options->radius);
      break;
    case PROP_OPAQUE:
      g_value_set_double (value, options->opaque);
      break;
    case PROP_HARDNESS:
      g_value_set_double (value, options->hardness);
      break;
    case PROP_ERASER:
      g_value_set_boolean (value, options->eraser);
      break;
    case PROP_NO_ERASING:
      g_value_set_boolean (value, options->no_erasing);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_mybrush_options_mybrush_changed (LigmaContext *context,
                                      LigmaMybrush *brush)
{
  if (brush)
    g_object_set (context,
                  "radius",   ligma_mybrush_get_radius (brush),
                  "opaque",   ligma_mybrush_get_opaque (brush),
                  "hardness", ligma_mybrush_get_hardness (brush),
                  "eraser",   ligma_mybrush_get_is_eraser (brush),
                  NULL);
}

static void
ligma_mybrush_options_reset (LigmaConfig *config)
{
  LigmaContext *context = LIGMA_CONTEXT (config);
  LigmaMybrush *brush   = ligma_context_get_mybrush (context);

  parent_config_iface->reset (config);

  ligma_mybrush_options_mybrush_changed (context, brush);
}
