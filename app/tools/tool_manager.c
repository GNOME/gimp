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

#include "apptypes.h"

#include "appenv.h"
#include "context_manager.h"
#include "gdisplay.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimpimage.h"
#include "gimpui.h"
#include "tool.h"
#include "tool_options.h"
#include "dialog_handler.h"

#include "libgimp/gimpintl.h"


/*  Global Data  */
GimpTool * active_tool = NULL;
GSList   * registered_tools = NULL;
/*  Local  Data  */


/*  Function definitions  */

static void
active_tool_unref (void)
{
  if (!active_tool)
    return;

  gimp_tool_hide_options (active_tool);

  gtk_object_unref (GTK_OBJECT(active_tool));

  active_tool = NULL;
}

void
tool_manager_select (GimpTool *tool)
{
  if (active_tool)
    active_tool_unref ();

  active_tool = tool;

  tool_options_show (tool);
}


void
tool_manager_control_active (ToolAction  action,
		     GDisplay   *gdisp)
{
  if (active_tool)
    gimp_tool_control (active_tool, action, gdisp);
}

/*  standard member functions  */


#warning bogosity alert
#if 0
void
tools_register (ToolType     tool_type,
		ToolOptions *tool_options)
{
  g_return_if_fail (tool_options != NULL);

  tool_info [(gint) tool_type].tool_options = tool_options;

  /*  need to check whether the widget is visible...this can happen
   *  because some tools share options such as the transformation tools
   */
  if (! GTK_WIDGET_VISIBLE (tool_options->main_vbox))
    {
      gtk_box_pack_start (GTK_BOX (options_vbox), tool_options->main_vbox,
			  TRUE, TRUE, 0);
      gtk_widget_show (tool_options->main_vbox);
    }

  gtk_label_set_text (GTK_LABEL (options_label), tool_options->title);

  gtk_pixmap_set (GTK_PIXMAP (options_pixmap),
		  tool_get_pixmap (tool_type), tool_get_mask (tool_type));

  gtk_widget_queue_draw (options_pixmap);

  gimp_help_set_help_data (options_eventbox,
			   gettext (tool_info[(gint) tool_type].tool_desc),
			   tool_info[(gint) tool_type].private_tip);
}
#endif
/*  Tool options function  */


gchar *
tool_manager_get_active_PDB_string (void)
{
  gchar *toolStr = "gimp_paintbrush_default";

  /*  Return the correct PDB function for the active tool
   *  The default is paintbrush if the tool is not recognised
   */

  if (!active_tool)
    return toolStr;

  return gimp_tool_get_PDB_string(active_tool);
}

gchar *      
tool_manager_active_get_help_data            (void)
{
  g_return_val_if_fail(active_tool, NULL);

  return gimp_tool_get_help_data (active_tool);
}

void tool_manager_register (GimpToolClass *tool_type)
{
	GtkObjectClass *objclass = GTK_OBJECT_CLASS(tool_type);
	g_message("registering %s.", gtk_type_name(objclass->type));
	
	registered_tools = g_slist_append (registered_tools, tool_type);
	active_tool = gtk_type_new(objclass->type);
}
