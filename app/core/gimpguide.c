/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaGuide
 * Copyright (C) 2003  Henrik Brix Andersen <brix@ligma.org>
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

#include <gio/gio.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligmaguide.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_POSITION,
  PROP_STYLE
};


struct _LigmaGuidePrivate
{
  LigmaOrientationType  orientation;
  gint                 position;

  LigmaGuideStyle       style;
};


static void   ligma_guide_get_property (GObject      *object,
                                       guint         property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);
static void   ligma_guide_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaGuide, ligma_guide, LIGMA_TYPE_AUX_ITEM)


static void
ligma_guide_class_init (LigmaGuideClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ligma_guide_get_property;
  object_class->set_property = ligma_guide_set_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_ORIENTATION,
                         "orientation",
                         NULL, NULL,
                         LIGMA_TYPE_ORIENTATION_TYPE,
                         LIGMA_ORIENTATION_UNKNOWN,
                         0);

  LIGMA_CONFIG_PROP_INT (object_class, PROP_POSITION,
                        "position",
                        NULL, NULL,
                        LIGMA_GUIDE_POSITION_UNDEFINED,
                        LIGMA_MAX_IMAGE_SIZE,
                        LIGMA_GUIDE_POSITION_UNDEFINED,
                        0);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_STYLE,
                         "style",
                         NULL, NULL,
                         LIGMA_TYPE_GUIDE_STYLE,
                         LIGMA_GUIDE_STYLE_NONE,
                         0);
}

static void
ligma_guide_init (LigmaGuide *guide)
{
  guide->priv = ligma_guide_get_instance_private (guide);
}

static void
ligma_guide_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  LigmaGuide *guide = LIGMA_GUIDE (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, guide->priv->orientation);
      break;
    case PROP_POSITION:
      g_value_set_int (value, guide->priv->position);
      break;
    case PROP_STYLE:
      g_value_set_enum (value, guide->priv->style);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_guide_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  LigmaGuide *guide = LIGMA_GUIDE (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      guide->priv->orientation = g_value_get_enum (value);
      break;
    case PROP_POSITION:
      guide->priv->position = g_value_get_int (value);
      break;
    case PROP_STYLE:
      guide->priv->style = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

LigmaGuide *
ligma_guide_new (LigmaOrientationType  orientation,
                guint32              guide_ID)
{
  return g_object_new (LIGMA_TYPE_GUIDE,
                       "id",          guide_ID,
                       "orientation", orientation,
                       "style",       LIGMA_GUIDE_STYLE_NORMAL,
                       NULL);
}

/**
 * ligma_guide_custom_new:
 * @orientation:       the #LigmaOrientationType
 * @guide_ID:          the unique guide ID
 * @guide_style:       the #LigmaGuideStyle
 *
 * This function returns a new guide and will flag it as "custom".
 * Custom guides are used for purpose "other" than the basic guides
 * a user can create oneself, for instance as symmetry guides, to
 * drive GEGL ops, etc.
 * They are not saved in the XCF file. If an op, a symmetry or a plugin
 * wishes to save its state, it has to do it internally.
 * Moreover they don't follow guide snapping settings and never snap.
 *
 * Returns: the custom #LigmaGuide.
 **/
LigmaGuide *
ligma_guide_custom_new (LigmaOrientationType  orientation,
                       guint32              guide_ID,
                       LigmaGuideStyle       guide_style)
{
  return g_object_new (LIGMA_TYPE_GUIDE,
                       "id",          guide_ID,
                       "orientation", orientation,
                       "style",       guide_style,
                       NULL);
}

LigmaOrientationType
ligma_guide_get_orientation (LigmaGuide *guide)
{
  g_return_val_if_fail (LIGMA_IS_GUIDE (guide), LIGMA_ORIENTATION_UNKNOWN);

  return guide->priv->orientation;
}

void
ligma_guide_set_orientation (LigmaGuide           *guide,
                            LigmaOrientationType  orientation)
{
  g_return_if_fail (LIGMA_IS_GUIDE (guide));

  guide->priv->orientation = orientation;

  g_object_notify (G_OBJECT (guide), "orientation");
}

gint
ligma_guide_get_position (LigmaGuide *guide)
{
  g_return_val_if_fail (LIGMA_IS_GUIDE (guide), LIGMA_GUIDE_POSITION_UNDEFINED);

  return guide->priv->position;
}

void
ligma_guide_set_position (LigmaGuide *guide,
                         gint       position)
{
  g_return_if_fail (LIGMA_IS_GUIDE (guide));

  guide->priv->position = position;

  g_object_notify (G_OBJECT (guide), "position");
}

LigmaGuideStyle
ligma_guide_get_style (LigmaGuide *guide)
{
  g_return_val_if_fail (LIGMA_IS_GUIDE (guide), LIGMA_GUIDE_STYLE_NONE);

  return guide->priv->style;
}

gboolean
ligma_guide_is_custom (LigmaGuide *guide)
{
  g_return_val_if_fail (LIGMA_IS_GUIDE (guide), FALSE);

  return guide->priv->style != LIGMA_GUIDE_STYLE_NORMAL;
}
