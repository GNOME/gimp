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

#include "tools-types.h"

#include "core/ligmachannel.h"
#include "core/ligmaimage.h"
#include "core/ligmapickable.h"

#include "paint/ligmasourcecore.h"
#include "paint/ligmasourceoptions.h"

#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvashandle.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-items.h"

#include "ligmasourcetool.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static gboolean      ligma_source_tool_has_display   (LigmaTool            *tool,
                                                     LigmaDisplay         *display);
static LigmaDisplay * ligma_source_tool_has_image     (LigmaTool            *tool,
                                                     LigmaImage           *image);
static void          ligma_source_tool_control       (LigmaTool            *tool,
                                                     LigmaToolAction       action,
                                                     LigmaDisplay         *display);
static void          ligma_source_tool_button_press  (LigmaTool            *tool,
                                                     const LigmaCoords    *coords,
                                                     guint32              time,
                                                     GdkModifierType      state,
                                                     LigmaButtonPressType  press_type,
                                                     LigmaDisplay         *display);
static void          ligma_source_tool_motion        (LigmaTool            *tool,
                                                     const LigmaCoords    *coords,
                                                     guint32              time,
                                                     GdkModifierType      state,
                                                     LigmaDisplay         *display);
static void          ligma_source_tool_cursor_update (LigmaTool            *tool,
                                                     const LigmaCoords    *coords,
                                                     GdkModifierType      state,
                                                     LigmaDisplay         *display);
static void          ligma_source_tool_modifier_key  (LigmaTool            *tool,
                                                     GdkModifierType      key,
                                                     gboolean             press,
                                                     GdkModifierType      state,
                                                     LigmaDisplay         *display);
static void          ligma_source_tool_oper_update   (LigmaTool            *tool,
                                                     const LigmaCoords    *coords,
                                                     GdkModifierType      state,
                                                     gboolean             proximity,
                                                     LigmaDisplay         *display);

static void          ligma_source_tool_draw          (LigmaDrawTool        *draw_tool);

static void          ligma_source_tool_paint_prepare (LigmaPaintTool       *paint_tool,
                                                     LigmaDisplay         *display);

static void          ligma_source_tool_set_src_display (LigmaSourceTool      *source_tool,
                                                       LigmaDisplay         *display);


G_DEFINE_TYPE (LigmaSourceTool, ligma_source_tool, LIGMA_TYPE_BRUSH_TOOL)

#define parent_class ligma_source_tool_parent_class


static void
ligma_source_tool_class_init (LigmaSourceToolClass *klass)
{
  LigmaToolClass      *tool_class       = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass  *draw_tool_class  = LIGMA_DRAW_TOOL_CLASS (klass);
  LigmaPaintToolClass *paint_tool_class = LIGMA_PAINT_TOOL_CLASS (klass);

  tool_class->has_display         = ligma_source_tool_has_display;
  tool_class->has_image           = ligma_source_tool_has_image;
  tool_class->control             = ligma_source_tool_control;
  tool_class->button_press        = ligma_source_tool_button_press;
  tool_class->motion              = ligma_source_tool_motion;
  tool_class->modifier_key        = ligma_source_tool_modifier_key;
  tool_class->oper_update         = ligma_source_tool_oper_update;
  tool_class->cursor_update       = ligma_source_tool_cursor_update;

  draw_tool_class->draw           = ligma_source_tool_draw;

  paint_tool_class->paint_prepare = ligma_source_tool_paint_prepare;
}

static void
ligma_source_tool_init (LigmaSourceTool *source)
{
  source->show_source_outline = TRUE;

  ligma_paint_tool_enable_multi_paint (LIGMA_PAINT_TOOL (source));
}

static gboolean
ligma_source_tool_has_display (LigmaTool    *tool,
                              LigmaDisplay *display)
{
  LigmaSourceTool *source_tool = LIGMA_SOURCE_TOOL (tool);

  return (display == source_tool->src_display ||
          LIGMA_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static LigmaDisplay *
ligma_source_tool_has_image (LigmaTool  *tool,
                            LigmaImage *image)
{
  LigmaSourceTool *source_tool = LIGMA_SOURCE_TOOL (tool);
  LigmaDisplay    *display;

  display = LIGMA_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && source_tool->src_display)
    {
      if (image && ligma_display_get_image (source_tool->src_display) == image)
        display = source_tool->src_display;

      /*  NULL image means any display  */
      if (! image)
        display = source_tool->src_display;
    }

  return display;
}

static void
ligma_source_tool_control (LigmaTool       *tool,
                          LigmaToolAction  action,
                          LigmaDisplay    *display)
{
  LigmaSourceTool    *source_tool = LIGMA_SOURCE_TOOL (tool);
  LigmaSourceOptions *options     = LIGMA_SOURCE_TOOL_GET_OPTIONS (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_source_tool_set_src_display (source_tool, NULL);
      g_object_set (options,
                    "src-drawables", NULL,
                    "src-x",         0,
                    "src-y",         0,
                    NULL);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_source_tool_button_press (LigmaTool            *tool,
                               const LigmaCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               LigmaButtonPressType  press_type,
                               LigmaDisplay         *display)
{
  LigmaPaintTool     *paint_tool  = LIGMA_PAINT_TOOL (tool);
  LigmaSourceTool    *source_tool = LIGMA_SOURCE_TOOL (tool);
  LigmaSourceCore    *source      = LIGMA_SOURCE_CORE (paint_tool->core);
  LigmaSourceOptions *options     = LIGMA_SOURCE_TOOL_GET_OPTIONS (tool);
  GdkModifierType    extend_mask = ligma_get_extend_selection_mask ();
  GdkModifierType    toggle_mask = ligma_get_toggle_behavior_mask ();

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  if ((state & (toggle_mask | extend_mask)) == toggle_mask)
    {
      source->set_source = TRUE;

      ligma_source_tool_set_src_display (source_tool, display);
    }
  else
    {
      source->set_source = FALSE;
    }

  LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);

  g_object_get (options,
                "src-x", &source_tool->src_x,
                "src-y", &source_tool->src_y,
                NULL);

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_source_tool_motion (LigmaTool         *tool,
                         const LigmaCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         LigmaDisplay      *display)
{
  LigmaSourceTool    *source_tool = LIGMA_SOURCE_TOOL (tool);
  LigmaPaintTool     *paint_tool  = LIGMA_PAINT_TOOL (tool);
  LigmaSourceCore    *source      = LIGMA_SOURCE_CORE (paint_tool->core);
  LigmaSourceOptions *options     = LIGMA_SOURCE_TOOL_GET_OPTIONS (tool);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  LIGMA_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  g_object_get (options,
                "src-x", &source_tool->src_x,
                "src-y", &source_tool->src_y,
                NULL);

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_source_tool_modifier_key (LigmaTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               LigmaDisplay     *display)
{
  LigmaSourceTool    *source_tool = LIGMA_SOURCE_TOOL (tool);
  LigmaPaintTool     *paint_tool  = LIGMA_PAINT_TOOL (tool);
  LigmaSourceOptions *options     = LIGMA_SOURCE_TOOL_GET_OPTIONS (tool);

  if (ligma_source_core_use_source (LIGMA_SOURCE_CORE (paint_tool->core),
                                   options) &&
      key == ligma_get_toggle_behavior_mask ())
    {
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

      if (press)
        {
          paint_tool->status = source_tool->status_set_source;

          source_tool->show_source_outline = FALSE;

          source_tool->saved_precision =
            ligma_tool_control_get_precision (tool->control);
          ligma_tool_control_set_precision (tool->control,
                                           LIGMA_CURSOR_PRECISION_PIXEL_CENTER);
        }
      else
        {
          paint_tool->status = source_tool->status_paint;

          source_tool->show_source_outline = TRUE;

          ligma_tool_control_set_precision (tool->control,
                                           source_tool->saved_precision);
        }

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
    }

  LIGMA_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
ligma_source_tool_cursor_update (LigmaTool         *tool,
                                const LigmaCoords *coords,
                                GdkModifierType   state,
                                LigmaDisplay      *display)
{
  LigmaPaintTool      *paint_tool = LIGMA_PAINT_TOOL (tool);
  LigmaSourceOptions  *options    = LIGMA_SOURCE_TOOL_GET_OPTIONS (tool);
  LigmaCursorType      cursor     = LIGMA_CURSOR_MOUSE;
  LigmaCursorModifier  modifier   = LIGMA_CURSOR_MODIFIER_NONE;

  if (ligma_source_core_use_source (LIGMA_SOURCE_CORE (paint_tool->core),
                                   options))
    {
      GdkModifierType extend_mask = ligma_get_extend_selection_mask ();
      GdkModifierType toggle_mask = ligma_get_toggle_behavior_mask ();

      if ((state & (toggle_mask | extend_mask)) == toggle_mask)
        {
          cursor = LIGMA_CURSOR_CROSSHAIR_SMALL;
        }
      else if (! options->src_drawables)
        {
          modifier = LIGMA_CURSOR_MODIFIER_BAD;
        }
    }

  ligma_tool_control_set_cursor          (tool->control, cursor);
  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_source_tool_oper_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              LigmaDisplay      *display)
{
  LigmaPaintTool     *paint_tool  = LIGMA_PAINT_TOOL (tool);
  LigmaSourceTool    *source_tool = LIGMA_SOURCE_TOOL (tool);
  LigmaSourceOptions *options     = LIGMA_SOURCE_TOOL_GET_OPTIONS (tool);
  LigmaSourceCore    *source;

  source = LIGMA_SOURCE_CORE (LIGMA_PAINT_TOOL (tool)->core);

  if (proximity)
    {
      if (ligma_source_core_use_source (source, options))
        paint_tool->status_ctrl = source_tool->status_set_source_ctrl;
      else
        paint_tool->status_ctrl = NULL;
    }

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  if (ligma_source_core_use_source (source, options))
    {
      if (options->src_drawables == NULL)
        {
          GdkModifierType toggle_mask = ligma_get_toggle_behavior_mask ();

          if (state & toggle_mask)
            {
              ligma_tool_replace_status (tool, display, "%s",
                                        source_tool->status_set_source);
            }
          else
            {
              ligma_tool_replace_status (tool, display, "%s-%s",
                                        ligma_get_mod_string (toggle_mask),
                                        source_tool->status_set_source);
            }
        }
      else
        {
          ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

          g_object_get (options,
                        "src-x", &source_tool->src_x,
                        "src-y", &source_tool->src_y,
                        NULL);

          if (! source->first_stroke)
            {
              switch (options->align_mode)
                {
                case LIGMA_SOURCE_ALIGN_YES:
                  source_tool->src_x = floor (coords->x) + source->offset_x;
                  source_tool->src_y = floor (coords->y) + source->offset_y;
                  break;

                case LIGMA_SOURCE_ALIGN_REGISTERED:
                  source_tool->src_x = floor (coords->x);
                  source_tool->src_y = floor (coords->y);
                  break;

                default:
                  break;
                }
            }

          ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
        }
    }
}

static void
ligma_source_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaSourceTool    *source_tool = LIGMA_SOURCE_TOOL (draw_tool);
  LigmaSourceOptions *options     = LIGMA_SOURCE_TOOL_GET_OPTIONS (draw_tool);
  LigmaSourceCore    *source;

  source = LIGMA_SOURCE_CORE (LIGMA_PAINT_TOOL (draw_tool)->core);

  LIGMA_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  if (ligma_source_core_use_source (source, options) &&
      options->src_drawables && source_tool->src_display)
    {
      LigmaDisplayShell *src_shell;
      gdouble           src_x;
      gdouble           src_y;

      src_shell = ligma_display_get_shell (source_tool->src_display);

      src_x = (gdouble) source_tool->src_x + 0.5;
      src_y = (gdouble) source_tool->src_y + 0.5;

      if (source_tool->src_outline)
        {
          ligma_display_shell_remove_tool_item (src_shell,
                                               source_tool->src_outline);
          source_tool->src_outline = NULL;
        }

      if (source_tool->show_source_outline)
        {
          source_tool->src_outline =
            ligma_brush_tool_create_outline (LIGMA_BRUSH_TOOL (source_tool),
                                            source_tool->src_display,
                                            src_x, src_y);

          if (source_tool->src_outline)
            {
              ligma_display_shell_add_tool_item (src_shell,
                                                source_tool->src_outline);
              g_object_unref (source_tool->src_outline);
            }
        }

      if (source_tool->src_outline)
        {
          if (source_tool->src_handle)
            {
              ligma_display_shell_remove_tool_item (src_shell,
                                                   source_tool->src_handle);
              source_tool->src_handle = NULL;
            }
        }
      else
        {
          if (! source_tool->src_handle)
            {
              source_tool->src_handle =
                ligma_canvas_handle_new (src_shell,
                                        LIGMA_HANDLE_CROSS,
                                        LIGMA_HANDLE_ANCHOR_CENTER,
                                        src_x, src_y,
                                        LIGMA_TOOL_HANDLE_SIZE_CROSS,
                                        LIGMA_TOOL_HANDLE_SIZE_CROSS);
              ligma_display_shell_add_tool_item (src_shell,
                                                source_tool->src_handle);
              g_object_unref (source_tool->src_handle);
            }
          else
            {
              ligma_canvas_handle_set_position (source_tool->src_handle,
                                               src_x, src_y);
            }
        }
    }
}

static void
ligma_source_tool_paint_prepare (LigmaPaintTool *paint_tool,
                                LigmaDisplay   *display)
{
  LigmaSourceTool *source_tool = LIGMA_SOURCE_TOOL (paint_tool);

  if (LIGMA_PAINT_TOOL_CLASS (parent_class)->paint_prepare)
    LIGMA_PAINT_TOOL_CLASS (parent_class)->paint_prepare (paint_tool, display);

  if (source_tool->src_display)
    {
      LigmaDisplayShell *src_shell;

      src_shell = ligma_display_get_shell (source_tool->src_display);

      ligma_paint_core_set_show_all (paint_tool->core, src_shell->show_all);
    }
}

static void
ligma_source_tool_set_src_display (LigmaSourceTool *source_tool,
                                  LigmaDisplay    *display)
{
  if (source_tool->src_display != display)
    {
      if (source_tool->src_display)
        {
          LigmaDisplayShell *src_shell;

          src_shell = ligma_display_get_shell (source_tool->src_display);

          if (source_tool->src_handle)
            {
              ligma_display_shell_remove_tool_item (src_shell,
                                                   source_tool->src_handle);
              source_tool->src_handle = NULL;
            }

          if (source_tool->src_outline)
            {
              ligma_display_shell_remove_tool_item (src_shell,
                                                   source_tool->src_outline);
              source_tool->src_outline = NULL;
            }
        }

      source_tool->src_display = display;
    }
}
