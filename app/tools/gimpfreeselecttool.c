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
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpfreeselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define DEFAULT_MAX_INC 1024


static void   gimp_free_select_tool_class_init (GimpFreeSelectToolClass *klass);
static void   gimp_free_select_tool_init       (GimpFreeSelectTool      *free_select);
static void   gimp_free_select_tool_finalize       (GObject         *object);

static void   gimp_free_select_tool_button_press   (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *gdisp);
static void   gimp_free_select_tool_button_release (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *gdisp);
static void   gimp_free_select_tool_motion         (GimpTool        *tool,
                                                    GimpCoords      *coords,
                                                    guint32          time,
                                                    GdkModifierType  state,
                                                    GimpDisplay     *gdisp);

static void   gimp_free_select_tool_draw           (GimpDrawTool    *draw_tool);

static void   gimp_free_select_tool_add_point      (GimpFreeSelectTool *free_sel,
                                                    gdouble             x,
                                                    gdouble             y);
static void   gimp_free_select_tool_move_points    (GimpFreeSelectTool *free_sel,
                                                    gdouble             dx,
                                                    gdouble             dy);


static GimpSelectionToolClass *parent_class = NULL;


/*  public functions  */

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

GType
gimp_free_select_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpFreeSelectToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_free_select_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpFreeSelectTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_free_select_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_SELECTION_TOOL,
                                          "GimpFreeSelectTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

/*  private functions  */

static void
gimp_free_select_tool_class_init (GimpFreeSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_free_select_tool_finalize;

  tool_class->button_press   = gimp_free_select_tool_button_press;
  tool_class->button_release = gimp_free_select_tool_button_release;
  tool_class->motion         = gimp_free_select_tool_motion;

  draw_tool_class->draw      = gimp_free_select_tool_draw;
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
gimp_free_select_tool_button_press (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  GimpFreeSelectTool *free_sel = GIMP_FREE_SELECT_TOOL (tool);

  gimp_tool_control_activate (tool->control);
  tool->gdisp = gdisp;

  if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (free_sel), coords))
    return;

  free_sel->last_coords = *coords;
  free_sel->num_points  = 0;

  gimp_free_select_tool_add_point (free_sel, coords->x, coords->y);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
}

static void
gimp_free_select_tool_button_release (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      guint32          time,
                                      GdkModifierType  state,
                                      GimpDisplay     *gdisp)
{
  GimpFreeSelectTool   *free_sel = GIMP_FREE_SELECT_TOOL (tool);
  GimpSelectionOptions *options;

  options  = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  /*  First take care of the case where the user "cancels" the action  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      if (free_sel->num_points == 1)
        {
          /*  If there is a floating selection, anchor it  */
          if (gimp_image_floating_sel (gdisp->gimage))
            floating_sel_anchor (gimp_image_floating_sel (gdisp->gimage));
          /*  Otherwise, clear the selection mask  */
          else
            gimp_channel_clear (gimp_image_get_mask (gdisp->gimage), NULL,
                                TRUE);

          gimp_image_flush (gdisp->gimage);
          return;
        }

      gimp_channel_select_polygon (gimp_image_get_mask (gdisp->gimage),
                                   tool->tool_info->blurb,
                                   free_sel->num_points,
                                   free_sel->points,
                                   GIMP_SELECTION_TOOL (tool)->op,
                                   options->antialias,
                                   options->feather,
                                   options->feather_radius,
                                   options->feather_radius);

      gimp_image_flush (gdisp->gimage);
    }
}

static void
gimp_free_select_tool_motion (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpFreeSelectTool *free_sel = GIMP_FREE_SELECT_TOOL (tool);
  GimpSelectionTool  *sel_tool = GIMP_SELECTION_TOOL (tool);

  if (sel_tool->op == SELECTION_ANCHOR)
    {
      sel_tool->op = SELECTION_REPLACE;

      gimp_tool_cursor_update (tool, coords, state, gdisp);
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
  gint                i;

  for (i = 1; i < free_sel->num_points; i++)
    {
      gimp_draw_tool_draw_line (draw_tool,
                                free_sel->points[i - 1].x,
                                free_sel->points[i - 1].y,
                                free_sel->points[i].x,
                                free_sel->points[i].y,
                                FALSE);
    }
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
