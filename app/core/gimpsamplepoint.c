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

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpmarshal.h"
#include "gimpsamplepoint.h"


enum
{
  REMOVED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ID,
  PROP_POSITION_X,
  PROP_POSITION_Y
};


struct _GimpSamplePointPrivate
{
  guint32  sample_point_ID;
  gint     position_x;
  gint     position_y;
};


static void   gimp_sample_point_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);
static void   gimp_sample_point_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (GimpSamplePoint, gimp_sample_point, G_TYPE_OBJECT)

static guint gimp_sample_point_signals[LAST_SIGNAL] = { 0 };


static void
gimp_sample_point_class_init (GimpSamplePointClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gimp_sample_point_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpSamplePointClass, removed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->get_property = gimp_sample_point_get_property;
  object_class->set_property = gimp_sample_point_set_property;

  klass->removed             = NULL;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_uint ("id", NULL, NULL,
                                                      0, G_MAXUINT32, 0,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      GIMP_PARAM_READWRITE));

  GIMP_CONFIG_PROP_INT (object_class, PROP_POSITION_X,
                        "position-x",
                        NULL, NULL,
                        GIMP_SAMPLE_POINT_POSITION_UNDEFINED,
                        GIMP_MAX_IMAGE_SIZE,
                        GIMP_SAMPLE_POINT_POSITION_UNDEFINED,
                        0);

  GIMP_CONFIG_PROP_INT (object_class, PROP_POSITION_Y,
                        "position-y",
                        NULL, NULL,
                        GIMP_SAMPLE_POINT_POSITION_UNDEFINED,
                        GIMP_MAX_IMAGE_SIZE,
                        GIMP_SAMPLE_POINT_POSITION_UNDEFINED,
                        0);

  g_type_class_add_private (klass, sizeof (GimpSamplePointPrivate));
}

static void
gimp_sample_point_init (GimpSamplePoint *sample_point)
{
  sample_point->priv = G_TYPE_INSTANCE_GET_PRIVATE (sample_point,
                                                    GIMP_TYPE_SAMPLE_POINT,
                                                    GimpSamplePointPrivate);
}

static void
gimp_sample_point_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpSamplePoint *sample_point = GIMP_SAMPLE_POINT (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_uint (value, sample_point->priv->sample_point_ID);
      break;
    case PROP_POSITION_X:
      g_value_set_int (value, sample_point->priv->position_x);
      break;
    case PROP_POSITION_Y:
      g_value_set_int (value, sample_point->priv->position_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_sample_point_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpSamplePoint *sample_point = GIMP_SAMPLE_POINT (object);

  switch (property_id)
    {
    case PROP_ID:
      sample_point->priv->sample_point_ID = g_value_get_uint (value);
      break;
    case PROP_POSITION_X:
      sample_point->priv->position_x = g_value_get_int (value);
      break;
    case PROP_POSITION_Y:
      sample_point->priv->position_y = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GimpSamplePoint *
gimp_sample_point_new (guint32 sample_point_ID)
{
  return g_object_new (GIMP_TYPE_SAMPLE_POINT,
                       "id", sample_point_ID,
                       NULL);
}

guint32
gimp_sample_point_get_ID (GimpSamplePoint *sample_point)
{
  g_return_val_if_fail (GIMP_IS_SAMPLE_POINT (sample_point), 0);

  return sample_point->priv->sample_point_ID;
}

void
gimp_sample_point_removed (GimpSamplePoint *sample_point)
{
  g_return_if_fail (GIMP_IS_SAMPLE_POINT (sample_point));

  g_signal_emit (sample_point, gimp_sample_point_signals[REMOVED], 0);
}

void
gimp_sample_point_get_position (GimpSamplePoint *sample_point,
                                gint            *position_x,
                                gint            *position_y)
{
  g_return_if_fail (GIMP_IS_SAMPLE_POINT (sample_point));
  g_return_if_fail (position_x != NULL);
  g_return_if_fail (position_y != NULL);

  *position_x = sample_point->priv->position_x;
  *position_y = sample_point->priv->position_y;
}

void
gimp_sample_point_set_position (GimpSamplePoint *sample_point,
                                gint             position_x,
                                gint             position_y)
{
  g_return_if_fail (GIMP_IS_SAMPLE_POINT (sample_point));

  sample_point->priv->position_x = position_x;
  sample_point->priv->position_y = position_y;

  g_object_freeze_notify (G_OBJECT (sample_point));

  g_object_notify (G_OBJECT (sample_point), "position-x");
  g_object_notify (G_OBJECT (sample_point), "position-y");

  g_object_thaw_notify (G_OBJECT (sample_point));
}
