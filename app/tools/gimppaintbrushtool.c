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
#include "paint/gimppaintoptions.h"

#include "gimppaintbrushtool.h"
#include "paint_options.h"

#include "gimp-intl.h"


static void   gimp_paintbrush_tool_class_init (GimpPaintbrushToolClass *klass);
static void   gimp_paintbrush_tool_init       (GimpPaintbrushTool      *tool);


static GimpPaintToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_paintbrush_tool_register (GimpToolRegisterCallback  callback,
                               gpointer                  data)
{
  (* callback) (GIMP_TYPE_PAINTBRUSH_TOOL,
                GIMP_TYPE_PAINT_OPTIONS,
                gimp_paint_options_gui,
                TRUE,
                "gimp-paintbrush-tool",
                _("Paintbrush"),
                _("Paint fuzzy brush strokes"),
                N_("/Tools/Paint Tools/Paintbrush"), "P",
                NULL, "tools/paintbrush.html",
                GIMP_STOCK_TOOL_PAINTBRUSH,
                data);
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

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_PAINTBRUSH_TOOL_CURSOR);

  paint_tool->pick_colors = TRUE;
  paint_tool->core        = g_object_new (GIMP_TYPE_PAINTBRUSH, NULL);
}
