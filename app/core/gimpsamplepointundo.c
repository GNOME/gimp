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

#include "core-types.h"

#include "gimpimage.h"
#include "gimpimage-sample-points.h"
#include "gimpsamplepoint.h"
#include "gimpsamplepointundo.h"


enum
{
  PROP_0,
  PROP_SAMPLE_POINT
};


static void   gimp_sample_point_undo_constructed  (GObject             *object);
static void   gimp_sample_point_undo_set_property (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void   gimp_sample_point_undo_get_property (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static void   gimp_sample_point_undo_pop          (GimpUndo            *undo,
                                                   GimpUndoMode         undo_mode,
                                                   GimpUndoAccumulator *accum);
static void   gimp_sample_point_undo_free         (GimpUndo            *undo,
                                                   GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpSamplePointUndo, gimp_sample_point_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_sample_point_undo_parent_class


static void
gimp_sample_point_undo_class_init (GimpSamplePointUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed  = gimp_sample_point_undo_constructed;
  object_class->set_property = gimp_sample_point_undo_set_property;
  object_class->get_property = gimp_sample_point_undo_get_property;

  undo_class->pop            = gimp_sample_point_undo_pop;
  undo_class->free           = gimp_sample_point_undo_free;

  g_object_class_install_property (object_class, PROP_SAMPLE_POINT,
                                   g_param_spec_boxed ("sample-point", NULL, NULL,
                                                       GIMP_TYPE_SAMPLE_POINT,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_sample_point_undo_init (GimpSamplePointUndo *undo)
{
}

static void
gimp_sample_point_undo_constructed (GObject *object)
{
  GimpSamplePointUndo *sample_point_undo = GIMP_SAMPLE_POINT_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_return_if_fail (sample_point_undo->sample_point != NULL);

  gimp_sample_point_get_position (sample_point_undo->sample_point,
                                  &sample_point_undo->x,
                                  &sample_point_undo->y);
}

static void
gimp_sample_point_undo_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpSamplePointUndo *sample_point_undo = GIMP_SAMPLE_POINT_UNDO (object);

  switch (property_id)
    {
    case PROP_SAMPLE_POINT:
      sample_point_undo->sample_point = g_value_dup_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_sample_point_undo_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpSamplePointUndo *sample_point_undo = GIMP_SAMPLE_POINT_UNDO (object);

  switch (property_id)
    {
    case PROP_SAMPLE_POINT:
      g_value_set_boxed (value, sample_point_undo->sample_point);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_sample_point_undo_pop (GimpUndo              *undo,
                            GimpUndoMode           undo_mode,
                            GimpUndoAccumulator   *accum)
{
  GimpSamplePointUndo *sample_point_undo = GIMP_SAMPLE_POINT_UNDO (undo);
  gint                 x;
  gint                 y;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  gimp_sample_point_get_position (sample_point_undo->sample_point, &x, &y);

  if (x == -1)
    {
      gimp_image_add_sample_point (undo->image,
                                   sample_point_undo->sample_point,
                                   sample_point_undo->x,
                                   sample_point_undo->y);
    }
  else if (sample_point_undo->x == -1)
    {
      gimp_image_remove_sample_point (undo->image,
                                      sample_point_undo->sample_point, FALSE);
    }
  else
    {
      gimp_sample_point_set_position (sample_point_undo->sample_point,
                                      sample_point_undo->x,
                                      sample_point_undo->y);

      gimp_image_sample_point_moved (undo->image,
                                     sample_point_undo->sample_point);
    }

  sample_point_undo->x = x;
  sample_point_undo->y = y;
}

static void
gimp_sample_point_undo_free (GimpUndo     *undo,
                             GimpUndoMode  undo_mode)
{
  GimpSamplePointUndo *sample_point_undo = GIMP_SAMPLE_POINT_UNDO (undo);

  g_clear_pointer (&sample_point_undo->sample_point, gimp_sample_point_unref);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
