/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-utils.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimplayer-floating-selection.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasgroup.h"
#include "display/gimpdisplay.h"

#include "gimpfreeselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define POINT_GRAB_THRESHOLD_SQ SQR (GIMP_TOOL_HANDLE_SIZE_CIRCLE / 2)
#define POINT_SHOW_THRESHOLD_SQ SQR (GIMP_TOOL_HANDLE_SIZE_CIRCLE * 7)
#define N_ITEMS_PER_ALLOC       1024
#define INVALID_INDEX           (-1)
#define NO_CLICK_TIME_AVAILABLE 0

#define GET_PRIVATE(fst)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((fst), \
    GIMP_TYPE_FREE_SELECT_TOOL, GimpFreeSelectToolPrivate))


typedef struct
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

  /* The selection operation active when the tool was started */
  GimpChannelOps     operation_at_start;

  /* Whether or not to constrain the angle for newly created polygonal
   * segments.
   */
  gboolean           constrain_angle;

  /* Whether or not to suppress handles (so that new segments can be
   * created immediately after an existing segment vertex.
   */
  gboolean           supress_handles;

  /* Last _oper_update or _motion coords */
  GimpVector2        last_coords;

  /* A double-click commits the selection, keep track of last
   * click-time and click-position.
   */
  guint32            last_click_time;
  GimpCoords         last_click_coord;

} GimpFreeSelectToolPrivate;


static void     gimp_free_select_tool_finalize            (GObject               *object);
static void     gimp_free_select_tool_control             (GimpTool              *tool,
                                                           GimpToolAction         action,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_oper_update         (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           GdkModifierType        state,
                                                           gboolean               proximity,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_cursor_update       (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           GdkModifierType        state,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_button_press        (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           GimpButtonPressType    press_type,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_button_release      (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           GimpButtonReleaseType  release_type,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_motion              (GimpTool              *tool,
                                                           const GimpCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           GimpDisplay           *display);
static gboolean gimp_free_select_tool_key_press           (GimpTool              *tool,
                                                           GdkEventKey           *kevent,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_modifier_key        (GimpTool              *tool,
                                                           GdkModifierType        key,
                                                           gboolean               press,
                                                           GdkModifierType        state,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_active_modifier_key (GimpTool              *tool,
                                                           GdkModifierType        key,
                                                           gboolean               press,
                                                           GdkModifierType        state,
                                                           GimpDisplay           *display);
static void     gimp_free_select_tool_draw                (GimpDrawTool          *draw_tool);
static void     gimp_free_select_tool_real_select         (GimpFreeSelectTool    *fst,
                                                           GimpDisplay           *display);


G_DEFINE_TYPE (GimpFreeSelectTool, gimp_free_select_tool,
               GIMP_TYPE_SELECTION_TOOL);

#define parent_class gimp_free_select_tool_parent_class


static const GimpVector2 vector2_zero = { 0.0, 0.0 };


void
gimp_free_select_tool_register (GimpToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (GIMP_TYPE_FREE_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-free-select-tool",
                _("Free Select"),
                _("Free Select Tool: Select a hand-drawn region with free "
                  "and polygonal segments"),
                N_("_Free Select"), "F",
                NULL, GIMP_HELP_TOOL_FREE_SELECT,
                GIMP_ICON_TOOL_FREE_SELECT,
                data);
}

static void
gimp_free_select_tool_class_init (GimpFreeSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize          = gimp_free_select_tool_finalize;

  tool_class->control             = gimp_free_select_tool_control;
  tool_class->oper_update         = gimp_free_select_tool_oper_update;
  tool_class->cursor_update       = gimp_free_select_tool_cursor_update;
  tool_class->button_press        = gimp_free_select_tool_button_press;
  tool_class->button_release      = gimp_free_select_tool_button_release;
  tool_class->motion              = gimp_free_select_tool_motion;
  tool_class->key_press           = gimp_free_select_tool_key_press;
  tool_class->modifier_key        = gimp_free_select_tool_modifier_key;
  tool_class->active_modifier_key = gimp_free_select_tool_active_modifier_key;

  draw_tool_class->draw           = gimp_free_select_tool_draw;

  klass->select                   = gimp_free_select_tool_real_select;

  g_type_class_add_private (klass, sizeof (GimpFreeSelectToolPrivate));
}

static void
gimp_free_select_tool_init (GimpFreeSelectTool *fst)
{
  GimpTool                  *tool = GIMP_TOOL (fst);
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_wants_click (tool->control, TRUE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_FREE_SELECT);

  priv->grabbed_segment_index = INVALID_INDEX;
  priv->last_click_time       = NO_CLICK_TIME_AVAILABLE;
}

static void
gimp_free_select_tool_finalize (GObject *object)
{
  GimpFreeSelectTool        *fst  = GIMP_FREE_SELECT_TOOL (object);
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  g_free (priv->points);
  g_free (priv->segment_indices);
  g_free (priv->saved_points_lower_segment);
  g_free (priv->saved_points_higher_segment);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_free_select_tool_get_segment (GimpFreeSelectTool  *fst,
                                   GimpVector2        **points,
                                   gint                *n_points,
                                   gint                 segment_start,
                                   gint                 segment_end)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  *points   = &priv->points[priv->segment_indices[segment_start]];
  *n_points = priv->segment_indices[segment_end] -
              priv->segment_indices[segment_start] +
              1;
}

static void
gimp_free_select_tool_get_segment_point (GimpFreeSelectTool *fst,
                                         gdouble            *start_point_x,
                                         gdouble            *start_point_y,
                                         gint                segment_index)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  *start_point_x = priv->points[priv->segment_indices[segment_index]].x;
  *start_point_y = priv->points[priv->segment_indices[segment_index]].y;
}

static gboolean
gimp_free_select_tool_should_close (GimpFreeSelectTool *fst,
                                    GimpDisplay        *display,
                                    guint32             time,
                                    const GimpCoords   *coords)
{
  GimpFreeSelectToolPrivate *priv         = GET_PRIVATE (fst);
  gboolean                   double_click = FALSE;
  gdouble                    dist;

  if (priv->polygon_modified       ||
      priv->n_segment_indices <= 0 ||
      GIMP_TOOL (fst)->display == NULL)
    return FALSE;

  dist = gimp_draw_tool_calc_distance_square (GIMP_DRAW_TOOL (fst),
                                              display,
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
      GimpDisplayShell *shell    = gimp_display_get_shell (display);
      GtkSettings      *settings = gtk_widget_get_settings (GTK_WIDGET (shell));
      gint              double_click_time;
      gint              double_click_distance;
      gint              click_time_passed;
      gdouble           dist_from_last_point;

      click_time_passed = time - priv->last_click_time;

      dist_from_last_point =
        gimp_draw_tool_calc_distance_square (GIMP_DRAW_TOOL (fst),
                                             display,
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

  return ((! priv->supress_handles && dist < POINT_GRAB_THRESHOLD_SQ) ||
          double_click);
}

static void
gimp_free_select_tool_revert_to_last_segment (GimpFreeSelectTool *fst)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  priv->n_points = priv->segment_indices[priv->n_segment_indices - 1] + 1;
}

static void
gimp_free_select_tool_remove_last_segment (GimpFreeSelectTool *fst)
{
  GimpFreeSelectToolPrivate *priv      = GET_PRIVATE (fst);
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (fst);

  gimp_draw_tool_pause (draw_tool);

  if (priv->n_segment_indices > 0)
    priv->n_segment_indices--;

  if (priv->n_segment_indices <= 0)
    {
      gimp_tool_control (GIMP_TOOL (fst), GIMP_TOOL_ACTION_HALT,
                         GIMP_TOOL (fst)->display);
    }
  else
    {
      gimp_free_select_tool_revert_to_last_segment (fst);
    }

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_free_select_tool_add_point (GimpFreeSelectTool *fst,
                                 gdouble             x,
                                 gdouble             y)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

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
gimp_free_select_tool_add_segment_index (GimpFreeSelectTool *fst,
                                         gint                index)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  if (priv->n_segment_indices >= priv->max_n_segment_indices)
    {
      priv->max_n_segment_indices += N_ITEMS_PER_ALLOC;

      priv->segment_indices = g_realloc (priv->segment_indices,
                                         sizeof (GimpVector2) *
                                         priv->max_n_segment_indices);
    }

  priv->segment_indices[priv->n_segment_indices] = index;

  priv->n_segment_indices++;
}

static gboolean
gimp_free_select_tool_is_point_grabbed (GimpFreeSelectTool *fst)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  return priv->grabbed_segment_index != INVALID_INDEX;
}

static void
gimp_free_select_tool_fit_segment (GimpFreeSelectTool *fst,
                                   GimpVector2        *dest_points,
                                   GimpVector2         dest_start_target,
                                   GimpVector2         dest_end_target,
                                   const GimpVector2  *source_points,
                                   gint                n_points)
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
gimp_free_select_tool_move_segment_vertex_to (GimpFreeSelectTool *fst,
                                              gint                segment_index,
                                              gdouble             new_x,
                                              gdouble             new_y)
{
  GimpFreeSelectToolPrivate *priv         = GET_PRIVATE (fst);
  GimpVector2                cursor_point = { new_x, new_y };
  GimpVector2               *dest;
  GimpVector2               *dest_start_target;
  GimpVector2               *dest_end_target;
  gint                       n_points;

  /* Handle the segment before the grabbed point */
  if (segment_index > 0)
    {
      gimp_free_select_tool_get_segment (fst,
                                         &dest,
                                         &n_points,
                                         priv->grabbed_segment_index - 1,
                                         priv->grabbed_segment_index);

      dest_start_target = &dest[0];
      dest_end_target   = &cursor_point;

      gimp_free_select_tool_fit_segment (fst,
                                         dest,
                                         *dest_start_target,
                                         *dest_end_target,
                                         priv->saved_points_lower_segment,
                                         n_points);
    }

  /* Handle the segment after the grabbed point */
  if (segment_index < priv->n_segment_indices - 1)
    {
      gimp_free_select_tool_get_segment (fst,
                                         &dest,
                                         &n_points,
                                         priv->grabbed_segment_index,
                                         priv->grabbed_segment_index + 1);

      dest_start_target = &cursor_point;
      dest_end_target   = &dest[n_points - 1];

      gimp_free_select_tool_fit_segment (fst,
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
gimp_free_select_tool_commit (GimpFreeSelectTool *fst,
                              GimpDisplay        *display)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  if (priv->n_points >= 3)
    {
      GIMP_FREE_SELECT_TOOL_GET_CLASS (fst)->select (fst, display);
    }

  gimp_image_flush (gimp_display_get_image (display));
}

static void
gimp_free_select_tool_revert_to_saved_state (GimpFreeSelectTool *fst)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);
  GimpVector2               *dest;
  gint                       n_points;

  /* Without a point grab we have no sensible information to fall back
   * on, bail out
   */
  if (! gimp_free_select_tool_is_point_grabbed (fst))
    {
      return;
    }

  if (priv->grabbed_segment_index > 0)
    {
      gimp_free_select_tool_get_segment (fst,
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
      gimp_free_select_tool_get_segment (fst,
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
gimp_free_select_tool_prepare_for_move (GimpFreeSelectTool *fst)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);
  GimpVector2               *source;
  gint                       n_points;

  if (priv->grabbed_segment_index > 0)
    {
      gimp_free_select_tool_get_segment (fst,
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
      gimp_free_select_tool_get_segment (fst,
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
gimp_free_select_tool_update_motion (GimpFreeSelectTool *fst,
                                     gdouble             new_x,
                                     gdouble             new_y)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  if (gimp_free_select_tool_is_point_grabbed (fst))
    {
      priv->polygon_modified = TRUE;

      if (priv->constrain_angle &&
          priv->n_segment_indices > 1 )
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

          gimp_free_select_tool_get_segment_point (fst,
                                                   &start_point_x,
                                                   &start_point_y,
                                                   segment_index);

          gimp_constrain_line (start_point_x,
                               start_point_y,
                               &new_x,
                               &new_y,
                               GIMP_CONSTRAIN_LINE_15_DEGREES);
        }

      gimp_free_select_tool_move_segment_vertex_to (fst,
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

      gimp_free_select_tool_add_point (fst, new_x, new_y);
    }
}

static void
gimp_free_select_tool_status_update (GimpFreeSelectTool *fst,
                                     GimpDisplay        *display,
                                     const GimpCoords   *coords,
                                     gboolean            proximity)
{
  GimpTool                  *tool = GIMP_TOOL (fst);
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  gimp_tool_pop_status (tool, display);

  if (proximity)
    {
      const gchar *status_text = NULL;

      if (gimp_free_select_tool_is_point_grabbed (fst))
        {
          if (gimp_free_select_tool_should_close (fst, display,
                                                  NO_CLICK_TIME_AVAILABLE,
                                                  coords))
            {
              status_text = _("Click to complete selection");
            }
          else
            {
              status_text = _("Click-Drag to move segment vertex");
            }
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
          gimp_tool_push_status (tool, display, "%s", status_text);
        }
    }
}

static void
gimp_free_select_tool_control (GimpTool       *tool,
                               GimpToolAction  action,
                               GimpDisplay    *display)
{
  GimpFreeSelectTool        *fst  = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      priv->grabbed_segment_index = INVALID_INDEX;
      priv->show_pending_point    = FALSE;
      priv->n_points              = 0;
      priv->n_segment_indices     = 0;
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_free_select_tool_commit (fst, display);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_free_select_tool_oper_update (GimpTool         *tool,
                                   const GimpCoords *coords,
                                   GdkModifierType   state,
                                   gboolean          proximity,
                                   GimpDisplay      *display)
{
  GimpFreeSelectTool        *fst  = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);
  gboolean                   hovering_first_point;

  if (display != tool->display)
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
      return;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  priv->grabbed_segment_index = INVALID_INDEX;

  if (tool->display && ! priv->supress_handles)
    {
      gdouble shortest_dist = POINT_GRAB_THRESHOLD_SQ;
      gint    i;

      for (i = 0; i < priv->n_segment_indices; i++)
        {
          gdouble      dist;
          GimpVector2 *point;

          point = &priv->points[priv->segment_indices[i]];

          dist = gimp_draw_tool_calc_distance_square (GIMP_DRAW_TOOL (tool),
                                                      display,
                                                      coords->x,
                                                      coords->y,
                                                      point->x,
                                                      point->y);

          if (dist < shortest_dist)
            {
              shortest_dist = dist;

              priv->grabbed_segment_index = i;
            }
        }
    }

  hovering_first_point =
    gimp_free_select_tool_should_close (fst, display,
                                        NO_CLICK_TIME_AVAILABLE,
                                        coords);

  priv->last_coords.x = coords->x;
  priv->last_coords.y = coords->y;

  if (priv->n_points == 0 ||
      (gimp_free_select_tool_is_point_grabbed (fst) &&
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
              gimp_free_select_tool_get_segment_point (fst,
                                                       &start_point_x,
                                                       &start_point_y,
                                                       priv->n_segment_indices - 1);

              gimp_constrain_line (start_point_x, start_point_y,
                                   &priv->pending_point.x,
                                   &priv->pending_point.y,
                                   GIMP_CONSTRAIN_LINE_15_DEGREES);
            }
        }
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  gimp_free_select_tool_status_update (fst, display, coords, proximity);
}

static void
gimp_free_select_tool_cursor_update (GimpTool         *tool,
                                     const GimpCoords *coords,
                                     GdkModifierType   state,
                                     GimpDisplay      *display)
{
  GimpFreeSelectTool *fst = GIMP_FREE_SELECT_TOOL (tool);
  GimpCursorModifier  modifier;

  if (tool->display == NULL)
    {
      GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
      return;
    }

  if (gimp_free_select_tool_is_point_grabbed (fst) &&
      ! gimp_free_select_tool_should_close (fst, display,
                                            NO_CLICK_TIME_AVAILABLE,
                                            coords))
    {
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
    }
  else
    {
      modifier = GIMP_CURSOR_MODIFIER_NONE;
    }

  gimp_tool_set_cursor (tool, display,
                        gimp_tool_control_get_cursor (tool->control),
                        gimp_tool_control_get_tool_cursor (tool->control),
                        modifier);
}

static void
gimp_free_select_tool_button_press (GimpTool            *tool,
                                    const GimpCoords    *coords,
                                    guint32              time,
                                    GdkModifierType      state,
                                    GimpButtonPressType  press_type,
                                    GimpDisplay         *display)
{
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (tool);
  GimpFreeSelectTool        *fst       = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv      = GET_PRIVATE (fst);
  GimpSelectionOptions      *options   = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  if (tool->display && tool->display != display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);

  if (tool->display == NULL)
    {
      /* First of all handle delegation to the selection mask edit logic
       * if appropriate.
       */
      if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (fst),
                                          display, coords))
        {
          return;
        }

      tool->display = display;

      /* We want to apply the selection operation that was current when
       * the tool was started, so we save this information
       */
      priv->operation_at_start = options->operation;
    }

  if (! gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_start (draw_tool, display);

  gimp_tool_control_activate (tool->control);

  gimp_draw_tool_pause (draw_tool);

  if (gimp_free_select_tool_is_point_grabbed (fst))
    {
      gimp_free_select_tool_prepare_for_move (fst);
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
      gimp_free_select_tool_add_point (fst,
                                       point_to_add.x,
                                       point_to_add.y);
      gimp_free_select_tool_add_segment_index (fst, priv->n_points - 1);
    }

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_free_select_tool_button_release (GimpTool              *tool,
                                      const GimpCoords      *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      GimpButtonReleaseType  release_type,
                                      GimpDisplay           *display)
{
  GimpFreeSelectTool        *fst   = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv  = GET_PRIVATE (fst);
  GimpImage                 *image = gimp_display_get_image (display);

  gimp_tool_control_halt (tool->control);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (fst));

  switch (release_type)
    {
    case GIMP_BUTTON_RELEASE_CLICK:
    case GIMP_BUTTON_RELEASE_NO_MOTION:
      /* If a click was made, we don't consider the polygon modified */
      priv->polygon_modified = FALSE;

      /*  If there is a floating selection, anchor it  */
      if (gimp_image_get_floating_selection (image))
        {
          floating_sel_anchor (gimp_image_get_floating_selection (image));

          gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
        }
      else
        {
          /* First finish of the line segment if no point was grabbed */
          if (! gimp_free_select_tool_is_point_grabbed (fst))
            {
              /* Revert any free segment points that might have been added */
              gimp_free_select_tool_revert_to_last_segment (fst);
            }

          /* After the segments are up to date and we have handled
           * double-click, see if it's committing time
           */
          if (gimp_free_select_tool_should_close (fst, display,
                                                  time, coords))
            {
              /* We can get a click notification even though the end point
               * has been moved a few pixels. Since a move will change the
               * free selection, revert it before doing the commit.
               */
              gimp_free_select_tool_revert_to_saved_state (fst);

              gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
            }

          priv->last_click_time  = time;
          priv->last_click_coord = *coords;
        }
      break;

    case GIMP_BUTTON_RELEASE_NORMAL:
      /* First finish of the free segment if no point was grabbed */
      if (! gimp_free_select_tool_is_point_grabbed (fst))
        {
          /* The points are all setup, just make a segment */
          gimp_free_select_tool_add_segment_index (fst, priv->n_points - 1);
        }

      /* After the segments are up to date, see if it's committing time */
      if (gimp_free_select_tool_should_close (fst, display,
                                              NO_CLICK_TIME_AVAILABLE,
                                              coords))
        {
          gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
        }
      break;

    case GIMP_BUTTON_RELEASE_CANCEL:
      if (gimp_free_select_tool_is_point_grabbed (fst))
        gimp_free_select_tool_revert_to_saved_state (fst);
      else
        gimp_free_select_tool_remove_last_segment (fst);
      break;

    default:
      break;
    }

  /* Reset */
  priv->polygon_modified = FALSE;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (fst));
}

static void
gimp_free_select_tool_motion (GimpTool         *tool,
                              const GimpCoords *coords,
                              guint32           time,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpFreeSelectTool        *fst       = GIMP_FREE_SELECT_TOOL (tool);
  GimpFreeSelectToolPrivate *priv      = GET_PRIVATE (fst);
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (tool);

  gimp_draw_tool_pause (draw_tool);

  priv->last_coords.x = coords->x;
  priv->last_coords.y = coords->y;

  gimp_free_select_tool_update_motion (fst,
                                       coords->x,
                                       coords->y);

  gimp_draw_tool_resume (draw_tool);
}

static gboolean
gimp_free_select_tool_key_press (GimpTool    *tool,
                                 GdkEventKey *kevent,
                                 GimpDisplay *display)
{
  GimpFreeSelectTool *fst = GIMP_FREE_SELECT_TOOL (tool);

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      gimp_free_select_tool_remove_last_segment (fst);
      return TRUE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
      return TRUE;

    case GDK_KEY_Escape:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_free_select_tool_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (tool);

  if (tool->display == display)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      priv->constrain_angle = ((state & gimp_get_constrain_behavior_mask ()) ?
                               TRUE : FALSE);

      priv->supress_handles = ((state & gimp_get_extend_selection_mask ()) ?
                               TRUE : FALSE);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
gimp_free_select_tool_active_modifier_key (GimpTool        *tool,
                                           GdkModifierType  key,
                                           gboolean         press,
                                           GdkModifierType  state,
                                           GimpDisplay     *display)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (tool);

  if (tool->display != display)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  priv->constrain_angle = ((state & gimp_get_constrain_behavior_mask ()) ?
                           TRUE : FALSE);

  /* If we didn't came here due to a mouse release, immediately update
   * the position of the thing we move.
   */
  if (state & GDK_BUTTON1_MASK)
    {
      gimp_free_select_tool_update_motion (GIMP_FREE_SELECT_TOOL (tool),
                                           priv->last_coords.x,
                                           priv->last_coords.y);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  GIMP_TOOL_CLASS (parent_class)->active_modifier_key (tool, key, press, state,
                                                       display);
}

/**
 * gimp_free_select_tool_draw:
 * @draw_tool:
 *
 * Draw the line segments and handles around segment vertices, the
 * latter if the they are in proximity to cursor.
 **/
static void
gimp_free_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpFreeSelectTool        *fst                   = GIMP_FREE_SELECT_TOOL (draw_tool);
  GimpFreeSelectToolPrivate *priv                  = GET_PRIVATE (fst);
  GimpTool                  *tool                  = GIMP_TOOL (draw_tool);
  GimpCanvasGroup           *stroke_group;
  gboolean                   hovering_first_point  = FALSE;
  gboolean                   handles_wants_to_show = FALSE;
  GimpCoords                 coords                = { priv->last_coords.x,
                                                       priv->last_coords.y,
                                                       /* pad with 0 */ };
  if (! tool->display)
    return;

  hovering_first_point =
    gimp_free_select_tool_should_close (fst, tool->display,
                                        NO_CLICK_TIME_AVAILABLE,
                                        &coords);

  stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

  gimp_draw_tool_push_group (draw_tool, stroke_group);
  gimp_draw_tool_add_lines (draw_tool,
                            priv->points, priv->n_points,
                            FALSE);
  gimp_draw_tool_pop_group (draw_tool);

  /* We always show the handle for the first point, even with button1
   * down, since releasing the button on the first point will close
   * the polygon, so it's a significant state which we must give
   * feedback for
   */
  handles_wants_to_show = (hovering_first_point ||
                           ! gimp_tool_control_is_active (tool->control));

  if (handles_wants_to_show &&
      ! priv->supress_handles)
    {
      gint i;
      gint n;

      /* If the first point is hovered while button1 is held down,
       * only draw the first handle, the other handles are not
       * relevant (see comment a few lines up)
       */
      if (gimp_tool_control_is_active (tool->control) && hovering_first_point)
        n = MIN (priv->n_segment_indices, 1);
      else
        n = priv->n_segment_indices;

      for (i = 0; i < n; i++)
        {
          GimpVector2   *point;
          gdouble        dist;
          GimpHandleType handle_type = -1;

          point = &priv->points[priv->segment_indices[i]];

          dist  = gimp_draw_tool_calc_distance_square (draw_tool,
                                                       tool->display,
                                                       priv->last_coords.x,
                                                       priv->last_coords.y,
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
              GimpCanvasItem *item;

              item = gimp_draw_tool_add_handle (draw_tool, handle_type,
                                                point->x,
                                                point->y,
                                                GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                                GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                                GIMP_HANDLE_ANCHOR_CENTER);

              if (dist < POINT_GRAB_THRESHOLD_SQ)
                gimp_canvas_item_set_highlight (item, TRUE);
            }
        }
    }

  if (priv->show_pending_point)
    {
      GimpVector2 last = priv->points[priv->n_points - 1];

      gimp_draw_tool_push_group (draw_tool, stroke_group);
      gimp_draw_tool_add_line (draw_tool,
                               last.x,
                               last.y,
                               priv->pending_point.x,
                               priv->pending_point.y);
      gimp_draw_tool_pop_group (draw_tool);
    }
}

static void
gimp_free_select_tool_real_select (GimpFreeSelectTool *fst,
                                   GimpDisplay        *display)
{
  GimpSelectionOptions      *options = GIMP_SELECTION_TOOL_GET_OPTIONS (fst);
  GimpFreeSelectToolPrivate *priv    = GET_PRIVATE (fst);
  GimpImage                 *image   = gimp_display_get_image (display);

  gimp_channel_select_polygon (gimp_image_get_mask (image),
                               C_("command", "Free Select"),
                               priv->n_points,
                               priv->points,
                               priv->operation_at_start,
                               options->antialias,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius,
                               TRUE);

  gimp_tool_control (GIMP_TOOL (fst), GIMP_TOOL_ACTION_HALT, display);
}

void
gimp_free_select_tool_get_points (GimpFreeSelectTool  *fst,
                                  const GimpVector2  **points,
                                  gint                *n_points)
{
  GimpFreeSelectToolPrivate *priv = GET_PRIVATE (fst);

  g_return_if_fail (points != NULL && n_points != NULL);

  *points   = priv->points;
  *n_points = priv->n_points;
}
