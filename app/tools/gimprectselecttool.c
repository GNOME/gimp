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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-utils.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimpmarshal.h"
#include "core/gimptoolinfo.h"
#include "core/gimpunit.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimprectselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_rect_select_tool_button_press     (GimpTool        *tool,
                                                      GimpCoords      *coords,
                                                      guint32          time,
                                                      GdkModifierType  state,
                                                      GimpDisplay     *display);
static void   gimp_rect_select_tool_button_release   (GimpTool        *tool,
                                                      GimpCoords      *coords,
                                                      guint32          time,
                                                      GdkModifierType  state,
                                                      GimpDisplay     *display);
static void   gimp_rect_select_tool_motion           (GimpTool        *tool,
                                                      GimpCoords      *coords,
                                                      guint32          time,
                                                      GdkModifierType  state,
                                                      GimpDisplay     *display);

static void   gimp_rect_select_tool_draw             (GimpDrawTool    *draw_tool);

static void   gimp_rect_select_tool_real_rect_select (GimpRectSelectTool *rect_tool,
                                                      gint                x,
                                                      gint                y,
                                                      gint                w,
                                                      gint                h);

static void   gimp_rect_select_tool_update_options   (GimpRectSelectTool *rect_sel,
                                                      GimpDisplay        *display);


G_DEFINE_TYPE (GimpRectSelectTool, gimp_rect_select_tool,
               GIMP_TYPE_SELECTION_TOOL);

#define parent_class gimp_rect_select_tool_parent_class


void
gimp_rect_select_tool_register (GimpToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (GIMP_TYPE_RECT_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-rect-select-tool",
                _("Rectangle Select"),
                _("Select rectangular regions"),
                N_("_Rectangle Select"), "R",
                NULL, GIMP_HELP_TOOL_RECT_SELECT,
                GIMP_STOCK_TOOL_RECT_SELECT,
                data);
}

static void
gimp_rect_select_tool_class_init (GimpRectSelectToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->button_press   = gimp_rect_select_tool_button_press;
  tool_class->button_release = gimp_rect_select_tool_button_release;
  tool_class->motion         = gimp_rect_select_tool_motion;

  draw_tool_class->draw      = gimp_rect_select_tool_draw;

  klass->rect_select         = gimp_rect_select_tool_real_rect_select;
}

static void
gimp_rect_select_tool_init (GimpRectSelectTool *rect_select)
{
  GimpTool *tool = GIMP_TOOL (rect_select);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_RECT_SELECT);

  rect_select->sx = rect_select->sy = 0;
  rect_select->x = rect_select->y = 0;
  rect_select->w = rect_select->h = 0;
}

static void
gimp_rect_select_tool_button_press (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpRectSelectTool   *rect_sel = GIMP_RECT_SELECT_TOOL (tool);
  GimpSelectionTool    *sel_tool = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions *options;
  GimpUnit              unit     = GIMP_UNIT_PIXEL;
  gdouble               unit_factor;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  rect_sel->sx     = RINT(coords->x);
  rect_sel->sy     = RINT(coords->y);
  rect_sel->x      = 0;
  rect_sel->y      = 0;
  rect_sel->w      = 0;
  rect_sel->h      = 0;
  rect_sel->lx     = RINT(coords->x);
  rect_sel->ly     = RINT(coords->y);
  rect_sel->center = FALSE;

  rect_sel->fixed_mode   = options->fixed_mode;
  rect_sel->fixed_width  = options->fixed_width;
  rect_sel->fixed_height = options->fixed_height;
  unit                   = options->fixed_unit;

  switch (unit)
    {
    case GIMP_UNIT_PIXEL:
      break;
    case GIMP_UNIT_PERCENT:
      rect_sel->fixed_width =
        display->image->width * rect_sel->fixed_width / 100;
      rect_sel->fixed_height =
        display->image->height * rect_sel->fixed_height / 100;
      break;
    default:
      unit_factor = _gimp_unit_get_factor (tool->tool_info->gimp, unit);
      rect_sel->fixed_width =
        rect_sel->fixed_width * display->image->xresolution / unit_factor;
      rect_sel->fixed_height =
        rect_sel->fixed_height * display->image->yresolution / unit_factor;
      break;
    }

  rect_sel->fixed_width  = MAX (1, rect_sel->fixed_width);
  rect_sel->fixed_height = MAX (1, rect_sel->fixed_height);

  gimp_tool_control_activate (tool->control);
  tool->display = display;

  if (gimp_selection_tool_start_edit (sel_tool, coords))
    return;

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_rect_select_tool_button_release (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      guint32          time,
                                      GdkModifierType  state,
                                      GimpDisplay     *display)
{
  GimpRectSelectTool *rect_sel = GIMP_RECT_SELECT_TOOL (tool);

  gimp_tool_pop_status (tool, display);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  /*  First take care of the case where the user "cancels" the action  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      if (rect_sel->w == 0 || rect_sel->h == 0)
        {
          /*  If there is a floating selection, anchor it  */
          if (gimp_image_floating_sel (display->image))
            floating_sel_anchor (gimp_image_floating_sel (display->image));
          /*  Otherwise, clear the selection mask  */
          else
            gimp_channel_clear (gimp_image_get_mask (display->image), NULL,
                                TRUE);

          gimp_image_flush (display->image);
          return;
        }

      gimp_rect_select_tool_rect_select (rect_sel,
                                         rect_sel->x, rect_sel->y,
                                         rect_sel->w, rect_sel->h);

      /*  show selection on all views  */
      gimp_image_flush (display->image);
    }
}

static void
gimp_rect_select_tool_motion (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpRectSelectTool *rect_sel = GIMP_RECT_SELECT_TOOL (tool);
  GimpSelectionTool  *sel_tool = GIMP_SELECTION_TOOL (tool);
  gint                mx, my;
  gdouble             ratio;

  if (sel_tool->op == SELECTION_ANCHOR)
    {
      sel_tool->op = SELECTION_REPLACE;

      gimp_tool_cursor_update (tool, coords, state, display);
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));


  if (state & GDK_MOD1_MASK)
    {
      /*  Just move the selection rectangle around  */

      mx = RINT(coords->x) - rect_sel->lx;
      my = RINT(coords->y) - rect_sel->ly;

      rect_sel->sx += mx;
      rect_sel->sy += my;
      rect_sel->x += mx;
      rect_sel->y += my;
    }
  else
    {
      /* Change the selection rectangle's size, first calculate absolute
       * width and height, then take care of quadrants.
       */

      if (rect_sel->fixed_mode == GIMP_RECT_SELECT_MODE_FIXED_SIZE)
        {
          rect_sel->w = RINT(rect_sel->fixed_width);
          rect_sel->h = RINT(rect_sel->fixed_height);
        }
      else
        {
          rect_sel->w = abs(RINT(coords->x) - rect_sel->sx);
          rect_sel->h = abs(RINT(coords->y) - rect_sel->sy);
        }

      if (rect_sel->fixed_mode == GIMP_RECT_SELECT_MODE_FIXED_RATIO)
        {
          ratio = rect_sel->fixed_height / rect_sel->fixed_width;
          if ( ( (gdouble) rect_sel->h) / ( (gdouble) rect_sel->w ) > ratio)
            {
              rect_sel->w = RINT(rect_sel->h / ratio);
            }
          else
            {
              rect_sel->h = RINT(rect_sel->w * ratio);
            }
        }


      /* If the shift key is down, then make the rectangle square (or
       * ellipse circular)
       */
      if ((state & GDK_SHIFT_MASK) &&
          rect_sel->fixed_mode == GIMP_RECT_SELECT_MODE_FREE)
        {
          if (rect_sel->w > rect_sel->h)
            {
              rect_sel->h = rect_sel->w;
            }
          else
            {
              rect_sel->w = rect_sel->h;
            }
        }

      if (state & GDK_CONTROL_MASK)
        {
          /*  If the control key is down, create the selection from the center out  */
          if (rect_sel->fixed_mode == GIMP_RECT_SELECT_MODE_FIXED_SIZE)
            {
              /*  Fixed size selection is simply centered over start point  */
              rect_sel->x = RINT(rect_sel->sx - rect_sel->w / 2.0);
              rect_sel->y = RINT(rect_sel->sy - rect_sel->h / 2.0);
            }
          else
            {
              /*  Non-fixed size is mirrored over starting point  */
              rect_sel->x = rect_sel->sx - rect_sel->w;
              rect_sel->y = rect_sel->sy - rect_sel->h;
              rect_sel->w = rect_sel->w * 2;
              rect_sel->h = rect_sel->h * 2;
            }
        }
      else
        {
          /*  Make (rect->x, rect->y) upper left hand point of selection  */
          if (coords->x > rect_sel->sx)
            {
              rect_sel->x = rect_sel->sx;
            }
          else
            {
              rect_sel->x = rect_sel->sx - rect_sel->w;
            }
          if (coords->y > rect_sel->sy)
            {
              rect_sel->y = rect_sel->sy;
            }
          else
            {
              rect_sel->y = rect_sel->sy - rect_sel->h;
            }
        }
    }

  rect_sel->lx = RINT(coords->x);
  rect_sel->ly = RINT(coords->y);

  gimp_rect_select_tool_update_options (rect_sel, display);

  gimp_tool_pop_status (tool, display);
  gimp_tool_push_status_coords (tool, display,
                                _("Selection: "),
                                rect_sel->w, " × ", rect_sel->h);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_rect_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpRectSelectTool *rect_sel = GIMP_RECT_SELECT_TOOL (draw_tool);

  gimp_draw_tool_draw_rectangle (draw_tool,
                                 FALSE,
                                 rect_sel->x, rect_sel->y,
                                 rect_sel->w, rect_sel->h,
                                 FALSE);
}

static void
gimp_rect_select_tool_real_rect_select (GimpRectSelectTool *rect_tool,
                                        gint                x,
                                        gint                y,
                                        gint                w,
                                        gint                h)
{
  GimpTool             *tool     = GIMP_TOOL (rect_tool);
  GimpSelectionTool    *sel_tool = GIMP_SELECTION_TOOL (rect_tool);
  GimpSelectionOptions *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_channel_select_rectangle (gimp_image_get_mask (tool->display->image),
                                 x, y, w, h,
                                 sel_tool->op,
                                 options->feather,
                                 options->feather_radius,
                                 options->feather_radius);
}

void
gimp_rect_select_tool_rect_select (GimpRectSelectTool *rect_tool,
                                   gint                x,
                                   gint                y,
                                   gint                w,
                                   gint                h)
{
  GimpTool             *tool;
  GimpSelectionOptions *options;

  g_return_if_fail (GIMP_IS_RECT_SELECT_TOOL (rect_tool));

  tool    = GIMP_TOOL (rect_tool);
  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if (options->auto_shrink)
    {
      gint off_x = 0;
      gint off_y = 0;
      gint x2, y2;

      if (! gimp_rectangle_intersect (x, y, w, h,
                                      0, 0,
                                      tool->display->image->width,
                                      tool->display->image->height,
                                      &x, &y, &w, &h))
        {
          return;
        }

      if (! options->shrink_merged)
        {
          GimpItem *item;
          gint      width, height;

          item = GIMP_ITEM (gimp_image_active_drawable (tool->display->image));

          gimp_item_offsets (item, &off_x, &off_y);
          width  = gimp_item_width  (item);
          height = gimp_item_height (item);

          if (! gimp_rectangle_intersect (x, y, w, h,
                                          off_x, off_y, width, height,
                                          &x, &y, &w, &h))
            {
              return;
            }

          x -= off_x;
          y -= off_y;
        }

      if (gimp_image_crop_auto_shrink (tool->display->image,
                                       x, y,
                                       x + w, y + h,
                                       ! options->shrink_merged,
                                       &x, &y,
                                       &x2, &y2))
        {
          w = x2 - x;
          h = y2 - y;
        }

      x += off_x;
      y += off_y;
    }

  GIMP_RECT_SELECT_TOOL_GET_CLASS (rect_tool)->rect_select (rect_tool,
                                                            x, y, w, h);
}

static void
gimp_rect_select_tool_update_options (GimpRectSelectTool *rect_sel,
                                      GimpDisplay        *display)
{
  GimpDisplayShell *shell;
  gdouble           width;
  gdouble           height;

  if (rect_sel->fixed_mode != GIMP_RECT_SELECT_MODE_FREE)
    return;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      width  = rect_sel->w;
      height = rect_sel->h;
    }
  else
    {
      GimpImage *image = display->image;

      width  = (rect_sel->w *
                _gimp_unit_get_factor (image->gimp,
                                       shell->unit) / image->xresolution);
      height = (rect_sel->h *
                _gimp_unit_get_factor (image->gimp,
                                       shell->unit) / image->yresolution);
    }

  g_object_set (GIMP_TOOL (rect_sel)->tool_info->tool_options,
                "fixed-width",  width,
                "fixed-height", height,
                "fixed-unit",   shell->unit,
                NULL);
}
