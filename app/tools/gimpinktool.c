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

#include "paint/gimpinkoptions.h"

#include "widgets/gimphelp-ids.h"

#include "gimpinkoptions-gui.h"
#include "gimpinktool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_ink_tool_init          (GimpInkTool      *tool);
static void   gimp_ink_tool_class_init    (GimpInkToolClass *klass);

static void   gimp_ink_tool_cursor_update (GimpTool         *tool,
                                           GimpCoords       *coords,
                                           GdkModifierType   state,
                                           GimpDisplay      *gdisp);


static GimpPaintToolClass *parent_class = NULL;


void
gimp_ink_tool_register (GimpToolRegisterCallback  callback,
                        gpointer                  data)
{
  (* callback) (GIMP_TYPE_INK_TOOL,
                GIMP_TYPE_INK_OPTIONS,
                gimp_ink_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK |
                GIMP_CONTEXT_BACKGROUND_MASK |
                GIMP_CONTEXT_OPACITY_MASK    |
                GIMP_CONTEXT_PAINT_MODE_MASK,
                "gimp-ink-tool",
                _("Ink"),
                _("Draw in ink"),
                N_("In_k"), "K",
                NULL, GIMP_HELP_TOOL_INK,
                GIMP_STOCK_TOOL_INK,
                data);
}

GType
gimp_ink_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpInkToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_ink_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpInkTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_ink_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_PAINT_TOOL,
                                          "GimpInkTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_ink_tool_class_init (GimpInkToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->cursor_update = gimp_ink_tool_cursor_update;
}

static void
gimp_ink_tool_init (GimpInkTool *ink_tool)
{
  GimpTool *tool = GIMP_TOOL (ink_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_INK);

  gimp_paint_tool_enable_color_picker (GIMP_PAINT_TOOL (ink_tool),
                                       GIMP_COLOR_PICK_MODE_FOREGROUND);
}

static void
gimp_ink_tool_cursor_update (GimpTool         *tool,
                             GimpCoords       *coords,
                             GdkModifierType   state,
                             GimpDisplay      *gdisp)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);

  paint_tool->show_cursor = TRUE;

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}
