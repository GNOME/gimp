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

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "gui/brush-select.h"

#include "gimptool.h"
#include "paint_options.h"
#include "tool_manager.h"
#include "tool_options.h"
#include "tools.h"

#include "gimpairbrushtool.h"
#include "gimppaintbrushtool.h"
#include "gimpclonetool.h"
#include "gimpconvolvetool.h"
#include "gimpdodgeburntool.h"
#include "gimperasertool.h"
#include "gimppenciltool.h"
#include "gimpsmudgetool.h"

#include "appenv.h"
#include "app_procs.h"
#include "gdisplay.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


#define PAINT_OPTIONS_MASK GIMP_CONTEXT_OPACITY_MASK | \
                           GIMP_CONTEXT_PAINT_MODE_MASK


typedef struct _GimpToolManager GimpToolManager;

struct _GimpToolManager
{
  GimpTool    *active_tool;
  GSList      *tool_stack;

  GimpContext *global_tool_context;
};


/*  local function prototypes  */

static GimpToolManager * tool_manager_get          (Gimp            *gimp);
static void              tool_manager_set          (Gimp            *gimp,
						    GimpToolManager *tool_manager);
static void              tool_manager_tool_changed (GimpContext     *user_context,
						    GimpToolInfo    *tool_info,
						    gpointer         data);


/*  public functions  */

void
tool_manager_init (Gimp *gimp)
{
  GimpToolManager *tool_manager;
  GimpContext     *user_context;
  GimpContext     *tool_context;

  tool_manager = g_new0 (GimpToolManager, 1);

  tool_manager->active_tool         = NULL;
  tool_manager->tool_stack          = NULL;
  tool_manager->global_tool_context = NULL;

  tool_manager_set (gimp, tool_manager);

  user_context = gimp_get_user_context (gimp);

  gtk_signal_connect (GTK_OBJECT (user_context), "tool_changed",
		      GTK_SIGNAL_FUNC (tool_manager_tool_changed),
		      tool_manager);

  /*  Create a context to store the paint options of the
   *  global paint options mode
   */
  tool_manager->global_tool_context = gimp_create_context (gimp,
							   "Global Tool Context",
							   user_context);

  /*  TODO: add foreground, background, brush, pattern, gradient  */
  gimp_context_define_args (tool_manager->global_tool_context,
			    PAINT_OPTIONS_MASK, FALSE);

  /* register internal tools */
  tools_init (gimp);

  if (! gimprc.global_paint_options && tool_manager->active_tool)
    {
      tool_context =
	tool_manager_get_info_by_tool (gimp,
				       tool_manager->active_tool)->context;

      if (tool_context)
	{
	  gimp_context_set_parent (tool_context, user_context);
	}
    }
  else if (gimprc.global_paint_options)
    {
      gimp_context_set_parent (tool_manager->global_tool_context, user_context);
    }

  gimp_container_thaw (gimp->tool_info_list);
}

void
tool_manager_exit (Gimp *gimp)
{
  GimpToolManager *tool_manager;

  tool_manager = tool_manager_get (gimp);

  gtk_object_unref (GTK_OBJECT (tool_manager->global_tool_context));

  g_free (tool_manager);

  tool_manager_set (gimp, NULL);
}

void
tool_manager_set_global_paint_options (Gimp     *gimp,
				       gboolean  global)
{
  GimpToolManager *tool_manager;
  GimpToolInfo    *tool_info;

  if (global == gimprc.global_paint_options)
    return;

  tool_manager = tool_manager_get (gimp);

  paint_options_set_global (global);

  /*  NULL is the main brush selection  */
  brush_select_show_paint_options (NULL, global);

  tool_info = gimp_context_get_tool (gimp_get_user_context (gimp));

  if (global)
    {
      if (tool_info && tool_info->context)
	{
	  gimp_context_unset_parent (tool_info->context);
	}

      gimp_context_copy_args (tool_manager->global_tool_context,
			      gimp_get_user_context (gimp),
			      PAINT_OPTIONS_MASK);
      gimp_context_set_parent (tool_manager->global_tool_context,
			       gimp_get_user_context (gimp));
    }
  else
    {
      gimp_context_unset_parent (tool_manager->global_tool_context);

      if (tool_info && tool_info->context)
	{
	  gimp_context_copy_args (tool_info->context,
				  gimp_get_user_context (gimp),
				  GIMP_CONTEXT_PAINT_ARGS_MASK);
	  gimp_context_set_parent (tool_info->context,
				   gimp_get_user_context (gimp));
	}
    }
}

GimpTool *
tool_manager_get_active (Gimp *gimp)
{
  GimpToolManager *tool_manager;

  tool_manager = tool_manager_get (gimp);

  return tool_manager->active_tool;
}

void
tool_manager_select_tool (Gimp     *gimp,
			  GimpTool *tool)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (tool != NULL);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    gtk_object_unref (GTK_OBJECT (tool_manager->active_tool));

  tool_manager->active_tool = tool;
}

void
tool_manager_push_tool (Gimp     *gimp,
			GimpTool *tool)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (tool != NULL);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gtk_object_ref (GTK_OBJECT (tool_manager->active_tool));

      tool_manager->tool_stack = g_slist_prepend (tool_manager->tool_stack,
						  tool_manager->active_tool);
    }

  tool_manager_select_tool (gimp, tool);
}

void
tool_manager_pop_tool (Gimp *gimp)
{
  GimpToolManager *tool_manager;

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->tool_stack)
    {
      tool_manager_select_tool (gimp,
				GIMP_TOOL (tool_manager->tool_stack->data));

      tool_manager->tool_stack = g_slist_remove (tool_manager->tool_stack,
						 tool_manager->active_tool);
    }
}


void
tool_manager_initialize_tool (Gimp     *gimp,
			      GimpTool *tool, /* FIXME: remove tool param */
			      GDisplay *gdisp)
{
  GimpToolManager *tool_manager;
  GimpToolInfo    *tool_info;

  tool_manager = tool_manager_get (gimp);

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
  tool_info = gimp_context_get_tool (gimp_get_user_context (gimp));

  if (GTK_OBJECT (tool)->klass->type == tool_info->tool_type)
    {
      gimp_context_tool_changed (gimp_get_user_context (gimp));
    }
  else
    {
      GList *list;

      for (list = GIMP_LIST (gimp->tool_info_list)->list;
	   list;
	   list = g_list_next (list))
	{
	  tool_info = GIMP_TOOL_INFO (list->data);

	  if (tool_info->tool_type == GTK_OBJECT (tool)->klass->type)
	    {
	      gimp_context_set_tool (gimp_get_user_context (gimp),
				     tool_info);

	      break;
	    }
	}
    }

  gimp_tool_initialize (tool_manager->active_tool, gdisp);

  if (gdisp)
    tool_manager->active_tool->drawable =
      gimp_image_active_drawable (gdisp->gimage);

  /*  don't set tool->gdisp here! (see commands.c)  */
}

void
tool_manager_control_active (Gimp       *gimp,
			     ToolAction  action,
			     GDisplay   *gdisp)
{
  GimpToolManager *tool_manager;

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      if (tool_manager->active_tool->gdisp == gdisp)
        {
          switch (action)
            {
            case PAUSE:
              if (tool_manager->active_tool->state == ACTIVE)
                {
                  if (! tool_manager->active_tool->paused_count)
                    {
                      tool_manager->active_tool->state = PAUSED;

		      gimp_tool_control (tool_manager->active_tool,
					 action, gdisp);
                    }
                }

              tool_manager->active_tool->paused_count++;
              break;

            case RESUME:
              tool_manager->active_tool->paused_count--;

              if (tool_manager->active_tool->state == PAUSED)
                {
                  if (! tool_manager->active_tool->paused_count)
                    {
                      tool_manager->active_tool->state = ACTIVE;

		      gimp_tool_control (tool_manager->active_tool,
					 action, gdisp);
                    }
                }
              break;

            case HALT:
              tool_manager->active_tool->state = INACTIVE;

	      gimp_tool_control (tool_manager->active_tool, action, gdisp);
              break;

            case DESTROY:
              gtk_object_unref (GTK_OBJECT (tool_manager->active_tool));
	      tool_manager->active_tool = NULL;
              break;

            default:
              break;
            }
        }
      else if (action == HALT)
        {
          tool_manager->active_tool->state = INACTIVE;
        }
    }
}

void
tool_manager_register_tool (Gimp         *gimp,
			    GtkType       tool_type,
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
  GimpToolManager *tool_manager;
  GimpToolInfo    *tool_info;

  tool_manager = tool_manager_get (gimp);

  tool_info = gimp_tool_info_new (tool_manager->global_tool_context,
				  tool_type,
				  tool_context,
				  identifier,
				  blurb,
				  help,
				  menu_path,
				  menu_accel,
				  help_domain,
				  help_data,
				  icon_data);

  gimp_container_add (gimp->tool_info_list, GIMP_OBJECT (tool_info));
}

void
tool_manager_register_tool_options (GtkType      tool_type,
				    ToolOptions *tool_options)
{
  GimpToolInfo *tool_info;

  tool_info = tool_manager_get_info_by_type (the_gimp, tool_type);

  if (! tool_info)
    {
      g_warning ("%s(): no tool info registered for %s",
		 G_GNUC_FUNCTION, gtk_type_name (tool_type));
      return;
    }

  tool_info->tool_options = tool_options;
}

GimpToolInfo *
tool_manager_get_info_by_type (Gimp    *gimp,
			       GtkType  tool_type)
{
  GimpToolInfo *tool_info;
  GList        *list;

  for (list = GIMP_LIST (gimp->tool_info_list)->list;
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
tool_manager_get_info_by_tool (Gimp     *gimp,
			       GimpTool *tool)
{
  g_return_val_if_fail (tool != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);

  return tool_manager_get_info_by_type (gimp, GTK_OBJECT (tool)->klass->type);
}

const gchar *
tool_manager_active_get_PDB_string (Gimp *gimp)
{
  GimpToolManager *tool_manager;
  GimpToolInfo    *tool_info;
  const gchar     *tool_str = "gimp_paintbrush_default";

  /*  Return the correct PDB function for the active tool
   *  The default is paintbrush if the tool is not recognized
   */

  tool_manager = tool_manager_get (gimp);

  if (! tool_manager->active_tool)
    return tool_str;

  tool_info = gimp_context_get_tool (gimp_get_user_context (gimp));

  if (tool_info->tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      tool_str = "gimp_pencil";
    }
  else if (tool_info->tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    {
      tool_str = "gimp_paintbrush_default";
    }
  else if (tool_info->tool_type == GIMP_TYPE_ERASER_TOOL)
    {
      tool_str = "gimp_eraser_default";
    }
  else if (tool_info->tool_type == GIMP_TYPE_AIRBRUSH_TOOL)
    {
      tool_str = "gimp_airbrush_default";
    }
  else if (tool_info->tool_type == GIMP_TYPE_CLONE_TOOL)
    {
      tool_str = "gimp_clone_default";
    }
  else if (tool_info->tool_type == GIMP_TYPE_CONVOLVE_TOOL)
    {
      tool_str = "gimp_convolve_default";
    }
  else if (tool_info->tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      tool_str = "gimp_smudge_default";
    }
  else if (tool_info->tool_type == GIMP_TYPE_DODGEBURN_TOOL)
    {
      tool_str = "gimp_dodgeburn_default";
    }
  
  return tool_str;
}

const gchar *
tool_manager_active_get_help_data (Gimp *gimp)
{
  GimpToolManager *tool_manager;

  tool_manager = tool_manager_get (gimp);

  if (! tool_manager->active_tool)
    return NULL;

  return tool_manager_get_info_by_tool (gimp,
					tool_manager->active_tool)->help_data;
}

void
tool_manager_help_func (const gchar *help_data)
{
  gimp_standard_help_func (tool_manager_active_get_help_data (the_gimp));
}


/*  private functions  */

#define TOOL_MANAGER_DATA_KEY "gimp-tool-manager"

static GimpToolManager *
tool_manager_get (Gimp *gimp)
{
  return gtk_object_get_data (GTK_OBJECT (gimp), TOOL_MANAGER_DATA_KEY);
}

static void
tool_manager_set (Gimp            *gimp,
		  GimpToolManager *tool_manager)
{
  gtk_object_set_data (GTK_OBJECT (gimp), TOOL_MANAGER_DATA_KEY,
		       tool_manager);
}

static void
tool_manager_tool_changed (GimpContext  *user_context,
			   GimpToolInfo *tool_info,
			   gpointer      data)
{
  GimpToolManager *tool_manager;
  GimpTool        *new_tool     = NULL;
  GimpContext     *tool_context = NULL;

  if (! tool_info)
    return;

  tool_manager = (GimpToolManager *) data;

  /* FIXME: gimp_busy HACK */
  if (gimp_busy)
    {
      /*  there may be contexts waiting for the user_context's "tool_changed"
       *  signal, so stop emitting it.
       */
      gtk_signal_emit_stop_by_name (GTK_OBJECT (user_context), "tool_changed");

      if (GTK_OBJECT (tool_manager->active_tool)->klass->type !=
	  tool_info->tool_type)
	{
	  gtk_signal_handler_block_by_func (GTK_OBJECT (user_context),
					    tool_manager_tool_changed,
					    data);

	  /*  explicitly set the current tool  */
	  gimp_context_set_tool (user_context,
				 tool_manager_get_info_by_tool (user_context->gimp,
								tool_manager->active_tool));

	  gtk_signal_handler_unblock_by_func (GTK_OBJECT (user_context),
					      tool_manager_tool_changed,
					      data);
	}

      return;
    }

  if (tool_info->tool_type != GTK_TYPE_NONE)
    {
      new_tool = gtk_type_new (tool_info->tool_type);
    }
  else
    {
      g_warning ("%s(): tool_info contains no valid GtkType",
		 G_GNUC_FUNCTION);
      return;
    }

  if (! gimprc.global_paint_options)
    {
      if (tool_manager->active_tool &&
	  (tool_context = tool_manager_get_info_by_tool (user_context->gimp,
							 tool_manager->active_tool)->context))
	{
	  gimp_context_unset_parent (tool_context);
	}

      if ((tool_context = tool_info->context))
	{
	  gimp_context_copy_args (tool_context, user_context,
				  PAINT_OPTIONS_MASK);
	  gimp_context_set_parent (tool_context, user_context);
	}
    }

  tool_manager_select_tool (user_context->gimp, new_tool);
}
