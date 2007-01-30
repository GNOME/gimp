/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


static GObject * gimp_sample_point_undo_constructor  (GType                  type,
                                                      guint                  n_params,
                                                      GObjectConstructParam *params);
static void      gimp_sample_point_undo_set_property (GObject               *object,
                                                      guint                  property_id,
                                                      const GValue          *value,
                                                      GParamSpec            *pspec);
static void      gimp_sample_point_undo_get_property (GObject               *object,
                                                      guint                  property_id,
                                                      GValue                *value,
                                                      GParamSpec            *pspec);

static void      gimp_sample_point_undo_pop          (GimpUndo              *undo,
                                                      GimpUndoMode           undo_mode,
                                                      GimpUndoAccumulator   *accum);
static void      gimp_sample_point_undo_free         (GimpUndo              *undo,
                                                      GimpUndoMode           undo_mode);


G_DEFINE_TYPE (GimpSamplePointUndo, gimp_sample_point_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_sample_point_undo_parent_class


static void
gimp_sample_point_undo_class_init (GimpSamplePointUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor  = gimp_sample_point_undo_constructor;
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

static GObject *
gimp_sample_point_undo_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject             *object;
  GimpSamplePointUndo *sample_point_undo;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  sample_point_undo = GIMP_SAMPLE_POINT_UNDO (object);

  g_assert (sample_point_undo->sample_point != NULL);

  sample_point_undo->x = sample_point_undo->sample_point->x;
  sample_point_undo->y = sample_point_undo->sample_point->y;

  return object;
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

  x = sample_point_undo->sample_point->x;
  y = sample_point_undo->sample_point->y;

  /*  add and move sample points manually (nor using the
   *  gimp_image_sample_point API), because we might be in the middle
   *  of an image resizing undo group and the sample point's position
   *  might be temporarily out of image.
   */

  if (x == -1)
    {
      undo->image->sample_points =
        g_list_append (undo->image->sample_points,
                       sample_point_undo->sample_point);

      sample_point_undo->sample_point->x = sample_point_undo->x;
      sample_point_undo->sample_point->y = sample_point_undo->y;
      gimp_sample_point_ref (sample_point_undo->sample_point);

      gimp_image_sample_point_added (undo->image,
                                     sample_point_undo->sample_point);
      gimp_image_update_sample_point (undo->image,
                                      sample_point_undo->sample_point);
    }
  else if (sample_point_undo->x == -1)
    {
      gimp_image_remove_sample_point (undo->image,
                                      sample_point_undo->sample_point, FALSE);
    }
  else
    {
      gimp_image_update_sample_point (undo->image,
                                      sample_point_undo->sample_point);
      sample_point_undo->sample_point->x = sample_point_undo->x;
      sample_point_undo->sample_point->y = sample_point_undo->y;
      gimp_image_update_sample_point (undo->image,
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

  if (sample_point_undo->sample_point)
    {
      gimp_sample_point_unref (sample_point_undo->sample_point);
      sample_point_undo->sample_point = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
