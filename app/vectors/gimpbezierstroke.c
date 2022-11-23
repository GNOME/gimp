/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabezierstroke.c
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

#include <glib-object.h>
#include <cairo.h>

#include "libligmamath/ligmamath.h"

#include "vectors-types.h"

#include "core/ligma-transform-utils.h"
#include "core/ligmabezierdesc.h"
#include "core/ligmacoords.h"
#include "core/ligmacoords-interpolate.h"

#include "ligmaanchor.h"
#include "ligmabezierstroke.h"


/*  local prototypes  */

static gdouble
    ligma_bezier_stroke_nearest_point_get   (LigmaStroke            *stroke,
                                            const LigmaCoords      *coord,
                                            gdouble                precision,
                                            LigmaCoords            *ret_point,
                                            LigmaAnchor           **ret_segment_start,
                                            LigmaAnchor           **ret_segment_end,
                                            gdouble               *ret_pos);
static gdouble
    ligma_bezier_stroke_segment_nearest_point_get
                                           (const LigmaCoords      *beziercoords,
                                            const LigmaCoords      *coord,
                                            gdouble                precision,
                                            LigmaCoords            *ret_point,
                                            gdouble               *ret_pos,
                                            gint                   depth);
static gdouble
    ligma_bezier_stroke_nearest_tangent_get (LigmaStroke            *stroke,
                                            const LigmaCoords      *coord1,
                                            const LigmaCoords      *coord2,
                                            gdouble                precision,
                                            LigmaCoords            *nearest,
                                            LigmaAnchor           **ret_segment_start,
                                            LigmaAnchor           **ret_segment_end,
                                            gdouble               *ret_pos);
static gdouble
    ligma_bezier_stroke_segment_nearest_tangent_get
                                           (const LigmaCoords      *beziercoords,
                                            const LigmaCoords      *coord1,
                                            const LigmaCoords      *coord2,
                                            gdouble                precision,
                                            LigmaCoords            *ret_point,
                                            gdouble               *ret_pos);
static void
    ligma_bezier_stroke_anchor_move_relative
                                           (LigmaStroke            *stroke,
                                            LigmaAnchor            *anchor,
                                            const LigmaCoords      *deltacoord,
                                            LigmaAnchorFeatureType  feature);
static void
    ligma_bezier_stroke_anchor_move_absolute
                                           (LigmaStroke            *stroke,
                                            LigmaAnchor            *anchor,
                                            const LigmaCoords      *coord,
                                            LigmaAnchorFeatureType  feature);
static void
    ligma_bezier_stroke_anchor_convert      (LigmaStroke            *stroke,
                                            LigmaAnchor            *anchor,
                                            LigmaAnchorFeatureType  feature);
static void
    ligma_bezier_stroke_anchor_delete       (LigmaStroke            *stroke,
                                            LigmaAnchor            *anchor);
static gboolean
    ligma_bezier_stroke_point_is_movable    (LigmaStroke            *stroke,
                                            LigmaAnchor            *predec,
                                            gdouble                position);
static void
    ligma_bezier_stroke_point_move_relative (LigmaStroke            *stroke,
                                            LigmaAnchor            *predec,
                                            gdouble                position,
                                            const LigmaCoords      *deltacoord,
                                            LigmaAnchorFeatureType  feature);
static void
    ligma_bezier_stroke_point_move_absolute (LigmaStroke            *stroke,
                                            LigmaAnchor            *predec,
                                            gdouble                position,
                                            const LigmaCoords      *coord,
                                            LigmaAnchorFeatureType  feature);

static void ligma_bezier_stroke_close       (LigmaStroke            *stroke);

static LigmaStroke *
    ligma_bezier_stroke_open                (LigmaStroke            *stroke,
                                            LigmaAnchor            *end_anchor);
static gboolean
    ligma_bezier_stroke_anchor_is_insertable
                                           (LigmaStroke            *stroke,
                                            LigmaAnchor            *predec,
                                            gdouble                position);
static LigmaAnchor *
    ligma_bezier_stroke_anchor_insert       (LigmaStroke            *stroke,
                                            LigmaAnchor            *predec,
                                            gdouble                position);
static gboolean
    ligma_bezier_stroke_is_extendable       (LigmaStroke            *stroke,
                                            LigmaAnchor            *neighbor);
static gboolean
    ligma_bezier_stroke_connect_stroke      (LigmaStroke            *stroke,
                                            LigmaAnchor            *anchor,
                                            LigmaStroke            *extension,
                                            LigmaAnchor            *neighbor);
static gboolean
    ligma_bezier_stroke_reverse             (LigmaStroke            *stroke);
static gboolean
    ligma_bezier_stroke_shift_start         (LigmaStroke            *stroke,
                                            LigmaAnchor            *anchor);
static GArray *
    ligma_bezier_stroke_interpolate         (LigmaStroke            *stroke,
                                            gdouble                precision,
                                            gboolean              *closed);
static LigmaBezierDesc *
    ligma_bezier_stroke_make_bezier         (LigmaStroke            *stroke);
static void ligma_bezier_stroke_transform   (LigmaStroke            *stroke,
                                            const LigmaMatrix3     *matrix,
                                            GQueue                *ret_strokes);

static void ligma_bezier_stroke_finalize    (GObject               *object);


static GList * ligma_bezier_stroke_get_anchor_listitem
                                           (GList                 *list);


G_DEFINE_TYPE (LigmaBezierStroke, ligma_bezier_stroke, LIGMA_TYPE_STROKE)

#define parent_class ligma_bezier_stroke_parent_class


static void
ligma_bezier_stroke_class_init (LigmaBezierStrokeClass *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  LigmaStrokeClass *stroke_class = LIGMA_STROKE_CLASS (klass);

  object_class->finalize             = ligma_bezier_stroke_finalize;

  stroke_class->nearest_point_get    = ligma_bezier_stroke_nearest_point_get;
  stroke_class->nearest_tangent_get  = ligma_bezier_stroke_nearest_tangent_get;
  stroke_class->nearest_intersection_get = NULL;
  stroke_class->anchor_move_relative = ligma_bezier_stroke_anchor_move_relative;
  stroke_class->anchor_move_absolute = ligma_bezier_stroke_anchor_move_absolute;
  stroke_class->anchor_convert       = ligma_bezier_stroke_anchor_convert;
  stroke_class->anchor_delete        = ligma_bezier_stroke_anchor_delete;
  stroke_class->point_is_movable     = ligma_bezier_stroke_point_is_movable;
  stroke_class->point_move_relative  = ligma_bezier_stroke_point_move_relative;
  stroke_class->point_move_absolute  = ligma_bezier_stroke_point_move_absolute;
  stroke_class->close                = ligma_bezier_stroke_close;
  stroke_class->open                 = ligma_bezier_stroke_open;
  stroke_class->anchor_is_insertable = ligma_bezier_stroke_anchor_is_insertable;
  stroke_class->anchor_insert        = ligma_bezier_stroke_anchor_insert;
  stroke_class->is_extendable        = ligma_bezier_stroke_is_extendable;
  stroke_class->extend               = ligma_bezier_stroke_extend;
  stroke_class->connect_stroke       = ligma_bezier_stroke_connect_stroke;
  stroke_class->reverse              = ligma_bezier_stroke_reverse;
  stroke_class->shift_start          = ligma_bezier_stroke_shift_start;
  stroke_class->interpolate          = ligma_bezier_stroke_interpolate;
  stroke_class->make_bezier          = ligma_bezier_stroke_make_bezier;
  stroke_class->transform            = ligma_bezier_stroke_transform;
}

static void
ligma_bezier_stroke_init (LigmaBezierStroke *stroke)
{
}

static void
ligma_bezier_stroke_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* Bezier specific functions */

LigmaStroke *
ligma_bezier_stroke_new (void)
{
  return g_object_new (LIGMA_TYPE_BEZIER_STROKE, NULL);
}


LigmaStroke *
ligma_bezier_stroke_new_from_coords (const LigmaCoords *coords,
                                    gint              n_coords,
                                    gboolean          closed)
{
  LigmaStroke *stroke;
  LigmaAnchor *last_anchor;
  gint        count;

  g_return_val_if_fail (coords != NULL, NULL);
  g_return_val_if_fail (n_coords >= 3, NULL);
  g_return_val_if_fail ((n_coords % 3) == 0, NULL);

  stroke = ligma_bezier_stroke_new ();

  last_anchor = NULL;

  for (count = 0; count < n_coords; count++)
    last_anchor = ligma_bezier_stroke_extend (stroke,
                                             &coords[count],
                                             last_anchor,
                                             EXTEND_SIMPLE);

  if (closed)
    ligma_stroke_close (stroke);

  return stroke;
}

static void
ligma_bezier_stroke_anchor_delete (LigmaStroke *stroke,
                                  LigmaAnchor *anchor)
{
  GList *list;
  GList *list2;
  gint   i;

  /* Anchors always are surrounded by two handles that have to
   * be deleted too
   */

  list2 = g_queue_find (stroke->anchors, anchor);
  list = g_list_previous (list2);

  for (i = 0; i < 3; i++)
    {
      g_return_if_fail (list != NULL);

      list2 = g_list_next (list);
      ligma_anchor_free (list->data);
      g_queue_delete_link (stroke->anchors, list);
      list = list2;
    }
}

static LigmaStroke *
ligma_bezier_stroke_open (LigmaStroke *stroke,
                         LigmaAnchor *end_anchor)
{
  GList      *list;
  GList      *list2;
  LigmaStroke *new_stroke = NULL;

  list = g_queue_find (stroke->anchors, end_anchor);

  g_return_val_if_fail (list != NULL && list->next != NULL, NULL);

  list = g_list_next (list);  /* protect the handle... */

  list2 = list->next;
  list->next = NULL;

  if (list2 != NULL)
    {
      GList *tail = stroke->anchors->tail;

      stroke->anchors->tail = list;
      stroke->anchors->length -= g_list_length (list2);

      list2->prev = NULL;

      if (stroke->closed)
        {
          GList *l;

          for (l = tail; l; l = g_list_previous (l))
            g_queue_push_head (stroke->anchors, l->data);

          g_list_free (list2);
        }
      else
        {
          new_stroke = ligma_bezier_stroke_new ();
          new_stroke->anchors->head = list2;
          new_stroke->anchors->tail = g_list_last (list2);
          new_stroke->anchors->length = g_list_length (list2);
        }
    }

  stroke->closed = FALSE;
  g_object_notify (G_OBJECT (stroke), "closed");

  return new_stroke;
}

static gboolean
ligma_bezier_stroke_anchor_is_insertable (LigmaStroke *stroke,
                                         LigmaAnchor *predec,
                                         gdouble     position)
{
  return (g_queue_find (stroke->anchors, predec) != NULL);
}


static LigmaAnchor *
ligma_bezier_stroke_anchor_insert (LigmaStroke *stroke,
                                  LigmaAnchor *predec,
                                  gdouble     position)
{
  GList      *segment_start;
  GList      *list;
  GList      *list2;
  LigmaCoords  subdivided[8];
  LigmaCoords  beziercoords[4];
  gint        i;

  segment_start = g_queue_find (stroke->anchors, predec);

  if (! segment_start)
    return NULL;

  list = segment_start;

  for (i = 0; i <= 3; i++)
    {
      beziercoords[i] = LIGMA_ANCHOR (list->data)->position;
      list = g_list_next (list);
      if (! list)
        list = stroke->anchors->head;
    }

  subdivided[0] = beziercoords[0];
  subdivided[6] = beziercoords[3];

  ligma_coords_mix (1-position, &(beziercoords[0]),
                   position,   &(beziercoords[1]),
                   &(subdivided[1]));

  ligma_coords_mix (1-position, &(beziercoords[1]),
                   position,   &(beziercoords[2]),
                   &(subdivided[7]));

  ligma_coords_mix (1-position, &(beziercoords[2]),
                   position,   &(beziercoords[3]),
                   &(subdivided[5]));

  ligma_coords_mix (1-position, &(subdivided[1]),
                   position,   &(subdivided[7]),
                   &(subdivided[2]));

  ligma_coords_mix (1-position, &(subdivided[7]),
                   position,   &(subdivided[5]),
                   &(subdivided[4]));

  ligma_coords_mix (1-position, &(subdivided[2]),
                   position,   &(subdivided[4]),
                   &(subdivided[3]));

  /* subdivided 0-6 contains the bezier segment subdivided at <position> */

  list = segment_start;

  for (i = 0; i <= 6; i++)
    {
      if (i >= 2 && i <= 4)
        {
          list2 = g_list_append (NULL,
                                 ligma_anchor_new ((i == 3 ?
                                                    LIGMA_ANCHOR_ANCHOR:
                                                    LIGMA_ANCHOR_CONTROL),
                                                  &(subdivided[i])));
          /* insert it *before* list manually. */
          list2->next = list;
          list2->prev = list->prev;
          if (list->prev)
            list->prev->next = list2;
          list->prev = list2;

          list = list2;

          if (i == 3)
            segment_start = list;
        }
      else
        {
          LIGMA_ANCHOR (list->data)->position = subdivided[i];
        }

      list = g_list_next (list);
      if (! list)
        list = stroke->anchors->head;
    }

  stroke->anchors->head = g_list_first (list);
  stroke->anchors->tail = g_list_last (list);
  stroke->anchors->length += 3;

  return LIGMA_ANCHOR (segment_start->data);
}


static gboolean
ligma_bezier_stroke_point_is_movable (LigmaStroke *stroke,
                                     LigmaAnchor *predec,
                                     gdouble     position)
{
  return (g_queue_find (stroke->anchors, predec) != NULL);
}


static void
ligma_bezier_stroke_point_move_relative (LigmaStroke            *stroke,
                                        LigmaAnchor            *predec,
                                        gdouble                position,
                                        const LigmaCoords      *deltacoord,
                                        LigmaAnchorFeatureType  feature)
{
  LigmaCoords  offsetcoords[2];
  GList      *segment_start;
  GList      *list;
  gint        i;
  gdouble     feel_good;

  segment_start = g_queue_find (stroke->anchors, predec);

  g_return_if_fail (segment_start != NULL);

  /* dragging close to endpoints just moves the handle related to
   * the endpoint. Just make sure that feel_good is in the range from
   * 0 to 1. The 1.0 / 6.0 and 5.0 / 6.0 are duplicated in
   * tools/ligmavectortool.c.
   */
  if (position <= 1.0 / 6.0)
    feel_good = 0;
  else if (position <= 0.5)
    feel_good = (pow((6 * position - 1) / 2.0, 3)) / 2;
  else if (position <= 5.0 / 6.0)
    feel_good = (1 - pow((6 * (1-position) - 1) / 2.0, 3)) / 2 + 0.5;
  else
    feel_good = 1;

  ligma_coords_scale ((1-feel_good)/(3*position*
                                    (1-position)*(1-position)),
                     deltacoord,
                     &(offsetcoords[0]));
  ligma_coords_scale (feel_good/(3*position*position*(1-position)),
                     deltacoord,
                     &(offsetcoords[1]));

  list = segment_start;
  list = g_list_next (list);
  if (! list)
    list = stroke->anchors->head;

  for (i = 0; i <= 1; i++)
    {
      ligma_stroke_anchor_move_relative (stroke, LIGMA_ANCHOR (list->data),
                                        &(offsetcoords[i]), feature);
      list = g_list_next (list);
      if (! list)
        list = stroke->anchors->head;
    }
}


static void
ligma_bezier_stroke_point_move_absolute (LigmaStroke            *stroke,
                                        LigmaAnchor            *predec,
                                        gdouble                position,
                                        const LigmaCoords      *coord,
                                        LigmaAnchorFeatureType  feature)
{
  LigmaCoords  deltacoord;
  LigmaCoords  tmp1, tmp2, abs_pos;
  LigmaCoords  beziercoords[4];
  GList      *segment_start;
  GList      *list;
  gint        i;

  segment_start = g_queue_find (stroke->anchors, predec);

  g_return_if_fail (segment_start != NULL);

  list = segment_start;

  for (i = 0; i <= 3; i++)
    {
      beziercoords[i] = LIGMA_ANCHOR (list->data)->position;
      list = g_list_next (list);
      if (! list)
        list = stroke->anchors->head;
    }

  ligma_coords_mix ((1-position)*(1-position)*(1-position), &(beziercoords[0]),
                   3*(1-position)*(1-position)*position, &(beziercoords[1]),
                   &tmp1);
  ligma_coords_mix (3*(1-position)*position*position, &(beziercoords[2]),
                   position*position*position, &(beziercoords[3]),
                   &tmp2);
  ligma_coords_add (&tmp1, &tmp2, &abs_pos);

  ligma_coords_difference (coord, &abs_pos, &deltacoord);

  ligma_bezier_stroke_point_move_relative (stroke, predec, position,
                                          &deltacoord, feature);
}

static void
ligma_bezier_stroke_close (LigmaStroke *stroke)
{
  GList      *start;
  GList      *end;
  LigmaAnchor *anchor;

  start = stroke->anchors->head;
  end   = stroke->anchors->tail;

  g_return_if_fail (start->next != NULL && end->prev != NULL);

  if (start->next != end->prev)
    {
      if (ligma_coords_equal (&(LIGMA_ANCHOR (start->next->data)->position),
                             &(LIGMA_ANCHOR (start->data)->position)) &&
          ligma_coords_equal (&(LIGMA_ANCHOR (start->data)->position),
                             &(LIGMA_ANCHOR (end->data)->position)) &&
          ligma_coords_equal (&(LIGMA_ANCHOR (end->data)->position),
                             &(LIGMA_ANCHOR (end->prev->data)->position)))
        {
          /* redundant segment */

          ligma_anchor_free (stroke->anchors->tail->data);
          g_queue_delete_link (stroke->anchors, stroke->anchors->tail);

          ligma_anchor_free (stroke->anchors->tail->data);
          g_queue_delete_link (stroke->anchors, stroke->anchors->tail);

          anchor = stroke->anchors->tail->data;
          g_queue_delete_link (stroke->anchors, stroke->anchors->tail);

          ligma_anchor_free (stroke->anchors->head->data);
          stroke->anchors->head->data = anchor;
        }
    }

  LIGMA_STROKE_CLASS (parent_class)->close (stroke);
}

static gdouble
ligma_bezier_stroke_nearest_point_get (LigmaStroke        *stroke,
                                      const LigmaCoords  *coord,
                                      gdouble            precision,
                                      LigmaCoords        *ret_point,
                                      LigmaAnchor       **ret_segment_start,
                                      LigmaAnchor       **ret_segment_end,
                                      gdouble           *ret_pos)
{
  gdouble     min_dist, dist, pos;
  LigmaCoords  point = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  LigmaCoords  segmentcoords[4];
  GList      *anchorlist;
  LigmaAnchor *segment_start;
  LigmaAnchor *segment_end = NULL;
  LigmaAnchor *anchor;
  gint        count;

  if (g_queue_is_empty (stroke->anchors))
    return -1.0;

  count = 0;
  min_dist = -1;
  pos = 0;

  for (anchorlist = stroke->anchors->head;
       LIGMA_ANCHOR (anchorlist->data)->type != LIGMA_ANCHOR_ANCHOR;
       anchorlist = g_list_next (anchorlist));

  segment_start = anchorlist->data;

  for ( ; anchorlist; anchorlist = g_list_next (anchorlist))
    {
      anchor = anchorlist->data;

      segmentcoords[count] = anchor->position;
      count++;

      if (count == 4)
        {
          segment_end = anchorlist->data;
          dist = ligma_bezier_stroke_segment_nearest_point_get (segmentcoords,
                                                               coord, precision,
                                                               &point, &pos,
                                                               10);

          if (dist < min_dist || min_dist < 0)
            {
              min_dist = dist;

              if (ret_pos)
                *ret_pos = pos;
              if (ret_point)
                *ret_point = point;
              if (ret_segment_start)
                *ret_segment_start = segment_start;
              if (ret_segment_end)
                *ret_segment_end = segment_end;
            }

          segment_start = anchorlist->data;
          segmentcoords[0] = segmentcoords[3];
          count = 1;
        }
    }

  if (stroke->closed && stroke->anchors->head)
    {
      anchorlist = stroke->anchors->head;

      while (count < 3)
        {
          segmentcoords[count] = LIGMA_ANCHOR (anchorlist->data)->position;
          count++;
        }

      anchorlist = g_list_next (anchorlist);

      if (anchorlist)
        {
          segment_end = LIGMA_ANCHOR (anchorlist->data);
          segmentcoords[3] = segment_end->position;
        }

      dist = ligma_bezier_stroke_segment_nearest_point_get (segmentcoords,
                                                           coord, precision,
                                                           &point, &pos,
                                                           10);

      if (dist < min_dist || min_dist < 0)
        {
          min_dist = dist;

          if (ret_pos)
            *ret_pos = pos;
          if (ret_point)
            *ret_point = point;
          if (ret_segment_start)
            *ret_segment_start = segment_start;
          if (ret_segment_end)
            *ret_segment_end = segment_end;
        }
    }

  return min_dist;
}


static gdouble
ligma_bezier_stroke_segment_nearest_point_get (const LigmaCoords *beziercoords,
                                              const LigmaCoords *coord,
                                              gdouble           precision,
                                              LigmaCoords       *ret_point,
                                              gdouble          *ret_pos,
                                              gint              depth)
{
  /*
   * beziercoords has to contain four LigmaCoords with the four control points
   * of the bezier segment. We subdivide it at the parameter 0.5.
   */

  LigmaCoords subdivided[8];
  gdouble    dist1, dist2;
  LigmaCoords point1, point2;
  gdouble    pos1, pos2;

  ligma_coords_difference (&beziercoords[1], &beziercoords[0], &point1);
  ligma_coords_difference (&beziercoords[3], &beziercoords[2], &point2);

  if (! depth || (ligma_coords_bezier_is_straight (beziercoords, precision) &&
                  ligma_coords_length_squared (&point1) < precision         &&
                  ligma_coords_length_squared (&point2) < precision))
    {
      LigmaCoords line, dcoord;
      gdouble    length2, scalar;
      gint       i;

      ligma_coords_difference (&(beziercoords[3]),
                              &(beziercoords[0]),
                              &line);

      ligma_coords_difference (coord,
                              &(beziercoords[0]),
                              &dcoord);

      length2 = ligma_coords_scalarprod (&line, &line);
      scalar = ligma_coords_scalarprod (&line, &dcoord) / length2;

      scalar = CLAMP (scalar, 0.0, 1.0);

      /* lines look the same as bezier curves where the handles
       * sit on the anchors, however, they are parametrized
       * differently. Hence we have to do some weird approximation.  */

      pos1 = pos2 = 0.5;

      for (i = 0; i <= 15; i++)
        {
          pos2 *= 0.5;

          if (3 * pos1 * pos1 * (1-pos1) + pos1 * pos1 * pos1 < scalar)
            pos1 += pos2;
          else
            pos1 -= pos2;
        }

      *ret_pos = pos1;

      ligma_coords_mix (1.0, &(beziercoords[0]),
                       scalar, &line,
                       ret_point);

      ligma_coords_difference (coord, ret_point, &dcoord);

      return ligma_coords_length (&dcoord);
    }

  /* ok, we have to subdivide */

  subdivided[0] = beziercoords[0];
  subdivided[6] = beziercoords[3];

  /* if (!depth) g_printerr ("Hit recursion depth limit!\n"); */

  ligma_coords_average (&(beziercoords[0]), &(beziercoords[1]),
                       &(subdivided[1]));

  ligma_coords_average (&(beziercoords[1]), &(beziercoords[2]),
                       &(subdivided[7]));

  ligma_coords_average (&(beziercoords[2]), &(beziercoords[3]),
                       &(subdivided[5]));

  ligma_coords_average (&(subdivided[1]), &(subdivided[7]),
                       &(subdivided[2]));

  ligma_coords_average (&(subdivided[7]), &(subdivided[5]),
                       &(subdivided[4]));

  ligma_coords_average (&(subdivided[2]), &(subdivided[4]),
                       &(subdivided[3]));

  /*
   * We now have the coordinates of the two bezier segments in
   * subdivided [0-3] and subdivided [3-6]
   */

  dist1 = ligma_bezier_stroke_segment_nearest_point_get (&(subdivided[0]),
                                                        coord, precision,
                                                        &point1, &pos1,
                                                        depth - 1);

  dist2 = ligma_bezier_stroke_segment_nearest_point_get (&(subdivided[3]),
                                                        coord, precision,
                                                        &point2, &pos2,
                                                        depth - 1);

  if (dist1 <= dist2)
    {
      *ret_point = point1;
      *ret_pos = 0.5 * pos1;
      return dist1;
    }
  else
    {
      *ret_point = point2;
      *ret_pos = 0.5 + 0.5 * pos2;
      return dist2;
    }
}


static gdouble
ligma_bezier_stroke_nearest_tangent_get (LigmaStroke        *stroke,
                                        const LigmaCoords  *coord1,
                                        const LigmaCoords  *coord2,
                                        gdouble            precision,
                                        LigmaCoords        *nearest,
                                        LigmaAnchor       **ret_segment_start,
                                        LigmaAnchor       **ret_segment_end,
                                        gdouble           *ret_pos)
{
  gdouble     min_dist, dist, pos;
  LigmaCoords  point;
  LigmaCoords  segmentcoords[4];
  GList      *anchorlist;
  LigmaAnchor *segment_start;
  LigmaAnchor *segment_end = NULL;
  LigmaAnchor *anchor;
  gint        count;

  if (g_queue_is_empty (stroke->anchors))
    return -1.0;

  count = 0;
  min_dist = -1;

  for (anchorlist = stroke->anchors->head;
       LIGMA_ANCHOR (anchorlist->data)->type != LIGMA_ANCHOR_ANCHOR;
       anchorlist = g_list_next (anchorlist));

  segment_start = anchorlist->data;

  for ( ; anchorlist; anchorlist = g_list_next (anchorlist))
    {
      anchor = anchorlist->data;

      segmentcoords[count] = anchor->position;
      count++;

      if (count == 4)
        {
          segment_end = anchorlist->data;
          dist = ligma_bezier_stroke_segment_nearest_tangent_get (segmentcoords,
                                                                 coord1, coord2,
                                                                 precision,
                                                                 &point, &pos);

          if (dist >= 0 && (dist < min_dist || min_dist < 0))
            {
              min_dist = dist;

              if (ret_pos)
                *ret_pos = pos;
              if (nearest)
                *nearest = point;
              if (ret_segment_start)
                *ret_segment_start = segment_start;
              if (ret_segment_end)
                *ret_segment_end = segment_end;
            }

          segment_start = anchorlist->data;
          segmentcoords[0] = segmentcoords[3];
          count = 1;
        }
    }

  if (stroke->closed && ! g_queue_is_empty (stroke->anchors))
    {
      anchorlist = stroke->anchors->head;

      while (count < 3)
        {
          segmentcoords[count] = LIGMA_ANCHOR (anchorlist->data)->position;
          count++;
        }

      anchorlist = g_list_next (anchorlist);

      if (anchorlist)
        {
          segment_end = LIGMA_ANCHOR (anchorlist->data);
          segmentcoords[3] = segment_end->position;
        }

      dist = ligma_bezier_stroke_segment_nearest_tangent_get (segmentcoords,
                                                             coord1, coord2,
                                                             precision,
                                                             &point, &pos);

      if (dist >= 0 && (dist < min_dist || min_dist < 0))
        {
          min_dist = dist;

          if (ret_pos)
            *ret_pos = pos;
          if (nearest)
            *nearest = point;
          if (ret_segment_start)
            *ret_segment_start = segment_start;
          if (ret_segment_end)
            *ret_segment_end = segment_end;
        }
    }

  return min_dist;
}

static gdouble
ligma_bezier_stroke_segment_nearest_tangent_get (const LigmaCoords *beziercoords,
                                                const LigmaCoords *coord1,
                                                const LigmaCoords *coord2,
                                                gdouble           precision,
                                                LigmaCoords       *ret_point,
                                                gdouble          *ret_pos)
{
  GArray     *ret_coords;
  GArray     *ret_params;
  LigmaCoords  dir, line, dcoord, min_point;
  gdouble     min_dist = -1;
  gdouble     dist, length2, scalar, ori, ori2;
  gint        i;

  ligma_coords_difference (coord2, coord1, &line);

  ret_coords = g_array_new (FALSE, FALSE, sizeof (LigmaCoords));
  ret_params = g_array_new (FALSE, FALSE, sizeof (gdouble));

  g_printerr ("(%.2f, %.2f)-(%.2f,%.2f): ", coord1->x, coord1->y,
              coord2->x, coord2->y);

  ligma_coords_interpolate_bezier (beziercoords, precision,
                                  ret_coords, ret_params);

  g_return_val_if_fail (ret_coords->len == ret_params->len, -1.0);

  if (ret_coords->len < 2)
    return -1;

  ligma_coords_difference (&g_array_index (ret_coords, LigmaCoords, 1),
                          &g_array_index (ret_coords, LigmaCoords, 0),
                          &dir);
  ori = dir.x * line.y - dir.y * line.x;

  for (i = 2; i < ret_coords->len; i++)
    {
      ligma_coords_difference (&g_array_index (ret_coords, LigmaCoords, i),
                              &g_array_index (ret_coords, LigmaCoords, i-1),
                              &dir);
      ori2 = dir.x * line.y - dir.y * line.x;

      if (ori * ori2 <= 0)
        {
          ligma_coords_difference (&g_array_index (ret_coords, LigmaCoords, i),
                                  coord1,
                                  &dcoord);

          length2 = ligma_coords_scalarprod (&line, &line);
          scalar = ligma_coords_scalarprod (&line, &dcoord) / length2;

          if (scalar >= 0 && scalar <= 1)
            {
              ligma_coords_mix (1.0, coord1,
                               scalar, &line,
                               &min_point);
              ligma_coords_difference (&min_point,
                                      &g_array_index (ret_coords, LigmaCoords, i),
                                      &dcoord);
              dist = ligma_coords_length (&dcoord);

              if (dist < min_dist || min_dist < 0)
                {
                  min_dist   = dist;
                  *ret_point = g_array_index (ret_coords, LigmaCoords, i);
                  *ret_pos   = g_array_index (ret_params, gdouble, i);
                }
            }
        }
      ori = ori2;
    }

  if (min_dist < 0)
    g_printerr ("-\n");
  else
    g_printerr ("%f: (%.2f, %.2f) /%.3f/\n", min_dist,
                (*ret_point).x, (*ret_point).y, *ret_pos);

  g_array_free (ret_coords, TRUE);
  g_array_free (ret_params, TRUE);

  return min_dist;
}

static gboolean
ligma_bezier_stroke_is_extendable (LigmaStroke *stroke,
                                  LigmaAnchor *neighbor)
{
  GList *listneighbor;
  gint   loose_end;

  if (stroke->closed)
    return FALSE;

  if (g_queue_is_empty (stroke->anchors))
    return TRUE;

  /* assure that there is a neighbor specified */
  g_return_val_if_fail (neighbor != NULL, FALSE);

  loose_end = 0;
  listneighbor = stroke->anchors->tail;

  /* Check if the neighbor is at an end of the control points */
  if (listneighbor->data == neighbor)
    {
      loose_end = 1;
    }
  else
    {
      listneighbor = g_list_first (stroke->anchors->head);

      if (listneighbor->data == neighbor)
        {
          loose_end = -1;
        }
      else
        {
          /*
           * It isn't. If we are on a handle go to the nearest
           * anchor and see if we can find an end from it.
           * Yes, this is tedious.
           */

          listneighbor = g_queue_find (stroke->anchors, neighbor);

          if (listneighbor && neighbor->type == LIGMA_ANCHOR_CONTROL)
            {
              if (listneighbor->prev &&
                  LIGMA_ANCHOR (listneighbor->prev->data)->type == LIGMA_ANCHOR_ANCHOR)
                {
                  listneighbor = listneighbor->prev;
                }
              else if (listneighbor->next &&
                       LIGMA_ANCHOR (listneighbor->next->data)->type == LIGMA_ANCHOR_ANCHOR)
                {
                  listneighbor = listneighbor->next;
                }
              else
                {
                  loose_end = 0;
                  listneighbor = NULL;
                }
            }

          if (listneighbor)
            /* we found a suitable ANCHOR_ANCHOR now, lets
             * search for its loose end.
             */
            {
              if (listneighbor->prev &&
                  listneighbor->prev->prev == NULL)
                {
                  loose_end = -1;
                }
              else if (listneighbor->next &&
                       listneighbor->next->next == NULL)
                {
                  loose_end = 1;
                }
            }
        }
    }

  return (loose_end != 0);
}

LigmaAnchor *
ligma_bezier_stroke_extend (LigmaStroke           *stroke,
                           const LigmaCoords     *coords,
                           LigmaAnchor           *neighbor,
                           LigmaVectorExtendMode  extend_mode)
{
  LigmaAnchor *anchor = NULL;
  GList      *listneighbor;
  gint        loose_end, control_count;

  if (g_queue_is_empty (stroke->anchors))
    {
      /* assure that there is no neighbor specified */
      g_return_val_if_fail (neighbor == NULL, NULL);

      anchor = ligma_anchor_new (LIGMA_ANCHOR_CONTROL, coords);

      g_queue_push_tail (stroke->anchors, anchor);

      switch (extend_mode)
        {
        case EXTEND_SIMPLE:
          break;

        case EXTEND_EDITABLE:
          anchor = ligma_bezier_stroke_extend (stroke,
                                              coords, anchor,
                                              EXTEND_SIMPLE);

          /* we return the LIGMA_ANCHOR_ANCHOR */
          ligma_bezier_stroke_extend (stroke,
                                     coords, anchor,
                                     EXTEND_SIMPLE);

          break;

        default:
          anchor = NULL;
        }

      return anchor;
    }
  else
    {
      /* assure that there is a neighbor specified */
      g_return_val_if_fail (neighbor != NULL, NULL);

      loose_end = 0;
      listneighbor = stroke->anchors->tail;

      /* Check if the neighbor is at an end of the control points */
      if (listneighbor->data == neighbor)
        {
          loose_end = 1;
        }
      else
        {
          listneighbor = stroke->anchors->head;

          if (listneighbor->data == neighbor)
            {
              loose_end = -1;
            }
          else
            {
              /*
               * It isn't. If we are on a handle go to the nearest
               * anchor and see if we can find an end from it.
               * Yes, this is tedious.
               */

              listneighbor = g_queue_find (stroke->anchors, neighbor);

              if (listneighbor && neighbor->type == LIGMA_ANCHOR_CONTROL)
                {
                  if (listneighbor->prev &&
                      LIGMA_ANCHOR (listneighbor->prev->data)->type == LIGMA_ANCHOR_ANCHOR)
                    {
                      listneighbor = listneighbor->prev;
                    }
                  else if (listneighbor->next &&
                           LIGMA_ANCHOR (listneighbor->next->data)->type == LIGMA_ANCHOR_ANCHOR)
                    {
                      listneighbor = listneighbor->next;
                    }
                  else
                    {
                      loose_end = 0;
                      listneighbor = NULL;
                    }
                }

              if (listneighbor)
                /* we found a suitable ANCHOR_ANCHOR now, lets
                 * search for its loose end.
                 */
                {
                  if (listneighbor->next &&
                      listneighbor->next->next == NULL)
                    {
                      loose_end = 1;
                      listneighbor = listneighbor->next;
                    }
                  else if (listneighbor->prev &&
                      listneighbor->prev->prev == NULL)
                    {
                      loose_end = -1;
                      listneighbor = listneighbor->prev;
                    }
                }
            }
        }

      if (loose_end)
        {
          LigmaAnchorType  type;

          /* We have to detect the type of the point to add... */

          control_count = 0;

          if (loose_end == 1)
            {
              while (listneighbor &&
                     LIGMA_ANCHOR (listneighbor->data)->type == LIGMA_ANCHOR_CONTROL)
                {
                  control_count++;
                  listneighbor = listneighbor->prev;
                }
            }
          else
            {
              while (listneighbor &&
                     LIGMA_ANCHOR (listneighbor->data)->type == LIGMA_ANCHOR_CONTROL)
                {
                  control_count++;
                  listneighbor = listneighbor->next;
                }
            }

          switch (extend_mode)
            {
            case EXTEND_SIMPLE:
              switch (control_count)
                {
                case 0:
                  type = LIGMA_ANCHOR_CONTROL;
                  break;
                case 1:
                  if (listneighbor)  /* only one handle in the path? */
                    type = LIGMA_ANCHOR_CONTROL;
                  else
                    type = LIGMA_ANCHOR_ANCHOR;
                  break;
                case 2:
                  type = LIGMA_ANCHOR_ANCHOR;
                  break;
                default:
                  g_warning ("inconsistent bezier curve: "
                             "%d successive control handles", control_count);
                  type = LIGMA_ANCHOR_ANCHOR;
                }

              anchor = ligma_anchor_new (type, coords);

              if (loose_end == 1)
                g_queue_push_tail (stroke->anchors, anchor);

              if (loose_end == -1)
                g_queue_push_head (stroke->anchors, anchor);
              break;

            case EXTEND_EDITABLE:
              switch (control_count)
                {
                case 0:
                  neighbor = ligma_bezier_stroke_extend (stroke,
                                                        &(neighbor->position),
                                                        neighbor,
                                                        EXTEND_SIMPLE);
                case 1:
                  neighbor = ligma_bezier_stroke_extend (stroke,
                                                        coords,
                                                        neighbor,
                                                        EXTEND_SIMPLE);
                case 2:
                  anchor = ligma_bezier_stroke_extend (stroke,
                                                      coords,
                                                      neighbor,
                                                      EXTEND_SIMPLE);

                  neighbor = ligma_bezier_stroke_extend (stroke,
                                                        coords,
                                                        anchor,
                                                        EXTEND_SIMPLE);
                  break;
                default:
                  g_warning ("inconsistent bezier curve: "
                             "%d successive control handles", control_count);
                }
            }

          return anchor;
        }

      return NULL;
    }
}

static gboolean
ligma_bezier_stroke_connect_stroke (LigmaStroke *stroke,
                                   LigmaAnchor *anchor,
                                   LigmaStroke *extension,
                                   LigmaAnchor *neighbor)
{
  GList *list1;
  GList *list2;

  list1 = g_queue_find (stroke->anchors, anchor);
  list1 = ligma_bezier_stroke_get_anchor_listitem (list1);
  list2 = g_queue_find (extension->anchors, neighbor);
  list2 = ligma_bezier_stroke_get_anchor_listitem (list2);

  g_return_val_if_fail (list1 != NULL && list2 != NULL, FALSE);

  if (stroke == extension)
    {
      g_return_val_if_fail ((list1->prev && list1->prev->prev == NULL &&
                             list2->next && list2->next->next == NULL) ||
                            (list1->next && list1->next->next == NULL &&
                             list2->prev && list2->prev->prev == NULL), FALSE);
      ligma_stroke_close (stroke);
      return TRUE;
    }

  if (list1->prev && list1->prev->prev == NULL)
    {
      g_queue_reverse (stroke->anchors);
    }

  g_return_val_if_fail (list1->next && list1->next->next == NULL, FALSE);

  if (list2->next && list2->next->next == NULL)
    {
      g_queue_reverse (extension->anchors);
    }

  g_return_val_if_fail (list2->prev && list2->prev->prev == NULL, FALSE);

  for (list1 = extension->anchors->head; list1; list1 = g_list_next (list1))
    g_queue_push_tail (stroke->anchors, list1->data);

  g_queue_clear (extension->anchors);

  return TRUE;
}


static gboolean
ligma_bezier_stroke_reverse (LigmaStroke *stroke)
{
  g_return_val_if_fail (LIGMA_IS_BEZIER_STROKE (stroke), FALSE);

  g_queue_reverse (stroke->anchors);

  /* keep the first nodegroup the same for closed strokes */
  if (stroke->closed && stroke->anchors->length >= 3)
    {
      g_queue_push_head_link (stroke->anchors,
                              g_queue_pop_tail_link (stroke->anchors));
      g_queue_push_head_link (stroke->anchors,
                              g_queue_pop_tail_link (stroke->anchors));
      g_queue_push_head_link (stroke->anchors,
                              g_queue_pop_tail_link (stroke->anchors));
    }

  return TRUE;
}


static gboolean
ligma_bezier_stroke_shift_start (LigmaStroke *stroke,
                                LigmaAnchor *new_start)
{
  GList *link;

  g_return_val_if_fail (LIGMA_IS_BEZIER_STROKE (stroke), FALSE);
  g_return_val_if_fail (new_start != NULL, FALSE);
  g_return_val_if_fail (new_start->type == LIGMA_ANCHOR_ANCHOR, FALSE);

  link = g_queue_find (stroke->anchors, new_start);
  if (!link)
    return FALSE;

  /* the preceding control anchor will be the new head */

  link = g_list_previous (link);
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


static void
ligma_bezier_stroke_anchor_move_relative (LigmaStroke            *stroke,
                                         LigmaAnchor            *anchor,
                                         const LigmaCoords      *deltacoord,
                                         LigmaAnchorFeatureType  feature)
{
  LigmaCoords  delta, coord1, coord2;
  GList      *anchor_list;

  delta = *deltacoord;
  delta.pressure = 0;
  delta.xtilt    = 0;
  delta.ytilt    = 0;
  delta.wheel    = 0;

  ligma_coords_add (&(anchor->position), &delta, &coord1);
  anchor->position = coord1;

  anchor_list = g_queue_find (stroke->anchors, anchor);
  g_return_if_fail (anchor_list != NULL);

  if (anchor->type == LIGMA_ANCHOR_ANCHOR)
    {
      if (g_list_previous (anchor_list))
        {
          coord2 = LIGMA_ANCHOR (g_list_previous (anchor_list)->data)->position;
          ligma_coords_add (&coord2, &delta, &coord1);
          LIGMA_ANCHOR (g_list_previous (anchor_list)->data)->position = coord1;
        }

      if (g_list_next (anchor_list))
        {
          coord2 = LIGMA_ANCHOR (g_list_next (anchor_list)->data)->position;
          ligma_coords_add (&coord2, &delta, &coord1);
          LIGMA_ANCHOR (g_list_next (anchor_list)->data)->position = coord1;
        }
    }
  else
    {
      if (feature == LIGMA_ANCHOR_FEATURE_SYMMETRIC)
        {
          GList *neighbour = NULL, *opposite = NULL;

          /* search for opposite control point. Sigh. */
          neighbour = g_list_previous (anchor_list);
          if (neighbour &&
              LIGMA_ANCHOR (neighbour->data)->type == LIGMA_ANCHOR_ANCHOR)
            {
              opposite = g_list_previous (neighbour);
            }
          else
            {
              neighbour = g_list_next (anchor_list);
              if (neighbour &&
                  LIGMA_ANCHOR (neighbour->data)->type == LIGMA_ANCHOR_ANCHOR)
                {
                  opposite = g_list_next (neighbour);
                }
            }
          if (opposite &&
              LIGMA_ANCHOR (opposite->data)->type == LIGMA_ANCHOR_CONTROL)
            {
              ligma_coords_difference (&(LIGMA_ANCHOR (neighbour->data)->position),
                                      &(anchor->position), &delta);
              ligma_coords_add (&(LIGMA_ANCHOR (neighbour->data)->position),
                               &delta, &coord1);
              LIGMA_ANCHOR (opposite->data)->position = coord1;
            }
        }
    }
}


static void
ligma_bezier_stroke_anchor_move_absolute (LigmaStroke            *stroke,
                                         LigmaAnchor            *anchor,
                                         const LigmaCoords      *coord,
                                         LigmaAnchorFeatureType  feature)
{
  LigmaCoords deltacoord;

  ligma_coords_difference (coord, &anchor->position, &deltacoord);
  ligma_bezier_stroke_anchor_move_relative (stroke, anchor,
                                           &deltacoord, feature);
}

static void
ligma_bezier_stroke_anchor_convert (LigmaStroke            *stroke,
                                   LigmaAnchor            *anchor,
                                   LigmaAnchorFeatureType  feature)
{
  GList *anchor_list;

  anchor_list = g_queue_find (stroke->anchors, anchor);

  g_return_if_fail (anchor_list != NULL);

  switch (feature)
    {
    case LIGMA_ANCHOR_FEATURE_EDGE:
      if (anchor->type == LIGMA_ANCHOR_ANCHOR)
        {
          if (g_list_previous (anchor_list))
            LIGMA_ANCHOR (g_list_previous (anchor_list)->data)->position =
              anchor->position;

          if (g_list_next (anchor_list))
            LIGMA_ANCHOR (g_list_next (anchor_list)->data)->position =
              anchor->position;
        }
      else
        {
          if (g_list_previous (anchor_list) &&
              LIGMA_ANCHOR (g_list_previous (anchor_list)->data)->type == LIGMA_ANCHOR_ANCHOR)
            anchor->position = LIGMA_ANCHOR (g_list_previous (anchor_list)->data)->position;
          if (g_list_next (anchor_list) &&
              LIGMA_ANCHOR (g_list_next (anchor_list)->data)->type == LIGMA_ANCHOR_ANCHOR)
            anchor->position = LIGMA_ANCHOR (g_list_next (anchor_list)->data)->position;
        }

      break;

    default:
      g_warning ("ligma_bezier_stroke_anchor_convert: "
                 "unimplemented anchor conversion %d\n", feature);
    }
}


static LigmaBezierDesc *
ligma_bezier_stroke_make_bezier (LigmaStroke *stroke)
{
  GArray            *points;
  GArray            *cmd_array;
  LigmaBezierDesc    *bezdesc;
  cairo_path_data_t  pathdata;
  gint               num_cmds, i;

  points = ligma_stroke_control_points_get (stroke, NULL);

  g_return_val_if_fail (points && points->len % 3 == 0, NULL);
  if (points->len < 3)
    return NULL;

  /* Moveto + (n-1) * curveto + (if closed) curveto + closepath */
  num_cmds = 2 + (points->len / 3 - 1) * 4;
  if (stroke->closed)
    num_cmds += 1 + 4;

  cmd_array = g_array_sized_new (FALSE, FALSE,
                                 sizeof (cairo_path_data_t),
                                 num_cmds);

  pathdata.header.type = CAIRO_PATH_MOVE_TO;
  pathdata.header.length = 2;
  g_array_append_val (cmd_array, pathdata);
  pathdata.point.x = g_array_index (points, LigmaAnchor, 1).position.x;
  pathdata.point.y = g_array_index (points, LigmaAnchor, 1).position.y;
  g_array_append_val (cmd_array, pathdata);

  for (i = 2; i+2 < points->len; i += 3)
    {
      pathdata.header.type = CAIRO_PATH_CURVE_TO;
      pathdata.header.length = 4;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, LigmaAnchor, i).position.x;
      pathdata.point.y = g_array_index (points, LigmaAnchor, i).position.y;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, LigmaAnchor, i+1).position.x;
      pathdata.point.y = g_array_index (points, LigmaAnchor, i+1).position.y;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, LigmaAnchor, i+2).position.x;
      pathdata.point.y = g_array_index (points, LigmaAnchor, i+2).position.y;
      g_array_append_val (cmd_array, pathdata);
    }

  if (stroke->closed)
    {
      pathdata.header.type = CAIRO_PATH_CURVE_TO;
      pathdata.header.length = 4;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, LigmaAnchor, i).position.x;
      pathdata.point.y = g_array_index (points, LigmaAnchor, i).position.y;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, LigmaAnchor, 0).position.x;
      pathdata.point.y = g_array_index (points, LigmaAnchor, 0).position.y;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, LigmaAnchor, 1).position.x;
      pathdata.point.y = g_array_index (points, LigmaAnchor, 1).position.y;
      g_array_append_val (cmd_array, pathdata);

      pathdata.header.type = CAIRO_PATH_CLOSE_PATH;
      pathdata.header.length = 1;
      g_array_append_val (cmd_array, pathdata);
    }

  if (cmd_array->len != num_cmds)
    g_printerr ("miscalculated path cmd length! (%d vs. %d)\n",
                cmd_array->len, num_cmds);

  bezdesc = ligma_bezier_desc_new ((cairo_path_data_t *) cmd_array->data,
                                  cmd_array->len);
  g_array_free (points, TRUE);
  g_array_free (cmd_array, FALSE);

  return bezdesc;
}


static GArray *
ligma_bezier_stroke_interpolate (LigmaStroke *stroke,
                                gdouble     precision,
                                gboolean   *ret_closed)
{
  GArray     *ret_coords;
  LigmaAnchor *anchor;
  GList      *anchorlist;
  LigmaCoords  segmentcoords[4];
  gint        count;
  gboolean    need_endpoint = FALSE;

  if (g_queue_is_empty (stroke->anchors))
    {
      if (ret_closed)
        *ret_closed = FALSE;
      return NULL;
    }

  ret_coords = g_array_new (FALSE, FALSE, sizeof (LigmaCoords));

  count = 0;

  for (anchorlist = stroke->anchors->head;
       anchorlist && LIGMA_ANCHOR (anchorlist->data)->type != LIGMA_ANCHOR_ANCHOR;
       anchorlist = g_list_next (anchorlist));

  for ( ; anchorlist; anchorlist = g_list_next (anchorlist))
    {
      anchor = anchorlist->data;

      segmentcoords[count] = anchor->position;
      count++;

      if (count == 4)
        {
          ligma_coords_interpolate_bezier (segmentcoords, precision,
                                          ret_coords, NULL);
          segmentcoords[0] = segmentcoords[3];
          count = 1;
          need_endpoint = TRUE;
        }
    }

  if (stroke->closed && ! g_queue_is_empty (stroke->anchors))
    {
      anchorlist = stroke->anchors->head;

      while (count < 3)
        {
          segmentcoords[count] = LIGMA_ANCHOR (anchorlist->data)->position;
          count++;
        }
      anchorlist = g_list_next (anchorlist);
      if (anchorlist)
        segmentcoords[3] = LIGMA_ANCHOR (anchorlist->data)->position;

      ligma_coords_interpolate_bezier (segmentcoords, precision,
                                      ret_coords, NULL);
      need_endpoint = TRUE;

    }

  if (need_endpoint)
    ret_coords = g_array_append_val (ret_coords, segmentcoords[3]);

  if (ret_closed)
    *ret_closed = stroke->closed;

  if (ret_coords->len == 0)
    {
      g_array_free (ret_coords, TRUE);
      ret_coords = NULL;
    }

  return ret_coords;
}


static void
ligma_bezier_stroke_transform (LigmaStroke        *stroke,
                              const LigmaMatrix3 *matrix,
                              GQueue            *ret_strokes)
{
  LigmaStroke *first_stroke = NULL;
  LigmaStroke *last_stroke  = NULL;
  GList      *anchorlist;
  LigmaAnchor *anchor;
  LigmaCoords  segmentcoords[4];
  GQueue     *transformed[2];
  gint        n_transformed;
  gint        count;
  gboolean    first;
  gboolean    last;

  /* if there's no need for clipping, use the default implementation */
  if (! ret_strokes                   ||
      ligma_matrix3_is_affine (matrix) ||
      g_queue_is_empty (stroke->anchors))
    {
      LIGMA_STROKE_CLASS (parent_class)->transform (stroke, matrix, ret_strokes);

      return;
    }

  /* transform the individual segments */
  count = 0;
  first = TRUE;
  last  = FALSE;

  /* find the first non-control anchor */
  for (anchorlist = stroke->anchors->head;
       anchorlist && LIGMA_ANCHOR (anchorlist->data)->type != LIGMA_ANCHOR_ANCHOR;
       anchorlist = g_list_next (anchorlist));

  for ( ; anchorlist || stroke->closed; anchorlist = g_list_next (anchorlist))
    {
      /* wrap around if 'stroke' is closed, so that we transform the final
       * segment
       */
      if (! anchorlist)
        {
          anchorlist = stroke->anchors->head;
          last       = TRUE;
        }

      anchor = anchorlist->data;

      segmentcoords[count] = anchor->position;
      count++;

      if (count == 4)
        {
          gboolean start_in;
          gboolean end_in;
          gint     i;

          ligma_transform_bezier_coords (matrix, segmentcoords,
                                        transformed, &n_transformed,
                                        &start_in, &end_in);

          for (i = 0; i < n_transformed; i++)
            {
              LigmaStroke *s = NULL;
              GList      *list;
              gint        j;

              if (i == 0 && start_in)
                {
                  /* current stroke is connected to last stroke */
                  s = last_stroke;
                }
              else if (last_stroke)
                {
                  /* current stroke is not connected to last stroke.  finalize
                   * last stroke.
                   */
                  anchor = g_queue_peek_tail (last_stroke->anchors);

                  g_queue_push_tail (last_stroke->anchors,
                                     ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                                      &anchor->position));
                }

              for (list = transformed[i]->head; list; list = g_list_next (list))
                {
                  LigmaCoords *transformedcoords = list->data;

                  if (! s)
                    {
                      /* start a new stroke */
                      s = ligma_bezier_stroke_new ();

                      g_queue_push_tail (s->anchors,
                                         ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                                          &transformedcoords[0]));

                      g_queue_push_tail (ret_strokes, s);

                      j = 0;
                    }
                  else
                    {
                      /* continue an existing stroke, skipping the first anchor,
                       * which is the same as the last anchor of the last stroke
                       */
                      j = 1;
                    }

                  for (; j < 4; j++)
                    {
                      LigmaAnchorType type;

                      if (j == 0 || j == 3)
                        type = LIGMA_ANCHOR_ANCHOR;
                      else
                        type = LIGMA_ANCHOR_CONTROL;

                      g_queue_push_tail (s->anchors,
                                         ligma_anchor_new (type,
                                                          &transformedcoords[j]));
                    }

                  g_free (transformedcoords);
                }

              g_queue_free (transformed[i]);

              /* if the current stroke is an initial segment of 'stroke',
               * remember it, so that we can possibly connect it to the last
               * stroke later.
               */
              if (i == 0 && start_in && first)
                first_stroke = s;

              last_stroke = s;
              first       = FALSE;
            }

          if (! end_in && last_stroke)
            {
              /* the next stroke is not connected to the last stroke.  finalize
               * the last stroke.
               */
              anchor = g_queue_peek_tail (last_stroke->anchors);

              g_queue_push_tail (last_stroke->anchors,
                                 ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                                  &anchor->position));

              last_stroke = NULL;
            }

          if (last)
            break;

          segmentcoords[0] = segmentcoords[3];
          count = 1;
        }
    }

  /* if the last stroke is a final segment of 'stroke'... */
  if (last_stroke)
    {
      /* ... and the first stroke is an initial segment of 'stroke', and
       * 'stroke' is closed ...
       */
      if (first_stroke && stroke->closed)
        {
          /* connect the first and last strokes */

          /* remove the first anchor, which is a synthetic control point */
          ligma_anchor_free (g_queue_pop_head (first_stroke->anchors));
          /* remove the last anchor, which is the same anchor point as the
           * first anchor
           */
          ligma_anchor_free (g_queue_pop_tail (last_stroke->anchors));

          if (first_stroke == last_stroke)
            {
              /* the result is a single stroke.  move the last anchor, which is
               * an orphan control point, to the front, to fill in the removed
               * control point of the first anchor, and close the stroke.
               */
              g_queue_push_head (first_stroke->anchors,
                                 g_queue_pop_tail (first_stroke->anchors));

              first_stroke->closed = TRUE;
            }
          else
            {
              /* the result is multiple strokes.  prepend the last stroke to
               * the first stroke, and discard it.
               */
              while ((anchor = g_queue_pop_tail (last_stroke->anchors)))
                g_queue_push_head (first_stroke->anchors, anchor);

              g_object_unref (g_queue_pop_tail (ret_strokes));
            }
        }
      else
        {
          /* otherwise, the first and last strokes are not connected.  finalize
           * the last stroke.
           */
          anchor = g_queue_peek_tail (last_stroke->anchors);

          g_queue_push_tail (last_stroke->anchors,
                             ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                              &anchor->position));
        }
    }
}


LigmaStroke *
ligma_bezier_stroke_new_moveto (const LigmaCoords *start)
{
  LigmaStroke *stroke = ligma_bezier_stroke_new ();

  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                      start));
  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_ANCHOR,
                                      start));
  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                      start));
  return stroke;
}

void
ligma_bezier_stroke_lineto (LigmaStroke       *stroke,
                           const LigmaCoords *end)
{
  g_return_if_fail (LIGMA_IS_BEZIER_STROKE (stroke));
  g_return_if_fail (stroke->closed == FALSE);
  g_return_if_fail (g_queue_is_empty (stroke->anchors) == FALSE);

  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                      end));
  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_ANCHOR,
                                      end));
  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                      end));
}

void
ligma_bezier_stroke_conicto (LigmaStroke       *stroke,
                            const LigmaCoords *control,
                            const LigmaCoords *end)
{
  LigmaCoords start, coords;

  g_return_if_fail (LIGMA_IS_BEZIER_STROKE (stroke));
  g_return_if_fail (stroke->closed == FALSE);
  g_return_if_fail (g_queue_get_length (stroke->anchors) > 1);

  start = LIGMA_ANCHOR (stroke->anchors->tail->prev->data)->position;

  ligma_coords_mix (2.0 / 3.0, control, 1.0 / 3.0, &start, &coords);

  LIGMA_ANCHOR (stroke->anchors->tail->data)->position = coords;

  ligma_coords_mix (2.0 / 3.0, control, 1.0 / 3.0, end, &coords);

  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                      &coords));
  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_ANCHOR,
                                      end));
  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                      end));
}

void
ligma_bezier_stroke_cubicto (LigmaStroke       *stroke,
                            const LigmaCoords *control1,
                            const LigmaCoords *control2,
                            const LigmaCoords *end)
{
  g_return_if_fail (LIGMA_IS_BEZIER_STROKE (stroke));
  g_return_if_fail (stroke->closed == FALSE);
  g_return_if_fail (g_queue_is_empty (stroke->anchors) == FALSE);

  LIGMA_ANCHOR (stroke->anchors->tail->data)->position = *control1;

  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                      control2));
  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_ANCHOR,
                                      end));
  g_queue_push_tail (stroke->anchors,
                     ligma_anchor_new (LIGMA_ANCHOR_CONTROL,
                                      end));
}

static gdouble
arcto_circleparam (gdouble  h,
                   gdouble *y)
{
  gdouble t0 = 0.5;
  gdouble dt = 0.25;
  gdouble pt0;
  gdouble y01, y12, y23, y012, y123, y0123;   /* subdividing y[] */

  while (dt >= 0.00001)
    {
      pt0 = (    y[0] * (1-t0) * (1-t0) * (1-t0) +
             3 * y[1] * (1-t0) * (1-t0) * t0     +
             3 * y[2] * (1-t0) * t0     * t0     +
                 y[3] * t0     * t0     * t0 );

      if (pt0 > h)
        t0 = t0 - dt;
      else if (pt0 < h)
        t0 = t0 + dt;
      else
        break;
      dt = dt/2;
    }

  y01   = y[0] * (1-t0) + y[1] * t0;
  y12   = y[1] * (1-t0) + y[2] * t0;
  y23   = y[2] * (1-t0) + y[3] * t0;
  y012  = y01  * (1-t0) + y12  * t0;
  y123  = y12  * (1-t0) + y23  * t0;
  y0123 = y012 * (1-t0) + y123 * t0;

  y[0] = y0123; y[1] = y123; y[2] = y23; /* y[3] unchanged */

  return t0;
}

static void
arcto_subdivide (gdouble     t,
                 gint        part,
                 LigmaCoords *p)
{
  LigmaCoords p01, p12, p23, p012, p123, p0123;

  ligma_coords_mix (1-t, &(p[0]), t, &(p[1]), &p01  );
  ligma_coords_mix (1-t, &(p[1]), t, &(p[2]), &p12  );
  ligma_coords_mix (1-t, &(p[2]), t, &(p[3]), &p23  );
  ligma_coords_mix (1-t,  &p01  , t,  &p12  , &p012 );
  ligma_coords_mix (1-t,  &p12  , t,  &p23  , &p123 );
  ligma_coords_mix (1-t,  &p012 , t,  &p123 , &p0123);

  if (part == 0)
    {
      /* p[0] unchanged */
      p[1] = p01;
      p[2] = p012;
      p[3] = p0123;
    }
  else
    {
      p[0] = p0123;
      p[1] = p123;
      p[2] = p23;
      /* p[3] unchanged */
    }
}

static void
arcto_ellipsesegment (gdouble     radius_x,
                      gdouble     radius_y,
                      gdouble     phi0,
                      gdouble     phi1,
                      LigmaCoords *ellips)
{
  const LigmaCoords  template    = LIGMA_COORDS_DEFAULT_VALUES;
  const gdouble     circlemagic = 4.0 * (G_SQRT2 - 1.0) / 3.0;

  gdouble  phi_s, phi_e;
  gdouble  y[4];
  gdouble  h0, h1;
  gdouble  t0, t1;

  g_return_if_fail (ellips != NULL);

  y[0] = 0.0;
  y[1] = circlemagic;
  y[2] = 1.0;
  y[3] = 1.0;

  ellips[0] = template;
  ellips[1] = template;
  ellips[2] = template;
  ellips[3] = template;

  if (phi0 < phi1)
    {
      phi_s = floor (phi0 / G_PI_2) * G_PI_2;
      while (phi_s < 0) phi_s += 2 * G_PI;
      phi_e = phi_s + G_PI_2;
    }
  else
    {
      phi_e = floor (phi1 / G_PI_2) * G_PI_2;
      while (phi_e < 0) phi_e += 2 * G_PI;
      phi_s = phi_e + G_PI_2;
    }

  h0 = sin (fabs (phi0-phi_s));
  h1 = sin (fabs (phi1-phi_s));

  ellips[0].x = cos (phi_s); ellips[0].y = sin (phi_s);
  ellips[3].x = cos (phi_e); ellips[3].y = sin (phi_e);

  ligma_coords_mix (1, &(ellips[0]), circlemagic, &(ellips[3]), &(ellips[1]));
  ligma_coords_mix (circlemagic, &(ellips[0]), 1, &(ellips[3]), &(ellips[2]));

  if (h0 > y[0])
    {
      t0 = arcto_circleparam (h0, y); /* also subdivides y[] at t0 */
      arcto_subdivide (t0, 1, ellips);
    }

  if (h1 < y[3])
    {
      t1 = arcto_circleparam (h1, y);
      arcto_subdivide (t1, 0, ellips);
    }

  ellips[0].x *= radius_x ; ellips[0].y *= radius_y;
  ellips[1].x *= radius_x ; ellips[1].y *= radius_y;
  ellips[2].x *= radius_x ; ellips[2].y *= radius_y;
  ellips[3].x *= radius_x ; ellips[3].y *= radius_y;
}

void
ligma_bezier_stroke_arcto (LigmaStroke       *bez_stroke,
                          gdouble           radius_x,
                          gdouble           radius_y,
                          gdouble           angle_rad,
                          gboolean          large_arc,
                          gboolean          sweep,
                          const LigmaCoords *end)
{
  LigmaCoords  start;
  LigmaCoords  middle;   /* between start and end */
  LigmaCoords  trans_delta;
  LigmaCoords  trans_center;
  LigmaCoords  tmp_center;
  LigmaCoords  center;
  LigmaCoords  ellips[4]; /* control points of untransformed ellipse segment */
  LigmaCoords  ctrl[4];   /* control points of next bezier segment           */

  LigmaMatrix3 anglerot;

  gdouble     lambda;
  gdouble     phi0, phi1, phi2;
  gdouble     tmpx, tmpy;

  g_return_if_fail (LIGMA_IS_BEZIER_STROKE (bez_stroke));
  g_return_if_fail (bez_stroke->closed == FALSE);
  g_return_if_fail (g_queue_get_length (bez_stroke->anchors) > 1);

  if (radius_x == 0 || radius_y == 0)
    {
      ligma_bezier_stroke_lineto (bez_stroke, end);
      return;
    }

  start = LIGMA_ANCHOR (bez_stroke->anchors->tail->prev->data)->position;

  ligma_matrix3_identity (&anglerot);
  ligma_matrix3_rotate (&anglerot, -angle_rad);

  ligma_coords_mix (0.5, &start, -0.5, end, &trans_delta);
  ligma_matrix3_transform_point (&anglerot,
                                trans_delta.x, trans_delta.y,
                                &tmpx, &tmpy);
  trans_delta.x = tmpx;
  trans_delta.y = tmpy;

  lambda = (SQR (trans_delta.x) / SQR (radius_x) +
            SQR (trans_delta.y) / SQR (radius_y));

  if (lambda < 0.00001)
    {
      /* don't bother with it - endpoint is too close to startpoint */
      return;
    }

  trans_center = trans_delta;

  if (lambda > 1.0)
    {
      /* The radii are too small for a matching ellipse. We expand them
       * so that they fit exactly (center of the ellipse between the
       * start- and endpoint
       */
      radius_x *= sqrt (lambda);
      radius_y *= sqrt (lambda);
      trans_center.x = 0.0;
      trans_center.y = 0.0;
    }
  else
    {
      gdouble factor = sqrt ((1.0 - lambda) / lambda);

      trans_center.x =   trans_delta.y * radius_x / radius_y * factor;
      trans_center.y = - trans_delta.x * radius_y / radius_x * factor;
    }

  if ((large_arc && sweep) || (!large_arc && !sweep))
    {
      trans_center.x *= -1;
      trans_center.y *= -1;
    }

  ligma_matrix3_identity (&anglerot);
  ligma_matrix3_rotate (&anglerot, angle_rad);

  tmp_center = trans_center;
  ligma_matrix3_transform_point (&anglerot,
                                tmp_center.x, tmp_center.y,
                                &tmpx, &tmpy);
  tmp_center.x = tmpx;
  tmp_center.y = tmpy;

  ligma_coords_mix (0.5, &start, 0.5, end, &middle);
  ligma_coords_add (&tmp_center, &middle, &center);

  phi1 = atan2 ((trans_delta.y - trans_center.y) / radius_y,
                (trans_delta.x - trans_center.x) / radius_x);

  phi2 = atan2 ((- trans_delta.y - trans_center.y) / radius_y,
                (- trans_delta.x - trans_center.x) / radius_x);

  if (phi1 < 0)
    phi1 += 2 * G_PI;

  if (phi2 < 0)
    phi2 += 2 * G_PI;

  if (sweep)
    {
      while (phi2 < phi1)
        phi2 += 2 * G_PI;

      phi0 = floor (phi1 / G_PI_2) * G_PI_2;

      while (phi0 < phi2)
        {
          arcto_ellipsesegment (radius_x, radius_y,
                                MAX (phi0, phi1), MIN (phi0 + G_PI_2, phi2),
                                ellips);

          ligma_matrix3_transform_point (&anglerot, ellips[0].x, ellips[0].y,
                                        &tmpx, &tmpy);
          ellips[0].x = tmpx; ellips[0].y = tmpy;
          ligma_matrix3_transform_point (&anglerot, ellips[1].x, ellips[1].y,
                                        &tmpx, &tmpy);
          ellips[1].x = tmpx; ellips[1].y = tmpy;
          ligma_matrix3_transform_point (&anglerot, ellips[2].x, ellips[2].y,
                                        &tmpx, &tmpy);
          ellips[2].x = tmpx; ellips[2].y = tmpy;
          ligma_matrix3_transform_point (&anglerot, ellips[3].x, ellips[3].y,
                                        &tmpx, &tmpy);
          ellips[3].x = tmpx; ellips[3].y = tmpy;

          ligma_coords_add (&center, &(ellips[1]), &(ctrl[1]));
          ligma_coords_add (&center, &(ellips[2]), &(ctrl[2]));
          ligma_coords_add (&center, &(ellips[3]), &(ctrl[3]));

          ligma_bezier_stroke_cubicto (bez_stroke,
                                      &(ctrl[1]), &(ctrl[2]), &(ctrl[3]));
          phi0 += G_PI_2;
        }
    }
  else
    {
      while (phi1 < phi2)
        phi1 += 2 * G_PI;

      phi0 = ceil (phi1 / G_PI_2) * G_PI_2;

      while (phi0 > phi2)
        {
          arcto_ellipsesegment (radius_x, radius_y,
                                MIN (phi0, phi1), MAX (phi0 - G_PI_2, phi2),
                                ellips);

          ligma_matrix3_transform_point (&anglerot, ellips[0].x, ellips[0].y,
                                        &tmpx, &tmpy);
          ellips[0].x = tmpx; ellips[0].y = tmpy;
          ligma_matrix3_transform_point (&anglerot, ellips[1].x, ellips[1].y,
                                        &tmpx, &tmpy);
          ellips[1].x = tmpx; ellips[1].y = tmpy;
          ligma_matrix3_transform_point (&anglerot, ellips[2].x, ellips[2].y,
                                        &tmpx, &tmpy);
          ellips[2].x = tmpx; ellips[2].y = tmpy;
          ligma_matrix3_transform_point (&anglerot, ellips[3].x, ellips[3].y,
                                        &tmpx, &tmpy);
          ellips[3].x = tmpx; ellips[3].y = tmpy;

          ligma_coords_add (&center, &(ellips[1]), &(ctrl[1]));
          ligma_coords_add (&center, &(ellips[2]), &(ctrl[2]));
          ligma_coords_add (&center, &(ellips[3]), &(ctrl[3]));

          ligma_bezier_stroke_cubicto (bez_stroke,
                                      &(ctrl[1]), &(ctrl[2]), &(ctrl[3]));
          phi0 -= G_PI_2;
        }
    }
}

LigmaStroke *
ligma_bezier_stroke_new_ellipse (const LigmaCoords *center,
                                gdouble           radius_x,
                                gdouble           radius_y,
                                gdouble           angle)
{
  LigmaStroke    *stroke;
  LigmaCoords     p1 = *center;
  LigmaCoords     p2 = *center;
  LigmaCoords     p3 = *center;
  LigmaCoords     dx = { 0, };
  LigmaCoords     dy = { 0, };
  const gdouble  circlemagic = 4.0 * (G_SQRT2 - 1.0) / 3.0;
  LigmaAnchor    *handle;

  dx.x =   radius_x * cos (angle);
  dx.y = - radius_x * sin (angle);
  dy.x =   radius_y * sin (angle);
  dy.y =   radius_y * cos (angle);

  ligma_coords_mix (1.0, center, 1.0, &dx, &p1);
  stroke = ligma_bezier_stroke_new_moveto (&p1);

  handle = g_queue_peek_head (stroke->anchors);
  ligma_coords_mix (1.0,    &p1, -circlemagic, &dy, &handle->position);

  ligma_coords_mix (1.0,    &p1,  circlemagic, &dy, &p1);
  ligma_coords_mix (1.0, center,          1.0, &dy, &p3);
  ligma_coords_mix (1.0,    &p3,  circlemagic, &dx, &p2);
  ligma_bezier_stroke_cubicto (stroke, &p1, &p2, &p3);

  ligma_coords_mix (1.0,    &p3, -circlemagic, &dx, &p1);
  ligma_coords_mix (1.0, center,         -1.0, &dx, &p3);
  ligma_coords_mix (1.0,    &p3,  circlemagic, &dy, &p2);
  ligma_bezier_stroke_cubicto (stroke, &p1, &p2, &p3);

  ligma_coords_mix (1.0,    &p3, -circlemagic, &dy, &p1);
  ligma_coords_mix (1.0, center,         -1.0, &dy, &p3);
  ligma_coords_mix (1.0,    &p3, -circlemagic, &dx, &p2);
  ligma_bezier_stroke_cubicto (stroke, &p1, &p2, &p3);

  handle = g_queue_peek_tail (stroke->anchors);
  ligma_coords_mix (1.0,    &p3,  circlemagic, &dx, &handle->position);

  ligma_stroke_close (stroke);

  return stroke;
}


/* helper function to get the associated anchor of a listitem */

static GList *
ligma_bezier_stroke_get_anchor_listitem (GList *list)
{
  if (!list)
    return NULL;

  if (LIGMA_ANCHOR (list->data)->type == LIGMA_ANCHOR_ANCHOR)
    return list;

  if (list->prev && LIGMA_ANCHOR (list->prev->data)->type == LIGMA_ANCHOR_ANCHOR)
    return list->prev;

  if (list->next && LIGMA_ANCHOR (list->next->data)->type == LIGMA_ANCHOR_ANCHOR)
    return list->next;

  g_return_val_if_fail (/* bezier stroke inconsistent! */ FALSE, NULL);

  return NULL;
}
