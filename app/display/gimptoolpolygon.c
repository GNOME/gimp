/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolpolygon.c
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
 *
 * Based on LigmaFreeSelectTool
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "core/ligma-utils.h"

#include "widgets/ligmawidgets-utils.h"

#include "ligmacanvashandle.h"
#include "ligmacanvasline.h"
#include "ligmacanvaspolygon.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-utils.h"
#include "ligmatoolpolygon.h"

#include "ligma-intl.h"


#define POINT_GRAB_THRESHOLD_SQ SQR (LIGMA_CANVAS_HANDLE_SIZE_CIRCLE / 2)
#define POINT_SHOW_THRESHOLD_SQ SQR (LIGMA_CANVAS_HANDLE_SIZE_CIRCLE * 7)
#define N_ITEMS_PER_ALLOC       1024
#define INVALID_INDEX           (-1)
#define NO_CLICK_TIME_AVAILABLE 0


enum
{
  CHANGE_COMPLETE,
  LAST_SIGNAL
};


struct _LigmaToolPolygonPrivate
{
  /* Index of grabbed segment index. */
  gint               grabbed_segment_index;

  /* We need to keep track of a number of points when we move a
   * segment vertex
   */
  LigmaVector2       *saved_points_lower_segment;
  LigmaVector2       *saved_points_higher_segment;
  gint               max_n_saved_points_lower_segment;
  gint               max_n_saved_points_higher_segment;

  /* Keeps track whether or not a modification of the polygon has been
   * made between _button_press and _button_release
   */
  gboolean           polygon_modified;

  /* Point which is used to draw the polygon but which is not part of
   * it yet
   */
  LigmaVector2        pending_point;
  gboolean           show_pending_point;

  /* The points of the polygon */
  LigmaVector2       *points;
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
  LigmaVector2        last_coords;

  /* A double-click commits the selection, keep track of last
   * click-time and click-position.
   */
  guint32            last_click_time;
  LigmaCoords         last_click_coord;

  /* Are we in a click-drag-release? */
  gboolean           button_down;

  LigmaCanvasItem    *polygon;
  LigmaCanvasItem    *pending_line;
  LigmaCanvasItem    *closing_line;
  GPtrArray         *handles;
};


/*  local function prototypes  */

static void     ligma_tool_polygon_constructed     (GObject               *object);
static void     ligma_tool_polygon_finalize        (GObject               *object);
static void     ligma_tool_polygon_set_property    (GObject               *object,
                                                   guint                  property_id,
                                                   const GValue          *value,
                                                   GParamSpec            *pspec);
static void     ligma_tool_polygon_get_property    (GObject               *object,
                                                   guint                  property_id,
                                                   GValue                *value,
                                                   GParamSpec            *pspec);

static void     ligma_tool_polygon_changed         (LigmaToolWidget        *widget);
static gint     ligma_tool_polygon_button_press    (LigmaToolWidget        *widget,
                                                   const LigmaCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   LigmaButtonPressType    press_type);
static void     ligma_tool_polygon_button_release  (LigmaToolWidget        *widget,
                                                   const LigmaCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   LigmaButtonReleaseType  release_type);
static void     ligma_tool_polygon_motion          (LigmaToolWidget        *widget,
                                                   const LigmaCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state);
static LigmaHit  ligma_tool_polygon_hit             (LigmaToolWidget        *widget,
                                                   const LigmaCoords      *coords,
                                                   GdkModifierType        state,
                                                   gboolean               proximity);
static void     ligma_tool_polygon_hover           (LigmaToolWidget        *widget,
                                                   const LigmaCoords      *coords,
                                                   GdkModifierType        state,
                                                   gboolean               proximity);
static void     ligma_tool_polygon_leave_notify    (LigmaToolWidget        *widget);
static gboolean ligma_tool_polygon_key_press       (LigmaToolWidget        *widget,
                                                   GdkEventKey           *kevent);
static void     ligma_tool_polygon_motion_modifier (LigmaToolWidget        *widget,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state);
static void     ligma_tool_polygon_hover_modifier  (LigmaToolWidget        *widget,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state);
static gboolean ligma_tool_polygon_get_cursor      (LigmaToolWidget        *widget,
                                                   const LigmaCoords      *coords,
                                                   GdkModifierType        state,
                                                   LigmaCursorType        *cursor,
                                                   LigmaToolCursorType    *tool_cursor,
                                                   LigmaCursorModifier    *modifier);

static void     ligma_tool_polygon_change_complete (LigmaToolPolygon       *polygon);

static gint   ligma_tool_polygon_get_segment_index (LigmaToolPolygon       *polygon,
                                                   const LigmaCoords      *coords);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolPolygon, ligma_tool_polygon,
                            LIGMA_TYPE_TOOL_WIDGET)

#define parent_class ligma_tool_polygon_parent_class

static guint polygon_signals[LAST_SIGNAL] = { 0, };

static const LigmaVector2 vector2_zero = { 0.0, 0.0 };


static void
ligma_tool_polygon_class_init (LigmaToolPolygonClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaToolWidgetClass *widget_class = LIGMA_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = ligma_tool_polygon_constructed;
  object_class->finalize        = ligma_tool_polygon_finalize;
  object_class->set_property    = ligma_tool_polygon_set_property;
  object_class->get_property    = ligma_tool_polygon_get_property;

  widget_class->changed         = ligma_tool_polygon_changed;
  widget_class->button_press    = ligma_tool_polygon_button_press;
  widget_class->button_release  = ligma_tool_polygon_button_release;
  widget_class->motion          = ligma_tool_polygon_motion;
  widget_class->hit             = ligma_tool_polygon_hit;
  widget_class->hover           = ligma_tool_polygon_hover;
  widget_class->leave_notify    = ligma_tool_polygon_leave_notify;
  widget_class->key_press       = ligma_tool_polygon_key_press;
  widget_class->motion_modifier = ligma_tool_polygon_motion_modifier;
  widget_class->hover_modifier  = ligma_tool_polygon_hover_modifier;
  widget_class->get_cursor      = ligma_tool_polygon_get_cursor;

  polygon_signals[CHANGE_COMPLETE] =
    g_signal_new ("change-complete",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolPolygonClass, change_complete),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
ligma_tool_polygon_init (LigmaToolPolygon *polygon)
{
  polygon->private = ligma_tool_polygon_get_instance_private (polygon);

  polygon->private->grabbed_segment_index = INVALID_INDEX;
  polygon->private->last_click_time       = NO_CLICK_TIME_AVAILABLE;
}

static void
ligma_tool_polygon_constructed (GObject *object)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (object);
  LigmaToolWidget         *widget  = LIGMA_TOOL_WIDGET (object);
  LigmaToolPolygonPrivate *private = polygon->private;
  LigmaCanvasGroup        *stroke_group;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  stroke_group = ligma_tool_widget_add_stroke_group (widget);

  ligma_tool_widget_push_group (widget, stroke_group);

  private->polygon = ligma_tool_widget_add_polygon (widget, NULL,
                                                   NULL, 0, FALSE);

  private->pending_line = ligma_tool_widget_add_line (widget, 0, 0, 0, 0);
  private->closing_line = ligma_tool_widget_add_line (widget, 0, 0, 0, 0);

  ligma_tool_widget_pop_group (widget);

  private->handles = g_ptr_array_new ();

  ligma_tool_polygon_changed (widget);
}

static void
ligma_tool_polygon_finalize (GObject *object)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (object);
  LigmaToolPolygonPrivate *private = polygon->private;

  g_free (private->points);
  g_free (private->segment_indices);
  g_free (private->saved_points_lower_segment);
  g_free (private->saved_points_higher_segment);

  g_clear_pointer (&private->handles, g_ptr_array_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_tool_polygon_set_property (GObject      *object,
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
ligma_tool_polygon_get_property (GObject    *object,
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
ligma_tool_polygon_get_segment (LigmaToolPolygon  *polygon,
                               LigmaVector2     **points,
                               gint             *n_points,
                               gint              segment_start,
                               gint              segment_end)
{
  LigmaToolPolygonPrivate *priv = polygon->private;

  *points   = &priv->points[priv->segment_indices[segment_start]];
  *n_points = priv->segment_indices[segment_end] -
              priv->segment_indices[segment_start] +
              1;
}

static void
ligma_tool_polygon_get_segment_point (LigmaToolPolygon *polygon,
                                     gdouble         *start_point_x,
                                     gdouble         *start_point_y,
                                     gint             segment_index)
{
  LigmaToolPolygonPrivate *priv = polygon->private;

  *start_point_x = priv->points[priv->segment_indices[segment_index]].x;
  *start_point_y = priv->points[priv->segment_indices[segment_index]].y;
}

static gboolean
ligma_tool_polygon_should_close (LigmaToolPolygon  *polygon,
                                guint32           time,
                                const LigmaCoords *coords)
{
  LigmaToolWidget         *widget       = LIGMA_TOOL_WIDGET (polygon);
  LigmaToolPolygonPrivate *priv         = polygon->private;
  gboolean                double_click = FALSE;
  gdouble                 dist;

  if (priv->polygon_modified      ||
      priv->n_segment_indices < 1 ||
      priv->n_points          < 3 ||
      priv->polygon_closed)
    return FALSE;

  dist = ligma_canvas_item_transform_distance_square (priv->polygon,
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
      LigmaDisplayShell *shell    = ligma_tool_widget_get_shell (widget);
      GtkSettings      *settings = gtk_widget_get_settings (GTK_WIDGET (shell));
      gint              double_click_time;
      gint              double_click_distance;
      gint              click_time_passed;
      gdouble           dist_from_last_point;

      click_time_passed = time - priv->last_click_time;

      dist_from_last_point =
        ligma_canvas_item_transform_distance_square (priv->polygon,
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
ligma_tool_polygon_revert_to_last_segment (LigmaToolPolygon *polygon)
{
  LigmaToolPolygonPrivate *priv = polygon->private;

  priv->n_points = priv->segment_indices[priv->n_segment_indices - 1] + 1;
}

static void
ligma_tool_polygon_remove_last_segment (LigmaToolPolygon *polygon)
{
  LigmaToolPolygonPrivate *priv = polygon->private;

  if (priv->polygon_closed)
    {
      priv->polygon_closed = FALSE;

      ligma_tool_polygon_changed (LIGMA_TOOL_WIDGET (polygon));

      return;
    }

  if (priv->n_segment_indices > 0)
    {
      LigmaCanvasItem *handle;

      priv->n_segment_indices--;

      handle = g_ptr_array_index (priv->handles,
                                  priv->n_segment_indices);

      ligma_tool_widget_remove_item (LIGMA_TOOL_WIDGET (polygon), handle);
      g_ptr_array_remove (priv->handles, handle);
    }

  if (priv->n_segment_indices <= 0)
    {
      priv->grabbed_segment_index = INVALID_INDEX;
      priv->show_pending_point    = FALSE;
      priv->n_points              = 0;
      priv->n_segment_indices     = 0;

      ligma_tool_widget_response (LIGMA_TOOL_WIDGET (polygon),
                                 LIGMA_TOOL_WIDGET_RESPONSE_CANCEL);
    }
  else
    {
      ligma_tool_polygon_revert_to_last_segment (polygon);

      ligma_tool_polygon_changed (LIGMA_TOOL_WIDGET (polygon));
    }
}

static void
ligma_tool_polygon_add_point (LigmaToolPolygon *polygon,
                             gdouble          x,
                             gdouble          y)
{
  LigmaToolPolygonPrivate *priv = polygon->private;

  if (priv->n_points >= priv->max_n_points)
    {
      priv->max_n_points += N_ITEMS_PER_ALLOC;

      priv->points = g_realloc (priv->points,
                                sizeof (LigmaVector2) * priv->max_n_points);
    }

  priv->points[priv->n_points].x = x;
  priv->points[priv->n_points].y = y;

  priv->n_points++;
}

static void
ligma_tool_polygon_add_segment_index (LigmaToolPolygon *polygon,
                                     gint             index)
{
  LigmaToolPolygonPrivate *priv = polygon->private;

  if (priv->n_segment_indices >= priv->max_n_segment_indices)
    {
      priv->max_n_segment_indices += N_ITEMS_PER_ALLOC;

      priv->segment_indices = g_realloc (priv->segment_indices,
                                         sizeof (LigmaVector2) *
                                         priv->max_n_segment_indices);
    }

  priv->segment_indices[priv->n_segment_indices] = index;

  g_ptr_array_add (priv->handles,
                   ligma_tool_widget_add_handle (LIGMA_TOOL_WIDGET (polygon),
                                                LIGMA_HANDLE_CROSS,
                                                0, 0,
                                                LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                LIGMA_HANDLE_ANCHOR_CENTER));

  priv->n_segment_indices++;
}

static gboolean
ligma_tool_polygon_is_point_grabbed (LigmaToolPolygon *polygon)
{
  LigmaToolPolygonPrivate *priv = polygon->private;

  return priv->grabbed_segment_index != INVALID_INDEX;
}

static void
ligma_tool_polygon_fit_segment (LigmaToolPolygon   *polygon,
                               LigmaVector2       *dest_points,
                               LigmaVector2        dest_start_target,
                               LigmaVector2        dest_end_target,
                               const LigmaVector2 *source_points,
                               gint               n_points)
{
  LigmaVector2 origo_translation_offset;
  LigmaVector2 untranslation_offset;
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
  memcpy (dest_points, source_points, sizeof (LigmaVector2) * n_points);

  /* Transform the destination end point */
  {
    LigmaVector2 *dest_end;
    LigmaVector2  origo_translated_end_target;
    gdouble      target_rotation;
    gdouble      current_rotation;
    gdouble      target_length;
    gdouble      current_length;

    dest_end = &dest_points[n_points - 1];

    /* Transate to origin */
    ligma_vector2_sub (&origo_translation_offset,
                      &vector2_zero,
                      &dest_points[0]);
    ligma_vector2_add (dest_end,
                      dest_end,
                      &origo_translation_offset);

    /* Calculate origo_translated_end_target */
    ligma_vector2_sub (&origo_translated_end_target,
                      &dest_end_target,
                      &dest_start_target);

    /* Rotate */
    target_rotation  = atan2 (vector2_zero.y - origo_translated_end_target.y,
                              vector2_zero.x - origo_translated_end_target.x);
    current_rotation = atan2 (vector2_zero.y - dest_end->y,
                              vector2_zero.x - dest_end->x);
    rotation         = current_rotation - target_rotation;

    ligma_vector2_rotate (dest_end, rotation);


    /* Scale */
    target_length  = ligma_vector2_length (&origo_translated_end_target);
    current_length = ligma_vector2_length (dest_end);
    scale          = target_length / current_length;

    ligma_vector2_mul (dest_end, scale);


    /* Untranslate */
    ligma_vector2_sub (&untranslation_offset,
                      &dest_end_target,
                      dest_end);
    ligma_vector2_add (dest_end,
                      dest_end,
                      &untranslation_offset);
  }

  /* Do the same transformation for the rest of the points */
  {
    gint i;

    for (i = 0; i < n_points - 1; i++)
      {
        /* Translate */
        ligma_vector2_add (&dest_points[i],
                          &origo_translation_offset,
                          &dest_points[i]);

        /* Rotate */
        ligma_vector2_rotate (&dest_points[i],
                             rotation);

        /* Scale */
        ligma_vector2_mul (&dest_points[i],
                          scale);

        /* Untranslate */
        ligma_vector2_add (&dest_points[i],
                          &dest_points[i],
                          &untranslation_offset);
      }
  }
}

static void
ligma_tool_polygon_move_segment_vertex_to (LigmaToolPolygon *polygon,
                                          gint             segment_index,
                                          gdouble          new_x,
                                          gdouble          new_y)
{
  LigmaToolPolygonPrivate *priv         = polygon->private;
  LigmaVector2             cursor_point = { new_x, new_y };
  LigmaVector2            *dest;
  LigmaVector2            *dest_start_target;
  LigmaVector2            *dest_end_target;
  gint                    n_points;

  /* Handle the segment before the grabbed point */
  if (segment_index > 0)
    {
      ligma_tool_polygon_get_segment (polygon,
                                     &dest,
                                     &n_points,
                                     priv->grabbed_segment_index - 1,
                                     priv->grabbed_segment_index);

      dest_start_target = &dest[0];
      dest_end_target   = &cursor_point;

      ligma_tool_polygon_fit_segment (polygon,
                                     dest,
                                     *dest_start_target,
                                     *dest_end_target,
                                     priv->saved_points_lower_segment,
                                     n_points);
    }

  /* Handle the segment after the grabbed point */
  if (segment_index < priv->n_segment_indices - 1)
    {
      ligma_tool_polygon_get_segment (polygon,
                                     &dest,
                                     &n_points,
                                     priv->grabbed_segment_index,
                                     priv->grabbed_segment_index + 1);

      dest_start_target = &cursor_point;
      dest_end_target   = &dest[n_points - 1];

      ligma_tool_polygon_fit_segment (polygon,
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
ligma_tool_polygon_revert_to_saved_state (LigmaToolPolygon *polygon)
{
  LigmaToolPolygonPrivate *priv = polygon->private;
  LigmaVector2            *dest;
  gint                    n_points;

  /* Without a point grab we have no sensible information to fall back
   * on, bail out
   */
  if (! ligma_tool_polygon_is_point_grabbed (polygon))
    {
      return;
    }

  if (priv->grabbed_segment_index > 0)
    {
      ligma_tool_polygon_get_segment (polygon,
                                     &dest,
                                     &n_points,
                                     priv->grabbed_segment_index - 1,
                                     priv->grabbed_segment_index);

      memcpy (dest,
              priv->saved_points_lower_segment,
              sizeof (LigmaVector2) * n_points);
    }

  if (priv->grabbed_segment_index < priv->n_segment_indices - 1)
    {
      ligma_tool_polygon_get_segment (polygon,
                                     &dest,
                                     &n_points,
                                     priv->grabbed_segment_index,
                                     priv->grabbed_segment_index + 1);

      memcpy (dest,
              priv->saved_points_higher_segment,
              sizeof (LigmaVector2) * n_points);
    }

  if (priv->grabbed_segment_index == 0 &&
      priv->n_segment_indices     == 1)
    {
      priv->points[0] = *priv->saved_points_lower_segment;
    }
}

static void
ligma_tool_polygon_prepare_for_move (LigmaToolPolygon *polygon)
{
  LigmaToolPolygonPrivate *priv = polygon->private;
  LigmaVector2            *source;
  gint                    n_points;

  if (priv->grabbed_segment_index > 0)
    {
      ligma_tool_polygon_get_segment (polygon,
                                     &source,
                                     &n_points,
                                     priv->grabbed_segment_index - 1,
                                     priv->grabbed_segment_index);

      if (n_points > priv->max_n_saved_points_lower_segment)
        {
          priv->max_n_saved_points_lower_segment = n_points;

          priv->saved_points_lower_segment =
            g_realloc (priv->saved_points_lower_segment,
                       sizeof (LigmaVector2) * n_points);
        }

      memcpy (priv->saved_points_lower_segment,
              source,
              sizeof (LigmaVector2) * n_points);
    }

  if (priv->grabbed_segment_index < priv->n_segment_indices - 1)
    {
      ligma_tool_polygon_get_segment (polygon,
                                     &source,
                                     &n_points,
                                     priv->grabbed_segment_index,
                                     priv->grabbed_segment_index + 1);

      if (n_points > priv->max_n_saved_points_higher_segment)
        {
          priv->max_n_saved_points_higher_segment = n_points;

          priv->saved_points_higher_segment =
            g_realloc (priv->saved_points_higher_segment,
                       sizeof (LigmaVector2) * n_points);
        }

      memcpy (priv->saved_points_higher_segment,
              source,
              sizeof (LigmaVector2) * n_points);
    }

  /* A special-case when there only is one point */
  if (priv->grabbed_segment_index == 0 &&
      priv->n_segment_indices     == 1)
    {
      if (priv->max_n_saved_points_lower_segment == 0)
        {
          priv->max_n_saved_points_lower_segment = 1;

          priv->saved_points_lower_segment = g_new0 (LigmaVector2, 1);
        }

      *priv->saved_points_lower_segment = priv->points[0];
    }
}

static void
ligma_tool_polygon_update_motion (LigmaToolPolygon *polygon,
                                 gdouble          new_x,
                                 gdouble          new_y)
{
  LigmaToolPolygonPrivate *priv = polygon->private;

  if (ligma_tool_polygon_is_point_grabbed (polygon))
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

          ligma_tool_polygon_get_segment_point (polygon,
                                               &start_point_x,
                                               &start_point_y,
                                               segment_index);

          ligma_display_shell_constrain_line (
            ligma_tool_widget_get_shell (LIGMA_TOOL_WIDGET (polygon)),
            start_point_x,
            start_point_y,
            &new_x,
            &new_y,
            LIGMA_CONSTRAIN_LINE_15_DEGREES);
        }

      ligma_tool_polygon_move_segment_vertex_to (polygon,
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

      ligma_tool_polygon_add_point (polygon, new_x, new_y);
    }
}

static void
ligma_tool_polygon_status_update (LigmaToolPolygon  *polygon,
                                 const LigmaCoords *coords,
                                 gboolean          proximity)
{
  LigmaToolWidget         *widget = LIGMA_TOOL_WIDGET (polygon);
  LigmaToolPolygonPrivate *priv   = polygon->private;

  if (proximity)
    {
      const gchar *status_text = NULL;

      if (ligma_tool_polygon_is_point_grabbed (polygon))
        {
          if (ligma_tool_polygon_should_close (polygon,
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
          ligma_tool_widget_set_status (widget, status_text);
        }
    }
  else
    {
      ligma_tool_widget_set_status (widget, NULL);
    }
}

static void
ligma_tool_polygon_changed (LigmaToolWidget *widget)
{
  LigmaToolPolygon        *polygon               = LIGMA_TOOL_POLYGON (widget);
  LigmaToolPolygonPrivate *private               = polygon->private;
  gboolean                hovering_first_point  = FALSE;
  gboolean                handles_wants_to_show = FALSE;
  LigmaCoords              coords                = { private->last_coords.x,
                                                    private->last_coords.y,
                                                    /* pad with 0 */ };
  gint                    i;

  ligma_canvas_polygon_set_points (private->polygon,
                                  private->points, private->n_points);

  if (private->show_pending_point)
    {
      LigmaVector2 last = private->points[private->n_points - 1];

      ligma_canvas_line_set (private->pending_line,
                            last.x,
                            last.y,
                            private->pending_point.x,
                            private->pending_point.y);
    }

  ligma_canvas_item_set_visible (private->pending_line,
                                private->show_pending_point);

  if (private->polygon_closed)
    {
      LigmaVector2 first = private->points[0];
      LigmaVector2 last  = private->points[private->n_points - 1];

      ligma_canvas_line_set (private->closing_line,
                            first.x, first.y,
                            last.x,  last.y);
    }

  ligma_canvas_item_set_visible (private->closing_line,
                                private->polygon_closed);

  hovering_first_point =
    ligma_tool_polygon_should_close (polygon,
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
      LigmaCanvasItem *handle;
      LigmaVector2    *point;

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
          LigmaHandleType  handle_type = -1;

          dist =
            ligma_canvas_item_transform_distance_square (handle,
                                                        private->last_coords.x,
                                                        private->last_coords.y,
                                                        point->x,
                                                        point->y);

          /* If the cursor is over the point, fill, if it's just
           * close, draw an outline
           */
          if (dist < POINT_GRAB_THRESHOLD_SQ)
            handle_type = LIGMA_HANDLE_FILLED_CIRCLE;
          else if (dist < POINT_SHOW_THRESHOLD_SQ)
            handle_type = LIGMA_HANDLE_CIRCLE;

          if (handle_type != -1)
            {
              gint size;

              ligma_canvas_item_begin_change (handle);

              ligma_canvas_handle_set_position (handle, point->x, point->y);

              g_object_set (handle,
                            "type", handle_type,
                            NULL);

              size = ligma_canvas_handle_calc_size (handle,
                                                   private->last_coords.x,
                                                   private->last_coords.y,
                                                   LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                   2 * LIGMA_CANVAS_HANDLE_SIZE_CIRCLE);
              if (FALSE)
                ligma_canvas_handle_set_size (handle, size, size);

              if (dist < POINT_GRAB_THRESHOLD_SQ)
                ligma_canvas_item_set_highlight (handle, TRUE);
              else
                ligma_canvas_item_set_highlight (handle, FALSE);

              ligma_canvas_item_set_visible (handle, TRUE);

              ligma_canvas_item_end_change (handle);
            }
          else
            {
              ligma_canvas_item_set_visible (handle, FALSE);
            }
        }
      else
        {
          ligma_canvas_item_set_visible (handle, FALSE);
       }
    }
}

static gboolean
ligma_tool_polygon_button_press (LigmaToolWidget      *widget,
                                const LigmaCoords    *coords,
                                guint32              time,
                                GdkModifierType      state,
                                LigmaButtonPressType  press_type)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (widget);
  LigmaToolPolygonPrivate *priv    = polygon->private;

  if (ligma_tool_polygon_is_point_grabbed (polygon))
    {
      ligma_tool_polygon_prepare_for_move (polygon);
    }
  else if (priv->polygon_closed)
    {
      if (press_type == LIGMA_BUTTON_PRESS_DOUBLE &&
          ligma_canvas_item_hit (priv->polygon, coords->x, coords->y))
        {
          ligma_tool_widget_response (widget, LIGMA_TOOL_WIDGET_RESPONSE_CONFIRM);
        }

      return 0;
    }
  else
    {
      LigmaVector2 point_to_add;

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
      ligma_tool_polygon_add_point (polygon,
                                   point_to_add.x,
                                   point_to_add.y);
      ligma_tool_polygon_add_segment_index (polygon, priv->n_points - 1);
    }

  priv->button_down = TRUE;

  ligma_tool_polygon_changed (widget);

  return 1;
}

static void
ligma_tool_polygon_button_release (LigmaToolWidget        *widget,
                                  const LigmaCoords      *coords,
                                  guint32                time,
                                  GdkModifierType        state,
                                  LigmaButtonReleaseType  release_type)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (widget);
  LigmaToolPolygonPrivate *priv    = polygon->private;

  g_object_ref (widget);

  priv->button_down = FALSE;

  switch (release_type)
    {
    case LIGMA_BUTTON_RELEASE_CLICK:
    case LIGMA_BUTTON_RELEASE_NO_MOTION:
      /* If a click was made, we don't consider the polygon modified */
      priv->polygon_modified = FALSE;

      /* First finish of the line segment if no point was grabbed */
      if (! ligma_tool_polygon_is_point_grabbed (polygon))
        {
          /* Revert any free segment points that might have been added */
          ligma_tool_polygon_revert_to_last_segment (polygon);
        }

      /* After the segments are up to date and we have handled
       * double-click, see if it's committing time
       */
      if (ligma_tool_polygon_should_close (polygon, time, coords))
        {
          /* We can get a click notification even though the end point
           * has been moved a few pixels. Since a move will change the
           * free selection, revert it before doing the commit.
           */
          ligma_tool_polygon_revert_to_saved_state (polygon);

          priv->polygon_closed = TRUE;

          ligma_tool_polygon_change_complete (polygon);
        }

      priv->last_click_time  = time;
      priv->last_click_coord = *coords;
      break;

    case LIGMA_BUTTON_RELEASE_NORMAL:
      /* First finish of the free segment if no point was grabbed */
      if (! ligma_tool_polygon_is_point_grabbed (polygon))
        {
          /* The points are all setup, just make a segment */
          ligma_tool_polygon_add_segment_index (polygon, priv->n_points - 1);
        }

      /* After the segments are up to date, see if it's committing time */
      if (ligma_tool_polygon_should_close (polygon,
                                          NO_CLICK_TIME_AVAILABLE,
                                          coords))
        {
          priv->polygon_closed = TRUE;
        }

      ligma_tool_polygon_change_complete (polygon);
      break;

    case LIGMA_BUTTON_RELEASE_CANCEL:
      if (ligma_tool_polygon_is_point_grabbed (polygon))
        ligma_tool_polygon_revert_to_saved_state (polygon);
      else
        ligma_tool_polygon_remove_last_segment (polygon);
      break;

    default:
      break;
    }

  /* Reset */
  priv->polygon_modified = FALSE;

  ligma_tool_polygon_changed (widget);

  g_object_unref (widget);
}

static void
ligma_tool_polygon_motion (LigmaToolWidget   *widget,
                          const LigmaCoords *coords,
                          guint32           time,
                          GdkModifierType   state)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (widget);
  LigmaToolPolygonPrivate *priv    = polygon->private;

  priv->last_coords.x = coords->x;
  priv->last_coords.y = coords->y;

  ligma_tool_polygon_update_motion (polygon,
                                   coords->x,
                                   coords->y);

  ligma_tool_polygon_changed (widget);
}

static LigmaHit
ligma_tool_polygon_hit (LigmaToolWidget   *widget,
                       const LigmaCoords *coords,
                       GdkModifierType   state,
                       gboolean          proximity)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (widget);
  LigmaToolPolygonPrivate *priv    = polygon->private;

  if ((priv->n_points > 0 && ! priv->polygon_closed) ||
      ligma_tool_polygon_get_segment_index (polygon, coords) != INVALID_INDEX)
    {
      return LIGMA_HIT_DIRECT;
    }
  else if (priv->polygon_closed &&
           ligma_canvas_item_hit (priv->polygon, coords->x, coords->y))
    {
      return LIGMA_HIT_INDIRECT;
    }

  return LIGMA_HIT_NONE;
}

static void
ligma_tool_polygon_hover (LigmaToolWidget   *widget,
                         const LigmaCoords *coords,
                         GdkModifierType   state,
                         gboolean          proximity)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (widget);
  LigmaToolPolygonPrivate *priv    = polygon->private;
  gboolean                hovering_first_point;

  priv->grabbed_segment_index = ligma_tool_polygon_get_segment_index (polygon,
                                                                     coords);
  priv->hover                 = TRUE;

  hovering_first_point =
    ligma_tool_polygon_should_close (polygon,
                                    NO_CLICK_TIME_AVAILABLE,
                                    coords);

  priv->last_coords.x = coords->x;
  priv->last_coords.y = coords->y;

  if (priv->n_points == 0  ||
      priv->polygon_closed ||
      (ligma_tool_polygon_is_point_grabbed (polygon) &&
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
              ligma_tool_polygon_get_segment_point (polygon,
                                                   &start_point_x,
                                                   &start_point_y,
                                                   priv->n_segment_indices - 1);

              ligma_display_shell_constrain_line (
                ligma_tool_widget_get_shell (widget),
                start_point_x, start_point_y,
                &priv->pending_point.x,
                &priv->pending_point.y,
                LIGMA_CONSTRAIN_LINE_15_DEGREES);
            }
        }
    }

  ligma_tool_polygon_status_update (polygon, coords, proximity);

  ligma_tool_polygon_changed (widget);
}

static void
ligma_tool_polygon_leave_notify (LigmaToolWidget *widget)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (widget);
  LigmaToolPolygonPrivate *priv    = polygon->private;

  priv->grabbed_segment_index = INVALID_INDEX;
  priv->hover                 = FALSE;
  priv->show_pending_point    = FALSE;

  ligma_tool_polygon_changed (widget);
}

static gboolean
ligma_tool_polygon_key_press (LigmaToolWidget *widget,
                             GdkEventKey    *kevent)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (widget);
  LigmaToolPolygonPrivate *priv    = polygon->private;

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      ligma_tool_polygon_remove_last_segment (polygon);

      if (priv->n_segment_indices > 0)
        ligma_tool_polygon_change_complete (polygon);
      return TRUE;

    default:
      break;
    }

  return LIGMA_TOOL_WIDGET_CLASS (parent_class)->key_press (widget, kevent);
}

static void
ligma_tool_polygon_motion_modifier (LigmaToolWidget  *widget,
                                   GdkModifierType  key,
                                   gboolean         press,
                                   GdkModifierType  state)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (widget);
  LigmaToolPolygonPrivate *priv    = polygon->private;

  priv->constrain_angle = ((state & ligma_get_constrain_behavior_mask ()) ?
                           TRUE : FALSE);

  /* If we didn't came here due to a mouse release, immediately update
   * the position of the thing we move.
   */
  if (state & GDK_BUTTON1_MASK)
    {
      ligma_tool_polygon_update_motion (polygon,
                                       priv->last_coords.x,
                                       priv->last_coords.y);
    }

  ligma_tool_polygon_changed (widget);
}

static void
ligma_tool_polygon_hover_modifier (LigmaToolWidget  *widget,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state)
{
  LigmaToolPolygon        *polygon = LIGMA_TOOL_POLYGON (widget);
  LigmaToolPolygonPrivate *priv    = polygon->private;

  priv->constrain_angle = ((state & ligma_get_constrain_behavior_mask ()) ?
                           TRUE : FALSE);

  priv->suppress_handles = ((state & ligma_get_extend_selection_mask ()) ?
                           TRUE : FALSE);

  ligma_tool_polygon_changed (widget);
}

static gboolean
ligma_tool_polygon_get_cursor (LigmaToolWidget     *widget,
                              const LigmaCoords   *coords,
                              GdkModifierType     state,
                              LigmaCursorType     *cursor,
                              LigmaToolCursorType *tool_cursor,
                              LigmaCursorModifier *modifier)
{
  LigmaToolPolygon *polygon = LIGMA_TOOL_POLYGON (widget);

  if (ligma_tool_polygon_is_point_grabbed (polygon) &&
      ! ligma_tool_polygon_should_close (polygon,
                                        NO_CLICK_TIME_AVAILABLE,
                                        coords))
    {
      *modifier = LIGMA_CURSOR_MODIFIER_MOVE;

      return TRUE;
    }

  return FALSE;
}

static void
ligma_tool_polygon_change_complete (LigmaToolPolygon *polygon)
{
  g_signal_emit (polygon, polygon_signals[CHANGE_COMPLETE], 0);
}

static gint
ligma_tool_polygon_get_segment_index (LigmaToolPolygon  *polygon,
                                     const LigmaCoords *coords)
{
  LigmaToolPolygonPrivate *priv          = polygon->private;
  gint                    segment_index = INVALID_INDEX;

  if (! priv->suppress_handles)
    {
      gdouble shortest_dist = POINT_GRAB_THRESHOLD_SQ;
      gint    i;

      for (i = 0; i < priv->n_segment_indices; i++)
        {
          gdouble      dist;
          LigmaVector2 *point;

          point = &priv->points[priv->segment_indices[i]];

          dist = ligma_canvas_item_transform_distance_square (priv->polygon,
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

LigmaToolWidget *
ligma_tool_polygon_new (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_TOOL_POLYGON,
                       "shell", shell,
                       NULL);
}

gboolean
ligma_tool_polygon_is_closed (LigmaToolPolygon *polygon)
{
  LigmaToolPolygonPrivate *private;

  g_return_val_if_fail (LIGMA_IS_TOOL_POLYGON (polygon), FALSE);

  private = polygon->private;

  return private->polygon_closed;
}

void
ligma_tool_polygon_get_points (LigmaToolPolygon    *polygon,
                              const LigmaVector2 **points,
                              gint               *n_points)
{
  LigmaToolPolygonPrivate *private;

  g_return_if_fail (LIGMA_IS_TOOL_POLYGON (polygon));

  private = polygon->private;

  if (points)   *points   = private->points;
  if (n_points) *n_points = private->n_points;
}
