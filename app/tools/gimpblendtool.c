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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-blend.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpblendoptions.h"
#include "gimpblendtool.h"
#include "gimptoolcontrol.h"
#include "tools-utils.h"

#include "gimp-intl.h"


#define TARGET_SIZE 15


/*  local function prototypes  */

static void    gimp_blend_tool_class_init        (GimpBlendToolClass *klass);
static void    gimp_blend_tool_init              (GimpBlendTool      *blend_tool);

static void    gimp_blend_tool_button_press      (GimpTool        *tool,
                                                  GimpCoords      *coords,
                                                  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void    gimp_blend_tool_button_release    (GimpTool        *tool,
                                                  GimpCoords      *coords,
                                                  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void    gimp_blend_tool_motion            (GimpTool        *tool,
                                                  GimpCoords      *coords,
                                                  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void    gimp_blend_tool_cursor_update     (GimpTool        *tool,
                                                  GimpCoords      *coords,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);

static void    gimp_blend_tool_draw              (GimpDrawTool    *draw_tool);


/*  private variables  */

static GimpDrawToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_blend_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_BLEND_TOOL,
                GIMP_TYPE_BLEND_OPTIONS,
                gimp_blend_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK |
                GIMP_CONTEXT_BACKGROUND_MASK |
                GIMP_CONTEXT_OPACITY_MASK    |
                GIMP_CONTEXT_PAINT_MODE_MASK |
                GIMP_CONTEXT_GRADIENT_MASK,
                "gimp-blend-tool",
                _("Blend"),
                _("Fill with a color gradient"),
                N_("Blen_d"), "L",
                NULL, GIMP_HELP_TOOL_BLEND,
                GIMP_STOCK_TOOL_BLEND,
                data);
}

GType
gimp_blend_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpBlendToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_blend_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpBlendTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_blend_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpBlendTool",
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_blend_tool_class_init (GimpBlendToolClass *klass)
{
  GObjectClass      *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  object_class    = G_OBJECT_CLASS (klass);
  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->button_press   = gimp_blend_tool_button_press;
  tool_class->button_release = gimp_blend_tool_button_release;
  tool_class->motion         = gimp_blend_tool_motion;
  tool_class->cursor_update  = gimp_blend_tool_cursor_update;

  draw_tool_class->draw      = gimp_blend_tool_draw;
}

static void
gimp_blend_tool_init (GimpBlendTool *blend_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (blend_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_BLEND);

}

static void
gimp_blend_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpBlendTool *blend_tool;
  GimpDrawable  *drawable;
  gint           off_x, off_y;

  blend_tool = GIMP_BLEND_TOOL (tool);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  switch (gimp_drawable_type (drawable))
    {
    case GIMP_INDEXED_IMAGE: case GIMP_INDEXEDA_IMAGE:
      g_message (_("Blend: Invalid for indexed images."));
      return;

      break;
    default:
      break;
    }

  gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

  blend_tool->endx = blend_tool->startx = coords->x - off_x;
  blend_tool->endy = blend_tool->starty = coords->y - off_y;

  tool->gdisp = gdisp;

  gimp_tool_control_activate (tool->control);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, _("Blend: "), 0, ", ", 0);

  /*  Start drawing the blend tool  */
  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
}

static void
gimp_blend_tool_button_release (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
                                GdkModifierType  state,
                                GimpDisplay     *gdisp)
{
  GimpBlendTool    *blend_tool;
  GimpPaintOptions *paint_options;
  GimpBlendOptions *options;
  GimpContext      *context;
  GimpImage        *gimage;

  blend_tool    = GIMP_BLEND_TOOL (tool);
  paint_options = GIMP_PAINT_OPTIONS (tool->tool_info->tool_options);
  options       = GIMP_BLEND_OPTIONS (paint_options);
  context       = GIMP_CONTEXT (options);

  gimage = gdisp->gimage;

  gimp_tool_pop_status (tool);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (state & GDK_BUTTON3_MASK) &&
      ((blend_tool->startx != blend_tool->endx) ||
       (blend_tool->starty != blend_tool->endy)))
    {
      GimpProgress *progress;

      progress = gimp_progress_start (GIMP_PROGRESS (gdisp),
                                      _("Blending..."), FALSE);

      gimp_drawable_blend (gimp_image_active_drawable (gimage),
                           context,
                           GIMP_CUSTOM_MODE,
                           gimp_context_get_paint_mode (context),
                           options->gradient_type,
                           gimp_context_get_opacity (context),
                           options->offset,
                           paint_options->gradient_options->gradient_repeat,
                           paint_options->gradient_options->gradient_reverse,
                           options->supersample,
                           options->supersample_depth,
                           options->supersample_threshold,
                           options->dither,
                           blend_tool->startx,
                           blend_tool->starty,
                           blend_tool->endx,
                           blend_tool->endy,
                           progress);

      if (progress)
        gimp_progress_end (progress);

      gimp_image_flush (gimage);
    }
}

static void
gimp_blend_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *gdisp)
{
  GimpBlendTool    *blend_tool;
  gint              off_x, off_y;

  blend_tool = GIMP_BLEND_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_item_offsets (GIMP_ITEM (gimp_image_active_drawable (gdisp->gimage)),
                     &off_x, &off_y);

  /*  Get the current coordinates  */
  blend_tool->endx = coords->x - off_x;
  blend_tool->endy = coords->y - off_y;

  /* Restrict to multiples of 15 degrees if ctrl is pressed */
  if (state & GDK_CONTROL_MASK)
    {
      gimp_tool_motion_constrain (blend_tool->startx, blend_tool->starty,
                                  &blend_tool->endx, &blend_tool->endy);
    }

  gimp_tool_pop_status (tool);

  gimp_tool_push_status_coords (tool,
                                _("Blend: "),
                                blend_tool->endx - blend_tool->startx,
                                ", ",
                                blend_tool->endy - blend_tool->starty);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_blend_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  switch (gimp_drawable_type (gimp_image_active_drawable (gdisp->gimage)))
    {
    case GIMP_INDEXED_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      gimp_tool_control_set_cursor (tool->control, GIMP_CURSOR_BAD);
      break;
    default:
      gimp_tool_control_set_cursor (tool->control, GIMP_CURSOR_MOUSE);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_blend_tool_draw (GimpDrawTool *draw_tool)
{
  GimpBlendTool *blend_tool;

  blend_tool = GIMP_BLEND_TOOL (draw_tool);

  /*  Draw start target  */
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_CROSS,
                              floor (blend_tool->startx) + 0.5,
                              floor (blend_tool->starty) + 0.5,
                              TARGET_SIZE,
                              TARGET_SIZE,
                              GTK_ANCHOR_CENTER,
                              TRUE);

  /*  Draw end target  */
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_CROSS,
                              floor (blend_tool->endx) + 0.5,
                              floor (blend_tool->endy) + 0.5,
                              TARGET_SIZE,
                              TARGET_SIZE,
                              GTK_ANCHOR_CENTER,
                              TRUE);

  /*  Draw the line between the start and end coords  */
  gimp_draw_tool_draw_line (draw_tool,
                            floor (blend_tool->startx) + 0.5,
                            floor (blend_tool->starty) + 0.5,
                            floor (blend_tool->endx) + 0.5,
                            floor (blend_tool->endy) + 0.5,
                            TRUE);
}
