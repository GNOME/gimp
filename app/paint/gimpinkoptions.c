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

#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimppaintinfo.h"

#include "gimpinkoptions.h"
#include "gimpink-blob.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_SIZE,
  PROP_TILT_ANGLE,
  PROP_SIZE_SENSITIVITY,
  PROP_VEL_SENSITIVITY,
  PROP_TILT_SENSITIVITY,
  PROP_BLOB_TYPE,
  PROP_BLOB_ASPECT,
  PROP_BLOB_ANGLE
};


static void   gimp_ink_options_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   gimp_ink_options_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);


G_DEFINE_TYPE (GimpInkOptions, gimp_ink_options, GIMP_TYPE_PAINT_OPTIONS)


static void
gimp_ink_options_class_init (GimpInkOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_ink_options_set_property;
  object_class->get_property = gimp_ink_options_get_property;

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SIZE,
                           "size",
                           _("Size"),
                           _("Ink Blob Size"),
                           0.0, 200.0, 16.0,
                           GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_TILT_ANGLE,
                           "tilt-angle",
                           _("Angle"),
                           NULL,
                           -90.0, 90.0, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SIZE_SENSITIVITY,
                           "size-sensitivity",
                           _("Size"),
                           NULL,
                           0.0, 1.0, 1.0,
                           GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_VEL_SENSITIVITY,
                           "vel-sensitivity",
                           _("Speed"),
                           NULL,
                           0.0, 1.0, 0.8,
                           GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_TILT_SENSITIVITY,
                           "tilt-sensitivity",
                           _("Tilt"),
                           NULL,
                           0.0, 1.0, 0.4,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_BLOB_TYPE,
                         "blob-type",
                         _("Shape"),
                         NULL,
                         GIMP_TYPE_INK_BLOB_TYPE,
                         GIMP_INK_BLOB_TYPE_CIRCLE,
                         GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_BLOB_ASPECT,
                           "blob-aspect",
                           _("Aspect ratio"),
                           _("Ink Blob Aspect Ratio"),
                           1.0, 10.0, 1.0,
                           GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_BLOB_ANGLE,
                           "blob-angle",
                           _("Angle"),
                           _("Ink Blob Angle"),
                           -G_PI, G_PI, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_ink_options_init (GimpInkOptions *options)
{
}

static void
gimp_ink_options_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpInkOptions *options = GIMP_INK_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SIZE:
      options->size = g_value_get_double (value);
      break;
    case PROP_TILT_ANGLE:
      options->tilt_angle = g_value_get_double (value);
      break;
    case PROP_SIZE_SENSITIVITY:
      options->size_sensitivity = g_value_get_double (value);
      break;
    case PROP_VEL_SENSITIVITY:
      options->vel_sensitivity = g_value_get_double (value);
      break;
    case PROP_TILT_SENSITIVITY:
      options->tilt_sensitivity = g_value_get_double (value);
      break;
    case PROP_BLOB_TYPE:
      options->blob_type = g_value_get_enum (value);
      break;
    case PROP_BLOB_ASPECT:
      options->blob_aspect = g_value_get_double (value);
      break;
    case PROP_BLOB_ANGLE:
      options->blob_angle = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_ink_options_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpInkOptions *options = GIMP_INK_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SIZE:
      g_value_set_double (value, options->size);
      break;
    case PROP_TILT_ANGLE:
      g_value_set_double (value, options->tilt_angle);
      break;
    case PROP_SIZE_SENSITIVITY:
      g_value_set_double (value, options->size_sensitivity);
      break;
    case PROP_VEL_SENSITIVITY:
      g_value_set_double (value, options->vel_sensitivity);
      break;
    case PROP_TILT_SENSITIVITY:
      g_value_set_double (value, options->tilt_sensitivity);
      break;
    case PROP_BLOB_TYPE:
      g_value_set_enum (value, options->blob_type);
      break;
    case PROP_BLOB_ASPECT:
      g_value_set_double (value, options->blob_aspect);
      break;
    case PROP_BLOB_ANGLE:
      g_value_set_double (value, options->blob_angle);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
