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

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"

#include "paint/gimpsourcecore.h"
#include "paint/gimpsourceoptions.h"

#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvashandle.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-items.h"

#include "gimpsourcetool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static gboolean      gimp_source_tool_has_display   (GimpTool            *tool,
                                                     GimpDisplay         *display);
static GimpDisplay * gimp_source_tool_has_image     (GimpTool            *tool,
                                                     GimpImage           *image);
static void          gimp_source_tool_control       (GimpTool            *tool,
                                                     GimpToolAction       action,
                                                     GimpDisplay         *display);
static void          gimp_source_tool_button_press  (GimpTool            *tool,
                                                     const GimpCoords    *coords,
                                                     guint32              time,
                                                     GdkModifierType      state,
                                                     GimpButtonPressType  press_type,
                                                     GimpDisplay         *display);
static void          gimp_source_tool_motion        (GimpTool            *tool,
                                                     const GimpCoords    *coords,
                                                     guint32              time,
                                                     GdkModifierType      state,
                                                     GimpDisplay         *display);
static void          gimp_source_tool_cursor_update (GimpTool            *tool,
                                                     const GimpCoords    *coords,
                                                     GdkModifierType      state,
                                                     GimpDisplay         *display);
static void          gimp_source_tool_modifier_key  (GimpTool            *tool,
                                                     GdkModifierType      key,
                                                     gboolean             press,
                                                     GdkModifierType      state,
                                                     GimpDisplay         *display);
static void          gimp_source_tool_oper_update   (GimpTool            *tool,
                                                     const GimpCoords    *coords,
                                                     GdkModifierType      state,
                                                     gboolean             proximity,
                                                     GimpDisplay         *display);

static void          gimp_source_tool_draw          (GimpDrawTool        *draw_tool);

static void          gimp_source_tool_paint_prepare (GimpPaintTool       *paint_tool,
                                                     GimpDisplay         *display);

static void          gimp_source_tool_set_src_display (GimpSourceTool      *source_tool,
                                                       GimpDisplay         *display);


G_DEFINE_TYPE (GimpSourceTool, gimp_source_tool, GIMP_TYPE_BRUSH_TOOL)

#define parent_class gimp_source_tool_parent_class


static void
gimp_source_tool_class_init (GimpSourceToolClass *klass)
{
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass  *draw_tool_class  = GIMP_DRAW_TOOL_CLASS (klass);
  GimpPaintToolClass *paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  tool_class->has_display         = gimp_source_tool_has_display;
  tool_class->has_image           = gimp_source_tool_has_image;
  tool_class->control             = gimp_source_tool_control;
  tool_class->button_press        = gimp_source_tool_button_press;
  tool_class->motion              = gimp_source_tool_motion;
  tool_class->modifier_key        = gimp_source_tool_modifier_key;
  tool_class->oper_update         = gimp_source_tool_oper_update;
  tool_class->cursor_update       = gimp_source_tool_cursor_update;

  draw_tool_class->draw           = gimp_source_tool_draw;

  paint_tool_class->paint_prepare = gimp_source_tool_paint_prepare;
}

static void
gimp_source_tool_init (GimpSourceTool *source)
{
  source->show_source_outline = TRUE;

  gimp_paint_tool_enable_multi_paint (GIMP_PAINT_TOOL (source));
}

static gboolean
gimp_source_tool_has_display (GimpTool    *tool,
                              GimpDisplay *display)
{
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (tool);

  return (display == source_tool->src_display ||
          GIMP_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static GimpDisplay *
gimp_source_tool_has_image (GimpTool  *tool,
                            GimpImage *image)
{
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpDisplay    *display;

  display = GIMP_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && source_tool->src_display)
    {
      if (image && gimp_display_get_image (source_tool->src_display) == image)
        display = source_tool->src_display;

      /*  NULL image means any display  */
      if (! image)
        display = source_tool->src_display;
    }

  return display;
}

static void
gimp_source_tool_control (GimpTool       *tool,
                          GimpToolAction  action,
                          GimpDisplay    *display)
{
  GimpSourceTool    *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpSourceOptions *options     = GIMP_SOURCE_TOOL_GET_OPTIONS (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_source_tool_set_src_display (source_tool, NULL);
      g_object_set (options,
                    "src-drawables", NULL,
                    "src-x",         0,
                    "src-y",         0,
                    NULL);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_source_tool_button_press (GimpTool            *tool,
                               const GimpCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               GimpButtonPressType  press_type,
                               GimpDisplay         *display)
{
  GimpPaintTool     *paint_tool  = GIMP_PAINT_TOOL (tool);
  GimpSourceTool    *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpSourceCore    *source      = GIMP_SOURCE_CORE (paint_tool->core);
  GimpSourceOptions *options     = GIMP_SOURCE_TOOL_GET_OPTIONS (tool);
  GdkModifierType    extend_mask = gimp_get_extend_selection_mask ();
  GdkModifierType    toggle_mask = gimp_get_toggle_behavior_mask ();

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if ((state & (toggle_mask | extend_mask)) == toggle_mask)
    {
      source->set_source = TRUE;

      gimp_source_tool_set_src_display (source_tool, display);
    }
  else
    {
      source->set_source = FALSE;
    }

  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);

  g_object_get (options,
                "src-x", &source_tool->src_x,
                "src-y", &source_tool->src_y,
                NULL);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_source_tool_motion (GimpTool         *tool,
                         const GimpCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         GimpDisplay      *display)
{
  GimpSourceTool    *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpSourceOptions *options     = GIMP_SOURCE_TOOL_GET_OPTIONS (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  g_object_get (options,
                "src-x", &source_tool->src_x,
                "src-y", &source_tool->src_y,
                NULL);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_source_tool_modifier_key (GimpTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  GimpSourceTool    *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpPaintTool     *paint_tool  = GIMP_PAINT_TOOL (tool);
  GimpSourceOptions *options     = GIMP_SOURCE_TOOL_GET_OPTIONS (tool);

  if (gimp_source_core_use_source (GIMP_SOURCE_CORE (paint_tool->core),
                                   options) &&
      key == gimp_get_toggle_behavior_mask ())
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      if (press)
        {
          paint_tool->status = source_tool->status_set_source;

          source_tool->show_source_outline = FALSE;

          source_tool->saved_precision =
            gimp_tool_control_get_precision (tool->control);
          gimp_tool_control_set_precision (tool->control,
                                           GIMP_CURSOR_PRECISION_PIXEL_CENTER);
        }
      else
        {
          paint_tool->status = source_tool->status_paint;

          source_tool->show_source_outline = TRUE;

          gimp_tool_control_set_precision (tool->control,
                                           source_tool->saved_precision);
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
gimp_source_tool_cursor_update (GimpTool         *tool,
                                const GimpCoords *coords,
                                GdkModifierType   state,
                                GimpDisplay      *display)
{
  GimpPaintTool      *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpSourceOptions  *options    = GIMP_SOURCE_TOOL_GET_OPTIONS (tool);
  GimpCursorType      cursor     = GIMP_CURSOR_MOUSE;
  GimpCursorModifier  modifier   = GIMP_CURSOR_MODIFIER_NONE;

  if (gimp_source_core_use_source (GIMP_SOURCE_CORE (paint_tool->core),
                                   options))
    {
      GdkModifierType extend_mask = gimp_get_extend_selection_mask ();
      GdkModifierType toggle_mask = gimp_get_toggle_behavior_mask ();

      if ((state & (toggle_mask | extend_mask)) == toggle_mask)
        {
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
        }
      else if (! options->src_drawables)
        {
          modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_source_tool_oper_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              GimpDisplay      *display)
{
  GimpPaintTool     *paint_tool  = GIMP_PAINT_TOOL (tool);
  GimpSourceTool    *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpSourceOptions *options     = GIMP_SOURCE_TOOL_GET_OPTIONS (tool);
  GimpSourceCore    *source;

  source = GIMP_SOURCE_CORE (GIMP_PAINT_TOOL (tool)->core);

  if (proximity)
    {
      if (gimp_source_core_use_source (source, options))
        paint_tool->status_ctrl = source_tool->status_set_source_ctrl;
      else
        paint_tool->status_ctrl = NULL;
    }

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  if (gimp_source_core_use_source (source, options))
    {
      if (options->src_drawables == NULL)
        {
          GdkModifierType toggle_mask = gimp_get_toggle_behavior_mask ();

          if (state & toggle_mask)
            {
              gimp_tool_replace_status (tool, display, "%s",
                                        source_tool->status_set_source);
            }
          else
            {
              gimp_tool_replace_status (tool, display, "%s-%s",
                                        gimp_get_mod_string (toggle_mask),
                                        source_tool->status_set_source);
            }
        }
      else
        {
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

          g_object_get (options,
                        "src-x", &source_tool->src_x,
                        "src-y", &source_tool->src_y,
                        NULL);

          if (! source->first_stroke)
            {
              switch (options->align_mode)
                {
                case GIMP_SOURCE_ALIGN_YES:
                  source_tool->src_x = floor (coords->x) + source->offset_x;
                  source_tool->src_y = floor (coords->y) + source->offset_y;
                  break;

                case GIMP_SOURCE_ALIGN_REGISTERED:
                  source_tool->src_x = floor (coords->x);
                  source_tool->src_y = floor (coords->y);
                  break;

                default:
                  break;
                }
            }

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
    }
}

static void
gimp_source_tool_draw (GimpDrawTool *draw_tool)
{
  GimpSourceTool    *source_tool = GIMP_SOURCE_TOOL (draw_tool);
  GimpSourceOptions *options     = GIMP_SOURCE_TOOL_GET_OPTIONS (draw_tool);
  GimpSourceCore    *source;

  source = GIMP_SOURCE_CORE (GIMP_PAINT_TOOL (draw_tool)->core);

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  if (gimp_source_core_use_source (source, options) &&
      options->src_drawables && source_tool->src_display)
    {
      GimpDisplayShell *src_shell;
      gdouble           src_x;
      gdouble           src_y;

      src_shell = gimp_display_get_shell (source_tool->src_display);

      src_x = (gdouble) source_tool->src_x + 0.5;
      src_y = (gdouble) source_tool->src_y + 0.5;

      if (source_tool->src_outline)
        {
          gimp_display_shell_remove_tool_item (src_shell,
                                               source_tool->src_outline);
          source_tool->src_outline = NULL;
        }

      if (source_tool->show_source_outline)
        {
          source_tool->src_outline =
            gimp_brush_tool_create_outline (GIMP_BRUSH_TOOL (source_tool),
                                            source_tool->src_display,
                                            src_x, src_y);

          if (source_tool->src_outline)
            {
              gimp_display_shell_add_tool_item (src_shell,
                                                source_tool->src_outline);
              g_object_unref (source_tool->src_outline);
            }
        }

      if (source_tool->src_outline)
        {
          if (source_tool->src_handle)
            {
              gimp_display_shell_remove_tool_item (src_shell,
                                                   source_tool->src_handle);
              source_tool->src_handle = NULL;
            }
        }
      else
        {
          if (! source_tool->src_handle)
            {
              source_tool->src_handle =
                gimp_canvas_handle_new (src_shell,
                                        GIMP_HANDLE_CROSS,
                                        GIMP_HANDLE_ANCHOR_CENTER,
                                        src_x, src_y,
                                        GIMP_TOOL_HANDLE_SIZE_CROSS,
                                        GIMP_TOOL_HANDLE_SIZE_CROSS);
              gimp_display_shell_add_tool_item (src_shell,
                                                source_tool->src_handle);
              g_object_unref (source_tool->src_handle);
            }
          else
            {
              gimp_canvas_handle_set_position (source_tool->src_handle,
                                               src_x, src_y);
            }
        }
    }
}

static void
gimp_source_tool_paint_prepare (GimpPaintTool *paint_tool,
                                GimpDisplay   *display)
{
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (paint_tool);

  if (GIMP_PAINT_TOOL_CLASS (parent_class)->paint_prepare)
    GIMP_PAINT_TOOL_CLASS (parent_class)->paint_prepare (paint_tool, display);

  if (source_tool->src_display)
    {
      GimpDisplayShell *src_shell;

      src_shell = gimp_display_get_shell (source_tool->src_display);

      gimp_paint_core_set_show_all (paint_tool->core, src_shell->show_all);
    }
}

static void
gimp_source_tool_set_src_display (GimpSourceTool *source_tool,
                                  GimpDisplay    *display)
{
  if (source_tool->src_display != display)
    {
      if (source_tool->src_display)
        {
          GimpDisplayShell *src_shell;

          src_shell = gimp_display_get_shell (source_tool->src_display);

          if (source_tool->src_handle)
            {
              gimp_display_shell_remove_tool_item (src_shell,
                                                   source_tool->src_handle);
              source_tool->src_handle = NULL;
            }

          if (source_tool->src_outline)
            {
              gimp_display_shell_remove_tool_item (src_shell,
                                                   source_tool->src_outline);
              source_tool->src_outline = NULL;
            }
        }

      source_tool->src_display = display;
    }
}
