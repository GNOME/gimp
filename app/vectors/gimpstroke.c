/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmastroke.c
 * Copyright (C) 2002 Simon Budig  <simon@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "vectors-types.h"

#include "core/ligma-memsize.h"
#include "core/ligmacoords.h"
#include "core/ligmaparamspecs.h"
#include "core/ligma-transform-utils.h"

#include "ligmaanchor.h"
#include "ligmastroke.h"

enum
{
  PROP_0,
  PROP_CONTROL_POINTS,
  PROP_CLOSED
};

/* Prototypes */

static void    ligma_stroke_set_property              (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);
static void    ligma_stroke_get_property              (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);
static void    ligma_stroke_finalize                  (GObject      *object);

static gint64  ligma_stroke_get_memsize               (LigmaObject   *object,
                                                      gint64       *gui_size);

static LigmaAnchor * ligma_stroke_real_anchor_get      (LigmaStroke       *stroke,
                                                      const LigmaCoords *coord);
static LigmaAnchor * ligma_stroke_real_anchor_get_next (LigmaStroke       *stroke,
                                                      const LigmaAnchor *prev);
static void         ligma_stroke_real_anchor_select   (LigmaStroke       *stroke,
                                                      LigmaAnchor       *anchor,
                                                      gboolean          selected,
                                                      gboolean          exclusive);
static void    ligma_stroke_real_anchor_move_relative (LigmaStroke       *stroke,
                                                      LigmaAnchor       *anchor,
                                                      const LigmaCoords *delta,
                                                      LigmaAnchorFeatureType feature);
static void    ligma_stroke_real_anchor_move_absolute (LigmaStroke       *stroke,
                                                      LigmaAnchor       *anchor,
                                                      const LigmaCoords *delta,
                                                      LigmaAnchorFeatureType feature);
static void         ligma_stroke_real_anchor_convert  (LigmaStroke       *stroke,
                                                      LigmaAnchor       *anchor,
                                                      LigmaAnchorFeatureType  feature);
static void         ligma_stroke_real_anchor_delete   (LigmaStroke       *stroke,
                                                      LigmaAnchor       *anchor);
static gboolean     ligma_stroke_real_point_is_movable
                                              (LigmaStroke            *stroke,
                                               LigmaAnchor            *predec,
                                               gdouble                position);
static void         ligma_stroke_real_point_move_relative
                                              (LigmaStroke            *stroke,
                                               LigmaAnchor            *predec,
                                               gdouble                position,
                                               const LigmaCoords      *deltacoord,
                                               LigmaAnchorFeatureType  feature);
static void         ligma_stroke_real_point_move_absolute
                                              (LigmaStroke            *stroke,
                                               LigmaAnchor            *predec,
                                               gdouble                position,
                                               const LigmaCoords      *coord,
                                               LigmaAnchorFeatureType  feature);

static void         ligma_stroke_real_close           (LigmaStroke       *stroke);
static LigmaStroke * ligma_stroke_real_open            (LigmaStroke       *stroke,
                                                      LigmaAnchor       *end_anchor);
static gboolean     ligma_stroke_real_anchor_is_insertable
                                                     (LigmaStroke       *stroke,
                                                      LigmaAnchor       *predec,
                                                      gdouble           position);
static LigmaAnchor * ligma_stroke_real_anchor_insert   (LigmaStroke       *stroke,
                                                      LigmaAnchor       *predec,
                                                      gdouble           position);

static gboolean     ligma_stroke_real_is_extendable   (LigmaStroke       *stroke,
                                                      LigmaAnchor       *neighbor);

static LigmaAnchor * ligma_stroke_real_extend (LigmaStroke           *stroke,
                                             const LigmaCoords     *coords,
                                             LigmaAnchor           *neighbor,
                                             LigmaVectorExtendMode  extend_mode);

gboolean     ligma_stroke_real_connect_stroke (LigmaStroke          *stroke,
                                              LigmaAnchor          *anchor,
                                              LigmaStroke          *extension,
                                              LigmaAnchor          *neighbor);


static gboolean     ligma_stroke_real_is_empty        (LigmaStroke       *stroke);
static gboolean     ligma_stroke_real_reverse         (LigmaStroke       *stroke);
static gboolean     ligma_stroke_real_shift_start     (LigmaStroke       *stroke,
                                                      LigmaAnchor       *anchor);

static gdouble      ligma_stroke_real_get_length      (LigmaStroke       *stroke,
                                                      gdouble           precision);
static gdouble      ligma_stroke_real_get_distance    (LigmaStroke       *stroke,
                                                      const LigmaCoords *coord);
static GArray *     ligma_stroke_real_interpolate     (LigmaStroke       *stroke,
                                                      gdouble           precision,
                                                      gboolean         *closed);
static LigmaStroke * ligma_stroke_real_duplicate       (LigmaStroke       *stroke);
static LigmaBezierDesc * ligma_stroke_real_make_bezier (LigmaStroke       *stroke);

static void         ligma_stroke_real_translate       (LigmaStroke       *stroke,
                                                      gdouble           offset_x,
                                                      gdouble           offset_y);
static void         ligma_stroke_real_scale           (LigmaStroke       *stroke,
                                                      gdouble           scale_x,
                                                      gdouble           scale_y);
static void         ligma_stroke_real_rotate          (LigmaStroke       *stroke,
                                                      gdouble           center_x,
                                                      gdouble           center_y,
                                                      gdouble           angle);
static void         ligma_stroke_real_flip            (LigmaStroke       *stroke,
                                                      LigmaOrientationType flip_type,
                                                      gdouble           axis);
static void         ligma_stroke_real_flip_free       (LigmaStroke       *stroke,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);
static void         ligma_stroke_real_transform       (LigmaStroke        *stroke,
                                                      const LigmaMatrix3 *matrix,
                                                      GQueue            *ret_strokes);

static GList    * ligma_stroke_real_get_draw_anchors  (LigmaStroke       *stroke);
static GList    * ligma_stroke_real_get_draw_controls (LigmaStroke       *stroke);
static GArray   * ligma_stroke_real_get_draw_lines    (LigmaStroke       *stroke);
static GArray *  ligma_stroke_real_control_points_get (LigmaStroke       *stroke,
                                                      gboolean         *ret_closed);
static gboolean   ligma_stroke_real_get_point_at_dist (LigmaStroke       *stroke,
                                                      gdouble           dist,
                                                      gdouble           precision,
                                                      LigmaCoords       *position,
                                                      gdouble          *slope);


G_DEFINE_TYPE (LigmaStroke, ligma_stroke, LIGMA_TYPE_OBJECT)

#define parent_class ligma_stroke_parent_class


static void
ligma_stroke_class_init (LigmaStrokeClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  GParamSpec      *param_spec;

  object_class->finalize          = ligma_stroke_finalize;
  object_class->get_property      = ligma_stroke_get_property;
  object_class->set_property      = ligma_stroke_set_property;

  ligma_object_class->get_memsize  = ligma_stroke_get_memsize;

  klass->changed                  = NULL;
  klass->removed                  = NULL;

  klass->anchor_get               = ligma_stroke_real_anchor_get;
  klass->anchor_get_next          = ligma_stroke_real_anchor_get_next;
  klass->anchor_select            = ligma_stroke_real_anchor_select;
  klass->anchor_move_relative     = ligma_stroke_real_anchor_move_relative;
  klass->anchor_move_absolute     = ligma_stroke_real_anchor_move_absolute;
  klass->anchor_convert           = ligma_stroke_real_anchor_convert;
  klass->anchor_delete            = ligma_stroke_real_anchor_delete;

  klass->point_is_movable         = ligma_stroke_real_point_is_movable;
  klass->point_move_relative      = ligma_stroke_real_point_move_relative;
  klass->point_move_absolute      = ligma_stroke_real_point_move_absolute;

  klass->nearest_point_get        = NULL;
  klass->nearest_tangent_get      = NULL;
  klass->nearest_intersection_get = NULL;
  klass->close                    = ligma_stroke_real_close;
  klass->open                     = ligma_stroke_real_open;
  klass->anchor_is_insertable     = ligma_stroke_real_anchor_is_insertable;
  klass->anchor_insert            = ligma_stroke_real_anchor_insert;
  klass->is_extendable            = ligma_stroke_real_is_extendable;
  klass->extend                   = ligma_stroke_real_extend;
  klass->connect_stroke           = ligma_stroke_real_connect_stroke;

  klass->is_empty                 = ligma_stroke_real_is_empty;
  klass->reverse                  = ligma_stroke_real_reverse;
  klass->shift_start              = ligma_stroke_real_shift_start;
  klass->get_length               = ligma_stroke_real_get_length;
  klass->get_distance             = ligma_stroke_real_get_distance;
  klass->get_point_at_dist        = ligma_stroke_real_get_point_at_dist;
  klass->interpolate              = ligma_stroke_real_interpolate;

  klass->duplicate                = ligma_stroke_real_duplicate;
  klass->make_bezier              = ligma_stroke_real_make_bezier;

  klass->translate                = ligma_stroke_real_translate;
  klass->scale                    = ligma_stroke_real_scale;
  klass->rotate                   = ligma_stroke_real_rotate;
  klass->flip                     = ligma_stroke_real_flip;
  klass->flip_free                = ligma_stroke_real_flip_free;
  klass->transform                = ligma_stroke_real_transform;


  klass->get_draw_anchors         = ligma_stroke_real_get_draw_anchors;
  klass->get_draw_controls        = ligma_stroke_real_get_draw_controls;
  klass->get_draw_lines           = ligma_stroke_real_get_draw_lines;
  klass->control_points_get       = ligma_stroke_real_control_points_get;

  param_spec = g_param_spec_boxed ("ligma-anchor",
                                   "Ligma Anchor",
                                   "The control points of a Stroke",
                                   LIGMA_TYPE_ANCHOR,
                                   LIGMA_PARAM_WRITABLE |
                                   G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_CONTROL_POINTS,
                                   ligma_param_spec_value_array ("control-points",
                                                                "Control Points",
                                                                "This is an ValueArray "
                                                                "with the initial "
                                                                "control points of "
                                                                "the new Stroke",
                                                                param_spec,
                                                                LIGMA_PARAM_WRITABLE |
                                                                G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CLOSED,
                                   g_param_spec_boolean ("closed",
                                                         "Close Flag",
                                                         "this flag indicates "
                                                         "whether the stroke "
                                                         "is closed or not",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_stroke_init (LigmaStroke *stroke)
{
  stroke->anchors = g_queue_new ();
}

static void
ligma_stroke_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  LigmaStroke     *stroke = LIGMA_STROKE (object);
  LigmaValueArray *val_array;
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

      length = ligma_value_array_length (val_array);

      for (i = 0; i < length; i++)
        {
          GValue *item = ligma_value_array_index (val_array, i);

          g_return_if_fail (G_VALUE_HOLDS (item, LIGMA_TYPE_ANCHOR));
          g_queue_push_tail (stroke->anchors, g_value_dup_boxed (item));
        }

      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_stroke_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  LigmaStroke *stroke = LIGMA_STROKE (object);

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
ligma_stroke_finalize (GObject *object)
{
  LigmaStroke *stroke = LIGMA_STROKE (object);

  g_queue_free_full (stroke->anchors, (GDestroyNotify) ligma_anchor_free);
  stroke->anchors = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_stroke_get_memsize (LigmaObject *object,
                         gint64     *gui_size)
{
  LigmaStroke *stroke  = LIGMA_STROKE (object);
  gint64      memsize = 0;

  memsize += ligma_g_queue_get_memsize (stroke->anchors, sizeof (LigmaAnchor));

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

void
ligma_stroke_set_id (LigmaStroke *stroke,
                    gint        id)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));
  g_return_if_fail (stroke->id == 0 /* we don't want changing IDs... */);

  stroke->id = id;
}

gint
ligma_stroke_get_id (LigmaStroke *stroke)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), -1);

  return stroke->id;
}


LigmaAnchor *
ligma_stroke_anchor_get (LigmaStroke       *stroke,
                        const LigmaCoords *coord)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->anchor_get (stroke, coord);
}


gdouble
ligma_stroke_nearest_point_get (LigmaStroke       *stroke,
                               const LigmaCoords *coord,
                               const gdouble     precision,
                               LigmaCoords       *ret_point,
                               LigmaAnchor      **ret_segment_start,
                               LigmaAnchor      **ret_segment_end,
                               gdouble          *ret_pos)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (LIGMA_STROKE_GET_CLASS (stroke)->nearest_point_get)
    return LIGMA_STROKE_GET_CLASS (stroke)->nearest_point_get (stroke,
                                                              coord,
                                                              precision,
                                                              ret_point,
                                                              ret_segment_start,
                                                              ret_segment_end,
                                                              ret_pos);
  return -1;
}

gdouble
ligma_stroke_nearest_tangent_get (LigmaStroke            *stroke,
                                 const LigmaCoords      *coords1,
                                 const LigmaCoords      *coords2,
                                 gdouble                precision,
                                 LigmaCoords            *nearest,
                                 LigmaAnchor           **ret_segment_start,
                                 LigmaAnchor           **ret_segment_end,
                                 gdouble               *ret_pos)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (coords1 != NULL, FALSE);
  g_return_val_if_fail (coords2 != NULL, FALSE);

  if (LIGMA_STROKE_GET_CLASS (stroke)->nearest_tangent_get)
    return LIGMA_STROKE_GET_CLASS (stroke)->nearest_tangent_get (stroke,
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
ligma_stroke_nearest_intersection_get (LigmaStroke        *stroke,
                                      const LigmaCoords  *coords1,
                                      const LigmaCoords  *direction,
                                      gdouble            precision,
                                      LigmaCoords        *nearest,
                                      LigmaAnchor       **ret_segment_start,
                                      LigmaAnchor       **ret_segment_end,
                                      gdouble           *ret_pos)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (coords1 != NULL, FALSE);
  g_return_val_if_fail (direction != NULL, FALSE);

  if (LIGMA_STROKE_GET_CLASS (stroke)->nearest_intersection_get)
    return LIGMA_STROKE_GET_CLASS (stroke)->nearest_intersection_get (stroke,
                                                                     coords1,
                                                                     direction,
                                                                     precision,
                                                                     nearest,
                                                                     ret_segment_start,
                                                                     ret_segment_end,
                                                                     ret_pos);
  return -1;
}

static LigmaAnchor *
ligma_stroke_real_anchor_get (LigmaStroke       *stroke,
                             const LigmaCoords *coord)
{
  gdouble     dx, dy;
  gdouble     mindist = -1;
  GList      *anchors;
  GList      *list;
  LigmaAnchor *anchor = NULL;

  anchors = ligma_stroke_get_draw_controls (stroke);

  for (list = anchors; list; list = g_list_next (list))
    {
      dx = coord->x - LIGMA_ANCHOR (list->data)->position.x;
      dy = coord->y - LIGMA_ANCHOR (list->data)->position.y;

      if (mindist < 0 || mindist > dx * dx + dy * dy)
        {
          mindist = dx * dx + dy * dy;
          anchor = LIGMA_ANCHOR (list->data);
        }
    }

  g_list_free (anchors);

  anchors = ligma_stroke_get_draw_anchors (stroke);

  for (list = anchors; list; list = g_list_next (list))
    {
      dx = coord->x - LIGMA_ANCHOR (list->data)->position.x;
      dy = coord->y - LIGMA_ANCHOR (list->data)->position.y;

      if (mindist < 0 || mindist > dx * dx + dy * dy)
        {
          mindist = dx * dx + dy * dy;
          anchor = LIGMA_ANCHOR (list->data);
        }
    }

  g_list_free (anchors);

  return anchor;
}


LigmaAnchor *
ligma_stroke_anchor_get_next (LigmaStroke       *stroke,
                             const LigmaAnchor *prev)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->anchor_get_next (stroke, prev);
}

static LigmaAnchor *
ligma_stroke_real_anchor_get_next (LigmaStroke       *stroke,
                                  const LigmaAnchor *prev)
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
    return LIGMA_ANCHOR (list->data);

  return NULL;
}


void
ligma_stroke_anchor_select (LigmaStroke *stroke,
                           LigmaAnchor *anchor,
                           gboolean    selected,
                           gboolean    exclusive)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  LIGMA_STROKE_GET_CLASS (stroke)->anchor_select (stroke, anchor,
                                                 selected, exclusive);
}

static void
ligma_stroke_real_anchor_select (LigmaStroke *stroke,
                                LigmaAnchor *anchor,
                                gboolean    selected,
                                gboolean    exclusive)
{
  GList *list = stroke->anchors->head;

  if (exclusive)
    {
      while (list)
        {
          LIGMA_ANCHOR (list->data)->selected = FALSE;
          list = g_list_next (list);
        }
    }

  list = g_queue_find (stroke->anchors, anchor);

  if (list)
    LIGMA_ANCHOR (list->data)->selected = selected;
}


void
ligma_stroke_anchor_move_relative (LigmaStroke            *stroke,
                                  LigmaAnchor            *anchor,
                                  const LigmaCoords      *delta,
                                  LigmaAnchorFeatureType  feature)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (g_queue_find (stroke->anchors, anchor));

  LIGMA_STROKE_GET_CLASS (stroke)->anchor_move_relative (stroke, anchor,
                                                        delta, feature);
}

static void
ligma_stroke_real_anchor_move_relative (LigmaStroke            *stroke,
                                       LigmaAnchor            *anchor,
                                       const LigmaCoords      *delta,
                                       LigmaAnchorFeatureType  feature)
{
  anchor->position.x += delta->x;
  anchor->position.y += delta->y;
}


void
ligma_stroke_anchor_move_absolute (LigmaStroke            *stroke,
                                  LigmaAnchor            *anchor,
                                  const LigmaCoords      *coord,
                                  LigmaAnchorFeatureType  feature)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (g_queue_find (stroke->anchors, anchor));

  LIGMA_STROKE_GET_CLASS (stroke)->anchor_move_absolute (stroke, anchor,
                                                        coord, feature);
}

static void
ligma_stroke_real_anchor_move_absolute (LigmaStroke            *stroke,
                                       LigmaAnchor            *anchor,
                                       const LigmaCoords      *coord,
                                       LigmaAnchorFeatureType  feature)
{
  anchor->position.x = coord->x;
  anchor->position.y = coord->y;
}

gboolean
ligma_stroke_point_is_movable (LigmaStroke *stroke,
                              LigmaAnchor *predec,
                              gdouble     position)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);

  return LIGMA_STROKE_GET_CLASS (stroke)->point_is_movable (stroke, predec,
                                                           position);
}


static gboolean
ligma_stroke_real_point_is_movable (LigmaStroke *stroke,
                                   LigmaAnchor *predec,
                                   gdouble     position)
{
  return FALSE;
}


void
ligma_stroke_point_move_relative (LigmaStroke            *stroke,
                                 LigmaAnchor            *predec,
                                 gdouble                position,
                                 const LigmaCoords      *deltacoord,
                                 LigmaAnchorFeatureType  feature)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  LIGMA_STROKE_GET_CLASS (stroke)->point_move_relative (stroke, predec,
                                                       position, deltacoord,
                                                       feature);
}


static void
ligma_stroke_real_point_move_relative (LigmaStroke           *stroke,
                                      LigmaAnchor           *predec,
                                      gdouble               position,
                                      const LigmaCoords     *deltacoord,
                                      LigmaAnchorFeatureType feature)
{
  g_printerr ("ligma_stroke_point_move_relative: default implementation\n");
}


void
ligma_stroke_point_move_absolute (LigmaStroke            *stroke,
                                 LigmaAnchor            *predec,
                                 gdouble                position,
                                 const LigmaCoords      *coord,
                                 LigmaAnchorFeatureType  feature)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  LIGMA_STROKE_GET_CLASS (stroke)->point_move_absolute (stroke, predec,
                                                       position, coord,
                                                       feature);
}

static void
ligma_stroke_real_point_move_absolute (LigmaStroke           *stroke,
                                      LigmaAnchor           *predec,
                                      gdouble               position,
                                      const LigmaCoords     *coord,
                                      LigmaAnchorFeatureType feature)
{
  g_printerr ("ligma_stroke_point_move_absolute: default implementation\n");
}


void
ligma_stroke_close (LigmaStroke *stroke)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));
  g_return_if_fail (g_queue_is_empty (stroke->anchors) == FALSE);

  LIGMA_STROKE_GET_CLASS (stroke)->close (stroke);
}

static void
ligma_stroke_real_close (LigmaStroke *stroke)
{
  stroke->closed = TRUE;
  g_object_notify (G_OBJECT (stroke), "closed");
}


void
ligma_stroke_anchor_convert (LigmaStroke            *stroke,
                            LigmaAnchor            *anchor,
                            LigmaAnchorFeatureType  feature)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  LIGMA_STROKE_GET_CLASS (stroke)->anchor_convert (stroke, anchor, feature);
}

static void
ligma_stroke_real_anchor_convert (LigmaStroke            *stroke,
                                 LigmaAnchor            *anchor,
                                 LigmaAnchorFeatureType  feature)
{
  g_printerr ("ligma_stroke_anchor_convert: default implementation\n");
}


void
ligma_stroke_anchor_delete (LigmaStroke *stroke,
                           LigmaAnchor *anchor)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));
  g_return_if_fail (anchor && anchor->type == LIGMA_ANCHOR_ANCHOR);

  LIGMA_STROKE_GET_CLASS (stroke)->anchor_delete (stroke, anchor);
}

static void
ligma_stroke_real_anchor_delete (LigmaStroke *stroke,
                                LigmaAnchor *anchor)
{
  g_printerr ("ligma_stroke_anchor_delete: default implementation\n");
}

LigmaStroke *
ligma_stroke_open (LigmaStroke *stroke,
                  LigmaAnchor *end_anchor)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);
  g_return_val_if_fail (end_anchor &&
                        end_anchor->type == LIGMA_ANCHOR_ANCHOR, NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->open (stroke, end_anchor);
}

static LigmaStroke *
ligma_stroke_real_open (LigmaStroke *stroke,
                       LigmaAnchor *end_anchor)
{
  g_printerr ("ligma_stroke_open: default implementation\n");
  return NULL;
}

gboolean
ligma_stroke_anchor_is_insertable (LigmaStroke *stroke,
                                  LigmaAnchor *predec,
                                  gdouble     position)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);

  return LIGMA_STROKE_GET_CLASS (stroke)->anchor_is_insertable (stroke,
                                                               predec,
                                                               position);
}

static gboolean
ligma_stroke_real_anchor_is_insertable (LigmaStroke *stroke,
                                       LigmaAnchor *predec,
                                       gdouble     position)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);

  return FALSE;
}

LigmaAnchor *
ligma_stroke_anchor_insert (LigmaStroke *stroke,
                           LigmaAnchor *predec,
                           gdouble     position)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);
  g_return_val_if_fail (predec->type == LIGMA_ANCHOR_ANCHOR, NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->anchor_insert (stroke,
                                                        predec, position);
}

static LigmaAnchor *
ligma_stroke_real_anchor_insert (LigmaStroke *stroke,
                                LigmaAnchor *predec,
                                gdouble     position)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);

  return NULL;
}


gboolean
ligma_stroke_is_extendable (LigmaStroke *stroke,
                           LigmaAnchor *neighbor)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);

  return LIGMA_STROKE_GET_CLASS (stroke)->is_extendable (stroke, neighbor);
}

static gboolean
ligma_stroke_real_is_extendable (LigmaStroke *stroke,
                                LigmaAnchor *neighbor)
{
  return FALSE;
}


LigmaAnchor *
ligma_stroke_extend (LigmaStroke           *stroke,
                    const LigmaCoords     *coords,
                    LigmaAnchor           *neighbor,
                    LigmaVectorExtendMode  extend_mode)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);
  g_return_val_if_fail (!stroke->closed, NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->extend (stroke, coords,
                                                 neighbor, extend_mode);
}

static LigmaAnchor *
ligma_stroke_real_extend (LigmaStroke           *stroke,
                         const LigmaCoords     *coords,
                         LigmaAnchor           *neighbor,
                         LigmaVectorExtendMode  extend_mode)
{
  g_printerr ("ligma_stroke_extend: default implementation\n");
  return NULL;
}

gboolean
ligma_stroke_connect_stroke (LigmaStroke *stroke,
                            LigmaAnchor *anchor,
                            LigmaStroke *extension,
                            LigmaAnchor *neighbor)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (LIGMA_IS_STROKE (extension), FALSE);
  g_return_val_if_fail (stroke->closed == FALSE &&
                        extension->closed == FALSE, FALSE);

  return LIGMA_STROKE_GET_CLASS (stroke)->connect_stroke (stroke, anchor,
                                                         extension, neighbor);
}

gboolean
ligma_stroke_real_connect_stroke (LigmaStroke *stroke,
                                 LigmaAnchor *anchor,
                                 LigmaStroke *extension,
                                 LigmaAnchor *neighbor)
{
  g_printerr ("ligma_stroke_connect_stroke: default implementation\n");
  return FALSE;
}

gboolean
ligma_stroke_is_empty (LigmaStroke *stroke)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);

  return LIGMA_STROKE_GET_CLASS (stroke)->is_empty (stroke);
}

static gboolean
ligma_stroke_real_is_empty (LigmaStroke *stroke)
{
  return g_queue_is_empty (stroke->anchors);
}


gboolean
ligma_stroke_reverse (LigmaStroke *stroke)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);

  return LIGMA_STROKE_GET_CLASS (stroke)->reverse (stroke);
}


static gboolean
ligma_stroke_real_reverse (LigmaStroke *stroke)
{
  g_queue_reverse (stroke->anchors);

  /* keep the first node the same for closed strokes */
  if (stroke->closed && stroke->anchors->length > 0)
    g_queue_push_head_link (stroke->anchors,
                            g_queue_pop_tail_link (stroke->anchors));

  return TRUE;
}


gboolean
ligma_stroke_shift_start (LigmaStroke *stroke,
                         LigmaAnchor *new_start)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (new_start != NULL, FALSE);

  return LIGMA_STROKE_GET_CLASS (stroke)->shift_start (stroke, new_start);
}

static gboolean
ligma_stroke_real_shift_start (LigmaStroke *stroke,
                              LigmaAnchor *new_start)
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
ligma_stroke_get_length (LigmaStroke *stroke,
                        gdouble     precision)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), 0.0);

  return LIGMA_STROKE_GET_CLASS (stroke)->get_length (stroke, precision);
}

static gdouble
ligma_stroke_real_get_length (LigmaStroke *stroke,
                             gdouble     precision)
{
  GArray     *points;
  gint        i;
  gdouble     length;
  LigmaCoords  difference;

  if (g_queue_is_empty (stroke->anchors))
    return -1;

  points = ligma_stroke_interpolate (stroke, precision, NULL);
  if (points == NULL)
    return -1;

  length = 0;

  for (i = 0; i < points->len - 1; i++ )
    {
       ligma_coords_difference (&(g_array_index (points, LigmaCoords, i)),
                               &(g_array_index (points, LigmaCoords, i+1)),
                               &difference);
       length += ligma_coords_length (&difference);
    }

  g_array_free(points, TRUE);

  return length;
}


gdouble
ligma_stroke_get_distance (LigmaStroke       *stroke,
                          const LigmaCoords *coord)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), 0.0);

  return LIGMA_STROKE_GET_CLASS (stroke)->get_distance (stroke, coord);
}

static gdouble
ligma_stroke_real_get_distance (LigmaStroke       *stroke,
                               const LigmaCoords *coord)
{
  g_printerr ("ligma_stroke_get_distance: default implementation\n");

  return 0.0;
}


GArray *
ligma_stroke_interpolate (LigmaStroke *stroke,
                         gdouble     precision,
                         gboolean   *ret_closed)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->interpolate (stroke, precision,
                                                      ret_closed);
}

static GArray *
ligma_stroke_real_interpolate (LigmaStroke *stroke,
                              gdouble     precision,
                              gboolean   *ret_closed)
{
  g_printerr ("ligma_stroke_interpolate: default implementation\n");

  return NULL;
}

LigmaStroke *
ligma_stroke_duplicate (LigmaStroke *stroke)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->duplicate (stroke);
}

static LigmaStroke *
ligma_stroke_real_duplicate (LigmaStroke *stroke)
{
  LigmaStroke *new_stroke;
  GList      *list;

  new_stroke = g_object_new (G_TYPE_FROM_INSTANCE (stroke),
                             "name", ligma_object_get_name (stroke),
                             NULL);

  g_queue_free_full (new_stroke->anchors, (GDestroyNotify) ligma_anchor_free);
  new_stroke->anchors = g_queue_copy (stroke->anchors);

  for (list = new_stroke->anchors->head; list; list = g_list_next (list))
    {
      list->data = ligma_anchor_copy (LIGMA_ANCHOR (list->data));
    }

  new_stroke->closed = stroke->closed;
  /* we do *not* copy the ID! */

  return new_stroke;
}


LigmaBezierDesc *
ligma_stroke_make_bezier (LigmaStroke *stroke)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->make_bezier (stroke);
}

static LigmaBezierDesc *
ligma_stroke_real_make_bezier (LigmaStroke *stroke)
{
  g_printerr ("ligma_stroke_make_bezier: default implementation\n");

  return NULL;
}


void
ligma_stroke_translate (LigmaStroke *stroke,
                       gdouble     offset_x,
                       gdouble     offset_y)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  LIGMA_STROKE_GET_CLASS (stroke)->translate (stroke, offset_x, offset_y);
}

static void
ligma_stroke_real_translate (LigmaStroke *stroke,
                            gdouble     offset_x,
                            gdouble     offset_y)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      LigmaAnchor *anchor = list->data;

      anchor->position.x += offset_x;
      anchor->position.y += offset_y;
    }
}


void
ligma_stroke_scale (LigmaStroke *stroke,
                   gdouble     scale_x,
                   gdouble     scale_y)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  LIGMA_STROKE_GET_CLASS (stroke)->scale (stroke, scale_x, scale_y);
}

static void
ligma_stroke_real_scale (LigmaStroke *stroke,
                        gdouble     scale_x,
                        gdouble     scale_y)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      LigmaAnchor *anchor = list->data;

      anchor->position.x *= scale_x;
      anchor->position.y *= scale_y;
    }
}

void
ligma_stroke_rotate (LigmaStroke *stroke,
                    gdouble     center_x,
                    gdouble     center_y,
                    gdouble     angle)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  LIGMA_STROKE_GET_CLASS (stroke)->rotate (stroke, center_x, center_y, angle);
}

static void
ligma_stroke_real_rotate (LigmaStroke *stroke,
                         gdouble     center_x,
                         gdouble     center_y,
                         gdouble     angle)
{
  LigmaMatrix3  matrix;

  angle = angle / 180.0 * G_PI;
  ligma_matrix3_identity (&matrix);
  ligma_transform_matrix_rotate_center (&matrix, center_x, center_y, angle);

  ligma_stroke_transform (stroke, &matrix, NULL);
}

void
ligma_stroke_flip (LigmaStroke          *stroke,
                  LigmaOrientationType  flip_type,
                  gdouble              axis)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  LIGMA_STROKE_GET_CLASS (stroke)->flip (stroke, flip_type, axis);
}

static void
ligma_stroke_real_flip (LigmaStroke          *stroke,
                       LigmaOrientationType  flip_type,
                       gdouble              axis)
{
  LigmaMatrix3  matrix;

  ligma_matrix3_identity (&matrix);
  ligma_transform_matrix_flip (&matrix, flip_type, axis);
  ligma_stroke_transform (stroke, &matrix, NULL);
}

void
ligma_stroke_flip_free (LigmaStroke *stroke,
                       gdouble     x1,
                       gdouble     y1,
                       gdouble     x2,
                       gdouble     y2)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  LIGMA_STROKE_GET_CLASS (stroke)->flip_free (stroke, x1, y1, x2, y2);
}

static void
ligma_stroke_real_flip_free (LigmaStroke *stroke,
                            gdouble     x1,
                            gdouble     y1,
                            gdouble     x2,
                            gdouble     y2)
{
  /* x, y, width and height parameter in ligma_transform_matrix_flip_free are unused */
  LigmaMatrix3  matrix;

  ligma_matrix3_identity (&matrix);
  ligma_transform_matrix_flip_free (&matrix, x1, y1, x2, y2);

  ligma_stroke_transform (stroke, &matrix, NULL);
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
ligma_stroke_transform (LigmaStroke        *stroke,
                       const LigmaMatrix3 *matrix,
                       GQueue            *ret_strokes)
{
  g_return_if_fail (LIGMA_IS_STROKE (stroke));

  LIGMA_STROKE_GET_CLASS (stroke)->transform (stroke, matrix, ret_strokes);
}

static void
ligma_stroke_real_transform (LigmaStroke        *stroke,
                            const LigmaMatrix3 *matrix,
                            GQueue            *ret_strokes)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      LigmaAnchor *anchor = list->data;

      ligma_matrix3_transform_point (matrix,
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
ligma_stroke_get_draw_anchors (LigmaStroke *stroke)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->get_draw_anchors (stroke);
}

static GList *
ligma_stroke_real_get_draw_anchors (LigmaStroke *stroke)
{
  GList *list;
  GList *ret_list = NULL;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      if (LIGMA_ANCHOR (list->data)->type == LIGMA_ANCHOR_ANCHOR)
        ret_list = g_list_prepend (ret_list, list->data);
    }

  return g_list_reverse (ret_list);
}


GList *
ligma_stroke_get_draw_controls (LigmaStroke *stroke)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->get_draw_controls (stroke);
}

static GList *
ligma_stroke_real_get_draw_controls (LigmaStroke *stroke)
{
  GList *list;
  GList *ret_list = NULL;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      LigmaAnchor *anchor = list->data;

      if (anchor->type == LIGMA_ANCHOR_CONTROL)
        {
          LigmaAnchor *next = list->next ? list->next->data : NULL;
          LigmaAnchor *prev = list->prev ? list->prev->data : NULL;

          if (next && next->type == LIGMA_ANCHOR_ANCHOR && next->selected)
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
          else if (prev && prev->type == LIGMA_ANCHOR_ANCHOR && prev->selected)
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
ligma_stroke_get_draw_lines (LigmaStroke *stroke)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->get_draw_lines (stroke);
}

static GArray *
ligma_stroke_real_get_draw_lines (LigmaStroke *stroke)
{
  GList  *list;
  GArray *ret_lines = NULL;
  gint    count = 0;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      LigmaAnchor *anchor = list->data;

      if (anchor->type == LIGMA_ANCHOR_ANCHOR && anchor->selected)
        {
          if (list->next)
            {
              LigmaAnchor *next = list->next->data;

              if (count == 0)
                ret_lines = g_array_new (FALSE, FALSE, sizeof (LigmaCoords));

              ret_lines = g_array_append_val (ret_lines, anchor->position);
              ret_lines = g_array_append_val (ret_lines, next->position);
              count += 1;
            }

          if (list->prev)
            {
              LigmaAnchor *prev = list->prev->data;

              if (count == 0)
                ret_lines = g_array_new (FALSE, FALSE, sizeof (LigmaCoords));

              ret_lines = g_array_append_val (ret_lines, anchor->position);
              ret_lines = g_array_append_val (ret_lines, prev->position);
              count += 1;
            }
        }
    }

  return ret_lines;
}

GArray *
ligma_stroke_control_points_get (LigmaStroke *stroke,
                                gboolean   *ret_closed)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), NULL);

  return LIGMA_STROKE_GET_CLASS (stroke)->control_points_get (stroke,
                                                             ret_closed);
}

static GArray *
ligma_stroke_real_control_points_get (LigmaStroke *stroke,
                                     gboolean   *ret_closed)
{
  guint   num_anchors;
  GArray *ret_array;
  GList  *list;

  num_anchors = g_queue_get_length (stroke->anchors);
  ret_array = g_array_sized_new (FALSE, FALSE,
                                 sizeof (LigmaAnchor), num_anchors);

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      g_array_append_vals (ret_array, list->data, 1);
    }

  if (ret_closed)
    *ret_closed = stroke->closed;

  return ret_array;
}

gboolean
ligma_stroke_get_point_at_dist (LigmaStroke *stroke,
                               gdouble     dist,
                               gdouble     precision,
                               LigmaCoords *position,
                               gdouble    *slope)
{
  g_return_val_if_fail (LIGMA_IS_STROKE (stroke), FALSE);

  return LIGMA_STROKE_GET_CLASS (stroke)->get_point_at_dist (stroke,
                                                            dist,
                                                            precision,
                                                            position,
                                                            slope);
}


static gboolean
ligma_stroke_real_get_point_at_dist (LigmaStroke *stroke,
                                    gdouble     dist,
                                    gdouble     precision,
                                    LigmaCoords *position,
                                    gdouble    *slope)
{
  GArray     *points;
  gint        i;
  gdouble     length;
  gdouble     segment_length;
  gboolean    ret = FALSE;
  LigmaCoords  difference;

  points = ligma_stroke_interpolate (stroke, precision, NULL);
  if (points == NULL)
    return ret;

  length = 0;
  for (i=0; i < points->len - 1; i++)
    {
      ligma_coords_difference (&(g_array_index (points, LigmaCoords , i)),
                              &(g_array_index (points, LigmaCoords , i+1)),
                              &difference);
      segment_length = ligma_coords_length (&difference);

      if (segment_length == 0 || length + segment_length < dist )
        {
          length += segment_length;
        }
      else
        {
          /* x = x1 + (x2 - x1 ) u  */
          /* x   = x1 (1-u) + u x2  */

          gdouble u = (dist - length) / segment_length;

          ligma_coords_mix (1 - u, &(g_array_index (points, LigmaCoords , i)),
                               u, &(g_array_index (points, LigmaCoords , i+1)),
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
