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

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"


void
tools_default_colors_cmd_callback (GtkWidget *widget,
				   gpointer   data)
{
  gimp_context_set_default_colors (gimp_get_user_context (GIMP (data)));
}

void
tools_swap_colors_cmd_callback (GtkWidget *widget,
				gpointer   data)
{
  gimp_context_swap_colors (gimp_get_user_context (GIMP (data)));
}

void
tools_swap_contexts_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  static GimpContext *swap_context = NULL;
  static GimpContext *temp_context = NULL;

  Gimp *gimp;

  gimp = GIMP (data);

  if (! swap_context)
    {
      swap_context = gimp_create_context (gimp,
					  "Swap Context",
					  gimp_get_user_context (gimp));
      temp_context = gimp_create_context (gimp,
					  "Temp Context",
					  NULL);
    }

  gimp_context_copy_properties (gimp_get_user_context (gimp),
				temp_context,
				GIMP_CONTEXT_ALL_PROPS_MASK);
  gimp_context_copy_properties (swap_context,
				gimp_get_user_context (gimp),
				GIMP_CONTEXT_ALL_PROPS_MASK);
  gimp_context_copy_properties (temp_context,
				swap_context,
				GIMP_CONTEXT_ALL_PROPS_MASK);
}

void
tools_select_cmd_callback (GtkWidget *widget,
			   gpointer   data,
			   guint      action)
{
  GimpToolInfo *tool_info;
  GimpContext  *context;
  GimpDisplay  *gdisp;

  tool_info = GIMP_TOOL_INFO (data);

  context = gimp_get_user_context (tool_info->gimp);

  /*  always allocate a new tool when selected from the image menu
   */
  if (gimp_context_get_tool (context) != tool_info)
    {
      gimp_context_set_tool (context, tool_info);
    }
  else
    {
      gimp_context_tool_changed (context);
    }

  gdisp = gimp_context_get_display (context);

  tool_manager_initialize_active (tool_info->gimp, gdisp);

  /*  setting active_tool->gdisp here is a HACK to allow the tools'
   *  dialog windows being hidden if the tool was selected from
   *  a tear-off-menu and there was no mouse click in the display
   *  before deleting it
   */
  tool_manager_get_active (tool_info->gimp)->gdisp = gdisp;
}
