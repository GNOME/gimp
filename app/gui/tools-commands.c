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

#include "tools/tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "app_procs.h"
#include "gdisplay.h"


#define return_if_no_display(gdisp) \
        gdisp = gdisplay_active (); \
        if (!gdisp) return


void
tools_default_colors_cmd_callback (GtkWidget *widget,
				   gpointer   data)
{
  gimp_context_set_default_colors (gimp_get_user_context (the_gimp));
}

void
tools_swap_colors_cmd_callback (GtkWidget *widget,
				gpointer   data)
{
  gimp_context_swap_colors (gimp_get_user_context (the_gimp));
}

void
tools_swap_contexts_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  static GimpContext *swap_context = NULL;
  static GimpContext *temp_context = NULL;

  if (! swap_context)
    {
      swap_context = gimp_create_context (the_gimp,
					  "Swap Context",
					  gimp_get_user_context (the_gimp));
      temp_context = gimp_create_context (the_gimp,
					  "Temp Context",
					  NULL);
    }

  gimp_context_copy_args (gimp_get_user_context (the_gimp),
			  temp_context,
			  GIMP_CONTEXT_ALL_ARGS_MASK);
  gimp_context_copy_args (swap_context,
			  gimp_get_user_context (the_gimp),
			  GIMP_CONTEXT_ALL_ARGS_MASK);
  gimp_context_copy_args (temp_context,
			  swap_context,
			  GIMP_CONTEXT_ALL_ARGS_MASK);
}

void
tools_select_cmd_callback (GtkWidget *widget,
			   gpointer   data,
			   guint      action)
{
  GtkType       tool_type;
  GimpToolInfo *tool_info;
  GimpTool     *active_tool;
  GDisplay     *gdisp;

  tool_type = (GtkType) action;

  tool_info = tool_manager_get_info_by_type (the_gimp, tool_type);
  gdisp     = gdisplay_active ();

  gimp_context_set_tool (gimp_get_user_context (the_gimp), tool_info);

#ifdef __GNUC__
#warning FIXME (let the tool manager to this stuff)
#endif

  active_tool = tool_manager_get_active (the_gimp);

  /*  Paranoia  */
  active_tool->drawable = NULL;

  /*  Complete the initialisation by doing the same stuff
   *  tools_initialize() does after it did what tools_select() does
   */
  if (GIMP_TOOL_GET_CLASS (active_tool)->initialize)
    {
      gimp_tool_initialize (active_tool, gdisp);

      active_tool->drawable = gimp_image_active_drawable (gdisp->gimage);
    }

  /*  setting the tool->gdisp here is a HACK to allow the tools'
   *  dialog windows being hidden if the tool was selected from
   *  a tear-off-menu and there was no mouse click in the display
   *  before deleting it
   */
  active_tool->gdisp = gdisp;
}
