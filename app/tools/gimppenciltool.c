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
gimp_pencil_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_PENCIL_TOOL,
                paint_options_new,
                TRUE,
                "gimp-pencil-tool",
                _("Pencil"),
                _("Paint hard edged pixels"),
                N_("/Tools/Paint Tools/Pencil"), "N",
                NULL, "tools/pencil.html",
                GIMP_STOCK_TOOL_PENCIL,
                data);
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

  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_PENCIL_TOOL_CURSOR);

  paint_tool->pick_colors = TRUE;
  paint_tool->core        = g_object_new (GIMP_TYPE_PENCIL, NULL);
}
