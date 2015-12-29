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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

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
  PROP_RADIUS,
  PROP_OPAQUE,
  PROP_HARDNESS,
  PROP_ERASER
};


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

static void    gimp_mybrush_options_reset           (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpMybrushOptions, gimp_mybrush_options,
               GIMP_TYPE_PAINT_OPTIONS)


static void
gimp_mybrush_options_class_init (GimpMybrushOptionsClass *klass)
{
  GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
  GimpContextClass     *context_class = GIMP_CONTEXT_CLASS (klass);
  GimpToolOptionsClass *options_class = GIMP_TOOL_OPTIONS_CLASS (klass);

  object_class->set_property     = gimp_mybrush_options_set_property;
  object_class->get_property     = gimp_mybrush_options_get_property;

  context_class->mybrush_changed = gimp_mybrush_options_mybrush_changed;

  options_class->reset           = gimp_mybrush_options_reset;

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_RADIUS,
                                   "radius", _("Radius"),
                                   -2.0, 6.0, 1.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_OPAQUE,
                                   "opaque", _("Base Opacity"),
                                   0.0, 2.0, 1.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_HARDNESS,
                                   "hardness", NULL,
                                   0.0, 1.0, 1.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ERASER,
                                   "eraser", NULL,
                                   FALSE,
                                   GIMP_PARAM_STATIC_STRINGS);
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
                  "radius",   gimp_mybrush_get_radius (brush),
                  "opaque",   gimp_mybrush_get_opaque (brush),
                  "hardness", gimp_mybrush_get_hardness (brush),
                  "eraser",   gimp_mybrush_get_is_eraser (brush),
                  NULL);
}

static void
gimp_mybrush_options_reset (GimpToolOptions *tool_options)
{
  GimpContext *context = GIMP_CONTEXT (tool_options);
  GimpMybrush *brush   = gimp_context_get_mybrush (context);

  gimp_mybrush_options_mybrush_changed (context, brush);
}