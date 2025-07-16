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

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpmybrush.h"
#include "core/gimppaintinfo.h"

#include "gimpmybrushoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_VIEW_ZOOM,
  PROP_VIEW_ROTATION,
  PROP_RADIUS,
  PROP_OPAQUE,
  PROP_HARDNESS,
  PROP_GAIN,
  PROP_PIGMENT,
  PROP_POSTERIZE,
  PROP_POSTERIZE_NUM,
  PROP_ERASER,
  PROP_NO_ERASING
};


static void   gimp_mybrush_options_config_iface_init (GimpConfigInterface *config_iface);

static void   gimp_mybrush_options_set_property     (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void   gimp_mybrush_options_get_property     (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static void    gimp_mybrush_options_mybrush_changed (GimpContext  *context,
                                                     GimpMybrush  *brush);

static void    gimp_mybrush_options_reset           (GimpConfig   *config);


G_DEFINE_TYPE_WITH_CODE (GimpMybrushOptions, gimp_mybrush_options,
                         GIMP_TYPE_PAINT_OPTIONS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_mybrush_options_config_iface_init))

static GimpConfigInterface *parent_config_iface = NULL;


static void
gimp_mybrush_options_class_init (GimpMybrushOptionsClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GimpContextClass *context_class = GIMP_CONTEXT_CLASS (klass);

  object_class->set_property     = gimp_mybrush_options_set_property;
  object_class->get_property     = gimp_mybrush_options_get_property;

  context_class->mybrush_changed = gimp_mybrush_options_mybrush_changed;

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_VIEW_ZOOM,
                           "viewzoom",
                           _("View Zoom"),
                           NULL,
                           0.0001, G_MAXFLOAT, 1.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_VIEW_ROTATION,
                           "viewrotation",
                           _("View Rotation"),
                           NULL,
                           -360.0, 360.0, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_RADIUS,
                           "radius",
                           _("Radius"),
                           NULL,
                           -2.0, 6.0, 1.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_OPAQUE,
                           "opaque",
                           _("Base Opacity"),
                           NULL,
                           0.0, 2.0, 1.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_HARDNESS,
                           "hardness",
                           _("Hardness"),
                           NULL,
                           0.0, 1.0, 1.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_GAIN,
                           "gain",
                           _("Gain"),
                           _("Adjust strength of input pressure"),
                           -1.8, 1.8, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_PIGMENT,
                           "pigment",
                           NULL, NULL,
                           0.0, 1.0, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_POSTERIZE,
                           "posterize",
                           NULL, NULL,
                           0.0, 1.0, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_POSTERIZE_NUM,
                           "posterizenum",
                           NULL, NULL,
                           0.0, 1.28, 1.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ERASER,
                            "eraser",
                            _("Erase with this brush"),
                            NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_NO_ERASING,
                            "no-erasing",
                            _("No erasing effect"),
                            _("Never decrease alpha of existing pixels"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_mybrush_options_config_iface_init (GimpConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = gimp_mybrush_options_reset;
}

static void
gimp_mybrush_options_init (GimpMybrushOptions *options)
{
}

static void
gimp_mybrush_options_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpMybrushOptions *options = GIMP_MYBRUSH_OPTIONS (object);

  switch (property_id)
    {
    case PROP_VIEW_ZOOM:
      options->view_zoom = g_value_get_double (value) > 0.0f ?
                           g_value_get_double (value) : 1.0f;
      break;
    case PROP_VIEW_ROTATION:
      options->view_rotation = g_value_get_double (value);
      break;
    case PROP_RADIUS:
      options->radius = g_value_get_double (value);
      break;
    case PROP_OPAQUE:
      options->opaque = g_value_get_double (value);
      break;
    case PROP_HARDNESS:
      options->hardness = g_value_get_double (value);
      break;
    case PROP_GAIN:
      options->gain = g_value_get_double (value);
      break;
    case PROP_PIGMENT:
      options->pigment = g_value_get_double (value);
      break;
    case PROP_POSTERIZE:
      options->posterize = g_value_get_double (value);
      break;
    case PROP_POSTERIZE_NUM:
      options->posterize_num = g_value_get_double (value);
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
gimp_mybrush_options_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpMybrushOptions *options = GIMP_MYBRUSH_OPTIONS (object);

  switch (property_id)
    {
    case PROP_VIEW_ZOOM:
      g_value_set_double (value, options->view_zoom);
      break;
    case PROP_VIEW_ROTATION:
      g_value_set_double (value, options->view_rotation);
      break;
    case PROP_RADIUS:
      g_value_set_double (value, options->radius);
      break;
    case PROP_OPAQUE:
      g_value_set_double (value, options->opaque);
      break;
    case PROP_HARDNESS:
      g_value_set_double (value, options->hardness);
      break;
    case PROP_GAIN:
      g_value_set_double (value, options->gain);
      break;
    case PROP_PIGMENT:
      g_value_set_double (value, options->pigment);
      break;
    case PROP_POSTERIZE:
      g_value_set_double (value, options->posterize);
      break;
    case PROP_POSTERIZE_NUM:
      g_value_set_double (value, options->posterize_num);
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
gimp_mybrush_options_mybrush_changed (GimpContext *context,
                                      GimpMybrush *brush)
{
  if (brush)
    g_object_set (context,
                  "viewzoom",     1.0f,
                  "viewrotation", 0.0f,
                  "radius",       gimp_mybrush_get_radius (brush),
                  "opaque",       gimp_mybrush_get_opaque (brush),
                  "hardness",     gimp_mybrush_get_hardness (brush),
                  "gain",         gimp_mybrush_get_gain (brush),
                  "pigment",      gimp_mybrush_get_pigment (brush),
                  "posterize",    gimp_mybrush_get_posterize (brush),
                  "posterizenum", gimp_mybrush_get_posterize_num (brush),
                  "eraser",       gimp_mybrush_get_is_eraser (brush),
                  NULL);
}

static void
gimp_mybrush_options_reset (GimpConfig *config)
{
  GimpContext *context = GIMP_CONTEXT (config);
  GimpMybrush *brush   = gimp_context_get_mybrush (context);

  parent_config_iface->reset (config);

  gimp_mybrush_options_mybrush_changed (context, brush);
}
