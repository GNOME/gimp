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

#include "display/gimpdisplay.h"

#include "gimptool.h"
#include "gimprectselecttool.h"
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

#include "app_procs.h"
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

  GQuark       image_dirty_handler_id;
};


/*  local function prototypes  */

static GimpToolManager * tool_manager_get (Gimp            *gimp);
static void              tool_manager_set (Gimp            *gimp,
					   GimpToolManager *tool_manager);
static void   tool_manager_tool_changed   (GimpContext     *user_context,
					   GimpToolInfo    *tool_info,
					   gpointer         data);
static void   tool_manager_image_dirty    (GimpImage       *gimage,
					   gpointer         data);


/*  public functions  */

void
tool_manager_init (Gimp *gimp)
{
  GimpToolManager *tool_manager;
  GimpContext     *user_context;
  GimpContext     *tool_context;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = g_new0 (GimpToolManager, 1);

  tool_manager->active_tool            = NULL;
  tool_manager->tool_stack             = NULL;
  tool_manager->global_tool_context    = NULL;
  tool_manager->image_dirty_handler_id = 0;

  tool_manager_set (gimp, tool_manager);

  tool_manager->image_dirty_handler_id =
    gimp_container_add_handler (gimp->images, "dirty",
				G_CALLBACK (tool_manager_image_dirty),
				tool_manager);

  user_context = gimp_get_user_context (gimp);

  g_signal_connect (G_OBJECT (user_context), "tool_changed",
		    G_CALLBACK (tool_manager_tool_changed),
		    tool_manager);

  /*  Create a context to store the paint options of the
   *  global paint options mode
   */
  tool_manager->global_tool_context = gimp_context_new (gimp,
                                                        "Global Tool Context",
                                                        user_context);

  /*  TODO: add foreground, background, brush, pattern, gradient  */
  gimp_context_define_properties (tool_manager->global_tool_context,
				  PAINT_OPTIONS_MASK, FALSE);

  /* register internal tools */
  tools_init (gimp);

  gimp_tool_info_set_standard (gimp, tool_manager_get_info_by_type (gimp, GIMP_TYPE_RECT_SELECT_TOOL));

  if (tool_manager->active_tool)
    {
      tool_context = tool_manager->active_tool->tool_info->context;

      if (tool_context)
	{
	  gimp_context_set_parent (tool_context, user_context);
	}
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

  g_object_unref (G_OBJECT (tool_manager->global_tool_context));

  gimp_container_remove_handler (gimp->images,
				 tool_manager->image_dirty_handler_id);

  if (tool_manager->active_tool)
    g_object_unref (G_OBJECT (tool_manager->active_tool));

  g_free (tool_manager);
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
      if (tool_manager->active_tool->gdisp)
        {
          tool_manager_control_active (gimp,
                                       HALT,
                                       tool_manager->active_tool->gdisp);
        }

      g_object_unref (G_OBJECT (tool_manager->active_tool));
    }

  tool_manager->active_tool = tool;

  g_object_ref (G_OBJECT (tool_manager->active_tool));
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

      g_object_ref (G_OBJECT (tool_manager->tool_stack->data));
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

      g_object_unref (G_OBJECT (tool_manager->tool_stack->data));

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

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      tool = tool_manager->active_tool;

      gimp_tool_initialize (tool, gdisp);

      if (gdisp)
        tool->drawable = gimp_image_active_drawable (gdisp->gimage);
      else
        tool->drawable = NULL;
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

  if (tool_manager->active_tool->gdisp == gdisp)
    {
      gimp_tool_control (tool_manager->active_tool,
                         action,
                         gdisp);
    }
  else if (action == HALT)
    {
      tool_manager->active_tool->state = INACTIVE;
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
tool_manager_register_tool (Gimp                   *gimp,
			    GType                   tool_type,
                            GimpToolOptionsNewFunc  options_new_func,
			    gboolean                tool_context,
			    const gchar            *identifier,
			    const gchar            *blurb,
			    const gchar            *help,
			    const gchar            *menu_path,
			    const gchar            *menu_accel,
			    const gchar            *help_domain,
			    const gchar            *help_data,
			    const gchar            *stock_id)
{
  GimpToolManager *tool_manager;
  GimpToolInfo    *tool_info;
  const gchar     *paint_core_name;
  GtkIconSet      *icon_set;
  GtkStyle        *style;
  GdkPixbuf       *pixbuf;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (g_type_is_a (tool_type, GIMP_TYPE_TOOL));

  if (tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      paint_core_name = "GimpPencil";
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

  icon_set = gtk_icon_factory_lookup_default (stock_id);

#ifdef __GNUC__
#warning FIXME: remove gtk_widget_get_default_style()
#endif
  style = gtk_widget_get_default_style ();

  pixbuf = gtk_icon_set_render_icon (icon_set,
				     style,
				     GTK_TEXT_DIR_LTR,
				     GTK_STATE_NORMAL,
				     GTK_ICON_SIZE_BUTTON,
				     NULL,
				     NULL);

  tool_manager = tool_manager_get (gimp);

  tool_info = gimp_tool_info_new (gimp,
                                  tool_manager->global_tool_context,
				  tool_type,
				  tool_context,
				  identifier,
				  blurb,
				  help,
				  menu_path,
				  menu_accel,
				  help_domain,
				  help_data,
                                  paint_core_name,
				  stock_id,
				  pixbuf);

  g_object_unref (G_OBJECT (pixbuf));

  if (options_new_func)
    {
      tool_info->tool_options = options_new_func (tool_info);
    }
  else
    {
      tool_info->tool_options = tool_options_new (tool_info);
    }

  gimp_container_add (gimp->tool_info_list, GIMP_OBJECT (tool_info));
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

const gchar *
tool_manager_active_get_help_data (Gimp *gimp)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      return tool_manager->active_tool->tool_info->help_data;
    }

  return NULL;
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
  return g_object_get_data (G_OBJECT (gimp), TOOL_MANAGER_DATA_KEY);
}

static void
tool_manager_set (Gimp            *gimp,
		  GimpToolManager *tool_manager)
{
  g_object_set_data (G_OBJECT (gimp), TOOL_MANAGER_DATA_KEY,
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
  if (user_context->gimp->busy)
    {
      /*  there may be contexts waiting for the user_context's "tool_changed"
       *  signal, so stop emitting it.
       */
      g_signal_stop_emission_by_name (G_OBJECT (user_context), "tool_changed");

      if (G_TYPE_FROM_INSTANCE (tool_manager->active_tool) !=
	  tool_info->tool_type)
	{
	  g_signal_handlers_block_by_func (G_OBJECT (user_context),
					   tool_manager_tool_changed,
					   data);

	  /*  explicitly set the current tool  */
	  gimp_context_set_tool (user_context,
                                 tool_manager->active_tool->tool_info);

	  g_signal_handlers_unblock_by_func (G_OBJECT (user_context),
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

  if (tool_manager->active_tool &&
      (tool_context = tool_manager->active_tool->tool_info->context))
    {
      gimp_context_unset_parent (tool_context);
    }

  if ((tool_context = tool_info->context))
    {
      gimp_context_copy_properties (tool_context, user_context,
                                    PAINT_OPTIONS_MASK);
      gimp_context_set_parent (tool_context, user_context);
    }

  tool_manager_select_tool (user_context->gimp, new_tool);

  g_object_unref (G_OBJECT (new_tool));
}

static void
tool_manager_image_dirty (GimpImage *gimage,
			  gpointer   data)
{
  GimpToolManager *tool_manager;

  tool_manager = (GimpToolManager *) data;

  if (tool_manager->active_tool && ! tool_manager->active_tool->preserve)
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
              tool_manager_initialize_active (gimage->gimp, gdisp);

              tool_manager->active_tool->gdisp = gdisp;
            }
	  else
            {
              tool_manager_initialize_active (gimage->gimp, NULL);
            }
	}
    }
}
