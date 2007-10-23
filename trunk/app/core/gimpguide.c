/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGuide
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpguide.h"
#include "gimpmarshal.h"

enum
{
  REMOVED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ID,
  PROP_ORIENTATION,
  PROP_POSITION
};


static void   gimp_guide_get_property (GObject      *object,
                                       guint         property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);
static void   gimp_guide_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);


G_DEFINE_TYPE (GimpGuide, gimp_guide, G_TYPE_OBJECT)

static guint gimp_guide_signals[LAST_SIGNAL] = { 0 };


static void
gimp_guide_class_init (GimpGuideClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gimp_guide_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpGuideClass, removed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);


  object_class->get_property = gimp_guide_get_property;
  object_class->set_property = gimp_guide_set_property;

  klass->removed             = NULL;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_uint ("id", NULL, NULL,
                                                      0, G_MAXUINT32, 0,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      GIMP_PARAM_READWRITE));

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_ORIENTATION,
                                 "orientation", NULL,
                                 GIMP_TYPE_ORIENTATION_TYPE,
                                 GIMP_ORIENTATION_UNKNOWN,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_POSITION,
                                "position", NULL,
                                -1, GIMP_MAX_IMAGE_SIZE, -1,
                                0);
}

static void
gimp_guide_init (GimpGuide *guide)
{
}

static void
gimp_guide_get_property (GObject      *object,
                         guint         property_id,
                         GValue       *value,
                         GParamSpec   *pspec)
{
  GimpGuide *guide = GIMP_GUIDE (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_uint (value, guide->guide_ID);
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, guide->orientation);
      break;
    case PROP_POSITION:
      g_value_set_int (value, guide->position);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_guide_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpGuide *guide = GIMP_GUIDE (object);

  switch (property_id)
    {
    case PROP_ID:
      guide->guide_ID = g_value_get_uint (value);
      break;
    case PROP_ORIENTATION:
      guide->orientation = g_value_get_enum (value);
      break;
    case PROP_POSITION:
      guide->position = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GimpGuide *
gimp_guide_new (GimpOrientationType  orientation,
                guint32              guide_ID)
{
  return g_object_new (GIMP_TYPE_GUIDE,
                       "id",          guide_ID,
                       "orientation", orientation,
                       NULL);
}

guint32
gimp_guide_get_ID (GimpGuide *guide)
{
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), 0);

  return guide->guide_ID;
}

GimpOrientationType
gimp_guide_get_orientation (GimpGuide *guide)
{
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), GIMP_ORIENTATION_UNKNOWN);

  return guide->orientation;
}

void
gimp_guide_set_orientation (GimpGuide           *guide,
                            GimpOrientationType  orientation)
{
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  guide->orientation = orientation;

  g_object_notify (G_OBJECT (guide), "orientation");
}

gint
gimp_guide_get_position (GimpGuide *guide)
{
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), -1);

  return guide->position;
}

void
gimp_guide_set_position (GimpGuide *guide,
                         gint       position)
{
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  guide->position = position;

  g_object_notify (G_OBJECT (guide), "position");
}

void
gimp_guide_removed (GimpGuide *guide)
{
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  g_signal_emit (guide, gimp_guide_signals[REMOVED], 0);
}
