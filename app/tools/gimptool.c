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

#include "core/gimpimage.h"
#include "core/gimpmarshal.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimptool.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"


static void   gimp_tool_class_init          (GimpToolClass   *klass);
static void   gimp_tool_init                (GimpTool        *tool);

static void   gimp_tool_real_initialize     (GimpTool        *tool,
					     GimpDisplay     *gdisp);
static void   gimp_tool_real_control        (GimpTool        *tool,
					     ToolAction       tool_action,
					     GimpDisplay     *gdisp);
static void   gimp_tool_real_button_press   (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             guint32          time,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);
static void   gimp_tool_real_button_release (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             guint32          time,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);
static void   gimp_tool_real_motion         (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             guint32          time,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);
static void   gimp_tool_real_arrow_key      (GimpTool        *tool,
					     GdkEventKey     *kevent,
					     GimpDisplay     *gdisp);
static void   gimp_tool_real_modifier_key   (GimpTool        *tool,
                                             GdkModifierType  key,
                                             gboolean         press,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);
static void   gimp_tool_real_oper_update    (GimpTool        *tool,
                                             GimpCoords      *coords,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);
static void   gimp_tool_real_cursor_update  (GimpTool        *tool,
                                             GimpCoords      *coords,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);


static GimpObjectClass *parent_class = NULL;

static gint global_tool_ID = 0;


GType
gimp_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_OBJECT,
					  "GimpTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_tool_class_init (GimpToolClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  klass->initialize     = gimp_tool_real_initialize;
  klass->control        = gimp_tool_real_control;
  klass->button_press   = gimp_tool_real_button_press;
  klass->button_release = gimp_tool_real_button_release;
  klass->motion         = gimp_tool_real_motion;
  klass->arrow_key      = gimp_tool_real_arrow_key;
  klass->modifier_key   = gimp_tool_real_modifier_key;
  klass->oper_update    = gimp_tool_real_oper_update;
  klass->cursor_update  = gimp_tool_real_cursor_update;
}

static void
gimp_tool_init (GimpTool *tool)
{
  tool->ID            = global_tool_ID++;

  tool->state         = INACTIVE;
  tool->paused_count  = 0;
  tool->scroll_lock   = FALSE;    /*  Allow scrolling  */
  tool->auto_snap_to  = TRUE;     /*  Snap to guides   */

  tool->preserve      = TRUE;     /*  Preserve tool across drawable changes  */
  tool->gdisp         = NULL;
  tool->drawable      = NULL;

  tool->tool_cursor   = GIMP_TOOL_CURSOR_NONE;
  tool->toggle_cursor = GIMP_TOOL_CURSOR_NONE;
  tool->toggled       = FALSE;
}

void
gimp_tool_initialize (GimpTool    *tool, 
		      GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  GIMP_TOOL_GET_CLASS (tool)->initialize (tool, gdisp);
}

void
gimp_tool_control (GimpTool    *tool, 
		   ToolAction   action, 
		   GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  GIMP_TOOL_GET_CLASS (tool)->control (tool, action, gdisp);
}

void
gimp_tool_button_press (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
			GdkModifierType  state,
			GimpDisplay     *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  GIMP_TOOL_GET_CLASS (tool)->button_press (tool, coords, time, state, gdisp);
}

void
gimp_tool_button_release (GimpTool        *tool,
                          GimpCoords      *coords,
                          guint32          time,
			  GdkModifierType  state,
			  GimpDisplay     *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  GIMP_TOOL_GET_CLASS (tool)->button_release (tool, coords, time, state, gdisp);
}

void
gimp_tool_motion (GimpTool        *tool,
                  GimpCoords      *coords,
                  guint32          time,
		  GdkModifierType  state,
		  GimpDisplay     *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  GIMP_TOOL_GET_CLASS (tool)->motion (tool, coords, time, state, gdisp);
}

void
gimp_tool_arrow_key (GimpTool    *tool,
		     GdkEventKey *kevent,
		     GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  GIMP_TOOL_GET_CLASS (tool)->arrow_key (tool, kevent, gdisp);
}

void
gimp_tool_modifier_key (GimpTool        *tool,
                        GdkModifierType  key,
                        gboolean         press,
			GdkModifierType  state,
			GimpDisplay     *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  GIMP_TOOL_GET_CLASS (tool)->modifier_key (tool, key, press, state, gdisp);
}

void
gimp_tool_oper_update (GimpTool        *tool,
                       GimpCoords      *coords,
                       GdkModifierType  state,
		       GimpDisplay     *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  GIMP_TOOL_GET_CLASS (tool)->oper_update (tool, coords, state, gdisp);
}

void
gimp_tool_cursor_update (GimpTool        *tool,
                         GimpCoords      *coords,
			 GdkModifierType  state,
			 GimpDisplay     *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  GIMP_TOOL_GET_CLASS (tool)->cursor_update (tool, coords, state, gdisp);
}


/*  standard member functions  */

static void
gimp_tool_real_initialize (GimpTool    *tool,
			   GimpDisplay *gdisp)
{
}

static void
gimp_tool_real_control (GimpTool    *tool,
			ToolAction   tool_action,
			GimpDisplay *gdisp)
{
}

static void
gimp_tool_real_button_press (GimpTool        *tool,
                             GimpCoords      *coords,
                             guint32          time,
			     GdkModifierType  state,
			     GimpDisplay     *gdisp)
{
  tool->gdisp    = gdisp;
  tool->drawable = gimp_image_active_drawable (gdisp->gimage);

  tool->state = ACTIVE;
}

static void
gimp_tool_real_button_release (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
			       GdkModifierType  state,
			       GimpDisplay     *gdisp)
{
  tool->state = INACTIVE;
}

static void
gimp_tool_real_motion (GimpTool        *tool,
                       GimpCoords      *coords,
                       guint32          time,
		       GdkModifierType  state,
		       GimpDisplay     *gdisp)
{
}

static void
gimp_tool_real_arrow_key (GimpTool    *tool,
			  GdkEventKey *kevent,
			  GimpDisplay *gdisp)
{
}

static void
gimp_tool_real_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
			     GdkModifierType  state,
			     GimpDisplay     *gdisp)
{
}

static void
gimp_tool_real_oper_update (GimpTool        *tool,
                            GimpCoords      *coords,
			    GdkModifierType  state,
			    GimpDisplay     *gdisp)
{
}

static void
gimp_tool_real_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_install_tool_cursor (shell,
                                          GIMP_MOUSE_CURSOR,
                                          tool->tool_cursor,
                                          GIMP_CURSOR_MODIFIER_NONE);
}


#define STUB(x) void * x (void){g_message ("stub function %s called",#x); return NULL;}

STUB(clone_non_gui)
STUB(clone_non_gui_default)
STUB(convolve_non_gui)
STUB(convolve_non_gui_default)
