/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationposterize.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"

#include "operations-types.h"

#include "gimpoperationposterize.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_LEVELS
};


static void     gimp_operation_posterize_get_property (GObject             *object,
                                                       guint                property_id,
                                                       GValue              *value,
                                                       GParamSpec          *pspec);
static void     gimp_operation_posterize_set_property (GObject             *object,
                                                       guint                property_id,
                                                       const GValue        *value,
                                                       GParamSpec          *pspec);

static void gimp_operation_posterize_prepare (GeglOperation *operation);

static gboolean gimp_operation_posterize_process      (GeglOperation       *operation,
                                                       void                *in_buf,
                                                       void                *out_buf,
                                                       glong                samples,
                                                       const GeglRectangle *roi,
                                                       gint                 level);


G_DEFINE_TYPE (GimpOperationPosterize, gimp_operation_posterize,
               GIMP_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_posterize_parent_class


static void
gimp_operation_posterize_class_init (GimpOperationPosterizeClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = gimp_operation_posterize_set_property;
  object_class->get_property = gimp_operation_posterize_get_property;
  operation_class->prepare   = gimp_operation_posterize_prepare;
  point_class->process       = gimp_operation_posterize_process;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:posterize",
                                 "categories",  "color",
                                 "description", _("Reduce to a limited set of colors"),
                                 NULL);

  GIMP_CONFIG_PROP_INT (object_class, PROP_LEVELS,
                        "levels",
                        _("Posterize levels"),
                        NULL,
                        2, 256, 3,
                        GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_operation_posterize_init (GimpOperationPosterize *self)
{
}

static void
gimp_operation_posterize_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpOperationPosterize *posterize = GIMP_OPERATION_POSTERIZE (object);

  switch (property_id)
    {
    case PROP_LEVELS:
      g_value_set_int (value, posterize->levels);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_posterize_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpOperationPosterize *posterize = GIMP_OPERATION_POSTERIZE (object);

  switch (property_id)
    {
    case PROP_LEVELS:
      posterize->levels = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_posterize_prepare (GeglOperation *operation)
{
  const Babl *format = babl_format_with_space ("R~G~B~A float",
                                               gegl_operation_get_format (operation, "input"));

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}


static gboolean
gimp_operation_posterize_process (GeglOperation       *operation,
                                  void                *in_buf,
                                  void                *out_buf,
                                  glong                samples,
                                  const GeglRectangle *roi,
                                  gint                 level)
{
  GimpOperationPosterize *posterize = GIMP_OPERATION_POSTERIZE (operation);
  gfloat                 *src       = in_buf;
  gfloat                 *dest      = out_buf;
  gfloat                  levels;

  levels = posterize->levels - 1.0;

  while (samples--)
    {
      dest[RED]   = RINT (src[RED]   * levels) / levels;
      dest[GREEN] = RINT (src[GREEN] * levels) / levels;
      dest[BLUE]  = RINT (src[BLUE]  * levels) / levels;
      dest[ALPHA] = RINT (src[ALPHA] * levels) / levels;

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
