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
#include "core/gimpimage.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "display/gimpdisplay.h"

#include "gimpairbrushtool.h"
#include "gimpclonetool.h"
#include "gimpconvolvetool.h"
#include "gimpdodgeburntool.h"
#include "gimperasertool.h"
#include "gimpimagemaptool.h"
#include "gimppaintbrushtool.h"
#include "gimppenciltool.h"
#include "gimprectselecttool.h"
#include "gimpsmudgetool.h"
#include "gimptoolcontrol.h"
#include "gimptooloptions-gui.h"
#include "tool_manager.h"
#include "tools.h"

#include "gimp-intl.h"


typedef struct _GimpToolManager GimpToolManager;

struct _GimpToolManager
{
  GimpTool    *active_tool;
  GSList      *tool_stack;

  GQuark       image_dirty_handler_id;
  GQuark       image_undo_start_handler_id;
};


/*  local function prototypes  */

static GimpToolManager * tool_manager_get   (Gimp            *gimp);
static void              tool_manager_set   (Gimp            *gimp,
                                             GimpToolManager *tool_manager);
static void   tool_manager_tool_changed     (GimpContext     *user_context,
                                             GimpToolInfo    *tool_info,
                                             gpointer         data);
static void   tool_manager_image_dirty      (GimpImage       *gimage,
                                             GimpToolManager *tool_manager);
static void   tool_manager_image_undo_start (GimpImage       *gimage,
                                             GimpToolManager *tool_manager);


/*  public functions  */

void
tool_manager_init (Gimp *gimp)
{
  GimpToolManager *tool_manager;
  GimpContext     *user_context;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = g_new0 (GimpToolManager, 1);

  tool_manager->active_tool                 = NULL;
  tool_manager->tool_stack                  = NULL;
  tool_manager->image_dirty_handler_id      = 0;
  tool_manager->image_undo_start_handler_id = 0;

  tool_manager_set (gimp, tool_manager);

  tool_manager->image_dirty_handler_id =
    gimp_container_add_handler (gimp->images, "dirty",
				G_CALLBACK (tool_manager_image_dirty),
				tool_manager);

  tool_manager->image_undo_start_handler_id =
    gimp_container_add_handler (gimp->images, "undo_start",
				G_CALLBACK (tool_manager_image_undo_start),
				tool_manager);

  user_context = gimp_get_user_context (gimp);

  g_signal_connect (user_context, "tool_changed",
		    G_CALLBACK (tool_manager_tool_changed),
		    tool_manager);

  /* register internal tools */
  tools_init (gimp);

  gimp_tool_info_set_standard (gimp, tool_manager_get_info_by_type (gimp, GIMP_TYPE_RECT_SELECT_TOOL));

  if (tool_manager->active_tool)
    {
      GimpToolInfo *tool_info;

      tool_info = tool_manager->active_tool->tool_info;

      if (tool_info->context_props)
        gimp_context_set_parent (GIMP_CONTEXT (tool_info->tool_options),
                                 gimp_get_user_context (gimp));
    }

  gimp_container_thaw (gimp->tool_info_list);
}

void
tool_manager_exit (Gimp *gimp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);
  tool_manager_set (gimp, NULL);

  gimp_container_remove_handler (gimp->images,
				 tool_manager->image_dirty_handler_id);
  gimp_container_remove_handler (gimp->images,
				 tool_manager->image_undo_start_handler_id);

  if (tool_manager->active_tool)
    g_object_unref (tool_manager->active_tool);

  g_free (tool_manager);
}

void
tool_manager_restore (Gimp *gimp)
{
  GimpToolManager *tool_manager;
  GimpToolInfo    *tool_info;
  GList           *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  for (list = GIMP_LIST (gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      GimpToolOptionsGUIFunc  options_gui_func;
      GtkWidget              *options_gui;

      tool_info = GIMP_TOOL_INFO (list->data);

      gimp_tool_options_deserialize (tool_info->tool_options, NULL, NULL);

      options_gui_func = g_object_get_data (G_OBJECT (tool_info),
                                            "gimp-tool-options-gui-func");

      if (options_gui_func)
        {
          options_gui = (* options_gui_func) (tool_info->tool_options);
        }
      else
        {
          GtkWidget *label;

          options_gui = gimp_tool_options_gui (tool_info->tool_options);

          label = gtk_label_new (_("This tool has no options."));
          gtk_box_pack_start (GTK_BOX (options_gui), label, FALSE, FALSE, 6);
          gtk_widget_show (label);
        }

      g_object_set_data (G_OBJECT (tool_info->tool_options),
                         "gimp-tool-options-gui", options_gui);
    }
}

void
tool_manager_save (Gimp *gimp)
{
  GimpToolManager *tool_manager;
  GimpToolInfo    *tool_info;
  GList           *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  for (list = GIMP_LIST (gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      tool_info = GIMP_TOOL_INFO (list->data);

      gimp_tool_options_serialize (tool_info->tool_options, NULL, NULL);
    }
}

GimpTool *
tool_manager_get_active (Gimp *gimp)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  tool_manager = tool_manager_get (gimp);

  return tool_manager->active_tool;
}

void
tool_manager_select_tool (Gimp     *gimp,
			  GimpTool *tool)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_TOOL (tool));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      GimpDisplay *gdisp;

      gdisp = tool_manager->active_tool->gdisp;

      if (! gdisp && GIMP_IS_DRAW_TOOL (tool_manager->active_tool))
        gdisp = GIMP_DRAW_TOOL (tool_manager->active_tool)->gdisp;

      if (gdisp)
        tool_manager_control_active (gimp, HALT, gdisp);

      g_object_unref (tool_manager->active_tool);
    }

  tool_manager->active_tool = g_object_ref (tool);
}

void
tool_manager_push_tool (Gimp     *gimp,
			GimpTool *tool)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_TOOL (tool));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      tool_manager->tool_stack = g_slist_prepend (tool_manager->tool_stack,
						  tool_manager->active_tool);

      g_object_ref (tool_manager->tool_stack->data);
    }

  tool_manager_select_tool (gimp, tool);
}

void
tool_manager_pop_tool (Gimp *gimp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->tool_stack)
    {
      tool_manager_select_tool (gimp,
				GIMP_TOOL (tool_manager->tool_stack->data));

      g_object_unref (tool_manager->tool_stack->data);

      tool_manager->tool_stack = g_slist_remove (tool_manager->tool_stack,
						 tool_manager->active_tool);
    }
}

void
tool_manager_initialize_active (Gimp        *gimp,
                                GimpDisplay *gdisp)
{
  GimpToolManager *tool_manager;
  GimpTool        *tool;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      tool = tool_manager->active_tool;

      gimp_tool_initialize (tool, gdisp);

      tool->drawable = gimp_image_active_drawable (gdisp->gimage);
    }
}

void
tool_manager_control_active (Gimp           *gimp,
			     GimpToolAction  action,
			     GimpDisplay    *gdisp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (! tool_manager->active_tool)
    return;

  if (gdisp && (tool_manager->active_tool->gdisp == gdisp ||
                (GIMP_IS_DRAW_TOOL (tool_manager->active_tool) &&
                 GIMP_DRAW_TOOL (tool_manager->active_tool)->gdisp == gdisp)))
    {
      gimp_tool_control (tool_manager->active_tool,
                         action,
                         gdisp);
    }
  else if (action == HALT)
    {
      if (gimp_tool_control_is_active (tool_manager->active_tool->control))
        gimp_tool_control_halt (tool_manager->active_tool->control);
    }
}

void
tool_manager_button_press_active (Gimp            *gimp,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_button_press (tool_manager->active_tool,
                              coords, time, state,
                              gdisp);
    }
}

void
tool_manager_button_release_active (Gimp            *gimp,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_button_release (tool_manager->active_tool,
                                coords, time, state,
                                gdisp);
    }
}

void
tool_manager_motion_active (Gimp            *gimp,
                            GimpCoords      *coords,
                            guint32          time,
                            GdkModifierType  state,
                            GimpDisplay     *gdisp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_motion (tool_manager->active_tool,
                        coords, time, state,
                        gdisp);
    }
}

void
tool_manager_arrow_key_active (Gimp        *gimp,
                               GdkEventKey *kevent,
                               GimpDisplay *gdisp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_arrow_key (tool_manager->active_tool,
                           kevent,
                           gdisp);
    }
}

void
tool_manager_modifier_key_active (Gimp            *gimp,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_modifier_key (tool_manager->active_tool,
                              key, press, state,
                              gdisp);
    }
}

void
tool_manager_oper_update_active (Gimp            *gimp,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 GimpDisplay     *gdisp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_oper_update (tool_manager->active_tool,
                             coords, state,
                             gdisp);
    }
}

void
tool_manager_cursor_update_active (Gimp            *gimp,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *gdisp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_cursor_update (tool_manager->active_tool,
                               coords, state,
                               gdisp);
    }
}

void
tool_manager_register_tool (GType                   tool_type,
                            GType                   tool_options_type,
                            GimpToolOptionsGUIFunc  options_gui_func,
			    GimpContextPropMask     context_props,
			    const gchar            *identifier,
			    const gchar            *blurb,
			    const gchar            *help,
			    const gchar            *menu_path,
			    const gchar            *menu_accel,
			    const gchar            *help_domain,
			    const gchar            *help_data,
			    const gchar            *stock_id,
			    gpointer                data)
{
  Gimp            *gimp = (Gimp *) data;
  GimpToolManager *tool_manager;
  GimpToolInfo    *tool_info;
  const gchar     *paint_core_name;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (g_type_is_a (tool_type, GIMP_TYPE_TOOL));
  g_return_if_fail (tool_options_type == G_TYPE_NONE ||
                    g_type_is_a (tool_options_type, GIMP_TYPE_TOOL_OPTIONS));

  if (tool_options_type == G_TYPE_NONE)
    tool_options_type = GIMP_TYPE_TOOL_OPTIONS;

  if (tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      paint_core_name = "GimpPaintbrush";
    }
  else if (tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    {
      paint_core_name = "GimpPaintbrush";
    }
  else if (tool_type == GIMP_TYPE_ERASER_TOOL)
    {
      paint_core_name = "GimpEraser";
    }
  else if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL)
    {
      paint_core_name = "GimpAirbrush";
    }
  else if (tool_type == GIMP_TYPE_CLONE_TOOL)
    {
      paint_core_name = "GimpClone";
    }
  else if (tool_type == GIMP_TYPE_CONVOLVE_TOOL)
    {
      paint_core_name = "GimpConvolve";
    }
  else if (tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      paint_core_name = "GimpSmudge";
    }
  else if (tool_type == GIMP_TYPE_DODGEBURN_TOOL)
    {
      paint_core_name = "GimpDodgeBurn";
    }
  else
    {
      paint_core_name = "GimpPaintbrush";
    }

  tool_manager = tool_manager_get (gimp);

  tool_info = gimp_tool_info_new (gimp,
				  tool_type,
                                  tool_options_type,
				  context_props,
				  identifier,
				  blurb,
				  help,
				  menu_path,
				  menu_accel,
				  help_domain,
				  help_data,
                                  paint_core_name,
				  stock_id);

  if (g_type_is_a (tool_type, GIMP_TYPE_IMAGE_MAP_TOOL))
    tool_info->in_toolbox = FALSE;

  g_object_set_data (G_OBJECT (tool_info), "gimp-tool-options-gui-func",
                     options_gui_func);

  tool_info->tool_options = g_object_new (tool_info->tool_options_type,
                                          "gimp",      gimp,
                                          "tool-info", tool_info,
                                          NULL);

  if (tool_info->context_props)
    {
      gimp_context_define_properties (GIMP_CONTEXT (tool_info->tool_options),
                                      tool_info->context_props, FALSE);

      gimp_context_copy_properties (gimp_get_user_context (gimp),
                                    GIMP_CONTEXT (tool_info->tool_options),
                                    GIMP_CONTEXT_ALL_PROPS_MASK);
    }

  gimp_container_add (gimp->tool_info_list, GIMP_OBJECT (tool_info));
  g_object_unref (tool_info);
}

GimpToolInfo *
tool_manager_get_info_by_type (Gimp  *gimp,
			       GType  tool_type)
{
  GimpToolInfo *tool_info;
  GList        *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (tool_type, GIMP_TYPE_TOOL), NULL);

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


/*  private functions  */

static GQuark tool_manager_quark = 0;


static GimpToolManager *
tool_manager_get (Gimp *gimp)
{
  if (! tool_manager_quark)
    tool_manager_quark = g_quark_from_static_string ("gimp-tool-manager");

  return g_object_get_qdata (G_OBJECT (gimp), tool_manager_quark);
}

static void
tool_manager_set (Gimp            *gimp,
		  GimpToolManager *tool_manager)
{
  if (! tool_manager_quark)
    tool_manager_quark = g_quark_from_static_string ("gimp-tool-manager");

  g_object_set_qdata (G_OBJECT (gimp), tool_manager_quark, tool_manager);
}

static void
tool_manager_tool_changed (GimpContext  *user_context,
			   GimpToolInfo *tool_info,
			   gpointer      data)
{
  GimpToolManager *tool_manager;
  GimpTool        *new_tool = NULL;

  if (! tool_info)
    return;

  tool_manager = (GimpToolManager *) data;

  /* FIXME: gimp_busy HACK */
  if (user_context->gimp->busy)
    {
      /*  there may be contexts waiting for the user_context's "tool_changed"
       *  signal, so stop emitting it.
       */
      g_signal_stop_emission_by_name (user_context, "tool_changed");

      if (G_TYPE_FROM_INSTANCE (tool_manager->active_tool) !=
	  tool_info->tool_type)
	{
	  g_signal_handlers_block_by_func (user_context,
					   tool_manager_tool_changed,
					   data);

	  /*  explicitly set the current tool  */
	  gimp_context_set_tool (user_context,
                                 tool_manager->active_tool->tool_info);

	  g_signal_handlers_unblock_by_func (user_context,
					     tool_manager_tool_changed,
					     data);
	}

      return;
    }

  if (g_type_is_a (tool_info->tool_type, GIMP_TYPE_TOOL))
    {
      new_tool = g_object_new (tool_info->tool_type, NULL);

      new_tool->tool_info = tool_info;
    }
  else
    {
      g_warning ("%s(): tool_info->tool_type is no GimpTool subclass",
		 G_GNUC_FUNCTION);
      return;
    }

  /*  disconnect the old tool's context  */
  if (tool_manager->active_tool            &&
      tool_manager->active_tool->tool_info &&
      tool_manager->active_tool->tool_info->context_props)
    {
      GimpToolInfo *old_tool_info;

      old_tool_info = tool_manager->active_tool->tool_info;

      gimp_context_set_parent (GIMP_CONTEXT (old_tool_info->tool_options), NULL);
    }

  /*  connect the new tool's context  */
  if (tool_info->context_props)
    {
      gimp_context_copy_properties (GIMP_CONTEXT (tool_info->tool_options),
                                    user_context,
                                    tool_info->context_props);
      gimp_context_set_parent (GIMP_CONTEXT (tool_info->tool_options),
                               user_context);
    }

  tool_manager_select_tool (user_context->gimp, new_tool);

  g_object_unref (new_tool);
}

static void
tool_manager_image_dirty (GimpImage       *gimage,
			  GimpToolManager *tool_manager)
{
  if (tool_manager->active_tool &&
      ! gimp_tool_control_preserve (tool_manager->active_tool->control))
    {
      GimpDisplay *gdisp;

      gdisp = tool_manager->active_tool->gdisp;

      if (gdisp)
	{
          /*  create a new one, deleting the current
           */
          gimp_context_tool_changed (gimp_get_user_context (gimage->gimp));

	  if (gdisp->gimage == gimage)
            {
              GimpTool *tool;

              tool = tool_manager->active_tool;

              if (gimp_image_active_drawable (gdisp->gimage) ||
                  gimp_tool_control_handles_empty_image (tool->control))
                {
                  tool_manager_initialize_active (gimage->gimp, gdisp);

                  tool->gdisp = gdisp;
                }
            }
	}
    }
}

static void
tool_manager_image_undo_start (GimpImage       *gimage,
                               GimpToolManager *tool_manager)
{
  if (tool_manager->active_tool &&
      ! gimp_tool_control_preserve (tool_manager->active_tool->control))
    {
      GimpDisplay *gdisp;

      gdisp = tool_manager->active_tool->gdisp;

      if (! gdisp || gdisp->gimage != gimage)
        if (GIMP_IS_DRAW_TOOL (tool_manager->active_tool))
          gdisp = GIMP_DRAW_TOOL (tool_manager->active_tool)->gdisp;

      if (gdisp && gdisp->gimage == gimage)
        tool_manager_control_active (gimage->gimp, HALT,
                                     tool_manager->active_tool->gdisp);
    }
}
