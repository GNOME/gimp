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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimplayer-floating-sel.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpfreeselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define DEFAULT_MAX_INC 1024


static void   gimp_free_select_tool_finalize       (GObject         *object);

static void   gimp_free_select_tool_control        (GimpTool        *tool,
                                                    GimpToolAction   action,
                                                    GimpDisplay     *display);
static void   gimp_free_select_tool_button_press   (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *display);
static void   gimp_free_select_tool_button_release (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *display);
static void   gimp_free_select_tool_motion         (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *display);

static void   gimp_free_select_tool_draw           (GimpDrawTool    *draw_tool);

static void   gimp_free_select_tool_real_select    (GimpFreeSelectTool *free_sel,
                                                    GimpDisplay        *display);

static void   gimp_free_select_tool_add_point      (GimpFreeSelectTool *free_sel,
                                                    gdouble             x,
                                                    gdouble             y);
static void   gimp_free_select_tool_move_points    (GimpFreeSelectTool *free_sel,
                                                    gdouble             dx,
                                                    gdouble             dy);


G_DEFINE_TYPE (GimpFreeSelectTool, gimp_free_select_tool,
               GIMP_TYPE_SELECTION_TOOL);

#define parent_class gimp_free_select_tool_parent_class


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
                _("Select hand-drawn regions"),
                N_("_Free Select"), "F",
                NULL, GIMP_HELP_TOOL_FREE_SELECT,
                GIMP_STOCK_TOOL_FREE_SELECT,
                data);
}

static void
gimp_free_select_tool_class_init (GimpFreeSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = gimp_free_select_tool_finalize;

  tool_class->control        = gimp_free_select_tool_control;
  tool_class->button_press   = gimp_free_select_tool_button_press;
  tool_class->button_release = gimp_free_select_tool_button_release;
  tool_class->motion         = gimp_free_select_tool_motion;

  draw_tool_class->draw      = gimp_free_select_tool_draw;

  klass->select              = gimp_free_select_tool_real_select;
}

static void
gimp_free_select_tool_init (GimpFreeSelectTool *free_select)
{
  GimpTool *tool = GIMP_TOOL (free_select);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_FREE_SELECT);

  free_select->points     = NULL;
  free_select->num_points = 0;
  free_select->max_segs   = 0;
}

static void
gimp_free_select_tool_finalize (GObject *object)
{
  GimpFreeSelectTool *free_sel = GIMP_FREE_SELECT_TOOL (object);

  if (free_sel->points)
    {
      g_free (free_sel->points);
      free_sel->points = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_free_select_tool_control (GimpTool       *tool,
                               GimpToolAction  action,
                               GimpDisplay    *display)
{
  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      GIMP_FREE_SELECT_TOOL (tool)->num_points = 0;
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_free_select_tool_button_press (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpFreeSelectTool *free_sel = GIMP_FREE_SELECT_TOOL (tool);

  gimp_tool_control_activate (tool->control);
  tool->display = display;

  if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (free_sel), coords))
    return;

  free_sel->last_coords = *coords;
  free_sel->num_points  = 0;

  gimp_free_select_tool_add_point (free_sel, coords->x, coords->y);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_free_select_tool_button_release (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      guint32          time,
                                      GdkModifierType  state,
                                      GimpDisplay     *display)
{
  GimpFreeSelectTool *free_sel = GIMP_FREE_SELECT_TOOL (tool);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  /*  First take care of the case where the user "cancels" the action  */
  if (state & GDK_BUTTON3_MASK)
    return;

  if (free_sel->num_points == 1)
    {
      /*  If there is a floating selection, anchor it  */
      if (gimp_image_floating_sel (display->image))
        {
          floating_sel_anchor (gimp_image_floating_sel (display->image));
        }
      /*  Otherwise, clear the selection mask  */
      else
        {
          gimp_channel_clear (gimp_image_get_mask (display->image), NULL, TRUE);
        }
    }
  else
    {
      GIMP_FREE_SELECT_TOOL_GET_CLASS (free_sel)->select (free_sel, display);
    }

  gimp_image_flush (display->image);
}

static void
gimp_free_select_tool_motion (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpFreeSelectTool *free_sel = GIMP_FREE_SELECT_TOOL (tool);
  GimpSelectionTool  *sel_tool = GIMP_SELECTION_TOOL (tool);

  if (sel_tool->op == SELECTION_ANCHOR)
    {
      sel_tool->op = SELECTION_REPLACE;

      gimp_tool_cursor_update (tool, coords, state, display);
    }

  if (state & GDK_MOD1_MASK)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      gimp_free_select_tool_move_points (free_sel,
                                         coords->x - free_sel->last_coords.x,
                                         coords->y - free_sel->last_coords.y);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
  else
    {
      gimp_free_select_tool_add_point (free_sel, coords->x, coords->y);

      gimp_draw_tool_draw_line (GIMP_DRAW_TOOL (tool),
                                free_sel->points[free_sel->num_points - 2].x,
                                free_sel->points[free_sel->num_points - 2].y,
                                free_sel->points[free_sel->num_points - 1].x,
                                free_sel->points[free_sel->num_points - 1].y,
                                FALSE);
    }

  free_sel->last_coords = *coords;
}

static void
gimp_free_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpFreeSelectTool *free_sel = GIMP_FREE_SELECT_TOOL (draw_tool);

  gimp_draw_tool_draw_lines (draw_tool,
                             (const gdouble *) free_sel->points,
                             free_sel->num_points,
                             FALSE, FALSE);
}


/*  public functions  */

void
gimp_free_select_tool_select (GimpFreeSelectTool *free_sel,
                              GimpDisplay        *display)
{
  g_return_if_fail (GIMP_IS_FREE_SELECT_TOOL (free_sel));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  GIMP_FREE_SELECT_TOOL_GET_CLASS (free_sel)->select (free_sel, display);
}


/*  private functions  */

static void
gimp_free_select_tool_real_select (GimpFreeSelectTool *free_sel,
                                   GimpDisplay        *display)
{
  GimpTool             *tool    = GIMP_TOOL (free_sel);
  GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (free_sel);

  gimp_channel_select_polygon (gimp_image_get_mask (display->image),
                               Q_("command|Free Select"),
                               free_sel->num_points,
                               free_sel->points,
                               GIMP_SELECTION_TOOL (tool)->op,
                               options->antialias,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius,
                               TRUE);
}

static void
gimp_free_select_tool_add_point (GimpFreeSelectTool *free_sel,
                                 gdouble             x,
                                 gdouble             y)
{
  if (free_sel->num_points >= free_sel->max_segs)
    {
      free_sel->max_segs += DEFAULT_MAX_INC;

      free_sel->points = g_realloc (free_sel->points,
                                    sizeof (GimpVector2) * free_sel->max_segs);
    }

  free_sel->points[free_sel->num_points].x = x;
  free_sel->points[free_sel->num_points].y = y;

  free_sel->num_points++;
}

static void
gimp_free_select_tool_move_points (GimpFreeSelectTool *free_sel,
                                   gdouble             dx,
                                   gdouble             dy)
{
  gint i;

  for (i = 0; i < free_sel->num_points; i++)
    {
      free_sel->points[i].x += dx;
      free_sel->points[i].y += dy;
    }
}
