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
#include "core/ligmapaintinfo.h"

#include "ligmainkoptions.h"
#include "ligmaink-blob.h"

#include "ligma-intl.h"


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


static void   ligma_ink_options_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   ligma_ink_options_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaInkOptions, ligma_ink_options, LIGMA_TYPE_PAINT_OPTIONS)


static void
ligma_ink_options_class_init (LigmaInkOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_ink_options_set_property;
  object_class->get_property = ligma_ink_options_get_property;

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_SIZE,
                           "size",
                           _("Size"),
                           _("Ink Blob Size"),
                           0.0, 200.0, 16.0,
                           LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_TILT_ANGLE,
                           "tilt-angle",
                           _("Angle"),
                           NULL,
                           -90.0, 90.0, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_SIZE_SENSITIVITY,
                           "size-sensitivity",
                           _("Size"),
                           NULL,
                           0.0, 1.0, 1.0,
                           LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_VEL_SENSITIVITY,
                           "vel-sensitivity",
                           _("Speed"),
                           NULL,
                           0.0, 1.0, 0.8,
                           LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_TILT_SENSITIVITY,
                           "tilt-sensitivity",
                           _("Tilt"),
                           NULL,
                           0.0, 1.0, 0.4,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_BLOB_TYPE,
                         "blob-type",
                         _("Shape"),
                         NULL,
                         LIGMA_TYPE_INK_BLOB_TYPE,
                         LIGMA_INK_BLOB_TYPE_CIRCLE,
                         LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_BLOB_ASPECT,
                           "blob-aspect",
                           _("Aspect ratio"),
                           _("Ink Blob Aspect Ratio"),
                           1.0, 10.0, 1.0,
                           LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_BLOB_ANGLE,
                           "blob-angle",
                           _("Angle"),
                           _("Ink Blob Angle"),
                           -G_PI, G_PI, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_ink_options_init (LigmaInkOptions *options)
{
}

static void
ligma_ink_options_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaInkOptions *options = LIGMA_INK_OPTIONS (object);

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
ligma_ink_options_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaInkOptions *options = LIGMA_INK_OPTIONS (object);

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
