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
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"


#define return_if_no_gimp(gimp,data) \
  if (GIMP_IS_DISPLAY (data)) \
    gimp = ((GimpDisplay *) data)->gimage->gimp; \
  else if (GIMP_IS_GIMP (data)) \
    gimp = data; \
  else \
    gimp = NULL; \
  if (! gimp) \
    return


void
tools_default_colors_cmd_callback (GtkWidget *widget,
				   gpointer   data,
                                   guint      action)
{
  Gimp *gimp;
  return_if_no_gimp (gimp, data);

  gimp_context_set_default_colors (gimp_get_user_context (gimp));
}

void
tools_swap_colors_cmd_callback (GtkWidget *widget,
				gpointer   data,
                                guint      action)
{
  Gimp *gimp;
  return_if_no_gimp (gimp, data);

  gimp_context_swap_colors (gimp_get_user_context (gimp));
}

void
tools_select_cmd_callback (GtkWidget *widget,
			   gpointer   data,
			   guint      action)
{
  Gimp         *gimp;
  GimpToolInfo *tool_info;
  GimpContext  *context;
  GimpDisplay  *gdisp;
  const gchar  *identifier;
  return_if_no_gimp (gimp, data);

  identifier = g_quark_to_string ((GQuark) action);

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (gimp->tool_info_list, identifier);

  context = gimp_get_user_context (gimp);

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

  if (gdisp)
    tool_manager_initialize_active (gimp, gdisp);
}
