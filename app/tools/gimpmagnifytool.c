/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scale.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpmagnifyoptions.h"
#include "gimpmagnifytool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_magnify_tool_button_press    (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 guint32          time,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *display);
static void   gimp_magnify_tool_button_release  (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 guint32          time,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *display);
static void   gimp_magnify_tool_motion          (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 guint32          time,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *display);
static void   gimp_magnify_tool_modifier_key    (GimpTool        *tool,
                                                 GdkModifierType  key,
                                                 gboolean         press,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *display);
static void   gimp_magnify_tool_cursor_update   (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *display);

static void   gimp_magnify_tool_draw            (GimpDrawTool    *draw_tool);


G_DEFINE_TYPE (GimpMagnifyTool, gimp_magnify_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_magnify_tool_parent_class


void
gimp_magnify_tool_register (GimpToolRegisterCallback  callback,
                            gpointer                  data)
{
  (* callback) (GIMP_TYPE_MAGNIFY_TOOL,
                GIMP_TYPE_MAGNIFY_OPTIONS,
                gimp_magnify_options_gui,
                0,
                "gimp-zoom-tool",
                _("Zoom"),
                _("Zoom in & out"),
                N_("tool|_Zoom"), "Z",
                NULL, GIMP_HELP_TOOL_ZOOM,
                GIMP_STOCK_TOOL_ZOOM,
                data);
}

static void
gimp_magnify_tool_class_init (GimpMagnifyToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->button_press   = gimp_magnify_tool_button_press;
  tool_class->button_release = gimp_magnify_tool_button_release;
  tool_class->motion         = gimp_magnify_tool_motion;
  tool_class->modifier_key   = gimp_magnify_tool_modifier_key;
  tool_class->cursor_update  = gimp_magnify_tool_cursor_update;

  draw_tool_class->draw      = gimp_magnify_tool_draw;
}

static void
gimp_magnify_tool_init (GimpMagnifyTool *magnify_tool)
{
  GimpTool *tool = GIMP_TOOL (magnify_tool);

  magnify_tool->x = 0;
  magnify_tool->y = 0;
  magnify_tool->w = 0;
  magnify_tool->h = 0;

  gimp_tool_control_set_scroll_lock            (tool->control, TRUE);
  gimp_tool_control_set_handle_empty_image     (tool->control, TRUE);
  gimp_tool_control_set_snap_to                (tool->control, FALSE);

  gimp_tool_control_set_cursor                 (tool->control,
                                                GIMP_CURSOR_MOUSE);
  gimp_tool_control_set_tool_cursor            (tool->control,
                                                GIMP_TOOL_CURSOR_ZOOM);
  gimp_tool_control_set_cursor_modifier        (tool->control,
                                                GIMP_CURSOR_MODIFIER_PLUS);
  gimp_tool_control_set_toggle_cursor_modifier (tool->control,
                                                GIMP_CURSOR_MODIFIER_MINUS);
}

static void
gimp_magnify_tool_button_press (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
                                GdkModifierType  state,
                                GimpDisplay     *display)
{
  GimpMagnifyTool *magnify = GIMP_MAGNIFY_TOOL (tool);

  magnify->x = coords->x;
  magnify->y = coords->y;
  magnify->w = 0;
  magnify->h = 0;

  gimp_tool_control_activate (tool->control);
  tool->display = display;

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_magnify_tool_button_release (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpMagnifyTool    *magnify = GIMP_MAGNIFY_TOOL (tool);
  GimpMagnifyOptions *options = GIMP_MAGNIFY_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell   *shell;
  gdouble             current;

  shell = GIMP_DISPLAY_SHELL (tool->display->shell);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  /*  First take care of the case where the user "cancels" the action  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      gint    x1, y1, x2, y2, w, h;
      gint    win_width, win_height;
      gint    offset_x, offset_y;
      gdouble new_scale;

      x1 = (magnify->w < 0) ?  magnify->x + magnify->w : magnify->x;
      y1 = (magnify->h < 0) ?  magnify->y + magnify->h : magnify->y;
      w  = (magnify->w < 0) ? -magnify->w : magnify->w;
      h  = (magnify->h < 0) ? -magnify->h : magnify->h;
      x2 = x1 + w;
      y2 = y1 + h;

      win_width  = shell->disp_width;
      win_height = shell->disp_height;

      current = gimp_zoom_model_get_factor (shell->zoom);

      /* we need to compute the mouse movement in screen coordinates */
      if ((SCALEX (shell, w) < options->threshold) ||
          (SCALEY (shell, h) < options->threshold))
        {
          new_scale = gimp_zoom_model_zoom_step (options->zoom_type, current);
        }
      else
        {
          gint    width, height;
          gdouble scale = 1.0;

          width  = UNSCALEX (shell, win_width);
          height = UNSCALEY (shell, win_height);

          switch (options->zoom_type)
            {
            case GIMP_ZOOM_IN:
              scale = MIN (((gdouble) width  / (gdouble) w),
                           ((gdouble) height / (gdouble) h));
              break;

            case GIMP_ZOOM_OUT:
              scale = MIN (((gdouble) w / (gdouble) width),
                           ((gdouble) h / (gdouble) height));
              break;

            default:
              break;
            }

          new_scale = current * scale;
        }

      offset_x = (new_scale * ((x1 + x2) / 2)
                  * SCREEN_XRES (shell) / display->image->xresolution
                  - (win_width  / 2));

      offset_y = (new_scale * ((y1 + y2) / 2)
                  * SCREEN_YRES (shell) / display->image->yresolution
                  - (win_height / 2));

      gimp_display_shell_scale_by_values (shell,
                                          new_scale,
                                          offset_x, offset_y,
                                          options->auto_resize);
    }
}

static void
gimp_magnify_tool_motion (GimpTool        *tool,
                          GimpCoords      *coords,
                          guint32          time,
                          GdkModifierType  state,
                          GimpDisplay     *display)
{
  GimpMagnifyTool *magnify = GIMP_MAGNIFY_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  magnify->w = (coords->x - magnify->x);
  magnify->h = (coords->y - magnify->y);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_magnify_tool_modifier_key (GimpTool        *tool,
                                GdkModifierType  key,
                                gboolean         press,
                                GdkModifierType  state,
                                GimpDisplay     *display)
{
  GimpMagnifyOptions *options = GIMP_MAGNIFY_TOOL_GET_OPTIONS (tool);

  if (key == GDK_CONTROL_MASK)
    {
      switch (options->zoom_type)
        {
        case GIMP_ZOOM_IN:
          g_object_set (options, "zoom-type", GIMP_ZOOM_OUT, NULL);
          break;

        case GIMP_ZOOM_OUT:
          g_object_set (options, "zoom-type", GIMP_ZOOM_IN, NULL);
          break;

        default:
          break;
        }
    }
}

static void
gimp_magnify_tool_cursor_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 GimpDisplay     *display)
{
  GimpMagnifyOptions *options = GIMP_MAGNIFY_TOOL_GET_OPTIONS (tool);

  gimp_tool_control_set_toggled (tool->control,
                                 options->zoom_type == GIMP_ZOOM_OUT);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_magnify_tool_draw (GimpDrawTool *draw_tool)
{
  GimpMagnifyTool *magnify = GIMP_MAGNIFY_TOOL (draw_tool);

  gimp_draw_tool_draw_rectangle (draw_tool,
                                 FALSE,
                                 magnify->x,
                                 magnify->y,
                                 magnify->w,
                                 magnify->h,
                                 FALSE);
}
