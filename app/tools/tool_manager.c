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

#include "apptypes.h"

#include "appenv.h"
#include "context_manager.h"
#include "dialog_handler.h"
#include "gdisplay.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimpimage.h"
#include "gimplist.h"
#include "gimpui.h"

#include "gimptool.h"
#include "gimptoolinfo.h"
#include "tool_manager.h"
#include "tool_options.h"
#include "tool_options_dialog.h"

#include "libgimp/gimpintl.h"


/*  Global Data  */
GimpTool      *active_tool           = NULL;
GimpContainer *global_tool_info_list = NULL;


static GSList *tool_stack = NULL;


/*  Function definitions  */

static void
active_tool_unref (void)
{
  if (! active_tool)
    return;

  gtk_object_unref (GTK_OBJECT (active_tool));

  active_tool = NULL;
}

void
tool_manager_select_tool (GimpTool *tool)
{
  g_return_if_fail (tool != NULL);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  if (active_tool)
    active_tool_unref ();

  active_tool = tool;
}

void
tool_manager_push_tool (GimpTool *tool)
{
  g_return_if_fail (tool != NULL);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  if (active_tool)
    {
      gtk_object_ref (GTK_OBJECT (active_tool));

      tool_stack = g_slist_prepend (tool_stack, active_tool);
    }

  tool_manager_select_tool (tool);
}

void
tool_manager_pop_tool (void)
{
  if (tool_stack)
    {
      tool_manager_select_tool (GIMP_TOOL (tool_stack->data));

      tool_stack = g_slist_remove (tool_stack, active_tool);
    }
}


void
tool_manager_initialize_tool (GimpTool *tool, /* FIXME: remove tool param */
			      GDisplay *gdisp)
{
  GimpToolInfo *tool_info;

  /*  Tools which have an init function have dialogs and
   *  cannot be initialized without a display
   */
  if (GIMP_TOOL_CLASS (GTK_OBJECT (tool)->klass)->initialize && ! gdisp)
    {
#ifdef __GNUC__
#warning FIXME   tool_type = RECT_SELECT;
#endif
    }

  /*  Force the emission of the "tool_changed" signal
   */
  tool_info = gimp_context_get_tool (gimp_context_get_user ());

  if (GTK_OBJECT (tool)->klass->type == tool_info->tool_type)
    {
      gimp_context_tool_changed (gimp_context_get_user ());
    }
  else
    {
      GList *list;

      for (list = GIMP_LIST (global_tool_info_list)->list;
	   list;
	   list = g_list_next (list))
	{
	  tool_info = GIMP_TOOL_INFO (list->data);

	  if (tool_info->tool_type == GTK_OBJECT (tool)->klass->type)
	    {
	      gimp_context_set_tool (gimp_context_get_user (), tool_info);

	      break;
	    }
	}
    }

  gimp_tool_initialize (active_tool, gdisp);

  if (gdisp)
    active_tool->drawable = gimp_image_active_drawable (gdisp->gimage);

  /*  don't set tool->gdisp here! (see commands.c)  */
}




void
tool_manager_control_active (ToolAction  action,
			     GDisplay   *gdisp)
{
  if (active_tool)
    {
      if (active_tool->gdisp == gdisp)
        {
          switch (action)
            {
            case PAUSE:
              if (active_tool->state == ACTIVE)
                {
                  if (! active_tool->paused_count)
                    {
                      active_tool->state = PAUSED;

		      gimp_tool_control (active_tool, action, gdisp);
                    }
                }
              active_tool->paused_count++;
              break;

            case RESUME:
              active_tool->paused_count--;
              if (active_tool->state == PAUSED)
                {
                  if (! active_tool->paused_count)
                    {
                      active_tool->state = ACTIVE;

		      gimp_tool_control (active_tool, action, gdisp);
                    }
                }
              break;

            case HALT:
              active_tool->state = INACTIVE;

	      gimp_tool_control (active_tool, action, gdisp);
              break;

            case DESTROY:
              gtk_object_unref (GTK_OBJECT (active_tool));
	      active_tool = NULL;
              break;

            default:
              break;
            }
        }
      else if (action == HALT)
        {
          active_tool->state = INACTIVE;
        }
    }
}


#ifdef __GNUC__
#warning bogosity alert
#endif
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


void
tool_manager_init (void)
{
  global_tool_info_list = gimp_list_new (GIMP_TYPE_TOOL_INFO,
					 GIMP_CONTAINER_POLICY_STRONG);
}

void
tool_manager_register_tool (GtkType       tool_type,
			    gboolean      tool_context,
			    const gchar  *identifier,
			    const gchar  *blurb,
			    const gchar  *help,
			    const gchar  *menu_path,
			    const gchar  *menu_accel,
			    const gchar  *help_domain,
			    const gchar  *help_data,
			    const gchar **icon_data)
{
  GimpToolInfo *tool_info;

  tool_info = gimp_tool_info_new (tool_type,
				  tool_context,
				  identifier,
				  blurb,
				  help,
				  menu_path,
				  menu_accel,
				  help_domain,
				  help_data,
				  icon_data);

  gimp_container_add (global_tool_info_list, GIMP_OBJECT (tool_info));
}

void
tool_manager_register_tool_options (GtkType      tool_type,
				    ToolOptions *tool_options)
{
  GimpToolInfo *tool_info;

  tool_info = tool_manager_get_info_by_type (tool_type);

  if (! tool_info)
    {
      g_warning ("%s(): no tool info registered for %s",
		 G_GNUC_FUNCTION, gtk_type_name (tool_type));
      return;
    }

  tool_info->tool_options = tool_options;

  tool_options_dialog_add (tool_options);
}

GimpToolInfo *
tool_manager_get_info_by_type (GtkType tool_type)
{
  GimpToolInfo *tool_info;
  GList        *list;

  for (list = GIMP_LIST (global_tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      tool_info = (GimpToolInfo *) list->data;

      if (tool_info->tool_type == tool_type)
	return tool_info;
    }

  return NULL;
}

GimpToolInfo *
tool_manager_get_info_by_tool (GimpTool *tool)
{
  g_return_val_if_fail (tool != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);

  return tool_manager_get_info_by_type (GTK_OBJECT (tool)->klass->type);
}

const gchar *
tool_manager_active_get_PDB_string (void)
{
  const gchar *toolStr = "gimp_paintbrush_default";

  /*  Return the correct PDB function for the active tool
   *  The default is paintbrush if the tool is not recognised
   */

  if (! active_tool)
    return toolStr;

  return gimp_tool_get_PDB_string (active_tool);
}

const gchar *
tool_manager_active_get_help_data (void)
{
  g_return_val_if_fail (active_tool != NULL, NULL);

  return tool_manager_get_info_by_tool (active_tool)->help_data;
}

void
tool_manager_help_func (const gchar *help_data)
{
  gimp_standard_help_func (tool_manager_active_get_help_data ());
}
