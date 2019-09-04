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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"

#include "paint/gimpperspectiveclone.h"
#include "paint/gimpperspectivecloneoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasgroup.h"
#include "display/gimpdisplay.h"
#include "display/gimptooltransformgrid.h"

#include "gimpperspectiveclonetool.h"
#include "gimpcloneoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  index into trans_info array  */
enum
{
  X0,
  Y0,
  X1,
  Y1,
  X2,
  Y2,
  X3,
  Y3,
  PIVOT_X,
  PIVOT_Y
};


static void     gimp_perspective_clone_tool_constructed    (GObject          *object);

static gboolean gimp_perspective_clone_tool_initialize     (GimpTool         *tool,
                                                            GimpDisplay      *display,
                                                            GError          **error);

static gboolean gimp_perspective_clone_tool_has_display    (GimpTool         *tool,
                                                            GimpDisplay      *display);
static GimpDisplay *
                gimp_perspective_clone_tool_has_image      (GimpTool         *tool,
                                                            GimpImage        *image);
static void     gimp_perspective_clone_tool_control        (GimpTool         *tool,
                                                            GimpToolAction    action,
                                                            GimpDisplay      *display);
static void     gimp_perspective_clone_tool_button_press   (GimpTool         *tool,
                                                            const GimpCoords *coords,
                                                            guint32           time,
                                                            GdkModifierType   state,
                                                            GimpButtonPressType  press_type,
                                                            GimpDisplay      *display);
static void     gimp_perspective_clone_tool_button_release (GimpTool         *tool,
                                                            const GimpCoords *coords,
                                                            guint32           time,
                                                            GdkModifierType   state,
                                                            GimpButtonReleaseType  release_type,
                                                            GimpDisplay      *display);
static void     gimp_perspective_clone_tool_motion         (GimpTool         *tool,
                                                            const GimpCoords *coords,
                                                            guint32           time,
                                                            GdkModifierType   state,
                                                            GimpDisplay      *display);
static void     gimp_perspective_clone_tool_modifier_key   (GimpTool         *tool,
                                                            GdkModifierType   key,
                                                            gboolean          press,
                                                            GdkModifierType   state,
                                                            GimpDisplay      *display);
static void     gimp_perspective_clone_tool_cursor_update  (GimpTool         *tool,
                                                            const GimpCoords *coords,
                                                            GdkModifierType   state,
                                                            GimpDisplay      *display);
static void     gimp_perspective_clone_tool_oper_update    (GimpTool         *tool,
                                                            const GimpCoords *coords,
                                                            GdkModifierType   state,
                                                            gboolean          proximity,
                                                            GimpDisplay      *display);
static void     gimp_perspective_clone_tool_options_notify (GimpTool         *tool,
                                                            GimpToolOptions  *options,
                                                            const GParamSpec *pspec);

static void     gimp_perspective_clone_tool_draw           (GimpDrawTool             *draw_tool);

static void     gimp_perspective_clone_tool_halt           (GimpPerspectiveCloneTool *clone_tool);
static void     gimp_perspective_clone_tool_bounds         (GimpPerspectiveCloneTool *clone_tool,
                                                            GimpDisplay              *display);
static void     gimp_perspective_clone_tool_prepare        (GimpPerspectiveCloneTool *clone_tool);
static void     gimp_perspective_clone_tool_recalc_matrix  (GimpPerspectiveCloneTool *clone_tool,
                                                            GimpToolWidget           *widget);

static void     gimp_perspective_clone_tool_widget_changed (GimpToolWidget           *widget,
                                                            GimpPerspectiveCloneTool *clone_tool);
static void     gimp_perspective_clone_tool_widget_status  (GimpToolWidget           *widget,
                                                            const gchar              *status,
                                                            GimpPerspectiveCloneTool *clone_tool);

static GtkWidget *
                gimp_perspective_clone_options_gui         (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpPerspectiveCloneTool, gimp_perspective_clone_tool,
               GIMP_TYPE_BRUSH_TOOL)

#define parent_class gimp_perspective_clone_tool_parent_class


void
gimp_perspective_clone_tool_register (GimpToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (GIMP_TYPE_PERSPECTIVE_CLONE_TOOL,
                GIMP_TYPE_PERSPECTIVE_CLONE_OPTIONS,
                gimp_perspective_clone_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PROP_MASK_PATTERN,
                "gimp-perspective-clone-tool",
                _("Perspective Clone"),
                _("Perspective Clone Tool: Clone from an image source "
                  "after applying a perspective transformation"),
                N_("_Perspective Clone"), NULL,
                NULL, GIMP_HELP_TOOL_PERSPECTIVE_CLONE,
                GIMP_ICON_TOOL_PERSPECTIVE_CLONE,
                data);
}

static void
gimp_perspective_clone_tool_class_init (GimpPerspectiveCloneToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = gimp_perspective_clone_tool_constructed;

  tool_class->initialize     = gimp_perspective_clone_tool_initialize;
  tool_class->has_display    = gimp_perspective_clone_tool_has_display;
  tool_class->has_image      = gimp_perspective_clone_tool_has_image;
  tool_class->control        = gimp_perspective_clone_tool_control;
  tool_class->button_press   = gimp_perspective_clone_tool_button_press;
  tool_class->button_release = gimp_perspective_clone_tool_button_release;
  tool_class->motion         = gimp_perspective_clone_tool_motion;
  tool_class->modifier_key   = gimp_perspective_clone_tool_modifier_key;
  tool_class->cursor_update  = gimp_perspective_clone_tool_cursor_update;
  tool_class->oper_update    = gimp_perspective_clone_tool_oper_update;
  tool_class->options_notify = gimp_perspective_clone_tool_options_notify;

  draw_tool_class->draw      = gimp_perspective_clone_tool_draw;
}

static void
gimp_perspective_clone_tool_init (GimpPerspectiveCloneTool *clone_tool)
{
  GimpTool *tool = GIMP_TOOL (clone_tool);

  gimp_tool_control_set_action_object_2 (tool->control,
                                         "context/context-pattern-select-set");

  gimp_matrix3_identity (&clone_tool->transform);
}

static void
gimp_perspective_clone_tool_constructed (GObject *object)
{
  GimpPerspectiveCloneOptions *options;

  options = GIMP_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (options->clone_mode == GIMP_PERSPECTIVE_CLONE_MODE_ADJUST)
    gimp_paint_tool_set_active (GIMP_PAINT_TOOL (object), FALSE);
}

static gboolean
gimp_perspective_clone_tool_initialize (GimpTool     *tool,
                                        GimpDisplay  *display,
                                        GError      **error)
{
  GimpPerspectiveCloneTool *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  if (display != tool->display)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (display);
      GimpImage        *image = gimp_display_get_image (display);
      gint              i;

      tool->display  = display;
      tool->drawable = gimp_image_get_active_drawable (image);

      /*  Find the transform bounds initializing */
      gimp_perspective_clone_tool_bounds (clone_tool, display);

      gimp_perspective_clone_tool_prepare (clone_tool);

      /*  Recalculate the transform tool  */
      gimp_perspective_clone_tool_recalc_matrix (clone_tool, NULL);

      clone_tool->widget =
        gimp_tool_transform_grid_new (shell,
                                      &clone_tool->transform,
                                      clone_tool->x1,
                                      clone_tool->y1,
                                      clone_tool->x2,
                                      clone_tool->y2);

      g_object_set (clone_tool->widget,
                    "pivot-x",                 (clone_tool->x1 + clone_tool->x2) / 2.0,
                    "pivot-y",                 (clone_tool->y1 + clone_tool->y2) / 2.0,
                    "inside-function",         GIMP_TRANSFORM_FUNCTION_MOVE,
                    "outside-function",        GIMP_TRANSFORM_FUNCTION_ROTATE,
                    "use-corner-handles",      TRUE,
                    "use-perspective-handles", TRUE,
                    "use-side-handles",        TRUE,
                    "use-shear-handles",       TRUE,
                    "use-pivot-handle",        TRUE,
                    NULL);

      g_signal_connect (clone_tool->widget, "changed",
                        G_CALLBACK (gimp_perspective_clone_tool_widget_changed),
                        clone_tool);
      g_signal_connect (clone_tool->widget, "status",
                        G_CALLBACK (gimp_perspective_clone_tool_widget_status),
                        clone_tool);

      /*  start drawing the bounding box and handles...  */
      if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
        gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

      /*  Save the current transformation info  */
      for (i = 0; i < TRANS_INFO_SIZE; i++)
        clone_tool->old_trans_info[i] = clone_tool->trans_info[i];
    }

  return TRUE;
}

static gboolean
gimp_perspective_clone_tool_has_display (GimpTool    *tool,
                                         GimpDisplay *display)
{
  GimpPerspectiveCloneTool *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);

  return (display == clone_tool->src_display ||
          GIMP_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static GimpDisplay *
gimp_perspective_clone_tool_has_image (GimpTool  *tool,
                                       GimpImage *image)
{
  GimpPerspectiveCloneTool *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpDisplay              *display;

  display = GIMP_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && clone_tool->src_display)
    {
      if (image && gimp_display_get_image (clone_tool->src_display) == image)
        display = clone_tool->src_display;

      /*  NULL image means any display  */
      if (! image)
        display = clone_tool->src_display;
    }

  return display;
}

static void
gimp_perspective_clone_tool_control (GimpTool       *tool,
                                     GimpToolAction  action,
                                     GimpDisplay    *display)
{
  GimpPerspectiveCloneTool *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      /* only in the case that "Modify Polygon" mode is set " */
      gimp_perspective_clone_tool_bounds (clone_tool, display);
      gimp_perspective_clone_tool_recalc_matrix (clone_tool, clone_tool->widget);
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_perspective_clone_tool_halt (clone_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_perspective_clone_tool_button_press (GimpTool            *tool,
                                          const GimpCoords    *coords,
                                          guint32              time,
                                          GdkModifierType      state,
                                          GimpButtonPressType  press_type,
                                          GimpDisplay         *display)
{
  GimpPaintTool               *paint_tool  = GIMP_PAINT_TOOL (tool);
  GimpPerspectiveCloneTool    *clone_tool  = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPerspectiveClone        *clone       = GIMP_PERSPECTIVE_CLONE (paint_tool->core);
  GimpSourceCore              *source_core = GIMP_SOURCE_CORE (clone);
  GimpPerspectiveCloneOptions *options;

  options = GIMP_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == GIMP_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          gimp_tool_widget_hover (clone_tool->widget, coords, state, TRUE);

          if (gimp_tool_widget_button_press (clone_tool->widget, coords,
                                             time, state, press_type))
            {
              clone_tool->grab_widget = clone_tool->widget;
            }
        }

      gimp_tool_control_activate (tool->control);
    }
  else
    {
      GdkModifierType extend_mask = gimp_get_extend_selection_mask ();
      GdkModifierType toggle_mask = gimp_get_toggle_behavior_mask ();
      gdouble         nnx, nny;

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      if ((state & (toggle_mask | extend_mask)) == toggle_mask)
        {
          source_core->set_source = TRUE;

          clone_tool->src_display = display;
        }
      else
        {
          source_core->set_source = FALSE;
        }

      GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);

      /* Set the coordinates for the reference cross */
      gimp_perspective_clone_get_source_point (clone,
                                               coords->x, coords->y,
                                               &nnx, &nny);

      clone_tool->src_x = floor (nnx);
      clone_tool->src_y = floor (nny);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_perspective_clone_tool_button_release (GimpTool              *tool,
                                            const GimpCoords      *coords,
                                            guint32                time,
                                            GdkModifierType        state,
                                            GimpButtonReleaseType  release_type,
                                            GimpDisplay           *display)
{
  GimpPerspectiveCloneTool    *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPerspectiveCloneOptions *options;

  options = GIMP_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  switch (options->clone_mode)
    {
    case GIMP_PERSPECTIVE_CLONE_MODE_ADJUST:
      gimp_tool_control_halt (tool->control);

      if (clone_tool->grab_widget)
        {
          gimp_tool_widget_button_release (clone_tool->grab_widget,
                                           coords, time, state, release_type);
          clone_tool->grab_widget = NULL;
        }
      break;

    case GIMP_PERSPECTIVE_CLONE_MODE_PAINT:
      GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
      break;
    }
}

static void
gimp_perspective_clone_tool_motion (GimpTool         *tool,
                                    const GimpCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state,
                                    GimpDisplay      *display)
{
  GimpPerspectiveCloneTool    *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPaintTool               *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpPerspectiveClone        *clone      = GIMP_PERSPECTIVE_CLONE (paint_tool->core);
  GimpPerspectiveCloneOptions *options;

  options = GIMP_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == GIMP_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->grab_widget)
        {
          gimp_tool_widget_motion (clone_tool->grab_widget, coords, time, state);
        }
    }
  else if (options->clone_mode == GIMP_PERSPECTIVE_CLONE_MODE_PAINT)
    {
      gdouble nnx, nny;

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);

      /* Set the coordinates for the reference cross */
      gimp_perspective_clone_get_source_point (clone,
                                               coords->x, coords->y,
                                               &nnx, &nny);

      clone_tool->src_x = floor (nnx);
      clone_tool->src_y = floor (nny);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_perspective_clone_tool_modifier_key (GimpTool        *tool,
                                          GdkModifierType  key,
                                          gboolean         press,
                                          GdkModifierType  state,
                                          GimpDisplay     *display)
{
  GimpPerspectiveCloneTool    *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPerspectiveCloneOptions *options;

  options = GIMP_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == GIMP_PERSPECTIVE_CLONE_MODE_PAINT &&
      key == gimp_get_toggle_behavior_mask ())
    {
      if (press)
        {
          clone_tool->saved_precision =
            gimp_tool_control_get_precision (tool->control);
          gimp_tool_control_set_precision (tool->control,
                                           GIMP_CURSOR_PRECISION_PIXEL_CENTER);
        }
      else
        {
          gimp_tool_control_set_precision (tool->control,
                                           clone_tool->saved_precision);
        }
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
gimp_perspective_clone_tool_cursor_update (GimpTool         *tool,
                                           const GimpCoords *coords,
                                           GdkModifierType   state,
                                           GimpDisplay      *display)
{
  GimpPerspectiveCloneTool    *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPerspectiveCloneOptions *options;
  GimpImage                   *image;
  GimpToolClass               *tool_class;
  GimpCursorType               cursor      = GIMP_CURSOR_MOUSE;
  GimpToolCursorType           tool_cursor = GIMP_TOOL_CURSOR_NONE;
  GimpCursorModifier           modifier    = GIMP_CURSOR_MODIFIER_NONE;

  options = GIMP_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  image = gimp_display_get_image (display);

  if (gimp_image_coords_in_active_pickable (image, coords,
                                            FALSE, FALSE, TRUE))
    {
      cursor = GIMP_CURSOR_MOUSE;
    }

  if (options->clone_mode == GIMP_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          if (display == tool->display)
            {
              gimp_tool_widget_get_cursor (clone_tool->widget,
                                           coords, state,
                                           &cursor, &tool_cursor, &modifier);
            }
        }
    }
  else
    {
      GdkModifierType extend_mask = gimp_get_extend_selection_mask ();
      GdkModifierType toggle_mask = gimp_get_toggle_behavior_mask ();

      if ((state & (toggle_mask | extend_mask)) == toggle_mask)
        {
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
        }
      else if (! GIMP_SOURCE_CORE (GIMP_PAINT_TOOL (tool)->core)->src_drawable)
        {
          modifier = GIMP_CURSOR_MODIFIER_BAD;
        }

      tool_cursor = GIMP_TOOL_CURSOR_CLONE;
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  /*  If we are in adjust mode, skip the GimpBrushClass when chaining up.
   *  This ensures that the cursor will be set regardless of
   *  GimpBrushTool::show_cursor (see bug #354933).
   */
  if (options->clone_mode == GIMP_PERSPECTIVE_CLONE_MODE_ADJUST)
    tool_class = GIMP_TOOL_CLASS (g_type_class_peek_parent (parent_class));
  else
    tool_class = GIMP_TOOL_CLASS (parent_class);

  tool_class->cursor_update (tool, coords, state, display);
}

static void
gimp_perspective_clone_tool_oper_update (GimpTool         *tool,
                                         const GimpCoords *coords,
                                         GdkModifierType   state,
                                         gboolean          proximity,
                                         GimpDisplay      *display)
{
  GimpPerspectiveCloneTool    *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPerspectiveCloneOptions *options;

  options = GIMP_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == GIMP_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          if (display == tool->display)
            {
              gimp_tool_widget_hover (clone_tool->widget, coords, state,
                                      proximity);
            }
        }
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);

      if (proximity)
        {
          GimpPaintCore        *core        = GIMP_PAINT_TOOL (tool)->core;
          GimpPerspectiveClone *clone       = GIMP_PERSPECTIVE_CLONE (core);
          GimpSourceCore       *source_core = GIMP_SOURCE_CORE (core);

          if (source_core->src_drawable == NULL)
            {
              gimp_tool_replace_status (tool, display,
                                        _("Ctrl-Click to set a clone source"));
            }
          else
            {
              gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

              clone_tool->src_x = source_core->src_x;
              clone_tool->src_y = source_core->src_y;

              if (! source_core->first_stroke)
                {
                  if (GIMP_SOURCE_OPTIONS (options)->align_mode ==
                      GIMP_SOURCE_ALIGN_YES)
                    {
                      gdouble nnx, nny;

                      /* Set the coordinates for the reference cross */
                      gimp_perspective_clone_get_source_point (clone,
                                                               coords->x,
                                                               coords->y,
                                                               &nnx, &nny);

                      clone_tool->src_x = floor (nnx);
                      clone_tool->src_y = floor (nny);
                    }
                }

              gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
            }
        }
    }
}

static void
gimp_perspective_clone_tool_options_notify (GimpTool         *tool,
                                            GimpToolOptions  *options,
                                            const GParamSpec *pspec)
{
  GimpPerspectiveCloneTool    *clone_tool = GIMP_PERSPECTIVE_CLONE_TOOL (tool);
  GimpPaintTool               *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpPerspectiveCloneOptions *clone_options;

  clone_options = GIMP_PERSPECTIVE_CLONE_OPTIONS (options);

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "clone-mode"))
    {
      GimpPerspectiveClone *clone;

      clone = GIMP_PERSPECTIVE_CLONE (GIMP_PAINT_TOOL (tool)->core);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (clone_tool));

      if (clone_options->clone_mode == GIMP_PERSPECTIVE_CLONE_MODE_PAINT)
        {
          gimp_perspective_clone_set_transform (clone, &clone_tool->transform);

          gimp_paint_tool_set_active (paint_tool, TRUE);
        }
      else
        {
          gimp_paint_tool_set_active (paint_tool, FALSE);

          gimp_tool_control_set_precision (tool->control,
                                           GIMP_CURSOR_PRECISION_SUBPIXEL);

          /*  start drawing the bounding box and handles...  */
          if (tool->display &&
              ! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (clone_tool)))
            {
              gimp_draw_tool_start (GIMP_DRAW_TOOL (clone_tool), tool->display);
            }
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (clone_tool));
    }
}

static void
gimp_perspective_clone_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool                    *tool        = GIMP_TOOL (draw_tool);
  GimpPerspectiveCloneTool    *clone_tool  = GIMP_PERSPECTIVE_CLONE_TOOL (draw_tool);
  GimpPerspectiveClone        *clone       = GIMP_PERSPECTIVE_CLONE (GIMP_PAINT_TOOL (tool)->core);
  GimpSourceCore              *source_core = GIMP_SOURCE_CORE (clone);
  GimpPerspectiveCloneOptions *options;

  options = GIMP_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == GIMP_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          GimpCanvasItem *item = gimp_tool_widget_get_item (clone_tool->widget);

          gimp_draw_tool_add_item (draw_tool, item);
        }
    }
  else
    {
      GimpCanvasGroup *stroke_group;

      stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

      /*  draw the bounding box  */
      gimp_draw_tool_push_group (draw_tool, stroke_group);

      gimp_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X0],
                               clone_tool->trans_info[Y0],
                               clone_tool->trans_info[X1],
                               clone_tool->trans_info[Y1]);
      gimp_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X1],
                               clone_tool->trans_info[Y1],
                               clone_tool->trans_info[X3],
                               clone_tool->trans_info[Y3]);
      gimp_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X2],
                               clone_tool->trans_info[Y2],
                               clone_tool->trans_info[X3],
                               clone_tool->trans_info[Y3]);
      gimp_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X2],
                               clone_tool->trans_info[Y2],
                               clone_tool->trans_info[X0],
                               clone_tool->trans_info[Y0]);

      gimp_draw_tool_pop_group (draw_tool);
    }

  if (source_core->src_drawable && clone_tool->src_display)
    {
      GimpDisplay *tmp_display;

      tmp_display = draw_tool->display;
      draw_tool->display = clone_tool->src_display;

      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_CROSS,
                                 clone_tool->src_x + 0.5,
                                 clone_tool->src_y + 0.5,
                                 GIMP_TOOL_HANDLE_SIZE_CROSS,
                                 GIMP_TOOL_HANDLE_SIZE_CROSS,
                                 GIMP_HANDLE_ANCHOR_CENTER);

      draw_tool->display = tmp_display;
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
gimp_perspective_clone_tool_halt (GimpPerspectiveCloneTool *clone_tool)
{
  GimpTool *tool = GIMP_TOOL (clone_tool);

  clone_tool->src_display = NULL;

  g_object_set (GIMP_PAINT_TOOL (tool)->core,
                "src-drawable", NULL,
                NULL);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  g_clear_object (&clone_tool->widget);

  tool->display  = NULL;
  tool->drawable = NULL;
}

static void
gimp_perspective_clone_tool_bounds (GimpPerspectiveCloneTool *clone_tool,
                                    GimpDisplay              *display)
{
  GimpImage *image = gimp_display_get_image (display);

  clone_tool->x1 = 0;
  clone_tool->y1 = 0;
  clone_tool->x2 = gimp_image_get_width  (image);
  clone_tool->y2 = gimp_image_get_height (image);
}

static void
gimp_perspective_clone_tool_prepare (GimpPerspectiveCloneTool *clone_tool)
{
  clone_tool->trans_info[PIVOT_X] = (gdouble) (clone_tool->x1 + clone_tool->x2) / 2.0;
  clone_tool->trans_info[PIVOT_Y] = (gdouble) (clone_tool->y1 + clone_tool->y2) / 2.0;

  clone_tool->trans_info[X0] = clone_tool->x1;
  clone_tool->trans_info[Y0] = clone_tool->y1;
  clone_tool->trans_info[X1] = clone_tool->x2;
  clone_tool->trans_info[Y1] = clone_tool->y1;
  clone_tool->trans_info[X2] = clone_tool->x1;
  clone_tool->trans_info[Y2] = clone_tool->y2;
  clone_tool->trans_info[X3] = clone_tool->x2;
  clone_tool->trans_info[Y3] = clone_tool->y2;
}

static void
gimp_perspective_clone_tool_recalc_matrix (GimpPerspectiveCloneTool *clone_tool,
                                           GimpToolWidget           *widget)
{
  gimp_matrix3_identity (&clone_tool->transform);
  gimp_transform_matrix_perspective (&clone_tool->transform,
                                     clone_tool->x1,
                                     clone_tool->y1,
                                     clone_tool->x2 - clone_tool->x1,
                                     clone_tool->y2 - clone_tool->y1,
                                     clone_tool->trans_info[X0],
                                     clone_tool->trans_info[Y0],
                                     clone_tool->trans_info[X1],
                                     clone_tool->trans_info[Y1],
                                     clone_tool->trans_info[X2],
                                     clone_tool->trans_info[Y2],
                                     clone_tool->trans_info[X3],
                                     clone_tool->trans_info[Y3]);

  if (widget)
    g_object_set (widget,
                  "transform", &clone_tool->transform,
                  "x1",        (gdouble) clone_tool->x1,
                  "y1",        (gdouble) clone_tool->y1,
                  "x2",        (gdouble) clone_tool->x2,
                  "y2",        (gdouble) clone_tool->y2,
                  "pivot-x",   clone_tool->trans_info[PIVOT_X],
                  "pivot-y",   clone_tool->trans_info[PIVOT_Y],
                  NULL);
}

static void
gimp_perspective_clone_tool_widget_changed (GimpToolWidget           *widget,
                                            GimpPerspectiveCloneTool *clone_tool)
{
  GimpMatrix3 *transform;

  g_object_get (widget,
                "transform", &transform,
                "pivot-x",   &clone_tool->trans_info[PIVOT_X],
                "pivot-y",   &clone_tool->trans_info[PIVOT_Y],
                NULL);

  gimp_matrix3_transform_point (transform,
                                clone_tool->x1, clone_tool->y1,
                                &clone_tool->trans_info[X0],
                                &clone_tool->trans_info[Y0]);
  gimp_matrix3_transform_point (transform,
                                clone_tool->x2, clone_tool->y1,
                                &clone_tool->trans_info[X1],
                                &clone_tool->trans_info[Y1]);
  gimp_matrix3_transform_point (transform,
                                clone_tool->x1, clone_tool->y2,
                                &clone_tool->trans_info[X2],
                                &clone_tool->trans_info[Y2]);
  gimp_matrix3_transform_point (transform,
                                clone_tool->x2, clone_tool->y2,
                                &clone_tool->trans_info[X3],
                                &clone_tool->trans_info[Y3]);

  g_free (transform);

  gimp_perspective_clone_tool_recalc_matrix (clone_tool, NULL);
}

static void
gimp_perspective_clone_tool_widget_status (GimpToolWidget           *widget,
                                           const gchar              *status,
                                           GimpPerspectiveCloneTool *clone_tool)
{
  GimpTool *tool = GIMP_TOOL (clone_tool);

  if (status)
    {
      gimp_tool_replace_status (tool, tool->display, "%s", status);
    }
  else
    {
      gimp_tool_pop_status (tool, tool->display);
    }
}


/*  tool options stuff  */

static GtkWidget *
gimp_perspective_clone_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_clone_options_gui (tool_options);
  GtkWidget *mode;

  /* radio buttons to set if you are modifying the perspective plane
   * or painting
   */
  mode = gimp_prop_enum_radio_box_new (config, "clone-mode", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), mode, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), mode, 0);
  gtk_widget_show (mode);

  return vbox;
}
