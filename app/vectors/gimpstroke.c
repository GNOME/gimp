/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstroke.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#include "glib-object.h"

#include "vectors-types.h"

#include "core/gimpdrawable-transform-utils.h"

#include "gimpanchor.h"
#include "gimpstroke.h"


/* Prototypes */

static void         gimp_stroke_class_init           (GimpStrokeClass  *klass);
static void         gimp_stroke_init                 (GimpStroke       *stroke);

static void         gimp_stroke_finalize             (GObject          *object);

static gsize        gimp_stroke_get_memsize          (GimpObject       *object);

static GimpAnchor * gimp_stroke_real_anchor_get      (const GimpStroke *stroke,
                                                      const GimpCoords *coord);
static GimpAnchor * gimp_stroke_real_anchor_get_next (const GimpStroke *stroke,
                                                      const GimpAnchor *prev);
static void         gimp_stroke_real_anchor_select   (GimpStroke       *stroke,
                                                      GimpAnchor       *anchor,
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
static gdouble      gimp_stroke_real_get_length      (const GimpStroke *stroke);
static gdouble      gimp_stroke_real_get_distance    (const GimpStroke *stroke,
                                                      const GimpCoords *coord);
static GArray *     gimp_stroke_real_interpolate     (const GimpStroke *stroke,
                                                      gdouble           precision,
                                                      gboolean         *closed);
static GimpAnchor * gimp_stroke_real_temp_anchor_get (const GimpStroke *stroke);
static GimpAnchor * gimp_stroke_real_temp_anchor_set (GimpStroke       *stroke,
                                                      const GimpCoords *coord);
static gboolean     gimp_stroke_real_temp_anchor_fix (GimpStroke       *stroke);
static GimpStroke * gimp_stroke_real_duplicate       (const GimpStroke *stroke);
static GimpStroke * gimp_stroke_real_make_bezier     (const GimpStroke *stroke);

static void gimp_stroke_real_translate (GimpStroke *stroke,
                                        gdouble offset_x, gdouble offset_y);
static void gimp_stroke_real_scale     (GimpStroke *stroke,
                                        gdouble scale_x, gdouble scale_y,
                                        gint offset_x, gint offset_y);
static void gimp_stroke_real_resize    (GimpStroke *stroke,
                                        gint new_width, gint new_height,
                                        gint offset_x, gint offset_y);
static void gimp_stroke_real_flip      (GimpStroke *stroke,
                                        GimpOrientationType flip_type,
                                        gdouble axis, gboolean clip_result);
static void gimp_stroke_real_rotate    (GimpStroke *stroke,
                                        GimpRotationType rotate_type,
                                        gdouble center_x, gdouble center_y,
                                        gboolean clip_result);
static void gimp_stroke_real_transform (GimpStroke *stroke,
                                        const GimpMatrix3 *matrix,
                                        GimpTransformDirection direction,
                                        GimpInterpolationType interp_type,
                                        gboolean clip_result,
                                        GimpProgressFunc progress_callback,
                                        gpointer progress_data);

static GList     * gimp_stroke_real_get_draw_anchors (const GimpStroke *stroke);
static GList    * gimp_stroke_real_get_draw_controls (const GimpStroke *stroke);
static GArray     * gimp_stroke_real_get_draw_lines  (const GimpStroke *stroke);


/*  private variables  */

static GimpObjectClass *parent_class = NULL;


GType
gimp_stroke_get_type (void)
{
  static GType stroke_type = 0;

  if (! stroke_type)
    {
      static const GTypeInfo stroke_info =
      {
        sizeof (GimpStrokeClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_stroke_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpStroke),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_stroke_init,
      };

      stroke_type = g_type_register_static (GIMP_TYPE_OBJECT,
                                            "GimpStroke",
                                            &stroke_info, 0);
    }

  return stroke_type;
}

static void
gimp_stroke_class_init (GimpStrokeClass *klass)
{
  GObjectClass    *object_class;
  GimpObjectClass *gimp_object_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize         = gimp_stroke_finalize;

  gimp_object_class->get_memsize = gimp_stroke_get_memsize;

  klass->changed                 = NULL;
  klass->removed                 = NULL;

  klass->anchor_get              = gimp_stroke_real_anchor_get;
  klass->anchor_get_next         = gimp_stroke_real_anchor_get_next;
  klass->anchor_select           = gimp_stroke_real_anchor_select;
  klass->anchor_move_relative    = gimp_stroke_real_anchor_move_relative;
  klass->anchor_move_absolute    = gimp_stroke_real_anchor_move_absolute;
  klass->anchor_convert          = gimp_stroke_real_anchor_convert;
  klass->anchor_delete           = gimp_stroke_real_anchor_delete;

  klass->get_length              = gimp_stroke_real_get_length;
  klass->get_distance            = gimp_stroke_real_get_distance;
  klass->interpolate             = gimp_stroke_real_interpolate;

  klass->temp_anchor_get         = gimp_stroke_real_temp_anchor_get;
  klass->temp_anchor_set         = gimp_stroke_real_temp_anchor_set;
  klass->temp_anchor_fix         = gimp_stroke_real_temp_anchor_fix;

  klass->duplicate               = gimp_stroke_real_duplicate;
  klass->make_bezier             = gimp_stroke_real_make_bezier;

  klass->translate               = gimp_stroke_real_translate;
  klass->scale                   = gimp_stroke_real_scale;
  klass->resize                  = gimp_stroke_real_resize;
  klass->flip                    = gimp_stroke_real_flip;
  klass->rotate                  = gimp_stroke_real_rotate;
  klass->transform               = gimp_stroke_real_transform;

  klass->get_draw_anchors        = gimp_stroke_real_get_draw_anchors;
  klass->get_draw_controls       = gimp_stroke_real_get_draw_controls;
  klass->get_draw_lines          = gimp_stroke_real_get_draw_lines;
}

static void
gimp_stroke_init (GimpStroke *stroke)
{
  stroke->anchors     = NULL;
  stroke->temp_anchor = NULL;
  stroke->closed      = FALSE;
}

static void
gimp_stroke_finalize (GObject *object)
{
  GimpStroke *stroke;
  GList      *list;

  stroke = GIMP_STROKE (object);

  for (list = stroke->anchors; list; list = list->next)
    gimp_anchor_free ((GimpAnchor *) list->data);

  if (stroke->temp_anchor)
    gimp_anchor_free (stroke->temp_anchor);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_stroke_get_memsize (GimpObject *object)
{
  GimpStroke *stroke;
  gsize       memsize = 0;

  stroke = GIMP_STROKE (object);

  memsize += g_list_length (stroke->anchors) * (sizeof (GList) +
                                                sizeof (GimpAnchor));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}


GimpAnchor *
gimp_stroke_anchor_get (const GimpStroke *stroke,
                        const GimpCoords *coord)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->anchor_get (stroke, coord);
}

static GimpAnchor *
gimp_stroke_real_anchor_get (const GimpStroke *stroke,
                             const GimpCoords *coord)
{
  gdouble     dx, dy;
  gdouble     mindist = -1;
  GList      *anchors;
  GList      *list;
  GimpAnchor *anchor = NULL;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  anchors = gimp_stroke_get_draw_controls (stroke);

  for (list = anchors; list; list = g_list_next (list))
    {
      dx = coord->x - ((GimpAnchor *) list->data)->position.x;
      dy = coord->y - ((GimpAnchor *) list->data)->position.y;

      if (mindist < 0 || mindist > dx * dx + dy * dy)
        {
          mindist = dx * dx + dy * dy;
          anchor = (GimpAnchor *) list->data;
        }
    }

  g_list_free (anchors);

  anchors = gimp_stroke_get_draw_anchors (stroke);

  for (list = anchors; list; list = g_list_next (list))
    {
      dx = coord->x - ((GimpAnchor *) list->data)->position.x;
      dy = coord->y - ((GimpAnchor *) list->data)->position.y;

      if (mindist < 0 || mindist > dx * dx + dy * dy)
        {
          mindist = dx * dx + dy * dy;
          anchor = (GimpAnchor *) list->data;
        }
    }

  g_list_free (anchors);

  return anchor;
}


GimpAnchor *
gimp_stroke_anchor_get_next (const GimpStroke *stroke,
                             const GimpAnchor *prev)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->anchor_get_next (stroke, prev);
}

static GimpAnchor *
gimp_stroke_real_anchor_get_next (const GimpStroke *stroke,
                                  const GimpAnchor *prev)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  if (prev)
    {
      list = g_list_find (stroke->anchors, prev);
      if (list)
        list = g_list_next (list);
    }
  else
    {
      list = stroke->anchors;
    }

  if (list)
    return (GimpAnchor *) list->data;

  return NULL;
}


void
gimp_stroke_anchor_select (GimpStroke *stroke,
                           GimpAnchor *anchor,
                           gboolean    exclusive)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->anchor_select (stroke, anchor, exclusive);
}

static void
gimp_stroke_real_anchor_select (GimpStroke *stroke,
                                GimpAnchor *anchor,
                                gboolean    exclusive)
{
  GList *list;

  list = stroke->anchors;

  if (exclusive)
    {
      while (list)
        {
          ((GimpAnchor *) list->data)->selected = FALSE;
          list = g_list_next (list);
        }
    }

  list = g_list_find (stroke->anchors, anchor);

  if (list)
    ((GimpAnchor *) list->data)->selected = TRUE;
}


void
gimp_stroke_anchor_move_relative (GimpStroke            *stroke,
                                  GimpAnchor            *anchor,
                                  const GimpCoords      *delta,
                                  GimpAnchorFeatureType  feature)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (g_list_find (stroke->anchors, anchor));

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
  g_return_if_fail (g_list_find (stroke->anchors, anchor));

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

void
gimp_stroke_close (GimpStroke *stroke)
{
  stroke->closed = TRUE;
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

  GIMP_STROKE_GET_CLASS (stroke)->anchor_delete (stroke, anchor);
}

static void
gimp_stroke_real_anchor_delete (GimpStroke *stroke,
                                GimpAnchor *anchor)
{
  g_printerr ("gimp_stroke_anchor_delete: default implementation\n");
}


gdouble
gimp_stroke_get_length (const GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0.0);

  return GIMP_STROKE_GET_CLASS (stroke)->get_length (stroke);
}

static gdouble
gimp_stroke_real_get_length (const GimpStroke *stroke)
{
  g_printerr ("gimp_stroke_get_length: default implementation\n");

  return 0.0;
}


gdouble
gimp_stroke_get_distance (const GimpStroke *stroke,
                          const GimpCoords  *coord)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0.0);

  return GIMP_STROKE_GET_CLASS (stroke)->get_distance (stroke, coord);
}

static gdouble
gimp_stroke_real_get_distance (const GimpStroke *stroke,
                               const GimpCoords  *coord)
{
  g_printerr ("gimp_stroke_get_distance: default implementation\n");

  return 0.0;
}


GArray *
gimp_stroke_interpolate (const GimpStroke *stroke,
                         gdouble           precision,
                         gboolean         *ret_closed)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), 0);

  return GIMP_STROKE_GET_CLASS (stroke)->interpolate (stroke, precision,
                                                      ret_closed);
}

static GArray *
gimp_stroke_real_interpolate (const GimpStroke  *stroke,
                              gdouble            precision,
                              gboolean          *ret_closed)
{
  g_printerr ("gimp_stroke_interpolate: default implementation\n");

  return NULL;
}


GimpAnchor *
gimp_stroke_temp_anchor_get (const GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->temp_anchor_get (stroke);
}

static GimpAnchor *
gimp_stroke_real_temp_anchor_get (const GimpStroke *stroke)
{
  g_printerr ("gimp_stroke_temp_anchor_get: default implementation\n");

  return NULL;
}


GimpAnchor *
gimp_stroke_temp_anchor_set (GimpStroke       *stroke,
                             const GimpCoords *coord)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->temp_anchor_set (stroke, coord);
}

static GimpAnchor *
gimp_stroke_real_temp_anchor_set (GimpStroke       *stroke,
                                  const GimpCoords *coord)
{
  g_printerr ("gimp_stroke_temp_anchor_set: default implementation\n");

  return NULL;
}


gboolean
gimp_stroke_temp_anchor_fix (GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), FALSE);

  return GIMP_STROKE_GET_CLASS (stroke)->temp_anchor_fix (stroke);
}

static gboolean
gimp_stroke_real_temp_anchor_fix (GimpStroke *stroke)
{
  g_printerr ("gimp_stroke_temp_anchor_fix: default implementation\n");

  return FALSE;
}


GimpStroke *
gimp_stroke_duplicate (const GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return (GIMP_STROKE_GET_CLASS (stroke))->duplicate (stroke);
}

GimpStroke *
gimp_stroke_real_duplicate (const GimpStroke *stroke)
{
  GimpStroke *new_stroke;
  GList      *list;

  new_stroke = g_object_new (G_TYPE_FROM_INSTANCE (stroke),
                             "name", GIMP_OBJECT (stroke)->name,
                             NULL);

  new_stroke->anchors = g_list_copy (stroke->anchors);

  for (list = new_stroke->anchors; list; list = g_list_next (list))
    {
      list->data = gimp_anchor_duplicate ((GimpAnchor *) list->data);
    }

  if (stroke->temp_anchor)
    {
      new_stroke->temp_anchor = gimp_anchor_duplicate (stroke->temp_anchor);
    }

  return new_stroke;
}


GimpStroke *
gimp_stroke_make_bezier (const GimpStroke *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->make_bezier (stroke);
}

static GimpStroke *
gimp_stroke_real_make_bezier (const GimpStroke *stroke)
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

  for (list = stroke->anchors; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      anchor->position.x += offset_x;
      anchor->position.y += offset_y;
    }
}


void
gimp_stroke_scale (GimpStroke *stroke,
                   gdouble     scale_x,
                   gdouble     scale_y,
                   gint        offset_x,
                   gint        offset_y)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->scale (stroke,
                                         scale_x, scale_y,
                                         offset_x, offset_y);
}

static void
gimp_stroke_real_scale (GimpStroke *stroke,
                        gdouble     scale_x,
                        gdouble     scale_y,
                        gint        offset_x,
                        gint        offset_y)
{
  GList *list;

  for (list = stroke->anchors; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      anchor->position.x *= scale_x;
      anchor->position.y *= scale_y;
    }
}


void
gimp_stroke_resize (GimpStroke *stroke,
                    gint        new_width,
                    gint        new_height,
                    gint        offset_x,
                    gint        offset_y)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->resize (stroke,
                                          new_width, new_height,
                                          offset_x, offset_y);
}

static void
gimp_stroke_real_resize (GimpStroke *stroke,
                         gint        new_width,
                         gint        new_height,
                         gint        offset_x,
                         gint        offset_y)
{
  GList *list;

  for (list = stroke->anchors; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      anchor->position.x += offset_x;
      anchor->position.y += offset_y;
    }
}


void
gimp_stroke_flip (GimpStroke          *stroke,
                  GimpOrientationType  flip_type,
                  gdouble              axis,
                  gboolean             clip_result)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->flip (stroke, flip_type,
                                        axis, clip_result);
}

static void
gimp_stroke_real_flip (GimpStroke          *stroke,
                       GimpOrientationType  flip_type,
                       gdouble              axis,
                       gboolean             clip_result)
{
  GList *list;

  for (list = stroke->anchors; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      switch (flip_type)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          anchor->position.x = -(anchor->position.x - axis) + axis;
          break;

        case GIMP_ORIENTATION_VERTICAL:
          anchor->position.y = -(anchor->position.y - axis) + axis;
          break;

        default:
          break;
        }
    }
}


void
gimp_stroke_rotate (GimpStroke       *stroke,
                    GimpRotationType  rotate_type,
                    gdouble           center_x,
                    gdouble           center_y,
                    gboolean          clip_result)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->rotate (stroke, rotate_type,
                                          center_x, center_y, clip_result);
}

static void
gimp_stroke_real_rotate (GimpStroke       *stroke,
                         GimpRotationType  rotate_type,
                         gdouble           center_x,
                         gdouble           center_y,
                         gboolean          clip_result)
{
  GList       *list;
  GimpMatrix3  matrix; 
  gdouble      angle = 0.0;

  switch (rotate_type)
    {
    case GIMP_ROTATE_90:
      angle = G_PI_2;
      break;
    case GIMP_ROTATE_180:
      angle = G_PI;
      break;
    case GIMP_ROTATE_270:
      angle = - G_PI_2;
      break;
    }

  gimp_drawable_transform_matrix_rotate_center (center_x, center_y, angle,
                                                &matrix);

  for (list = stroke->anchors; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      gimp_matrix3_transform_point (&matrix,
                                    anchor->position.x,
                                    anchor->position.y,
                                    &anchor->position.x,
                                    &anchor->position.y);
    }
}


void
gimp_stroke_transform (GimpStroke            *stroke,
                       const GimpMatrix3     *matrix,
                       GimpTransformDirection direction,
                       GimpInterpolationType  interp_type,
                       gboolean               clip_result,
                       GimpProgressFunc       progress_callback,
                       gpointer               progress_data)
{
  g_return_if_fail (GIMP_IS_STROKE (stroke));

  GIMP_STROKE_GET_CLASS (stroke)->transform (stroke, matrix, direction,
                                             interp_type, clip_result,
                                             progress_callback, progress_data);
}

static void
gimp_stroke_real_transform (GimpStroke            *stroke,
                            const GimpMatrix3     *matrix,
                            GimpTransformDirection direction,
                            GimpInterpolationType  interp_type,
                            gboolean               clip_result,
                            GimpProgressFunc       progress_callback,
                            gpointer               progress_data)
{
  GimpMatrix3  local_matrix;
  GList       *list;

  local_matrix = *matrix;

  if (direction == GIMP_TRANSFORM_BACKWARD)
    gimp_matrix3_invert (&local_matrix);

  for (list = stroke->anchors; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      gimp_matrix3_transform_point (&local_matrix,
                                    anchor->position.x,
                                    anchor->position.y,
                                    &anchor->position.x,
                                    &anchor->position.y);
    }
}


GList *
gimp_stroke_get_draw_anchors (const GimpStroke  *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->get_draw_anchors (stroke);
}

static GList *
gimp_stroke_real_get_draw_anchors (const GimpStroke  *stroke)
{
  GList *list;
  GList *ret_list = NULL;

  for (list = stroke->anchors; list; list = g_list_next (list))
    {
      if (((GimpAnchor *) list->data)->type == GIMP_ANCHOR_ANCHOR)
        ret_list = g_list_prepend (ret_list, list->data);
    }

  return g_list_reverse (ret_list);
}


GList *
gimp_stroke_get_draw_controls (const GimpStroke  *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->get_draw_controls (stroke);
}

static GList *
gimp_stroke_real_get_draw_controls (const GimpStroke  *stroke)
{
  GList *list;
  GList *ret_list = NULL;

  for (list = stroke->anchors; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      if (anchor->type == GIMP_ANCHOR_CONTROL)
        {
          GimpAnchor *next = list->next ? list->next->data : NULL;
          GimpAnchor *prev = list->prev ? list->prev->data : NULL;

          if (next && next->type == GIMP_ANCHOR_ANCHOR && next->selected)
            {
              ret_list = g_list_prepend (ret_list, anchor);
            }
          else if (prev && prev->type == GIMP_ANCHOR_ANCHOR && prev->selected)
            {
              ret_list = g_list_prepend (ret_list, anchor);
            }
        }
    }

  return g_list_reverse (ret_list);
}


GArray *
gimp_stroke_get_draw_lines (const GimpStroke  *stroke)
{
  g_return_val_if_fail (GIMP_IS_STROKE (stroke), NULL);

  return GIMP_STROKE_GET_CLASS (stroke)->get_draw_lines (stroke);
}

static GArray *
gimp_stroke_real_get_draw_lines (const GimpStroke  *stroke)
{
  GList  *list;
  GArray *ret_lines = g_array_new (FALSE, FALSE, sizeof (GimpCoords));

  for (list = stroke->anchors; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      if (anchor->type == GIMP_ANCHOR_ANCHOR && anchor->selected)
        {
          if (list->next)
            {
              GimpAnchor *next = list->next->data;

              ret_lines = g_array_append_val (ret_lines, anchor->position);
              ret_lines = g_array_append_val (ret_lines, next->position);
            }

          if (list->prev)
            {
              GimpAnchor *prev = list->prev->data;

              ret_lines = g_array_append_val (ret_lines, anchor->position);
              ret_lines = g_array_append_val (ret_lines, prev->position);
            }
        }
    }

  return ret_lines;
}
