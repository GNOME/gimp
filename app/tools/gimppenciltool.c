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

#include "paint/gimppencil.h"

#include "gimppenciltool.h"
#include "paint_options.h"

#include "libgimp/gimpintl.h"


static void   gimp_pencil_tool_class_init (GimpPencilToolClass *klass);
static void   gimp_pencil_tool_init       (GimpPencilTool      *pancil);


static GimpPaintToolClass *parent_class = NULL;


/*  functions  */

void
gimp_pencil_tool_register (Gimp                     *gimp,
                           GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_PENCIL_TOOL,
                paint_options_new,
                TRUE,
                "gimp:pencil_tool",
                _("Pencil"),
                _("Paint hard edged pixels"),
                N_("/Tools/Paint Tools/Pencil"), "P",
                NULL, "tools/pencil.html",
                GIMP_STOCK_TOOL_PENCIL);
}

GType
gimp_pencil_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpPencilToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_pencil_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPencilTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_pencil_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_PAINT_TOOL,
					  "GimpPencilTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void 
gimp_pencil_tool_class_init (GimpPencilToolClass *klass)
{  
  GimpPaintToolClass *paint_tool_class;

  paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
}

static void
gimp_pencil_tool_init (GimpPencilTool *pencil)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (pencil);
  paint_tool = GIMP_PAINT_TOOL (pencil);

  tool->tool_cursor       = GIMP_PENCIL_TOOL_CURSOR;

  paint_tool->pick_colors = TRUE;
  paint_tool->core        = g_object_new (GIMP_TYPE_PENCIL, NULL);
}

#if 0

/*  non-gui stuff  */

gboolean
pencil_non_gui (GimpDrawable *drawable,
		gint          num_strokes,
		gdouble      *stroke_array)
{
  static GimpPencil *non_gui_pencil = NULL;

  GimpPaintCore *core;
  gint           i;
  
  if (! non_gui_pencil)
    {
      non_gui_pencil = g_object_new (GIMP_TYPE_PENCIL, NULL);
    }

  core = GIMP_PAINT_CORE (non_gui_pencil);

  if (gimp_paint_core_start (core,
                             drawable,
                             stroke_array[0],
                             stroke_array[1]))
    {
      core->start_coords.x = core->last_coords.x = stroke_array[0];
      core->start_coords.y = core->last_coords.y = stroke_array[1];

      gimp_paint_core_paint (core, drawable, NULL, MOTION_PAINT);

      for (i = 1; i < num_strokes; i++)
	{
	  core->cur_coords.x = stroke_array[i * 2 + 0];
	  core->cur_coords.y = stroke_array[i * 2 + 1];

	  gimp_paint_core_interpolate (core, drawable);

	  core->last_coords.x = core->cur_coords.x;
	  core->last_coords.y = core->cur_coords.y;
	}

      gimp_paint_core_finish (core, drawable);

      return TRUE;
    }

  return FALSE;
}

#endif
