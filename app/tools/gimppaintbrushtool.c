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

#include "paint/gimppaintbrush.h"

#include "gimppaintbrushtool.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


#define PAINT_LEFT_THRESHOLD           0.05
#define PAINTBRUSH_DEFAULT_INCREMENTAL FALSE


static void   gimp_paintbrush_tool_class_init (GimpPaintbrushToolClass *klass);
static void   gimp_paintbrush_tool_init       (GimpPaintbrushTool      *tool);


static GimpPaintToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_paintbrush_tool_register (Gimp                     *gimp,
                               GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_PAINTBRUSH_TOOL,
                paint_options_new,
                TRUE,
                "gimp:paintbrush_tool",
                _("Paintbrush"),
                _("Paint fuzzy brush strokes"),
                N_("/Tools/Paint Tools/Paintbrush"), "P",
                NULL, "tools/paintbrush.html",
                GIMP_STOCK_TOOL_PAINTBRUSH);
}

GType
gimp_paintbrush_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpPaintbrushToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_paintbrush_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPaintbrushTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_paintbrush_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_PAINT_TOOL,
					  "GimpPaintbrushTool",
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_paintbrush_tool_class_init (GimpPaintbrushToolClass *klass)
{
  GimpPaintToolClass *paint_tool_class;

  paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
}

static void
gimp_paintbrush_tool_init (GimpPaintbrushTool *paintbrush)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (paintbrush);
  paint_tool = GIMP_PAINT_TOOL (paintbrush);

  tool->tool_cursor       = GIMP_PAINTBRUSH_TOOL_CURSOR;

  paint_tool->pick_colors = TRUE;
  paint_tool->core        = g_object_new (GIMP_TYPE_PAINTBRUSH, NULL);
}


#if 0

static GimpPaintbrushTool *non_gui_paintbrush = NULL;

gboolean
gimp_paintbrush_tool_non_gui_default (GimpDrawable *drawable,
				      gint          num_strokes,
				      gdouble      *stroke_array)
{
  GimpPaintTool *paint_tool;
  gint           i;

  if (! non_gui_paintbrush)
    {
      non_gui_paintbrush = g_object_new (GIMP_TYPE_PAINTBRUSH_TOOL, NULL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_paintbrush);

  /* Hmmm... PDB paintbrush should have gradient type added to it!
   * thats why the code below is duplicated.
   */
  if (gimp_paint_tool_start (paint_tool, drawable,
			     stroke_array[0],
			     stroke_array[1]))
    {
      paint_tool->start_coords.x = paint_tool->last_coords.x = stroke_array[0];
      paint_tool->start_coords.y = paint_tool->last_coords.y = stroke_array[1];

      gimp_paint_tool_paint (paint_tool, drawable, MOTION_PAINT);

      for (i = 1; i < num_strokes; i++)
	{
	  paint_tool->cur_coords.x = stroke_array[i * 2 + 0];
	  paint_tool->cur_coords.y = stroke_array[i * 2 + 1];

	  gimp_paint_tool_interpolate (paint_tool, drawable);

	  paint_tool->last_coords.x = paint_tool->cur_coords.x;
	  paint_tool->last_coords.y = paint_tool->cur_coords.y;
	}

      gimp_paint_tool_finish (paint_tool, drawable);

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_paintbrush_tool_non_gui (GimpDrawable *drawable,
			      gint          num_strokes,
			      gdouble      *stroke_array,
			      gdouble       fade_out,
			      gint          method,
			      gdouble       gradient_length)
{
  GimpPaintTool *paint_tool;
  gint           i;

  if (! non_gui_paintbrush)
    {
      non_gui_paintbrush = g_object_new (GIMP_TYPE_PAINTBRUSH_TOOL, NULL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_paintbrush);

  /* Code duplicated above */
  if (gimp_paint_tool_start (paint_tool, drawable,
			     stroke_array[0],
			     stroke_array[1]))
    {
      non_gui_gradient_options.fade_out        = fade_out;
      non_gui_gradient_options.gradient_length = gradient_length;
      non_gui_gradient_options.gradient_type   = LOOP_TRIANGLE;
      non_gui_incremental                      = method;

      paint_tool->start_coords.x = paint_tool->last_coords.x = stroke_array[0];
      paint_tool->start_coords.y = paint_tool->last_coords.y = stroke_array[1];

      gimp_paint_tool_paint (paint_tool, drawable, MOTION_PAINT);

      for (i = 1; i < num_strokes; i++)
       {
         paint_tool->cur_coords.x = stroke_array[i * 2 + 0];
         paint_tool->cur_coords.y = stroke_array[i * 2 + 1];

         gimp_paint_tool_interpolate (paint_tool, drawable);

	 paint_tool->last_coords.x = paint_tool->cur_coords.x;
         paint_tool->last_coords.y = paint_tool->cur_coords.y;
       }

      gimp_paint_tool_finish (paint_tool, drawable);

      return TRUE;
    }

  return FALSE;
}

#endif
