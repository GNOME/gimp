/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationpointlayermode.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Martin Nordholts <martinn@svn.gnome.org>
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
#include <gegl-plugin.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "../operations-types.h"

#include "gimpoperationpointlayermode.h"


enum
{
  PROP_0,
  PROP_LINEAR,
  PROP_OPACITY
};


static void     gimp_operation_point_layer_mode_set_property (GObject              *object,
                                                              guint                 property_id,
                                                              const GValue         *value,
                                                              GParamSpec           *pspec);
static void     gimp_operation_point_layer_mode_get_property (GObject              *object,
                                                              guint                 property_id,
                                                              GValue               *value,
                                                              GParamSpec           *pspec);

static void     gimp_operation_point_layer_mode_prepare      (GeglOperation        *operation);
static gboolean gimp_operation_point_layer_mode_process      (GeglOperation        *operation,
                                                              GeglOperationContext *context,
                                                              const gchar          *output_prop,
                                                              const GeglRectangle  *result,
                                                              gint                  level);


G_DEFINE_TYPE (GimpOperationPointLayerMode, gimp_operation_point_layer_mode,
               GEGL_TYPE_OPERATION_POINT_COMPOSER3)

#define parent_class gimp_operation_point_layer_mode_parent_class


static void
gimp_operation_point_layer_mode_class_init (GimpOperationPointLayerModeClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = gimp_operation_point_layer_mode_set_property;
  object_class->get_property = gimp_operation_point_layer_mode_get_property;

  operation_class->prepare   = gimp_operation_point_layer_mode_prepare;
  operation_class->process   = gimp_operation_point_layer_mode_process;

  g_object_class_install_property (object_class, PROP_LINEAR,
                                   g_param_spec_boolean ("linear",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity",
                                                        NULL, NULL,
                                                        0.0, 1.0, 1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_operation_point_layer_mode_init (GimpOperationPointLayerMode *self)
{
}

static void
gimp_operation_point_layer_mode_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  GimpOperationPointLayerMode *self = GIMP_OPERATION_POINT_LAYER_MODE (object);

  switch (property_id)
    {
    case PROP_LINEAR:
      self->linear = g_value_get_boolean (value);
      break;

    case PROP_OPACITY:
      self->opacity = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_point_layer_mode_get_property (GObject    *object,
                                              guint       property_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  GimpOperationPointLayerMode *self = GIMP_OPERATION_POINT_LAYER_MODE (object);

  switch (property_id)
    {
    case PROP_LINEAR:
      g_value_set_boolean (value, self->linear);
      break;

    case PROP_OPACITY:
      g_value_set_double (value, self->opacity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_point_layer_mode_prepare (GeglOperation *operation)
{
  GimpOperationPointLayerMode *self = GIMP_OPERATION_POINT_LAYER_MODE (operation);
  const Babl                  *format;

  if (self->linear)
    format = babl_format ("RGBA float");
  else
    format = babl_format ("R'G'B'A float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "aux2",   babl_format ("Y float"));
}

static gboolean
gimp_operation_point_layer_mode_process (GeglOperation        *operation,
                                         GeglOperationContext *context,
                                         const gchar          *output_prop,
                                         const GeglRectangle  *result,
                                         gint                  level)
{
  GimpOperationPointLayerMode *point;

  point = GIMP_OPERATION_POINT_LAYER_MODE (operation);

  if (point->opacity == 0.0 ||
      ! gegl_operation_context_get_object (context, "aux"))
    {
      GObject *input;

      /* get the raw values this does not increase the reference count */
      input = gegl_operation_context_get_object (context, "input");

      if (input)
        {
          gegl_operation_context_set_object (context, "output", input);
          return TRUE;
        }
    }

  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return GEGL_OPERATION_CLASS (parent_class)->process (operation, context,
                                                       output_prop, result,
                                                       level);
}
