/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma-transform-utils.h"
#include "core/ligmaimage.h"

#include "paint/ligmaperspectiveclone.h"
#include "paint/ligmaperspectivecloneoptions.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaviewablebox.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvasgroup.h"
#include "display/ligmadisplay.h"
#include "display/ligmatooltransformgrid.h"

#include "ligmaperspectiveclonetool.h"
#include "ligmacloneoptions-gui.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


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


static void     ligma_perspective_clone_tool_constructed    (GObject          *object);

static gboolean ligma_perspective_clone_tool_initialize     (LigmaTool         *tool,
                                                            LigmaDisplay      *display,
                                                            GError          **error);

static gboolean ligma_perspective_clone_tool_has_display    (LigmaTool         *tool,
                                                            LigmaDisplay      *display);
static LigmaDisplay *
                ligma_perspective_clone_tool_has_image      (LigmaTool         *tool,
                                                            LigmaImage        *image);
static void     ligma_perspective_clone_tool_control        (LigmaTool         *tool,
                                                            LigmaToolAction    action,
                                                            LigmaDisplay      *display);
static void     ligma_perspective_clone_tool_button_press   (LigmaTool         *tool,
                                                            const LigmaCoords *coords,
                                                            guint32           time,
                                                            GdkModifierType   state,
                                                            LigmaButtonPressType  press_type,
                                                            LigmaDisplay      *display);
static void     ligma_perspective_clone_tool_button_release (LigmaTool         *tool,
                                                            const LigmaCoords *coords,
                                                            guint32           time,
                                                            GdkModifierType   state,
                                                            LigmaButtonReleaseType  release_type,
                                                            LigmaDisplay      *display);
static void     ligma_perspective_clone_tool_motion         (LigmaTool         *tool,
                                                            const LigmaCoords *coords,
                                                            guint32           time,
                                                            GdkModifierType   state,
                                                            LigmaDisplay      *display);
static void     ligma_perspective_clone_tool_modifier_key   (LigmaTool         *tool,
                                                            GdkModifierType   key,
                                                            gboolean          press,
                                                            GdkModifierType   state,
                                                            LigmaDisplay      *display);
static void     ligma_perspective_clone_tool_cursor_update  (LigmaTool         *tool,
                                                            const LigmaCoords *coords,
                                                            GdkModifierType   state,
                                                            LigmaDisplay      *display);
static void     ligma_perspective_clone_tool_oper_update    (LigmaTool         *tool,
                                                            const LigmaCoords *coords,
                                                            GdkModifierType   state,
                                                            gboolean          proximity,
                                                            LigmaDisplay      *display);
static void     ligma_perspective_clone_tool_options_notify (LigmaTool         *tool,
                                                            LigmaToolOptions  *options,
                                                            const GParamSpec *pspec);

static void     ligma_perspective_clone_tool_draw           (LigmaDrawTool             *draw_tool);

static void     ligma_perspective_clone_tool_halt           (LigmaPerspectiveCloneTool *clone_tool);
static void     ligma_perspective_clone_tool_bounds         (LigmaPerspectiveCloneTool *clone_tool,
                                                            LigmaDisplay              *display);
static void     ligma_perspective_clone_tool_prepare        (LigmaPerspectiveCloneTool *clone_tool);
static void     ligma_perspective_clone_tool_recalc_matrix  (LigmaPerspectiveCloneTool *clone_tool,
                                                            LigmaToolWidget           *widget);

static void     ligma_perspective_clone_tool_widget_changed (LigmaToolWidget           *widget,
                                                            LigmaPerspectiveCloneTool *clone_tool);
static void     ligma_perspective_clone_tool_widget_status  (LigmaToolWidget           *widget,
                                                            const gchar              *status,
                                                            LigmaPerspectiveCloneTool *clone_tool);

static GtkWidget *
                ligma_perspective_clone_options_gui         (LigmaToolOptions *tool_options);


G_DEFINE_TYPE (LigmaPerspectiveCloneTool, ligma_perspective_clone_tool,
               LIGMA_TYPE_BRUSH_TOOL)

#define parent_class ligma_perspective_clone_tool_parent_class


void
ligma_perspective_clone_tool_register (LigmaToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (LIGMA_TYPE_PERSPECTIVE_CLONE_TOOL,
                LIGMA_TYPE_PERSPECTIVE_CLONE_OPTIONS,
                ligma_perspective_clone_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK |
                LIGMA_CONTEXT_PROP_MASK_PATTERN,
                "ligma-perspective-clone-tool",
                _("Perspective Clone"),
                _("Perspective Clone Tool: Clone from an image source "
                  "after applying a perspective transformation"),
                N_("_Perspective Clone"), NULL,
                NULL, LIGMA_HELP_TOOL_PERSPECTIVE_CLONE,
                LIGMA_ICON_TOOL_PERSPECTIVE_CLONE,
                data);
}

static void
ligma_perspective_clone_tool_class_init (LigmaPerspectiveCloneToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = ligma_perspective_clone_tool_constructed;

  tool_class->initialize     = ligma_perspective_clone_tool_initialize;
  tool_class->has_display    = ligma_perspective_clone_tool_has_display;
  tool_class->has_image      = ligma_perspective_clone_tool_has_image;
  tool_class->control        = ligma_perspective_clone_tool_control;
  tool_class->button_press   = ligma_perspective_clone_tool_button_press;
  tool_class->button_release = ligma_perspective_clone_tool_button_release;
  tool_class->motion         = ligma_perspective_clone_tool_motion;
  tool_class->modifier_key   = ligma_perspective_clone_tool_modifier_key;
  tool_class->cursor_update  = ligma_perspective_clone_tool_cursor_update;
  tool_class->oper_update    = ligma_perspective_clone_tool_oper_update;
  tool_class->options_notify = ligma_perspective_clone_tool_options_notify;

  draw_tool_class->draw      = ligma_perspective_clone_tool_draw;
}

static void
ligma_perspective_clone_tool_init (LigmaPerspectiveCloneTool *clone_tool)
{
  LigmaTool *tool = LIGMA_TOOL (clone_tool);

  ligma_tool_control_set_action_object_2 (tool->control,
                                         "context/context-pattern-select-set");

  ligma_matrix3_identity (&clone_tool->transform);
  ligma_paint_tool_enable_multi_paint (LIGMA_PAINT_TOOL (clone_tool));
}

static void
ligma_perspective_clone_tool_constructed (GObject *object)
{
  LigmaPerspectiveCloneOptions *options;

  options = LIGMA_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (options->clone_mode == LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST)
    ligma_paint_tool_set_active (LIGMA_PAINT_TOOL (object), FALSE);
}

static gboolean
ligma_perspective_clone_tool_initialize (LigmaTool     *tool,
                                        LigmaDisplay  *display,
                                        GError      **error)
{
  LigmaPerspectiveCloneTool *clone_tool = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);
  LigmaImage                *image      = ligma_display_get_image (display);
  GList                    *drawables;

  if (! LIGMA_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  drawables = ligma_image_get_selected_drawables (image);
  if (g_list_length (drawables) == 0)
    {
      ligma_tool_message_literal (tool, display, _("No selected drawables."));

      g_list_free (drawables);

      return FALSE;
    }

  if (display != tool->display)
    {
      LigmaDisplayShell *shell = ligma_display_get_shell (display);
      gint              i;

      tool->display   = display;
      tool->drawables = drawables;

      /*  Find the transform bounds initializing */
      ligma_perspective_clone_tool_bounds (clone_tool, display);

      ligma_perspective_clone_tool_prepare (clone_tool);

      /*  Recalculate the transform tool  */
      ligma_perspective_clone_tool_recalc_matrix (clone_tool, NULL);

      clone_tool->widget =
        ligma_tool_transform_grid_new (shell,
                                      &clone_tool->transform,
                                      clone_tool->x1,
                                      clone_tool->y1,
                                      clone_tool->x2,
                                      clone_tool->y2);

      g_object_set (clone_tool->widget,
                    "pivot-x",                 (clone_tool->x1 + clone_tool->x2) / 2.0,
                    "pivot-y",                 (clone_tool->y1 + clone_tool->y2) / 2.0,
                    "inside-function",         LIGMA_TRANSFORM_FUNCTION_MOVE,
                    "outside-function",        LIGMA_TRANSFORM_FUNCTION_ROTATE,
                    "use-corner-handles",      TRUE,
                    "use-perspective-handles", TRUE,
                    "use-side-handles",        TRUE,
                    "use-shear-handles",       TRUE,
                    "use-pivot-handle",        TRUE,
                    NULL);

      g_signal_connect (clone_tool->widget, "changed",
                        G_CALLBACK (ligma_perspective_clone_tool_widget_changed),
                        clone_tool);
      g_signal_connect (clone_tool->widget, "status",
                        G_CALLBACK (ligma_perspective_clone_tool_widget_status),
                        clone_tool);

      /*  start drawing the bounding box and handles...  */
      if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tool)))
        ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

      ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);

      /*  Save the current transformation info  */
      for (i = 0; i < TRANS_INFO_SIZE; i++)
        clone_tool->old_trans_info[i] = clone_tool->trans_info[i];
    }
  else
    {
      g_list_free (drawables);
    }

  return TRUE;
}

static gboolean
ligma_perspective_clone_tool_has_display (LigmaTool    *tool,
                                         LigmaDisplay *display)
{
  LigmaPerspectiveCloneTool *clone_tool = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);

  return (display == clone_tool->src_display ||
          LIGMA_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static LigmaDisplay *
ligma_perspective_clone_tool_has_image (LigmaTool  *tool,
                                       LigmaImage *image)
{
  LigmaPerspectiveCloneTool *clone_tool = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);
  LigmaDisplay              *display;

  display = LIGMA_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && clone_tool->src_display)
    {
      if (image && ligma_display_get_image (clone_tool->src_display) == image)
        display = clone_tool->src_display;

      /*  NULL image means any display  */
      if (! image)
        display = clone_tool->src_display;
    }

  return display;
}

static void
ligma_perspective_clone_tool_control (LigmaTool       *tool,
                                     LigmaToolAction  action,
                                     LigmaDisplay    *display)
{
  LigmaPerspectiveCloneTool *clone_tool = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_perspective_clone_tool_halt (clone_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_perspective_clone_tool_button_press (LigmaTool            *tool,
                                          const LigmaCoords    *coords,
                                          guint32              time,
                                          GdkModifierType      state,
                                          LigmaButtonPressType  press_type,
                                          LigmaDisplay         *display)
{
  LigmaPaintTool               *paint_tool  = LIGMA_PAINT_TOOL (tool);
  LigmaPerspectiveCloneTool    *clone_tool  = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);
  LigmaPerspectiveClone        *clone       = LIGMA_PERSPECTIVE_CLONE (paint_tool->core);
  LigmaSourceCore              *source_core = LIGMA_SOURCE_CORE (clone);
  LigmaPerspectiveCloneOptions *options;

  options = LIGMA_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          ligma_tool_widget_hover (clone_tool->widget, coords, state, TRUE);

          if (ligma_tool_widget_button_press (clone_tool->widget, coords,
                                             time, state, press_type))
            {
              clone_tool->grab_widget = clone_tool->widget;
            }
        }

      ligma_tool_control_activate (tool->control);
    }
  else
    {
      GdkModifierType extend_mask = ligma_get_extend_selection_mask ();
      GdkModifierType toggle_mask = ligma_get_toggle_behavior_mask ();
      gdouble         nnx, nny;

      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

      if ((state & (toggle_mask | extend_mask)) == toggle_mask)
        {
          source_core->set_source = TRUE;

          clone_tool->src_display = display;
        }
      else
        {
          source_core->set_source = FALSE;
        }

      LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);

      /* Set the coordinates for the reference cross */
      ligma_perspective_clone_get_source_point (clone,
                                               coords->x, coords->y,
                                               &nnx, &nny);

      clone_tool->src_x = floor (nnx);
      clone_tool->src_y = floor (nny);

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
    }
}

static void
ligma_perspective_clone_tool_button_release (LigmaTool              *tool,
                                            const LigmaCoords      *coords,
                                            guint32                time,
                                            GdkModifierType        state,
                                            LigmaButtonReleaseType  release_type,
                                            LigmaDisplay           *display)
{
  LigmaPerspectiveCloneTool    *clone_tool = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);
  LigmaPerspectiveCloneOptions *options;

  options = LIGMA_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  switch (options->clone_mode)
    {
    case LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST:
      ligma_tool_control_halt (tool->control);

      if (clone_tool->grab_widget)
        {
          ligma_tool_widget_button_release (clone_tool->grab_widget,
                                           coords, time, state, release_type);
          clone_tool->grab_widget = NULL;
        }
      break;

    case LIGMA_PERSPECTIVE_CLONE_MODE_PAINT:
      LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
      break;
    }
}

static void
ligma_perspective_clone_tool_motion (LigmaTool         *tool,
                                    const LigmaCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state,
                                    LigmaDisplay      *display)
{
  LigmaPerspectiveCloneTool    *clone_tool = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);
  LigmaPaintTool               *paint_tool = LIGMA_PAINT_TOOL (tool);
  LigmaPerspectiveClone        *clone      = LIGMA_PERSPECTIVE_CLONE (paint_tool->core);
  LigmaPerspectiveCloneOptions *options;

  options = LIGMA_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->grab_widget)
        {
          ligma_tool_widget_motion (clone_tool->grab_widget, coords, time, state);
        }
    }
  else if (options->clone_mode == LIGMA_PERSPECTIVE_CLONE_MODE_PAINT)
    {
      gdouble nnx, nny;

      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

      LIGMA_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);

      /* Set the coordinates for the reference cross */
      ligma_perspective_clone_get_source_point (clone,
                                               coords->x, coords->y,
                                               &nnx, &nny);

      clone_tool->src_x = floor (nnx);
      clone_tool->src_y = floor (nny);

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
    }
}

static void
ligma_perspective_clone_tool_modifier_key (LigmaTool        *tool,
                                          GdkModifierType  key,
                                          gboolean         press,
                                          GdkModifierType  state,
                                          LigmaDisplay     *display)
{
  LigmaPerspectiveCloneTool    *clone_tool = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);
  LigmaPerspectiveCloneOptions *options;

  options = LIGMA_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == LIGMA_PERSPECTIVE_CLONE_MODE_PAINT &&
      key == ligma_get_toggle_behavior_mask ())
    {
      if (press)
        {
          clone_tool->saved_precision =
            ligma_tool_control_get_precision (tool->control);
          ligma_tool_control_set_precision (tool->control,
                                           LIGMA_CURSOR_PRECISION_PIXEL_CENTER);
        }
      else
        {
          ligma_tool_control_set_precision (tool->control,
                                           clone_tool->saved_precision);
        }
    }

  LIGMA_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
ligma_perspective_clone_tool_cursor_update (LigmaTool         *tool,
                                           const LigmaCoords *coords,
                                           GdkModifierType   state,
                                           LigmaDisplay      *display)
{
  LigmaPerspectiveCloneTool    *clone_tool = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);
  LigmaPerspectiveCloneOptions *options;
  LigmaImage                   *image;
  LigmaToolClass               *tool_class;
  LigmaCursorType               cursor      = LIGMA_CURSOR_MOUSE;
  LigmaToolCursorType           tool_cursor = LIGMA_TOOL_CURSOR_NONE;
  LigmaCursorModifier           modifier    = LIGMA_CURSOR_MODIFIER_NONE;

  options = LIGMA_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  image = ligma_display_get_image (display);

  if (ligma_image_coords_in_active_pickable (image, coords,
                                            FALSE, FALSE, TRUE))
    {
      cursor = LIGMA_CURSOR_MOUSE;
    }

  if (options->clone_mode == LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          if (display == tool->display)
            {
              ligma_tool_widget_get_cursor (clone_tool->widget,
                                           coords, state,
                                           &cursor, &tool_cursor, &modifier);
            }
        }
    }
  else
    {
      GdkModifierType extend_mask = ligma_get_extend_selection_mask ();
      GdkModifierType toggle_mask = ligma_get_toggle_behavior_mask ();

      if ((state & (toggle_mask | extend_mask)) == toggle_mask)
        {
          cursor = LIGMA_CURSOR_CROSSHAIR_SMALL;
        }
      else if (! LIGMA_SOURCE_OPTIONS (options)->src_drawables)
        {
          modifier = LIGMA_CURSOR_MODIFIER_BAD;
        }

      tool_cursor = LIGMA_TOOL_CURSOR_CLONE;
    }

  ligma_tool_control_set_cursor          (tool->control, cursor);
  ligma_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  /*  If we are in adjust mode, skip the LigmaBrushClass when chaining up.
   *  This ensures that the cursor will be set regardless of
   *  LigmaBrushTool::show_cursor (see bug #354933).
   */
  if (options->clone_mode == LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST)
    tool_class = LIGMA_TOOL_CLASS (g_type_class_peek_parent (parent_class));
  else
    tool_class = LIGMA_TOOL_CLASS (parent_class);

  tool_class->cursor_update (tool, coords, state, display);
}

static void
ligma_perspective_clone_tool_oper_update (LigmaTool         *tool,
                                         const LigmaCoords *coords,
                                         GdkModifierType   state,
                                         gboolean          proximity,
                                         LigmaDisplay      *display)
{
  LigmaPerspectiveCloneTool    *clone_tool = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);
  LigmaPerspectiveCloneOptions *options;

  options = LIGMA_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          if (display == tool->display)
            {
              ligma_tool_widget_hover (clone_tool->widget, coords, state,
                                      proximity);
            }
        }
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);

      if (proximity)
        {
          LigmaPaintCore        *core        = LIGMA_PAINT_TOOL (tool)->core;
          LigmaPerspectiveClone *clone       = LIGMA_PERSPECTIVE_CLONE (core);
          LigmaSourceCore       *source_core = LIGMA_SOURCE_CORE (core);

          if (LIGMA_SOURCE_OPTIONS (options)->src_drawables == NULL)
            {
              ligma_tool_replace_status (tool, display,
                                        _("Ctrl-Click to set a clone source"));
            }
          else
            {
              ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

              g_object_get (options,
                            "src-x", &clone_tool->src_x,
                            "src-y", &clone_tool->src_y,
                            NULL);

              if (! source_core->first_stroke)
                {
                  if (LIGMA_SOURCE_OPTIONS (options)->align_mode ==
                      LIGMA_SOURCE_ALIGN_YES)
                    {
                      gdouble nnx, nny;

                      /* Set the coordinates for the reference cross */
                      ligma_perspective_clone_get_source_point (clone,
                                                               coords->x,
                                                               coords->y,
                                                               &nnx, &nny);

                      clone_tool->src_x = floor (nnx);
                      clone_tool->src_y = floor (nny);
                    }
                }

              ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
            }
        }
    }
}

static void
ligma_perspective_clone_tool_options_notify (LigmaTool         *tool,
                                            LigmaToolOptions  *options,
                                            const GParamSpec *pspec)
{
  LigmaPerspectiveCloneTool    *clone_tool = LIGMA_PERSPECTIVE_CLONE_TOOL (tool);
  LigmaPaintTool               *paint_tool = LIGMA_PAINT_TOOL (tool);
  LigmaPerspectiveCloneOptions *clone_options;

  clone_options = LIGMA_PERSPECTIVE_CLONE_OPTIONS (options);

  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "clone-mode"))
    {
      LigmaPerspectiveClone *clone;

      clone = LIGMA_PERSPECTIVE_CLONE (LIGMA_PAINT_TOOL (tool)->core);

      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (clone_tool));

      if (clone_options->clone_mode == LIGMA_PERSPECTIVE_CLONE_MODE_PAINT)
        {
          ligma_perspective_clone_set_transform (clone, &clone_tool->transform);

          ligma_paint_tool_set_active (paint_tool, TRUE);
        }
      else
        {
          ligma_paint_tool_set_active (paint_tool, FALSE);

          ligma_tool_control_set_precision (tool->control,
                                           LIGMA_CURSOR_PRECISION_SUBPIXEL);

          /*  start drawing the bounding box and handles...  */
          if (tool->display &&
              ! ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (clone_tool)))
            {
              ligma_draw_tool_start (LIGMA_DRAW_TOOL (clone_tool), tool->display);
            }
        }

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (clone_tool));
    }
}

static void
ligma_perspective_clone_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaTool                    *tool        = LIGMA_TOOL (draw_tool);
  LigmaPerspectiveCloneTool    *clone_tool  = LIGMA_PERSPECTIVE_CLONE_TOOL (draw_tool);
  LigmaPerspectiveCloneOptions *options;

  options = LIGMA_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          LigmaCanvasItem *item = ligma_tool_widget_get_item (clone_tool->widget);

          ligma_draw_tool_add_item (draw_tool, item);
        }
    }
  else
    {
      LigmaCanvasGroup *stroke_group;

      stroke_group = ligma_draw_tool_add_stroke_group (draw_tool);

      /*  draw the bounding box  */
      ligma_draw_tool_push_group (draw_tool, stroke_group);

      ligma_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X0],
                               clone_tool->trans_info[Y0],
                               clone_tool->trans_info[X1],
                               clone_tool->trans_info[Y1]);
      ligma_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X1],
                               clone_tool->trans_info[Y1],
                               clone_tool->trans_info[X3],
                               clone_tool->trans_info[Y3]);
      ligma_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X2],
                               clone_tool->trans_info[Y2],
                               clone_tool->trans_info[X3],
                               clone_tool->trans_info[Y3]);
      ligma_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X2],
                               clone_tool->trans_info[Y2],
                               clone_tool->trans_info[X0],
                               clone_tool->trans_info[Y0]);

      ligma_draw_tool_pop_group (draw_tool);
    }

  if (LIGMA_SOURCE_OPTIONS (options)->src_drawables && clone_tool->src_display)
    {
      LigmaDisplay *tmp_display;

      tmp_display = draw_tool->display;
      draw_tool->display = clone_tool->src_display;

      ligma_draw_tool_add_handle (draw_tool,
                                 LIGMA_HANDLE_CROSS,
                                 clone_tool->src_x + 0.5,
                                 clone_tool->src_y + 0.5,
                                 LIGMA_TOOL_HANDLE_SIZE_CROSS,
                                 LIGMA_TOOL_HANDLE_SIZE_CROSS,
                                 LIGMA_HANDLE_ANCHOR_CENTER);

      draw_tool->display = tmp_display;
    }

  LIGMA_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
ligma_perspective_clone_tool_halt (LigmaPerspectiveCloneTool *clone_tool)
{
  LigmaTool                    *tool = LIGMA_TOOL (clone_tool);
  LigmaPerspectiveCloneOptions *options;

  options = LIGMA_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  clone_tool->src_display = NULL;

  g_object_set (options,
                "src-drawables", NULL,
                NULL);

  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tool)))
    ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

  g_clear_object (&clone_tool->widget);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
ligma_perspective_clone_tool_bounds (LigmaPerspectiveCloneTool *clone_tool,
                                    LigmaDisplay              *display)
{
  LigmaImage *image = ligma_display_get_image (display);

  clone_tool->x1 = 0;
  clone_tool->y1 = 0;
  clone_tool->x2 = ligma_image_get_width  (image);
  clone_tool->y2 = ligma_image_get_height (image);
}

static void
ligma_perspective_clone_tool_prepare (LigmaPerspectiveCloneTool *clone_tool)
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
ligma_perspective_clone_tool_recalc_matrix (LigmaPerspectiveCloneTool *clone_tool,
                                           LigmaToolWidget           *widget)
{
  ligma_matrix3_identity (&clone_tool->transform);
  ligma_transform_matrix_perspective (&clone_tool->transform,
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
ligma_perspective_clone_tool_widget_changed (LigmaToolWidget           *widget,
                                            LigmaPerspectiveCloneTool *clone_tool)
{
  LigmaMatrix3 *transform;

  g_object_get (widget,
                "transform", &transform,
                "pivot-x",   &clone_tool->trans_info[PIVOT_X],
                "pivot-y",   &clone_tool->trans_info[PIVOT_Y],
                NULL);

  ligma_matrix3_transform_point (transform,
                                clone_tool->x1, clone_tool->y1,
                                &clone_tool->trans_info[X0],
                                &clone_tool->trans_info[Y0]);
  ligma_matrix3_transform_point (transform,
                                clone_tool->x2, clone_tool->y1,
                                &clone_tool->trans_info[X1],
                                &clone_tool->trans_info[Y1]);
  ligma_matrix3_transform_point (transform,
                                clone_tool->x1, clone_tool->y2,
                                &clone_tool->trans_info[X2],
                                &clone_tool->trans_info[Y2]);
  ligma_matrix3_transform_point (transform,
                                clone_tool->x2, clone_tool->y2,
                                &clone_tool->trans_info[X3],
                                &clone_tool->trans_info[Y3]);

  g_free (transform);

  ligma_perspective_clone_tool_recalc_matrix (clone_tool, NULL);
}

static void
ligma_perspective_clone_tool_widget_status (LigmaToolWidget           *widget,
                                           const gchar              *status,
                                           LigmaPerspectiveCloneTool *clone_tool)
{
  LigmaTool *tool = LIGMA_TOOL (clone_tool);

  if (status)
    {
      ligma_tool_replace_status (tool, tool->display, "%s", status);
    }
  else
    {
      ligma_tool_pop_status (tool, tool->display);
    }
}


/*  tool options stuff  */

static GtkWidget *
ligma_perspective_clone_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_clone_options_gui (tool_options);
  GtkWidget *mode;

  /* radio buttons to set if you are modifying the perspective plane
   * or painting
   */
  mode = ligma_prop_enum_radio_box_new (config, "clone-mode", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), mode, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), mode, 0);

  return vbox;
}
