/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "gimpcoloroptions.h"
#include "gimpcolortool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void 	  gimp_color_tool_class_init  (GimpColorToolClass *klass);
static void       gimp_color_tool_init        (GimpColorTool      *color_tool);

static void       gimp_color_tool_button_press   (GimpTool        *tool,
						  GimpCoords      *coords,
						  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void       gimp_color_tool_button_release (GimpTool        *tool,
						  GimpCoords      *coords,
						  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void       gimp_color_tool_motion         (GimpTool        *tool,
						  GimpCoords      *coords,
						  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void       gimp_color_tool_cursor_update  (GimpTool        *tool,
						  GimpCoords      *coords,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);

static void       gimp_color_tool_draw           (GimpDrawTool    *draw_tool);



static GimpDrawToolClass *parent_class = NULL;


GtkType
gimp_color_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpColorToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpColorTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_color_tool_class_init (GimpColorToolClass *klass)
{
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_class;

  tool_class = GIMP_TOOL_CLASS (klass);
  draw_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->button_press   = gimp_color_tool_button_press;
  tool_class->button_release = gimp_color_tool_button_release;
  tool_class->motion         = gimp_color_tool_motion;
  tool_class->cursor_update  = gimp_color_tool_cursor_update;

  draw_class->draw	     = gimp_color_tool_draw;

  klass->pick                = NULL;
}

static void
gimp_color_tool_init (GimpColorTool *color_tool)
{
  GimpTool *tool = GIMP_TOOL (color_tool);

  gimp_tool_control_set_tool_cursor (tool->control,
				     GIMP_COLOR_PICKER_TOOL_CURSOR);
}

static void
gimp_color_tool_button_press (GimpTool        *tool,
			      GimpCoords      *coords,
			      guint32          time,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  GimpColorTool    *cp_tool;
  GimpColorOptions *options;
  gint              off_x, off_y;

  cp_tool = GIMP_COLOR_TOOL (tool);
  options = GIMP_COLOR_OPTIONS (tool->tool_info->tool_options);

  /*  Make the tool active and set it's gdisplay & drawable  */
  tool->gdisp    = gdisp;
  tool->drawable = gimp_image_active_drawable (gdisp->gimage);
  gimp_tool_control_activate (tool->control);

  if (GIMP_COLOR_TOOL_GET_CLASS (tool)->pick)
    {
      /*  Keep the coordinates of the target  */
      gimp_item_offsets (GIMP_ITEM (tool->drawable), &off_x, &off_y);

      cp_tool->centerx = coords->x - off_x;
      cp_tool->centery = coords->y - off_y;

      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
    }
}

static void
gimp_color_tool_button_release (GimpTool        *tool,
				GimpCoords      *coords,
				guint32          time,
				GdkModifierType  state,
				GimpDisplay     *gdisp)
{
  if (GIMP_COLOR_TOOL_GET_CLASS (tool)->pick)
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);
}

static void
gimp_color_tool_motion (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
		               GdkModifierType  state,
		               GimpDisplay     *gdisp)
{
  GimpColorTool    *cp_tool;
  GimpColorOptions *options;
  gint              off_x, off_y;

  if (! GIMP_COLOR_TOOL_GET_CLASS (tool)->pick)
    return;

  cp_tool = GIMP_COLOR_TOOL (tool);
  options = GIMP_COLOR_OPTIONS (tool->tool_info->tool_options);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_item_offsets (GIMP_ITEM (tool->drawable), &off_x, &off_y);

  cp_tool->centerx = coords->x - off_x;
  cp_tool->centery = coords->y - off_y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_color_tool_cursor_update (GimpTool        *tool,
			       GimpCoords      *coords,
			       GdkModifierType  state,
			       GimpDisplay     *gdisp)
{
  if (! GIMP_COLOR_TOOL_GET_CLASS (tool)->pick)
    return;

  if (gdisp->gimage &&
      coords->x > 0 &&
      coords->x < gdisp->gimage->width &&
      coords->y > 0 &&
      coords->y < gdisp->gimage->height)
    {
      gimp_tool_control_set_cursor (tool->control, GIMP_COLOR_PICKER_CURSOR);
    }
  else
    {
      gimp_tool_control_set_cursor (tool->control, GIMP_BAD_CURSOR);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_color_tool_draw (GimpDrawTool *draw_tool)
{
  GimpColorTool    *cp_tool;
  GimpColorOptions *options;
  GimpTool         *tool;

  if (! GIMP_COLOR_TOOL_GET_CLASS (draw_tool)->pick)
    return;

  cp_tool = GIMP_COLOR_TOOL (draw_tool);
  tool    = GIMP_TOOL (draw_tool);
  options = GIMP_COLOR_OPTIONS (tool->tool_info->tool_options);

  if (options->sample_average)
    gimp_draw_tool_draw_rectangle (draw_tool,
				   FALSE,
				   cp_tool->centerx - options->average_radius,
				   cp_tool->centery - options->average_radius,
				   2 * options->average_radius + 1,
				   2 * options->average_radius + 1,
				   TRUE);
}

gboolean
gimp_color_tool_pick (GimpColorTool    *tool,
		      GimpColorOptions *options,
		      GimpDisplay      *gdisp,
		      gint              x,
		      gint              y)
{
  GimpColorToolClass *klass;

  g_return_val_if_fail (GIMP_IS_COLOR_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_COLOR_OPTIONS (options), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), FALSE);

  klass = GIMP_COLOR_TOOL_GET_CLASS (tool);

  if (! klass->pick)
    return FALSE;

  return klass->pick (tool, options, gdisp, x, y);
}
