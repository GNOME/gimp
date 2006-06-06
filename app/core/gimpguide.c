/* The GIMP -- an image manipulation program
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

#include <string.h> /* strcmp */

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimplimits.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpguide.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_POSITION,
  PROP_ORIENTATION,
  PROP_ID
};


static void   gimp_guide_get_property (GObject      *object,
                                      guint         property_id,
                                      GValue       *value,
                                      GParamSpec   *pspec);
static void   gimp_guide_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec);


G_DEFINE_TYPE (GimpGuide, gimp_guide, GIMP_TYPE_OBJECT)


static void
gimp_guide_class_init (GimpGuideClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gimp_guide_get_property;
  object_class->set_property = gimp_guide_set_property;

  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_POSITION,
                                "position",
                                N_("Offset of the guide."),
                                -1,
                                GIMP_MAX_IMAGE_SIZE, -1,
                                GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_ORIENTATION,
                                 "orientation",
                                 N_("Orientation of the guide."),
                                 GIMP_TYPE_ORIENTATION_TYPE,
                                 GIMP_ORIENTATION_VERTICAL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_ID,
                                "guide-id",
                                N_("Identifying number for the guide."),
                                0,
                                G_MAXINT, 0,
                                GIMP_PARAM_STATIC_STRINGS);
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
    case PROP_POSITION:
      g_value_set_int (value, guide->position);
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, guide->orientation);
      break;
    case PROP_ID:
      g_value_set_int (value, guide->guide_ID);
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
    case PROP_POSITION:
      guide->position = g_value_get_int (value);
      break;
    case PROP_ORIENTATION:
      guide->orientation = g_value_get_enum (value);
      break;
    case PROP_ID:
      guide->guide_ID = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

gint
gimp_guide_get_position (GimpGuide *guide)
{
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), 0);

  return guide->position;
}

void
gimp_guide_set_position (GimpGuide *guide,
                         gint       position)
{
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  guide->position = position;
}

GimpOrientationType
gimp_guide_get_orientation (GimpGuide *guide)
{
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), 0);

  return guide->orientation;
}
