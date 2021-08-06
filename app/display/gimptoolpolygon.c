/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolpolygon.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *
 * Based on GimpFreeSelectTool
 *
 * Major improvement to support polygonal segments
 * Copyright (C) 2008 Martin Nordholts
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimp-utils.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvashandle.h"
#include "gimpcanvasline.h"
#include "gimpcanvaspolygon.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-utils.h"
#include "gimptoolpolygon.h"

#include "gimp-intl.h"


#define POINT_GRAB_THRESHOLD_SQ SQR (GIMP_CANVAS_HANDLE_SIZE_CIRCLE / 2)
#define POINT_SHOW_THRESHOLD_SQ SQR (GIMP_CANVAS_HANDLE_SIZE_CIRCLE * 7)
#define N_ITEMS_PER_ALLOC       1024
#define INVALID_INDEX           (-1)
#define NO_CLICK_TIME_AVAILABLE 0


enum
{
  CHANGE_COMPLETE,
  LAST_SIGNAL
};


struct _GimpToolPolygonPrivate
{
  /* Index of grabbed segment index. */
  gint               grabbed_segment_index;

  /* We need to keep track of a number of points when we move a
   * segment vertex
   */
  GimpVector2       *saved_points_lower_segment;
  GimpVector2       *saved_points_higher_segment;
  gint               max_n_saved_points_lower_segment;
  gint               max_n_saved_points_higher_segment;

  /* Keeps track whether or not a modification of the polygon has been
   * made between _button_press and _button_release
   */
  gboolean           polygon_modified;

  /* Point which is used to draw the polygon but which is not part of
   * it yet
   */
  GimpVector2        pending_point;
  gboolean           show_pending_point;

  /* The points of the polygon */
  GimpVector2       *points;
  gint               max_n_points;

  /* The number of points actually in use */
  gint               n_points;


  /* Any int array containing the indices for the points in the
   * polygon that connects different segments together
   */
  gint              *segment_indices;
  gint               max_n_segment_indices;

  /* The number of segment indices actually in use */
  gint               n_segment_indices;

  /* Is the polygon closed? */
  gboolean           polygon_closed;

  /* Whether or not to constrain the angle for newly created polygonal
   * segments.
   */
  gboolean           constrain_angle;

  /* Whether or not to suppress handles (so that new segments can be
   * created immediately after an existing segment vertex.
   */
  gboolean           suppress_handles;

  /* Last _oper_update or _motion coords */
  gboolean           hover;
  GimpVector2        last_coords;

  /* A double-click commits the selection, keep track of last
   * click-time and click-position.
   */
  guint32            last_click_time;
  GimpCoords         last_click_coord;

  /* Are we in a click-drag-release? */
  gboolean           button_down;

  GimpCanvasItem    *polygon;
  GimpCanvasItem    *pending_line;
  GimpCanvasItem    *closing_line;
  GPtrArray         *handles;
};


/*  local function prototypes  */

static void     gimp_tool_polygon_constructed     (GObject               *object);
static void     gimp_tool_polygon_finalize        (GObject               *object);
static void     gimp_tool_polygon_set_property    (GObject               *object,
                                                   guint                  property_id,
                                                   const GValue          *value,
                                                   GParamSpec            *pspec);
static void     gimp_tool_polygon_get_property    (GObject               *object,
                                                   guint                  property_id,
                                                   GValue                *value,
                                                   GParamSpec            *pspec);

static void     gimp_tool_polygon_changed         (GimpToolWidget        *widget);
static gint     gimp_tool_polygon_button_press    (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonPressType    press_type);
static void     gimp_tool_polygon_button_release  (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonReleaseType  release_type);
static void     gimp_tool_polygon_motion          (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state);
static GimpHit  gimp_tool_polygon_hit             (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   gboolean               proximity);
static void     gimp_tool_polygon_hover           (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   gboolean               proximity);
static void     gimp_tool_polygon_leave_notify    (GimpToolWidget        *widget);
static gboolean gimp_tool_polygon_key_press       (GimpToolWidget        *widget,
                                                   GdkEventKey           *kevent);
static void     gimp_tool_polygon_motion_modifier (GimpToolWidget        *widget,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state);
static void     gimp_tool_polygon_hover_modifier  (GimpToolWidget        *widget,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state);
static gboolean gimp_tool_polygon_get_cursor      (GimpToolWidget        *widget,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   GimpCursorType        *cursor,
                                                   GimpToolCursorType    *tool_cursor,
                                                   GimpCursorModifier    *modifier);

static void     gimp_tool_polygon_change_complete (GimpToolPolygon       *polygon);

static gint   gimp_tool_polygon_get_segment_index (GimpToolPolygon       *polygon,
                                                   const GimpCoords      *coords);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolPolygon, gimp_tool_polygon,
                            GIMP_TYPE_TOOL_WIDGET)

#define parent_class gimp_tool_polygon_parent_class

static guint polygon_signals[LAST_SIGNAL] = { 0, };

static const GimpVector2 vector2_zero = { 0.0, 0.0 };


static void
gimp_tool_polygon_class_init (GimpToolPolygonClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = gimp_tool_polygon_constructed;
  object_class->finalize        = gimp_tool_polygon_finalize;
  object_class->set_property    = gimp_tool_polygon_set_property;
  object_class->get_property    = gimp_tool_polygon_get_property;

  widget_class->changed         = gimp_tool_polygon_changed;
  widget_class->button_press    = gimp_tool_polygon_button_press;
  widget_class->button_release  = gimp_tool_polygon_button_release;
  widget_class->motion          = gimp_tool_polygon_motion;
  widget_class->hit             = gimp_tool_polygon_hit;
  widget_class->hover           = gimp_tool_polygon_hover;
  widget_class->leave_notify    = gimp_tool_polygon_leave_notify;
  widget_class->key_press       = gimp_tool_polygon_key_press;
  widget_class->motion_modifier = gimp_tool_polygon_motion_modifier;
  widget_class->hover_modifier  = gimp_tool_polygon_hover_modifier;
  widget_class->get_cursor      = gimp_tool_polygon_get_cursor;

  polygon_signals[CHANGE_COMPLETE] =
    g_signal_new ("change-complete",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolPolygonClass, change_complete),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
gimp_tool_polygon_init (GimpToolPolygon *polygon)
{
  polygon->private = gimp_tool_polygon_get_instance_private (polygon);

  polygon->private->grabbed_segment_index = INVALID_INDEX;
  polygon->private->last_click_time       = NO_CLICK_TIME_AVAILABLE;
}

static void
gimp_tool_polygon_constructed (GObject *object)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (object);
  GimpToolWidget         *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolPolygonPrivate *private = polygon->private;
  GimpCanvasGroup        *stroke_group;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  stroke_group = gimp_tool_widget_add_stroke_group (widget);

  gimp_tool_widget_push_group (widget, stroke_group);

  private->polygon = gimp_tool_widget_add_polygon (widget, NULL,
                                                   NULL, 0, FALSE);

  private->pending_line = gimp_tool_widget_add_line (widget, 0, 0, 0, 0);
  private->closing_line = gimp_tool_widget_add_line (widget, 0, 0, 0, 0);

  gimp_tool_widget_pop_group (widget);

  private->handles = g_ptr_array_new ();

  gimp_tool_polygon_changed (widget);
}

static void
gimp_tool_polygon_finalize (GObject *object)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (object);
  GimpToolPolygonPrivate *private = polygon->private;

  g_free (private->points);
  g_free (private->segment_indices);
  g_free (private->saved_points_lower_segment);
  g_free (private->saved_points_higher_segment);

  g_clear_pointer (&private->handles, g_ptr_array_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_polygon_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_polygon_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_polygon_get_segment (GimpToolPolygon  *polygon,
                               GimpVector2     **points,
                               gint             *n_points,
                               gint              segment_start,
                               gint              segment_end)
{
  GimpToolPolygonPrivate *priv = polygon->private;

  *points   = &priv->points[priv->segment_indices[segment_start]];
  *n_points = priv->segment_indices[segment_end] -
              priv->segment_indices[segment_start] +
              1;
}

static void
gimp_tool_polygon_get_segment_point (GimpToolPolygon *polygon,
                                     gdouble         *start_point_x,
                                     gdouble         *start_point_y,
                                     gint             segment_index)
{
  GimpToolPolygonPrivate *priv = polygon->private;

  *start_point_x = priv->points[priv->segment_indices[segment_index]].x;
  *start_point_y = priv->points[priv->segment_indices[segment_index]].y;
}

static gboolean
gimp_tool_polygon_should_close (GimpToolPolygon  *polygon,
                                guint32           time,
                                const GimpCoords *coords)
{
  GimpToolWidget         *widget       = GIMP_TOOL_WIDGET (polygon);
  GimpToolPolygonPrivate *priv         = polygon->private;
  gboolean                double_click = FALSE;
  gdouble                 dist;

  if (priv->polygon_modified      ||
      priv->n_segment_indices < 1 ||
      priv->n_points          < 3 ||
      priv->polygon_closed)
    return FALSE;

  dist = gimp_canvas_item_transform_distance_square (priv->polygon,
                                                     coords->x,
                                                     coords->y,
                                                     priv->points[0].x,
                                                     priv->points[0].y);

  /* Handle double-click. It must be within GTK+ global double-click
   * time since last click, and it must be within GTK+ global
   * double-click distance away from the last point
   */
  if (time != NO_CLICK_TIME_AVAILABLE)
    {
      GimpDisplayShell *shell    = gimp_tool_widget_get_shell (widget);
      GtkSettings      *settings = gtk_widget_get_settings (GTK_WIDGET (shell));
      gint              double_click_time;
      gint              double_click_distance;
      gint              click_time_passed;
      gdouble           dist_from_last_point;

      click_time_passed = time - priv->last_click_time;

      dist_from_last_point =
        gimp_canvas_item_transform_distance_square (priv->polygon,
                                                    coords->x,
                                                    coords->y,
                                                    priv->last_click_coord.x,
                                                    priv->last_click_coord.y);

      g_object_get (settings,
                    "gtk-double-click-time",     &double_click_time,
                    "gtk-double-click-distance", &double_click_distance,
                    NULL);

      double_click = click_time_passed    < double_click_time &&
                     dist_from_last_point < double_click_distance;
    }

  return ((! priv->suppress_handles && dist < POINT_GRAB_THRESHOLD_SQ) ||
          double_click);
}

static void
gimp_tool_polygon_revert_to_last_segment (GimpToolPolygon *polygon)
{
  GimpToolPolygonPrivate *priv = polygon->private;

  priv->n_points = priv->segment_indices[priv->n_segment_indices - 1] + 1;
}

static void
gimp_tool_polygon_remove_last_segment (GimpToolPolygon *polygon)
{
  GimpToolPolygonPrivate *priv = polygon->private;

  if (priv->polygon_closed)
    {
      priv->polygon_closed = FALSE;

      gimp_tool_polygon_changed (GIMP_TOOL_WIDGET (polygon));

      return;
    }

  if (priv->n_segment_indices > 0)
    {
      GimpCanvasItem *handle;

      priv->n_segment_indices--;

      handle = g_ptr_array_index (priv->handles,
                                  priv->n_segment_indices);

      gimp_tool_widget_remove_item (GIMP_TOOL_WIDGET (polygon), handle);
      g_ptr_array_remove (priv->handles, handle);
    }

  if (priv->n_segment_indices <= 0)
    {
      priv->grabbed_segment_index = INVALID_INDEX;
      priv->show_pending_point    = FALSE;
      priv->n_points              = 0;
      priv->n_segment_indices     = 0;

      gimp_tool_widget_response (GIMP_TOOL_WIDGET (polygon),
                                 GIMP_TOOL_WIDGET_RESPONSE_CANCEL);
    }
  else
    {
      gimp_tool_polygon_revert_to_last_segment (polygon);

      gimp_tool_polygon_changed (GIMP_TOOL_WIDGET (polygon));
    }
}

static void
gimp_tool_polygon_add_point (GimpToolPolygon *polygon,
                             gdouble          x,
                             gdouble          y)
{
  GimpToolPolygonPrivate *priv = polygon->private;

  if (priv->n_points >= priv->max_n_points)
    {
      priv->max_n_points += N_ITEMS_PER_ALLOC;

      priv->points = g_realloc (priv->points,
                                sizeof (GimpVector2) * priv->max_n_points);
    }

  priv->points[priv->n_points].x = x;
  priv->points[priv->n_points].y = y;

  priv->n_points++;
}

static void
gimp_tool_polygon_add_segment_index (GimpToolPolygon *polygon,
                                     gint             index)
{
  GimpToolPolygonPrivate *priv = polygon->private;

  if (priv->n_segment_indices >= priv->max_n_segment_indices)
    {
      priv->max_n_segment_indices += N_ITEMS_PER_ALLOC;

      priv->segment_indices = g_realloc (priv->segment_indices,
                                         sizeof (GimpVector2) *
                                         priv->max_n_segment_indices);
    }

  priv->segment_indices[priv->n_segment_indices] = index;

  g_ptr_array_add (priv->handles,
                   gimp_tool_widget_add_handle (GIMP_TOOL_WIDGET (polygon),
                                                GIMP_HANDLE_CROSS,
                                                0, 0,
                                                GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                GIMP_HANDLE_ANCHOR_CENTER));

  priv->n_segment_indices++;
}

static gboolean
gimp_tool_polygon_is_point_grabbed (GimpToolPolygon *polygon)
{
  GimpToolPolygonPrivate *priv = polygon->private;

  return priv->grabbed_segment_index != INVALID_INDEX;
}

static void
gimp_tool_polygon_fit_segment (GimpToolPolygon   *polygon,
                               GimpVector2       *dest_points,
                               GimpVector2        dest_start_target,
                               GimpVector2        dest_end_target,
                               const GimpVector2 *source_points,
                               gint               n_points)
{
  GimpVector2 origo_translation_offset;
  GimpVector2 untranslation_offset;
  gdouble     rotation;
  gdouble     scale;

  /* Handle some quick special cases */
  if (n_points <= 0)
    {
      return;
    }
  else if (n_points == 1)
    {
      dest_points[0] = dest_end_target;
      return;
    }
  else if (n_points == 2)
    {
      dest_points[0] = dest_start_target;
      dest_points[1] = dest_end_target;
      return;
    }

  /* Copy from source to dest; we work on the dest data */
  memcpy (dest_points, source_points, sizeof (GimpVector2) * n_points);

  /* Transform the destination end point */
  {
    GimpVector2 *dest_end;
    GimpVector2  origo_translated_end_target;
    gdouble      target_rotation;
    gdouble      current_rotation;
    gdouble      target_length;
    gdouble      current_length;

    dest_end = &dest_points[n_points - 1];

    /* Transate to origin */
    gimp_vector2_sub (&origo_translation_offset,
                      &vector2_zero,
                      &dest_points[0]);
    gimp_vector2_add (dest_end,
                      dest_end,
                      &origo_translation_offset);

    /* Calculate origo_translated_end_target */
    gimp_vector2_sub (&origo_translated_end_target,
                      &dest_end_target,
                      &dest_start_target);

    /* Rotate */
    target_rotation  = atan2 (vector2_zero.y - origo_translated_end_target.y,
                              vector2_zero.x - origo_translated_end_target.x);
    current_rotation = atan2 (vector2_zero.y - dest_end->y,
                              vector2_zero.x - dest_end->x);
    rotation         = current_rotation - target_rotation;

    gimp_vector2_rotate (dest_end, rotation);


    /* Scale */
    target_length  = gimp_vector2_length (&origo_translated_end_target);
    current_length = gimp_vector2_length (dest_end);
    scale          = target_length / current_length;

    gimp_vector2_mul (dest_end, scale);


    /* Untranslate */
    gimp_vector2_sub (&untranslation_offset,
                      &dest_end_target,
                      dest_end);
    gimp_vector2_add (dest_end,
                      dest_end,
                      &untranslation_offset);
  }

  /* Do the same transformation for the rest of the points */
  {
    gint i;

    for (i = 0; i < n_points - 1; i++)
      {
        /* Translate */
        gimp_vector2_add (&dest_points[i],
                          &origo_translation_offset,
                          &dest_points[i]);

        /* Rotate */
        gimp_vector2_rotate (&dest_points[i],
                             rotation);

        /* Scale */
        gimp_vector2_mul (&dest_points[i],
                          scale);

        /* Untranslate */
        gimp_vector2_add (&dest_points[i],
                          &dest_points[i],
                          &untranslation_offset);
      }
  }
}

static void
gimp_tool_polygon_move_segment_vertex_to (GimpToolPolygon *polygon,
                                          gint             segment_index,
                                          gdouble          new_x,
                                          gdouble          new_y)
{
  GimpToolPolygonPrivate *priv         = polygon->private;
  GimpVector2             cursor_point = { new_x, new_y };
  GimpVector2            *dest;
  GimpVector2            *dest_start_target;
  GimpVector2            *dest_end_target;
  gint                    n_points;

  /* Handle the segment before the grabbed point */
  if (segment_index > 0)
    {
      gimp_tool_polygon_get_segment (polygon,
                                     &dest,
                                     &n_points,
                                     priv->grabbed_segment_index - 1,
                                     priv->grabbed_segment_index);

      dest_start_target = &dest[0];
      dest_end_target   = &cursor_point;

      gimp_tool_polygon_fit_segment (polygon,
                                     dest,
                                     *dest_start_target,
                                     *dest_end_target,
                                     priv->saved_points_lower_segment,
                                     n_points);
    }

  /* Handle the segment after the grabbed point */
  if (segment_index < priv->n_segment_indices - 1)
    {
      gimp_tool_polygon_get_segment (polygon,
                                     &dest,
                                     &n_points,
                                     priv->grabbed_segment_index,
                                     priv->grabbed_segment_index + 1);

      dest_start_target = &cursor_point;
      dest_end_target   = &dest[n_points - 1];

      gimp_tool_polygon_fit_segment (polygon,
                                     dest,
                                     *dest_start_target,
                                     *dest_end_target,
                                     priv->saved_points_higher_segment,
                                     n_points);
    }

  /* Handle when there only is one point */
  if (segment_index == 0 &&
      priv->n_segment_indices == 1)
    {
      priv->points[0].x = new_x;
      priv->points[0].y = new_y;
    }
}

static void
gimp_tool_polygon_revert_to_saved_state (GimpToolPolygon *polygon)
{
  GimpToolPolygonPrivate *priv = polygon->private;
  GimpVector2            *dest;
  gint                    n_points;

  /* Without a point grab we have no sensible information to fall back
   * on, bail out
   */
  if (! gimp_tool_polygon_is_point_grabbed (polygon))
    {
      return;
    }

  if (priv->grabbed_segment_index > 0)
    {
      gimp_tool_polygon_get_segment (polygon,
                                     &dest,
                                     &n_points,
                                     priv->grabbed_segment_index - 1,
                                     priv->grabbed_segment_index);

      memcpy (dest,
              priv->saved_points_lower_segment,
              sizeof (GimpVector2) * n_points);
    }

  if (priv->grabbed_segment_index < priv->n_segment_indices - 1)
    {
      gimp_tool_polygon_get_segment (polygon,
                                     &dest,
                                     &n_points,
                                     priv->grabbed_segment_index,
                                     priv->grabbed_segment_index + 1);

      memcpy (dest,
              priv->saved_points_higher_segment,
              sizeof (GimpVector2) * n_points);
    }

  if (priv->grabbed_segment_index == 0 &&
      priv->n_segment_indices     == 1)
    {
      priv->points[0] = *priv->saved_points_lower_segment;
    }
}

static void
gimp_tool_polygon_prepare_for_move (GimpToolPolygon *polygon)
{
  GimpToolPolygonPrivate *priv = polygon->private;
  GimpVector2            *source;
  gint                    n_points;

  if (priv->grabbed_segment_index > 0)
    {
      gimp_tool_polygon_get_segment (polygon,
                                     &source,
                                     &n_points,
                                     priv->grabbed_segment_index - 1,
                                     priv->grabbed_segment_index);

      if (n_points > priv->max_n_saved_points_lower_segment)
        {
          priv->max_n_saved_points_lower_segment = n_points;

          priv->saved_points_lower_segment =
            g_realloc (priv->saved_points_lower_segment,
                       sizeof (GimpVector2) * n_points);
        }

      memcpy (priv->saved_points_lower_segment,
              source,
              sizeof (GimpVector2) * n_points);
    }

  if (priv->grabbed_segment_index < priv->n_segment_indices - 1)
    {
      gimp_tool_polygon_get_segment (polygon,
                                     &source,
                                     &n_points,
                                     priv->grabbed_segment_index,
                                     priv->grabbed_segment_index + 1);

      if (n_points > priv->max_n_saved_points_higher_segment)
        {
          priv->max_n_saved_points_higher_segment = n_points;

          priv->saved_points_higher_segment =
            g_realloc (priv->saved_points_higher_segment,
                       sizeof (GimpVector2) * n_points);
        }

      memcpy (priv->saved_points_higher_segment,
              source,
              sizeof (GimpVector2) * n_points);
    }

  /* A special-case when there only is one point */
  if (priv->grabbed_segment_index == 0 &&
      priv->n_segment_indices     == 1)
    {
      if (priv->max_n_saved_points_lower_segment == 0)
        {
          priv->max_n_saved_points_lower_segment = 1;

          priv->saved_points_lower_segment = g_new0 (GimpVector2, 1);
        }

      *priv->saved_points_lower_segment = priv->points[0];
    }
}

static void
gimp_tool_polygon_update_motion (GimpToolPolygon *polygon,
                                 gdouble          new_x,
                                 gdouble          new_y)
{
  GimpToolPolygonPrivate *priv = polygon->private;

  if (gimp_tool_polygon_is_point_grabbed (polygon))
    {
      priv->polygon_modified = TRUE;

      if (priv->constrain_angle &&
          priv->n_segment_indices > 1)
        {
          gdouble start_point_x;
          gdouble start_point_y;
          gint    segment_index;

          /* Base constraints on the last segment vertex if we move
           * the first one, otherwise base on the previous segment
           * vertex
           */
          if (priv->grabbed_segment_index == 0)
            {
              segment_index = priv->n_segment_indices - 1;
            }
          else
            {
              segment_index = priv->grabbed_segment_index - 1;
            }

          gimp_tool_polygon_get_segment_point (polygon,
                                               &start_point_x,
                                               &start_point_y,
                                               segment_index);

          gimp_display_shell_constrain_line (
            gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (polygon)),
            start_point_x,
            start_point_y,
            &new_x,
            &new_y,
            GIMP_CONSTRAIN_LINE_15_DEGREES);
        }

      gimp_tool_polygon_move_segment_vertex_to (polygon,
                                                priv->grabbed_segment_index,
                                                new_x,
                                                new_y);

      /* We also must update the pending point if we are moving the
       * first point
       */
      if (priv->grabbed_segment_index == 0)
        {
          priv->pending_point.x = new_x;
          priv->pending_point.y = new_y;
        }
    }
  else
    {
      /* Don't show the pending point while we are adding points */
      priv->show_pending_point = FALSE;

      gimp_tool_polygon_add_point (polygon, new_x, new_y);
    }
}

static void
gimp_tool_polygon_status_update (GimpToolPolygon  *polygon,
                                 const GimpCoords *coords,
                                 gboolean          proximity)
{
  GimpToolWidget         *widget = GIMP_TOOL_WIDGET (polygon);
  GimpToolPolygonPrivate *priv   = polygon->private;

  if (proximity)
    {
      const gchar *status_text = NULL;

      if (gimp_tool_polygon_is_point_grabbed (polygon))
        {
          if (gimp_tool_polygon_should_close (polygon,
                                              NO_CLICK_TIME_AVAILABLE,
                                              coords))
            {
              status_text = _("Click to close shape");
            }
          else
            {
              status_text = _("Click-Drag to move segment vertex");
            }
        }
      else if (priv->polygon_closed)
        {
          status_text = _("Return commits, Escape cancels, Backspace re-opens shape");
        }
      else if (priv->n_segment_indices >= 3)
        {
          status_text = _("Return commits, Escape cancels, Backspace removes last segment");
        }
      else
        {
          status_text = _("Click-Drag adds a free segment, Click adds a polygonal segment");
        }

      if (status_text)
        {
          gimp_tool_widget_set_status (widget, status_text);
        }
    }
  else
    {
      gimp_tool_widget_set_status (widget, NULL);
    }
}

static void
gimp_tool_polygon_changed (GimpToolWidget *widget)
{
  GimpToolPolygon        *polygon               = GIMP_TOOL_POLYGON (widget);
  GimpToolPolygonPrivate *private               = polygon->private;
  gboolean                hovering_first_point  = FALSE;
  gboolean                handles_wants_to_show = FALSE;
  GimpCoords              coords                = { private->last_coords.x,
                                                    private->last_coords.y,
                                                    /* pad with 0 */ };
  gint                    i;

  gimp_canvas_polygon_set_points (private->polygon,
                                  private->points, private->n_points);

  if (private->show_pending_point)
    {
      GimpVector2 last = private->points[private->n_points - 1];

      gimp_canvas_line_set (private->pending_line,
                            last.x,
                            last.y,
                            private->pending_point.x,
                            private->pending_point.y);
    }

  gimp_canvas_item_set_visible (private->pending_line,
                                private->show_pending_point);

  if (private->polygon_closed)
    {
      GimpVector2 first = private->points[0];
      GimpVector2 last  = private->points[private->n_points - 1];

      gimp_canvas_line_set (private->closing_line,
                            first.x, first.y,
                            last.x,  last.y);
    }

  gimp_canvas_item_set_visible (private->closing_line,
                                private->polygon_closed);

  hovering_first_point =
    gimp_tool_polygon_should_close (polygon,
                                    NO_CLICK_TIME_AVAILABLE,
                                    &coords);

  /* We always show the handle for the first point, even with button1
   * down, since releasing the button on the first point will close
   * the polygon, so it's a significant state which we must give
   * feedback for
   */
  handles_wants_to_show = (hovering_first_point || ! private->button_down);

  for (i = 0; i < private->n_segment_indices; i++)
    {
      GimpCanvasItem *handle;
      GimpVector2    *point;

      handle = g_ptr_array_index (private->handles, i);
      point  = &private->points[private->segment_indices[i]];

      if (private->hover             &&
          handles_wants_to_show      &&
          ! private->suppress_handles &&

          /* If the first point is hovered while button1 is held down,
           * only draw the first handle, the other handles are not
           * relevant (see comment a few lines up)
           */
          (i == 0 ||
           ! (private->button_down || hovering_first_point)))
        {
          gdouble         dist;
          GimpHandleType  handle_type = -1;

          dist =
            gimp_canvas_item_transform_distance_square (handle,
                                                        private->last_coords.x,
                                                        private->last_coords.y,
                                                        point->x,
                                                        point->y);

          /* If the cursor is over the point, fill, if it's just
           * close, draw an outline
           */
          if (dist < POINT_GRAB_THRESHOLD_SQ)
            handle_type = GIMP_HANDLE_FILLED_CIRCLE;
          else if (dist < POINT_SHOW_THRESHOLD_SQ)
            handle_type = GIMP_HANDLE_CIRCLE;

          if (handle_type != -1)
            {
              gint size;

              gimp_canvas_item_begin_change (handle);

              gimp_canvas_handle_set_position (handle, point->x, point->y);

              g_object_set (handle,
                            "type", handle_type,
                            NULL);

              size = gimp_canvas_handle_calc_size (handle,
                                                   private->last_coords.x,
                                                   private->last_coords.y,
                                                   GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                   2 * GIMP_CANVAS_HANDLE_SIZE_CIRCLE);
              if (FALSE)
                gimp_canvas_handle_set_size (handle, size, size);

              if (dist < POINT_GRAB_THRESHOLD_SQ)
                gimp_canvas_item_set_highlight (handle, TRUE);
              else
                gimp_canvas_item_set_highlight (handle, FALSE);

              gimp_canvas_item_set_visible (handle, TRUE);

              gimp_canvas_item_end_change (handle);
            }
          else
            {
              gimp_canvas_item_set_visible (handle, FALSE);
            }
        }
      else
        {
          gimp_canvas_item_set_visible (handle, FALSE);
       }
    }
}

static gboolean
gimp_tool_polygon_button_press (GimpToolWidget      *widget,
                                const GimpCoords    *coords,
                                guint32              time,
                                GdkModifierType      state,
                                GimpButtonPressType  press_type)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (widget);
  GimpToolPolygonPrivate *priv    = polygon->private;

  if (gimp_tool_polygon_is_point_grabbed (polygon))
    {
      gimp_tool_polygon_prepare_for_move (polygon);
    }
  else if (priv->polygon_closed)
    {
      if (press_type == GIMP_BUTTON_PRESS_DOUBLE &&
          gimp_canvas_item_hit (priv->polygon, coords->x, coords->y))
        {
          gimp_tool_widget_response (widget, GIMP_TOOL_WIDGET_RESPONSE_CONFIRM);
        }

      return 0;
    }
  else
    {
      GimpVector2 point_to_add;

      /* Note that we add the pending point (unless it is the first
       * point we add) because the pending point is setup correctly
       * with regards to angle constraints.
       */
      if (priv->n_points > 0)
        {
          point_to_add = priv->pending_point;
        }
      else
        {
          point_to_add.x = coords->x;
          point_to_add.y = coords->y;
        }

      /* No point was grabbed, add a new point and mark this as a
       * segment divider. For a line segment, this will be the only
       * new point. For a free segment, this will be the first point
       * of the free segment.
       */
      gimp_tool_polygon_add_point (polygon,
                                   point_to_add.x,
                                   point_to_add.y);
      gimp_tool_polygon_add_segment_index (polygon, priv->n_points - 1);
    }

  priv->button_down = TRUE;

  gimp_tool_polygon_changed (widget);

  return 1;
}

static void
gimp_tool_polygon_button_release (GimpToolWidget        *widget,
                                  const GimpCoords      *coords,
                                  guint32                time,
                                  GdkModifierType        state,
                                  GimpButtonReleaseType  release_type)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (widget);
  GimpToolPolygonPrivate *priv    = polygon->private;

  g_object_ref (widget);

  priv->button_down = FALSE;

  switch (release_type)
    {
    case GIMP_BUTTON_RELEASE_CLICK:
    case GIMP_BUTTON_RELEASE_NO_MOTION:
      /* If a click was made, we don't consider the polygon modified */
      priv->polygon_modified = FALSE;

      /* First finish of the line segment if no point was grabbed */
      if (! gimp_tool_polygon_is_point_grabbed (polygon))
        {
          /* Revert any free segment points that might have been added */
          gimp_tool_polygon_revert_to_last_segment (polygon);
        }

      /* After the segments are up to date and we have handled
       * double-click, see if it's committing time
       */
      if (gimp_tool_polygon_should_close (polygon, time, coords))
        {
          /* We can get a click notification even though the end point
           * has been moved a few pixels. Since a move will change the
           * free selection, revert it before doing the commit.
           */
          gimp_tool_polygon_revert_to_saved_state (polygon);

          priv->polygon_closed = TRUE;

          gimp_tool_polygon_change_complete (polygon);
        }

      priv->last_click_time  = time;
      priv->last_click_coord = *coords;
      break;

    case GIMP_BUTTON_RELEASE_NORMAL:
      /* First finish of the free segment if no point was grabbed */
      if (! gimp_tool_polygon_is_point_grabbed (polygon))
        {
          /* The points are all setup, just make a segment */
          gimp_tool_polygon_add_segment_index (polygon, priv->n_points - 1);
        }

      /* After the segments are up to date, see if it's committing time */
      if (gimp_tool_polygon_should_close (polygon,
                                          NO_CLICK_TIME_AVAILABLE,
                                          coords))
        {
          priv->polygon_closed = TRUE;
        }

      gimp_tool_polygon_change_complete (polygon);
      break;

    case GIMP_BUTTON_RELEASE_CANCEL:
      if (gimp_tool_polygon_is_point_grabbed (polygon))
        gimp_tool_polygon_revert_to_saved_state (polygon);
      else
        gimp_tool_polygon_remove_last_segment (polygon);
      break;

    default:
      break;
    }

  /* Reset */
  priv->polygon_modified = FALSE;

  gimp_tool_polygon_changed (widget);

  g_object_unref (widget);
}

static void
gimp_tool_polygon_motion (GimpToolWidget   *widget,
                          const GimpCoords *coords,
                          guint32           time,
                          GdkModifierType   state)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (widget);
  GimpToolPolygonPrivate *priv    = polygon->private;

  priv->last_coords.x = coords->x;
  priv->last_coords.y = coords->y;

  gimp_tool_polygon_update_motion (polygon,
                                   coords->x,
                                   coords->y);

  gimp_tool_polygon_changed (widget);
}

static GimpHit
gimp_tool_polygon_hit (GimpToolWidget   *widget,
                       const GimpCoords *coords,
                       GdkModifierType   state,
                       gboolean          proximity)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (widget);
  GimpToolPolygonPrivate *priv    = polygon->private;

  if ((priv->n_points > 0 && ! priv->polygon_closed) ||
      gimp_tool_polygon_get_segment_index (polygon, coords) != INVALID_INDEX)
    {
      return GIMP_HIT_DIRECT;
    }
  else if (priv->polygon_closed &&
           gimp_canvas_item_hit (priv->polygon, coords->x, coords->y))
    {
      return GIMP_HIT_INDIRECT;
    }

  return GIMP_HIT_NONE;
}

static void
gimp_tool_polygon_hover (GimpToolWidget   *widget,
                         const GimpCoords *coords,
                         GdkModifierType   state,
                         gboolean          proximity)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (widget);
  GimpToolPolygonPrivate *priv    = polygon->private;
  gboolean                hovering_first_point;

  priv->grabbed_segment_index = gimp_tool_polygon_get_segment_index (polygon,
                                                                     coords);
  priv->hover                 = TRUE;

  hovering_first_point =
    gimp_tool_polygon_should_close (polygon,
                                    NO_CLICK_TIME_AVAILABLE,
                                    coords);

  priv->last_coords.x = coords->x;
  priv->last_coords.y = coords->y;

  if (priv->n_points == 0  ||
      priv->polygon_closed ||
      (gimp_tool_polygon_is_point_grabbed (polygon) &&
       ! hovering_first_point) ||
      ! proximity)
    {
      priv->show_pending_point = FALSE;
    }
  else
    {
      priv->show_pending_point = TRUE;

      if (hovering_first_point)
        {
          priv->pending_point = priv->points[0];
        }
      else
        {
          priv->pending_point.x = coords->x;
          priv->pending_point.y = coords->y;

          if (priv->constrain_angle && priv->n_points > 0)
            {
              gdouble start_point_x;
              gdouble start_point_y;

              /*  the last point is the line's start point  */
              gimp_tool_polygon_get_segment_point (polygon,
                                                   &start_point_x,
                                                   &start_point_y,
                                                   priv->n_segment_indices - 1);

              gimp_display_shell_constrain_line (
                gimp_tool_widget_get_shell (widget),
                start_point_x, start_point_y,
                &priv->pending_point.x,
                &priv->pending_point.y,
                GIMP_CONSTRAIN_LINE_15_DEGREES);
            }
        }
    }

  gimp_tool_polygon_status_update (polygon, coords, proximity);

  gimp_tool_polygon_changed (widget);
}

static void
gimp_tool_polygon_leave_notify (GimpToolWidget *widget)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (widget);
  GimpToolPolygonPrivate *priv    = polygon->private;

  priv->grabbed_segment_index = INVALID_INDEX;
  priv->hover                 = FALSE;
  priv->show_pending_point    = FALSE;

  gimp_tool_polygon_changed (widget);
}

static gboolean
gimp_tool_polygon_key_press (GimpToolWidget *widget,
                             GdkEventKey    *kevent)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (widget);
  GimpToolPolygonPrivate *priv    = polygon->private;

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      gimp_tool_polygon_remove_last_segment (polygon);

      if (priv->n_segment_indices > 0)
        gimp_tool_polygon_change_complete (polygon);
      return TRUE;

    default:
      break;
    }

  return GIMP_TOOL_WIDGET_CLASS (parent_class)->key_press (widget, kevent);
}

static void
gimp_tool_polygon_motion_modifier (GimpToolWidget  *widget,
                                   GdkModifierType  key,
                                   gboolean         press,
                                   GdkModifierType  state)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (widget);
  GimpToolPolygonPrivate *priv    = polygon->private;

  priv->constrain_angle = ((state & gimp_get_constrain_behavior_mask ()) ?
                           TRUE : FALSE);

  /* If we didn't came here due to a mouse release, immediately update
   * the position of the thing we move.
   */
  if (state & GDK_BUTTON1_MASK)
    {
      gimp_tool_polygon_update_motion (polygon,
                                       priv->last_coords.x,
                                       priv->last_coords.y);
    }

  gimp_tool_polygon_changed (widget);
}

static void
gimp_tool_polygon_hover_modifier (GimpToolWidget  *widget,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state)
{
  GimpToolPolygon        *polygon = GIMP_TOOL_POLYGON (widget);
  GimpToolPolygonPrivate *priv    = polygon->private;

  priv->constrain_angle = ((state & gimp_get_constrain_behavior_mask ()) ?
                           TRUE : FALSE);

  priv->suppress_handles = ((state & gimp_get_extend_selection_mask ()) ?
                           TRUE : FALSE);

  gimp_tool_polygon_changed (widget);
}

static gboolean
gimp_tool_polygon_get_cursor (GimpToolWidget     *widget,
                              const GimpCoords   *coords,
                              GdkModifierType     state,
                              GimpCursorType     *cursor,
                              GimpToolCursorType *tool_cursor,
                              GimpCursorModifier *modifier)
{
  GimpToolPolygon *polygon = GIMP_TOOL_POLYGON (widget);

  if (gimp_tool_polygon_is_point_grabbed (polygon) &&
      ! gimp_tool_polygon_should_close (polygon,
                                        NO_CLICK_TIME_AVAILABLE,
                                        coords))
    {
      *modifier = GIMP_CURSOR_MODIFIER_MOVE;

      return TRUE;
    }

  return FALSE;
}

static void
gimp_tool_polygon_change_complete (GimpToolPolygon *polygon)
{
  g_signal_emit (polygon, polygon_signals[CHANGE_COMPLETE], 0);
}

static gint
gimp_tool_polygon_get_segment_index (GimpToolPolygon  *polygon,
                                     const GimpCoords *coords)
{
  GimpToolPolygonPrivate *priv          = polygon->private;
  gint                    segment_index = INVALID_INDEX;

  if (! priv->suppress_handles)
    {
      gdouble shortest_dist = POINT_GRAB_THRESHOLD_SQ;
      gint    i;

      for (i = 0; i < priv->n_segment_indices; i++)
        {
          gdouble      dist;
          GimpVector2 *point;

          point = &priv->points[priv->segment_indices[i]];

          dist = gimp_canvas_item_transform_distance_square (priv->polygon,
                                                             coords->x,
                                                             coords->y,
                                                             point->x,
                                                             point->y);

          if (dist < shortest_dist)
            {
              shortest_dist = dist;

              segment_index = i;
            }
        }
    }

  return segment_index;
}


/*  public functions  */

GimpToolWidget *
gimp_tool_polygon_new (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_POLYGON,
                       "shell", shell,
                       NULL);
}

gboolean
gimp_tool_polygon_is_closed (GimpToolPolygon *polygon)
{
  GimpToolPolygonPrivate *private;

  g_return_val_if_fail (GIMP_IS_TOOL_POLYGON (polygon), FALSE);

  private = polygon->private;

  return private->polygon_closed;
}

void
gimp_tool_polygon_get_points (GimpToolPolygon    *polygon,
                              const GimpVector2 **points,
                              gint               *n_points)
{
  GimpToolPolygonPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_POLYGON (polygon));

  private = polygon->private;

  if (points)   *points   = private->points;
  if (n_points) *n_points = private->n_points;
}
