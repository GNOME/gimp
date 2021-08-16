/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstroke.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "vectors-types.h"

#include "core/gimp-memsize.h"
#include "core/gimpcoords.h"
#include "core/gimpparamspecs.h"
#include "core/gimp-transform-utils.h"

#include "gimpanchor.h"
#include "gimpstroke.h"

enum
{
  PROP_0,
  PROP_CONTROL_POINTS,
  PROP_CLOSED
};

/* Prototypes */

static void    gimp_stroke_set_property              (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);
static void    gimp_stroke_get_property              (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);
static void    gimp_stroke_finalize                  (GObject      *object);

static gint64  gimp_stroke_get_memsize               (GimpObject   *object,
                                                      gint64       *gui_size);

static GimpAnchor * gimp_stroke_real_anchor_get      (GimpStroke       *stroke,
                                                      const GimpCoords *coord);
static GimpAnchor * gimp_stroke_real_anchor_get_next (GimpStroke       *stroke,
                                                      const GimpAnchor *prev);
static void         gimp_stroke_real_anchor_select   (GimpStroke       *stroke,
                                                      GimpAnchor       *anchor,
                                                      gboolean          selected,
                                                      gboolean          exclusive);
static void    gimp_stroke_real_anchor_move_relative (GimpStroke       *stroke,
                                                      GimpAnchor       *anchor,
                                                      const GimpCoords *delta,
                                                      GimpAnchorFeatureType feature);
static void    gimp_stroke_real_anchor_move_absolute (GimpStroke       *stroke,
                                                      GimpAnchor       *anchor,
                                                      const GimpCoords *delta,
                                                      GimpAnchorFeatureType feature);
static void         gimp_stroke_real_anchor_convert  (GimpStroke       *stroke,
                                                      GimpAnchor       *anchor,
                                                      GimpAnchorFeatureType  feature);
static void         gimp_stroke_real_anchor_delete   (GimpStroke       *stroke,
                                                      GimpAnchor       *anchor);
static gboolean     gimp_stroke_real_point_is_movable
                                              (GimpStroke            *stroke,
                                               GimpAnchor            *predec,
                                               gdouble                position);
static void         gimp_stroke_real_point_move_relative
                                              (GimpStroke            *stroke,
                                               GimpAnchor            *predec,
                                               gdouble                position,
                                               const GimpCoords      *deltacoord,
                                               GimpAnchorFeatureType  feature);
static void         gimp_stroke_real_point_move_absolute
                                              (GimpStroke            *stroke,
                                               GimpAnchor            *predec,
                                               gdouble                position,
                                               const GimpCoords      *coord,
                                               GimpAnchorFeatureType  feature);

static void         gimp_stroke_real_close           (GimpStroke       *stroke);
static GimpStroke * gimp_stroke_real_open            (GimpStroke       *stroke,
                                                      GimpAnchor       *end_anchor);
static gboolean     gimp_stroke_real_anchor_is_insertable
                                                     (GimpStroke       *stroke,
                                                      GimpAnchor       *predec,
                                                      gdouble           position);
static GimpAnchor * gimp_stroke_real_anchor_insert   (GimpStroke       *stroke,
                                                      GimpAnchor       *predec,
                                                      gdouble           position);

static gboolean     gimp_stroke_real_is_extendable   (GimpStroke       *stroke,
                                                      GimpAnchor       *neighbor);

static GimpAnchor * gimp_stroke_real_extend (GimpStroke           *stroke,
                                             const GimpCoords     *coords,
                                             GimpAnchor           *neighbor,
                                             GimpVectorExtendMode  extend_mode);

gboolean     gimp_stroke_real_connect_stroke (GimpStroke          *stroke,
                                              GimpAnchor          *anchor,
                                              GimpStroke          *extension,
                                              GimpAnchor          *neighbor);


static gboolean     gimp_stroke_real_is_empty        (GimpStroke       *stroke);
static gboolean     gimp_stroke_real_reverse         (GimpStroke       *stroke);
static gboolean     gimp_stroke_real_shift_start     (GimpStroke       *stroke,
                                                      GimpAnchor       *anchor);

static gdouble      gimp_stroke_real_get_length      (GimpStroke       *stroke,
                                                      gdouble           precision);
static gdouble      gimp_stroke_real_get_distance    (GimpStroke       *stroke,
                                                      const GimpCoords *coord);
static GArray *     gimp_stroke_real_interpolate     (GimpStroke       *stroke,
                                                      gdouble           precision,
                                                      gboolean         *closed);
static GimpStroke * gimp_stroke_real_duplicate       (GimpStroke       *stroke);
static GimpBezierDesc * gimp_stroke_real_make_bezier (GimpStroke       *stroke);

static void         gimp_stroke_real_translate       (GimpStroke       *stroke,
                                                      gdouble           offset_x,
                                                      gdouble           offset_y);
static void         gimp_stroke_real_scale           (GimpStroke       *stroke,
                                                      gdouble           scale_x,
                                                      gdouble           scale_y);
static void         gimp_stroke_real_rotate          (GimpStroke       *stroke,
                                                      gdouble           center_x,
                                                      gdouble           center_y,
                                                      gdouble           angle);
static void         gimp_stroke_real_flip            (GimpStroke       *stroke,
                                                      GimpOrientationType flip_type,
                                                      gdouble           axis);
static void         gimp_stroke_real_flip_free       (GimpStroke       *stroke,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);
static void         gimp_stroke_real_transform       (GimpStroke        *stroke,
                                                      const GimpMatrix3 *matrix,
                                                      GQueue            *ret_strokes);

static GList    * gimp_stroke_real_get_draw_anchors  (GimpStroke       *stroke);
static GList    * gimp_stroke_real_get_draw_controls (GimpStroke       *stroke);
static GArray   * gimp_stroke_real_get_draw_lines    (GimpStroke       *stroke);
static GArray *  gimp_stroke_real_control_points_get (GimpStroke       *stroke,
                                                      gboolean         *ret_closed);
static gboolean   gimp_stroke_real_get_point_at_dist (GimpStroke       *stroke,
                                                      gdouble           dist,
                                                      gdouble           precision,
                                                      GimpCoords       *position,
                                                      gdouble          *slope);


G_DEFINE_TYPE (GimpStroke, gimp_stroke, GIMP_TYPE_OBJECT)

#define parent_class gimp_stroke_parent_class


static void
gimp_stroke_class_init (GimpStrokeClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GParamSpec      *param_spec;

  object_class->finalize          = gimp_stroke_finalize;
  object_class->get_property      = gimp_stroke_get_property;
  object_class->set_property      = gimp_stroke_set_property;

  gimp_object_class->get_memsize  = gimp_stroke_get_memsize;

  klass->changed                  = NULL;
  klass->removed                  = NULL;

  klass->anchor_get               = gimp_stroke_real_anchor_get;
  klass->anchor_get_next          = gimp_stroke_real_anchor_get_next;
  klass->anchor_select            = gimp_stroke_real_anchor_select;
  klass->anchor_move_relative     = gimp_stroke_real_anchor_move_relative;
  klass->anchor_move_absolute     = gimp_stroke_real_anchor_move_absolute;
  klass->anchor_convert           = gimp_stroke_real_anchor_convert;
  klass->anchor_delete            = gimp_stroke_real_anchor_delete;

  klass->point_is_movable         = gimp_stroke_real_point_is_movable;
  klass->point_move_relative      = gimp_stroke_real_point_move_relative;
  klass->point_move_absolute      = gimp_stroke_real_point_move_absolute;

  klass->nearest_point_get        = NULL;
  klass->nearest_tangent_get      = NULL;
  klass->nearest_intersection_get = NULL;
  klass->close                    = gimp_stroke_real_close;
  klass->open                     = gimp_stroke_real_open;
  klass->anchor_is_insertable     = gimp_stroke_real_anchor_is_insertable;
  klass->anchor_insert            = gimp_stroke_real_anchor_insert;
  klass->is_extendable            = gimp_stroke_real_is_extendable;
  klass->extend                   = gimp_stroke_real_extend;
  klass->connect_stroke           = gimp_stroke_real_connect_stroke;

  klass->is_empty                 = gimp_stroke_real_is_empty;
  klass->reverse                  = gimp_stroke_real_reverse;
  klass->shift_start              = gimp_stroke_real_shift_start;
  klass->get_length               = gimp_stroke_real_get_length;
  klass->get_distance             = gimp_stroke_real_get_distance;
  klass->get_point_at_dist        = gimp_stroke_real_get_point_at_dist;
  klass->interpolate              = gimp_stroke_real_interpolate;

  klass->duplicate                = gimp_stroke_real_duplicate;
  klass->make_bezier              = gimp_stroke_real_make_bezier;

  klass->translate                = gimp_stroke_real_translate;
  klass->scale                    = gimp_stroke_real_scale;
  klass->rotate                   = gimp_stroke_real_rotate;
  klass->flip                     = gimp_stroke_real_flip;
  klass->flip_free                = gimp_stroke_real_flip_free;
  klass->transform                = gimp_stroke_real_transform;


  klass->get_draw_anchors         = gimp_stroke_real_get_draw_anchors;
  klass->get_draw_controls        = gimp_stroke_real_get_draw_controls;
  klass->get_draw_lines           = gimp_stroke_real_get_draw_lines;
  klass->control_points_get       = gimp_stroke_real_control_points_get;

  param_spec = g_param_spec_boxed ("gimp-anchor",
                                   "Gimp Anchor",
                                   "The control points of a Stroke",
                                   GIMP_TYPE_ANCHOR,
                                   GIMP_PARAM_WRITABLE |
                                   G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_CONTROL_POINTS,
                                   gimp_param_spec_value_array ("control-points",
                                                                "Control Points",
                                                                "This is an ValueArray "
                                                                "with the initial "
                                                                "control points of "
                                                                "the new Stroke",
                                                                param_spec,
                                                                GIMP_PARAM_WRITABLE |
                                                                G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CLOSED,
                                   g_param_spec_boolean ("closed",
                                                         "Close Flag",
                                                         "this flag indicates "
                                                         "whether the stroke "
                                                         "is closed or not",
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_stroke_init (GimpStroke *stroke)
{
  stroke->anchors = g_queue_new ();
}

static void
gimp_stroke_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpStroke     *stroke = GIMP_STROKE (object);
  GimpValueArray *val_array;
  gint            length;
  gint            i;

  switch (property_id)
    {
    case PROP_CLOSED:
      stroke->closed = g_value_get_boolean (value);
      break;

    case PROP_CONTROL_POINTS:
      g_return_if_fail (g_queue_is_empty (stroke->anchors));
      g_return_if_fail (value != NULL);

      val_array = g_value_get_boxed (value);

      if (val_array == NULL)
        return;

      length = gimp_value_array_length (val_array);

      for (i = 0; i < length; i++)
        {
          GValue *item = gimp_value_array_index (val_array, i);

          g_return_if_fail (G_VALUE_HOLDS (item, GIMP_TYPE_ANCHOR));
          g_queue_push_tail (stroke->anchors, g_value_dup_boxed (item));
        }

      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_stroke_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpStroke *stroke = GIMP_STROKE (object);

  switch (property_id)
    {
    case PROP_CLOSED:
      g_value_set_boolean (value, stroke->closed);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_stroke_finalize (GObject *object)
{
  GimpStroke *stroke = GIMP_STROKE (object);

  g_queue_free_full (stroke->anchors, (GDestroyNotify) gimp_anchor_free);
  stroke->anchors = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_stroke_get_memsize (GimpObject *object,
                         gint64     *gui_size)
{
  GimpStroke *stroke  = GIMP_STROKE (object);
  gint64      memsize = 0;

  memsize += gimp_g_queue_get_memsize (stroke->anchors, sizeof (GimpAnchor));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

void
gimp_stroke_set_id (GimpStroke *stroke,
                    gint        id)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));
  g_return_if_fail (stroke->id == 0 /* we don't want changing IDs... */);

  stroke->id = id;
}

gint
gimp_stroke_get_id (GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), -1);

  return stroke->id;
}


GimpAnchor *
gimp_stroke_anchor_get (GimpStroke       *stroke,
                        const GimpCoords *coord)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->anchor_get (stroke, coord);
}


gdouble
gimp_stroke_nearest_point_get (GimpStroke       *stroke,
                               const GimpCoords *coord,
                               const gdouble     precision,
                               GimpCoords       *ret_point,
                               GimpAnchor      **ret_segment_start,
                               GimpAnchor      **ret_segment_end,
                               gdouble          *ret_pos)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (GIMP_STROKE_GET_CLASS (stroke)->nearest_point_get)
    return GIMP_STROKE_GET_CLASS (stroke)->nearest_point_get (stroke,
                                                              coord,
                                                              precision,
                                                              ret_point,
                                                              ret_segment_start,
                                                              ret_segment_end,
                                                              ret_pos);
  return -1;
}

gdouble
gimp_stroke_nearest_tangent_get (GimpStroke            *stroke,
                                 const GimpCoords      *coords1,
                                 const GimpCoords      *coords2,
                                 gdouble                precision,
                                 GimpCoords            *nearest,
                                 GimpAnchor           **ret_segment_start,
                                 GimpAnchor           **ret_segment_end,
                                 gdouble               *ret_pos)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (coords1 != NULL, FALSE);
  g_return_val_if_fail (coords2 != NULL, FALSE);

  if (GIMP_STROKE_GET_CLASS (stroke)->nearest_tangent_get)
    return GIMP_STROKE_GET_CLASS (stroke)->nearest_tangent_get (stroke,
                                                                coords1,
                                                                coords2,
                                                                precision,
                                                                nearest,
                                                                ret_segment_start,
                                                                ret_segment_end,
                                                                ret_pos);
  return -1;
}

gdouble
gimp_stroke_nearest_intersection_get (GimpStroke        *stroke,
                                      const GimpCoords  *coords1,
                                      const GimpCoords  *direction,
                                      gdouble            precision,
                                      GimpCoords        *nearest,
                                      GimpAnchor       **ret_segment_start,
                                      GimpAnchor       **ret_segment_end,
                                      gdouble           *ret_pos)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (coords1 != NULL, FALSE);
  g_return_val_if_fail (direction != NULL, FALSE);

  if (GIMP_STROKE_GET_CLASS (stroke)->nearest_intersection_get)
    return GIMP_STROKE_GET_CLASS (stroke)->nearest_intersection_get (stroke,
                                                                     coords1,
                                                                     direction,
                                                                     precision,
                                                                     nearest,
                                                                     ret_segment_start,
                                                                     ret_segment_end,
                                                                     ret_pos);
  return -1;
}

static GimpAnchor *
gimp_stroke_real_anchor_get (GimpStroke       *stroke,
                             const GimpCoords *coord)
{
  gdouble     dx, dy;
  gdouble     mindist = -1;
  GList      *anchors;
  GList      *list;
  GimpAnchor *anchor = NULL;

  anchors = gimp_stroke_get_draw_controls (stroke);

  for (list = anchors; list; list = g_list_next (list))
    {
      dx = coord->x - GIMP_ANCHOR (list->data)->position.x;
      dy = coord->y - GIMP_ANCHOR (list->data)->position.y;

      if (mindist < 0 || mindist > dx * dx + dy * dy)
        {
          mindist = dx * dx + dy * dy;
          anchor = GIMP_ANCHOR (list->data);
        }
    }

  g_list_free (anchors);

  anchors = gimp_stroke_get_draw_anchors (stroke);

  for (list = anchors; list; list = g_list_next (list))
    {
      dx = coord->x - GIMP_ANCHOR (list->data)->position.x;
      dy = coord->y - GIMP_ANCHOR (list->data)->position.y;

      if (mindist < 0 || mindist > dx * dx + dy * dy)
        {
          mindist = dx * dx + dy * dy;
          anchor = GIMP_ANCHOR (list->data);
        }
    }

  g_list_free (anchors);

  return anchor;
}


GimpAnchor *
gimp_stroke_anchor_get_next (GimpStroke       *stroke,
                             const GimpAnchor *prev)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->anchor_get_next (stroke, prev);
}

static GimpAnchor *
gimp_stroke_real_anchor_get_next (GimpStroke       *stroke,
                                  const GimpAnchor *prev)
{
  GList *list;

  if (prev)
    {
      list = g_queue_find (stroke->anchors, prev);
      if (list)
        list = g_list_next (list);
    }
  else
    {
      list = stroke->anchors->head;
    }

  if (list)
    return GIMP_ANCHOR (list->data);

  return NULL;
}


void
gimp_stroke_anchor_select (GimpStroke *stroke,
                           GimpAnchor *anchor,
                           gboolean    selected,
                           gboolean    exclusive)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->anchor_select (stroke, anchor,
                                                 selected, exclusive);
}

static void
gimp_stroke_real_anchor_select (GimpStroke *stroke,
                                GimpAnchor *anchor,
                                gboolean    selected,
                                gboolean    exclusive)
{
  GList *list = stroke->anchors->head;

  if (exclusive)
    {
      while (list)
        {
          GIMP_ANCHOR (list->data)->selected = FALSE;
          list = g_list_next (list);
        }
    }

  list = g_queue_find (stroke->anchors, anchor);

  if (list)
    GIMP_ANCHOR (list->data)->selected = selected;
}


void
gimp_stroke_anchor_move_relative (GimpStroke            *stroke,
                                  GimpAnchor            *anchor,
                                  const GimpCoords      *delta,
                                  GimpAnchorFeatureType  feature)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (g_queue_find (stroke->anchors, anchor));

  GIMP_STROKE_GET_CLASS (stroke)->anchor_move_relative (stroke, anchor,
                                                        delta, feature);
}

static void
gimp_stroke_real_anchor_move_relative (GimpStroke            *stroke,
                                       GimpAnchor            *anchor,
                                       const GimpCoords      *delta,
                                       GimpAnchorFeatureType  feature)
{
  anchor->position.x += delta->x;
  anchor->position.y += delta->y;
}


void
gimp_stroke_anchor_move_absolute (GimpStroke            *stroke,
                                  GimpAnchor            *anchor,
                                  const GimpCoords      *coord,
                                  GimpAnchorFeatureType  feature)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (g_queue_find (stroke->anchors, anchor));

  GIMP_STROKE_GET_CLASS (stroke)->anchor_move_absolute (stroke, anchor,
                                                        coord, feature);
}

static void
gimp_stroke_real_anchor_move_absolute (GimpStroke            *stroke,
                                       GimpAnchor            *anchor,
                                       const GimpCoords      *coord,
                                       GimpAnchorFeatureType  feature)
{
  anchor->position.x = coord->x;
  anchor->position.y = coord->y;
}

gboolean
gimp_stroke_point_is_movable (GimpStroke *stroke,
                              GimpAnchor *predec,
                              gdouble     position)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);

  return GIMP_STROKE_GET_CLASS (stroke)->point_is_movable (stroke, predec,
                                                           position);
}


static gboolean
gimp_stroke_real_point_is_movable (GimpStroke *stroke,
                                   GimpAnchor *predec,
                                   gdouble     position)
{
  return FALSE;
}


void
gimp_stroke_point_move_relative (GimpStroke            *stroke,
                                 GimpAnchor            *predec,
                                 gdouble                position,
                                 const GimpCoords      *deltacoord,
                                 GimpAnchorFeatureType  feature)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->point_move_relative (stroke, predec,
                                                       position, deltacoord,
                                                       feature);
}


static void
gimp_stroke_real_point_move_relative (GimpStroke           *stroke,
                                      GimpAnchor           *predec,
                                      gdouble               position,
                                      const GimpCoords     *deltacoord,
                                      GimpAnchorFeatureType feature)
{
  g_printerr ("gimp_stroke_point_move_relative: default implementation\n");
}


void
gimp_stroke_point_move_absolute (GimpStroke            *stroke,
                                 GimpAnchor            *predec,
                                 gdouble                position,
                                 const GimpCoords      *coord,
                                 GimpAnchorFeatureType  feature)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->point_move_absolute (stroke, predec,
                                                       position, coord,
                                                       feature);
}

static void
gimp_stroke_real_point_move_absolute (GimpStroke           *stroke,
                                      GimpAnchor           *predec,
                                      gdouble               position,
                                      const GimpCoords     *coord,
                                      GimpAnchorFeatureType feature)
{
  g_printerr ("gimp_stroke_point_move_absolute: default implementation\n");
}


void
gimp_stroke_close (GimpStroke *stroke)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));
  g_return_if_fail (g_queue_is_empty (stroke->anchors) == FALSE);

  GIMP_STROKE_GET_CLASS (stroke)->close (stroke);
}

static void
gimp_stroke_real_close (GimpStroke *stroke)
{
  stroke->closed = TRUE;
  g_object_notify (G_OBJECT (stroke), "closed");
}


void
gimp_stroke_anchor_convert (GimpStroke            *stroke,
                            GimpAnchor            *anchor,
                            GimpAnchorFeatureType  feature)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->anchor_convert (stroke, anchor, feature);
}

static void
gimp_stroke_real_anchor_convert (GimpStroke            *stroke,
                                 GimpAnchor            *anchor,
                                 GimpAnchorFeatureType  feature)
{
  g_printerr ("gimp_stroke_anchor_convert: default implementation\n");
}


void
gimp_stroke_anchor_delete (GimpStroke *stroke,
                           GimpAnchor *anchor)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));
  g_return_if_fail (anchor && anchor->type == GIMP_ANCHOR_ANCHOR);

  GIMP_STROKE_GET_CLASS (stroke)->anchor_delete (stroke, anchor);
}

static void
gimp_stroke_real_anchor_delete (GimpStroke *stroke,
                                GimpAnchor *anchor)
{
  g_printerr ("gimp_stroke_anchor_delete: default implementation\n");
}

GimpStroke *
gimp_stroke_open (GimpStroke *stroke,
                  GimpAnchor *end_anchor)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);
  g_return_val_if_fail (end_anchor &&
                        end_anchor->type == GIMP_ANCHOR_ANCHOR, NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->open (stroke, end_anchor);
}

static GimpStroke *
gimp_stroke_real_open (GimpStroke *stroke,
                       GimpAnchor *end_anchor)
{
  g_printerr ("gimp_stroke_open: default implementation\n");
  return NULL;
}

gboolean
gimp_stroke_anchor_is_insertable (GimpStroke *stroke,
                                  GimpAnchor *predec,
                                  gdouble     position)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);

  return GIMP_STROKE_GET_CLASS (stroke)->anchor_is_insertable (stroke,
                                                               predec,
                                                               position);
}

static gboolean
gimp_stroke_real_anchor_is_insertable (GimpStroke *stroke,
                                       GimpAnchor *predec,
                                       gdouble     position)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);

  return FALSE;
}

GimpAnchor *
gimp_stroke_anchor_insert (GimpStroke *stroke,
                           GimpAnchor *predec,
                           gdouble     position)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);
  g_return_val_if_fail (predec->type == GIMP_ANCHOR_ANCHOR, NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->anchor_insert (stroke,
                                                        predec, position);
}

static GimpAnchor *
gimp_stroke_real_anchor_insert (GimpStroke *stroke,
                                GimpAnchor *predec,
                                gdouble     position)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return NULL;
}


gboolean
gimp_stroke_is_extendable (GimpStroke *stroke,
                           GimpAnchor *neighbor)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);

  return GIMP_STROKE_GET_CLASS (stroke)->is_extendable (stroke, neighbor);
}

static gboolean
gimp_stroke_real_is_extendable (GimpStroke *stroke,
                                GimpAnchor *neighbor)
{
  return FALSE;
}


GimpAnchor *
gimp_stroke_extend (GimpStroke           *stroke,
                    const GimpCoords     *coords,
                    GimpAnchor           *neighbor,
                    GimpVectorExtendMode  extend_mode)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);
  g_return_val_if_fail (!stroke->closed, NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->extend (stroke, coords,
                                                 neighbor, extend_mode);
}

static GimpAnchor *
gimp_stroke_real_extend (GimpStroke           *stroke,
                         const GimpCoords     *coords,
                         GimpAnchor           *neighbor,
                         GimpVectorExtendMode  extend_mode)
{
  g_printerr ("gimp_stroke_extend: default implementation\n");
  return NULL;
}

gboolean
gimp_stroke_connect_stroke (GimpStroke *stroke,
                            GimpAnchor *anchor,
                            GimpStroke *extension,
                            GimpAnchor *neighbor)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (GIMP_IS_STROKE (extension), FALSE);
  g_return_val_if_fail (stroke->closed == FALSE &&
                        extension->closed == FALSE, FALSE);

  return GIMP_STROKE_GET_CLASS (stroke)->connect_stroke (stroke, anchor,
                                                         extension, neighbor);
}

gboolean
gimp_stroke_real_connect_stroke (GimpStroke *stroke,
                                 GimpAnchor *anchor,
                                 GimpStroke *extension,
                                 GimpAnchor *neighbor)
{
  g_printerr ("gimp_stroke_connect_stroke: default implementation\n");
  return FALSE;
}

gboolean
gimp_stroke_is_empty (GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);

  return GIMP_STROKE_GET_CLASS (stroke)->is_empty (stroke);
}

static gboolean
gimp_stroke_real_is_empty (GimpStroke *stroke)
{
  return g_queue_is_empty (stroke->anchors);
}


gboolean
gimp_stroke_reverse (GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);

  return GIMP_STROKE_GET_CLASS (stroke)->reverse (stroke);
}


static gboolean
gimp_stroke_real_reverse (GimpStroke *stroke)
{
  g_queue_reverse (stroke->anchors);

  /* keep the first node the same for closed strokes */
  if (stroke->closed && stroke->anchors->length > 0)
    g_queue_push_head_link (stroke->anchors,
                            g_queue_pop_tail_link (stroke->anchors));

  return TRUE;
}


gboolean
gimp_stroke_shift_start (GimpStroke *stroke,
                         GimpAnchor *new_start)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (new_start != NULL, FALSE);

  return GIMP_STROKE_GET_CLASS (stroke)->shift_start (stroke, new_start);
}

static gboolean
gimp_stroke_real_shift_start (GimpStroke *stroke,
                              GimpAnchor *new_start)
{
  GList *link;

  link = g_queue_find (stroke->anchors, new_start);
  if (!link)
    return FALSE;

  if (link == stroke->anchors->head)
    return TRUE;

  stroke->anchors->tail->next = stroke->anchors->head;
  stroke->anchors->head->prev = stroke->anchors->tail;
  stroke->anchors->tail = link->prev;
  stroke->anchors->head = link;
  stroke->anchors->tail->next = NULL;
  stroke->anchors->head->prev = NULL;

  return TRUE;
}


gdouble
gimp_stroke_get_length (GimpStroke *stroke,
                        gdouble     precision)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0.0);

  return GIMP_STROKE_GET_CLASS (stroke)->get_length (stroke, precision);
}

static gdouble
gimp_stroke_real_get_length (GimpStroke *stroke,
                             gdouble     precision)
{
  GArray     *points;
  gint        i;
  gdouble     length;
  GimpCoords  difference;

  if (g_queue_is_empty (stroke->anchors))
    return -1;

  points = gimp_stroke_interpolate (stroke, precision, NULL);
  if (points == NULL)
    return -1;

  length = 0;

  for (i = 0; i < points->len - 1; i++ )
    {
       gimp_coords_difference (&(g_array_index (points, GimpCoords, i)),
                               &(g_array_index (points, GimpCoords, i+1)),
                               &difference);
       length += gimp_coords_length (&difference);
    }

  g_array_free(points, TRUE);

  return length;
}


gdouble
gimp_stroke_get_distance (GimpStroke       *stroke,
                          const GimpCoords *coord)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0.0);

  return GIMP_STROKE_GET_CLASS (stroke)->get_distance (stroke, coord);
}

static gdouble
gimp_stroke_real_get_distance (GimpStroke       *stroke,
                               const GimpCoords *coord)
{
  g_printerr ("gimp_stroke_get_distance: default implementation\n");

  return 0.0;
}


GArray *
gimp_stroke_interpolate (GimpStroke *stroke,
                         gdouble     precision,
                         gboolean   *ret_closed)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->interpolate (stroke, precision,
                                                      ret_closed);
}

static GArray *
gimp_stroke_real_interpolate (GimpStroke *stroke,
                              gdouble     precision,
                              gboolean   *ret_closed)
{
  g_printerr ("gimp_stroke_interpolate: default implementation\n");

  return NULL;
}

GimpStroke *
gimp_stroke_duplicate (GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->duplicate (stroke);
}

static GimpStroke *
gimp_stroke_real_duplicate (GimpStroke *stroke)
{
  GimpStroke *new_stroke;
  GList      *list;

  new_stroke = g_object_new (G_TYPE_FROM_INSTANCE (stroke),
                             "name", gimp_object_get_name (stroke),
                             NULL);

  g_queue_free_full (new_stroke->anchors, (GDestroyNotify) gimp_anchor_free);
  new_stroke->anchors = g_queue_copy (stroke->anchors);

  for (list = new_stroke->anchors->head; list; list = g_list_next (list))
    {
      list->data = gimp_anchor_copy (GIMP_ANCHOR (list->data));
    }

  new_stroke->closed = stroke->closed;
  /* we do *not* copy the ID! */

  return new_stroke;
}


GimpBezierDesc *
gimp_stroke_make_bezier (GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->make_bezier (stroke);
}

static GimpBezierDesc *
gimp_stroke_real_make_bezier (GimpStroke *stroke)
{
  g_printerr ("gimp_stroke_make_bezier: default implementation\n");

  return NULL;
}


void
gimp_stroke_translate (GimpStroke *stroke,
                       gdouble     offset_x,
                       gdouble     offset_y)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->translate (stroke, offset_x, offset_y);
}

static void
gimp_stroke_real_translate (GimpStroke *stroke,
                            gdouble     offset_x,
                            gdouble     offset_y)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      anchor->position.x += offset_x;
      anchor->position.y += offset_y;
    }
}


void
gimp_stroke_scale (GimpStroke *stroke,
                   gdouble     scale_x,
                   gdouble     scale_y)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->scale (stroke, scale_x, scale_y);
}

static void
gimp_stroke_real_scale (GimpStroke *stroke,
                        gdouble     scale_x,
                        gdouble     scale_y)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      anchor->position.x *= scale_x;
      anchor->position.y *= scale_y;
    }
}

void
gimp_stroke_rotate (GimpStroke *stroke,
                    gdouble     center_x,
                    gdouble     center_y,
                    gdouble     angle)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->rotate (stroke, center_x, center_y, angle);
}

static void
gimp_stroke_real_rotate (GimpStroke *stroke,
                         gdouble     center_x,
                         gdouble     center_y,
                         gdouble     angle)
{
  GimpMatrix3  matrix;

  angle = angle / 180.0 * G_PI;
  gimp_matrix3_identity (&matrix);
  gimp_transform_matrix_rotate_center (&matrix, center_x, center_y, angle);

  gimp_stroke_transform (stroke, &matrix, NULL);
}

void
gimp_stroke_flip (GimpStroke          *stroke,
                  GimpOrientationType  flip_type,
                  gdouble              axis)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->flip (stroke, flip_type, axis);
}

static void
gimp_stroke_real_flip (GimpStroke          *stroke,
                       GimpOrientationType  flip_type,
                       gdouble              axis)
{
  GimpMatrix3  matrix;

  gimp_matrix3_identity (&matrix);
  gimp_transform_matrix_flip (&matrix, flip_type, axis);
  gimp_stroke_transform (stroke, &matrix, NULL);
}

void
gimp_stroke_flip_free (GimpStroke *stroke,
                       gdouble     x1,
                       gdouble     y1,
                       gdouble     x2,
                       gdouble     y2)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->flip_free (stroke, x1, y1, x2, y2);
}

static void
gimp_stroke_real_flip_free (GimpStroke *stroke,
                            gdouble     x1,
                            gdouble     y1,
                            gdouble     x2,
                            gdouble     y2)
{
  /* x, y, width and height parameter in gimp_transform_matrix_flip_free are unused */
  GimpMatrix3  matrix;

  gimp_matrix3_identity (&matrix);
  gimp_transform_matrix_flip_free (&matrix, x1, y1, x2, y2);

  gimp_stroke_transform (stroke, &matrix, NULL);
}

/* transforms 'stroke' by 'matrix'.  due to clipping, the transformation may
 * result in multiple strokes.
 *
 * if 'ret_strokes' is not NULL, the transformed strokes are appended to the
 * queue, and 'stroke' is left in an unspecified state.  one of the resulting
 * strokes may alias 'stroke'.
 *
 * if 'ret_strokes' is NULL, the transformation is performed in-place.  if the
 * transformation results in multiple strokes (which, atm, can only happen for
 * non-affine transformation), the result is undefined.
 */
void
gimp_stroke_transform (GimpStroke        *stroke,
                       const GimpMatrix3 *matrix,
                       GQueue            *ret_strokes)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->transform (stroke, matrix, ret_strokes);
}

static void
gimp_stroke_real_transform (GimpStroke        *stroke,
                            const GimpMatrix3 *matrix,
                            GQueue            *ret_strokes)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      gimp_matrix3_transform_point (matrix,
                                    anchor->position.x,
                                    anchor->position.y,
                                    &anchor->position.x,
                                    &anchor->position.y);
    }

  if (ret_strokes)
    {
      stroke->id = 0;

      g_queue_push_tail (ret_strokes, g_object_ref (stroke));
    }
}


GList *
gimp_stroke_get_draw_anchors (GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->get_draw_anchors (stroke);
}

static GList *
gimp_stroke_real_get_draw_anchors (GimpStroke *stroke)
{
  GList *list;
  GList *ret_list = NULL;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      if (GIMP_ANCHOR (list->data)->type == GIMP_ANCHOR_ANCHOR)
        ret_list = g_list_prepend (ret_list, list->data);
    }

  return g_list_reverse (ret_list);
}


GList *
gimp_stroke_get_draw_controls (GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->get_draw_controls (stroke);
}

static GList *
gimp_stroke_real_get_draw_controls (GimpStroke *stroke)
{
  GList *list;
  GList *ret_list = NULL;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      if (anchor->type == GIMP_ANCHOR_CONTROL)
        {
          GimpAnchor *next = list->next ? list->next->data : NULL;
          GimpAnchor *prev = list->prev ? list->prev->data : NULL;

          if (next && next->type == GIMP_ANCHOR_ANCHOR && next->selected)
            {
              /* Ok, this is a hack.
               * The idea is to give control points at the end of a
               * stroke a higher priority for the interactive tool.
               */
              if (prev)
                ret_list = g_list_prepend (ret_list, anchor);
              else
                ret_list = g_list_append (ret_list, anchor);
            }
          else if (prev && prev->type == GIMP_ANCHOR_ANCHOR && prev->selected)
            {
              /* same here... */
              if (next)
                ret_list = g_list_prepend (ret_list, anchor);
              else
                ret_list = g_list_append (ret_list, anchor);
            }
        }
    }

  return g_list_reverse (ret_list);
}


GArray *
gimp_stroke_get_draw_lines (GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->get_draw_lines (stroke);
}

static GArray *
gimp_stroke_real_get_draw_lines (GimpStroke *stroke)
{
  GList  *list;
  GArray *ret_lines = NULL;
  gint    count = 0;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      if (anchor->type == GIMP_ANCHOR_ANCHOR && anchor->selected)
        {
          if (list->next)
            {
              GimpAnchor *next = list->next->data;

              if (count == 0)
                ret_lines = g_array_new (FALSE, FALSE, sizeof (GimpCoords));

              ret_lines = g_array_append_val (ret_lines, anchor->position);
              ret_lines = g_array_append_val (ret_lines, next->position);
              count += 1;
            }

          if (list->prev)
            {
              GimpAnchor *prev = list->prev->data;

              if (count == 0)
                ret_lines = g_array_new (FALSE, FALSE, sizeof (GimpCoords));

              ret_lines = g_array_append_val (ret_lines, anchor->position);
              ret_lines = g_array_append_val (ret_lines, prev->position);
              count += 1;
            }
        }
    }

  return ret_lines;
}

GArray *
gimp_stroke_control_points_get (GimpStroke *stroke,
                                gboolean   *ret_closed)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->control_points_get (stroke,
                                                             ret_closed);
}

static GArray *
gimp_stroke_real_control_points_get (GimpStroke *stroke,
                                     gboolean   *ret_closed)
{
  guint   num_anchors;
  GArray *ret_array;
  GList  *list;

  num_anchors = g_queue_get_length (stroke->anchors);
  ret_array = g_array_sized_new (FALSE, FALSE,
                                 sizeof (GimpAnchor), num_anchors);

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      g_array_append_vals (ret_array, list->data, 1);
    }

  if (ret_closed)
    *ret_closed = stroke->closed;

  return ret_array;
}

gboolean
gimp_stroke_get_point_at_dist (GimpStroke *stroke,
                               gdouble     dist,
                               gdouble     precision,
                               GimpCoords *position,
                               gdouble    *slope)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);

  return GIMP_STROKE_GET_CLASS (stroke)->get_point_at_dist (stroke,
                                                            dist,
                                                            precision,
                                                            position,
                                                            slope);
}


static gboolean
gimp_stroke_real_get_point_at_dist (GimpStroke *stroke,
                                    gdouble     dist,
                                    gdouble     precision,
                                    GimpCoords *position,
                                    gdouble    *slope)
{
  GArray     *points;
  gint        i;
  gdouble     length;
  gdouble     segment_length;
  gboolean    ret = FALSE;
  GimpCoords  difference;

  points = gimp_stroke_interpolate (stroke, precision, NULL);
  if (points == NULL)
    return ret;

  length = 0;
  for (i=0; i < points->len - 1; i++)
    {
      gimp_coords_difference (&(g_array_index (points, GimpCoords , i)),
                              &(g_array_index (points, GimpCoords , i+1)),
                              &difference);
      segment_length = gimp_coords_length (&difference);

      if (segment_length == 0 || length + segment_length < dist )
        {
          length += segment_length;
        }
      else
        {
          /* x = x1 + (x2 - x1 ) u  */
          /* x   = x1 (1-u) + u x2  */

          gdouble u = (dist - length) / segment_length;

          gimp_coords_mix (1 - u, &(g_array_index (points, GimpCoords , i)),
                               u, &(g_array_index (points, GimpCoords , i+1)),
                           position);

          if (difference.x == 0)
            *slope = G_MAXDOUBLE;
          else
            *slope = difference.y / difference.x;

          ret = TRUE;
          break;
        }
    }

  g_array_free (points, TRUE);

  return ret;
}
