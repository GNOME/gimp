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

#include "gimptool.h"
#include "tool_manager.h"

#include "app_procs.h"
#include "gdisplay.h"

#include "libgimp/gimpintl.h"


enum
{
  INITIALIZE,
  CONTROL,
  BUTTON_PRESS,
  BUTTON_RELEASE,
  MOTION,
  ARROW_KEY,
  MODIFIER_KEY,
  CURSOR_UPDATE,
  OPER_UPDATE,
  LAST_SIGNAL
};


static void   gimp_tool_class_init          (GimpToolClass  *klass);
static void   gimp_tool_init                (GimpTool       *tool);

static void   gimp_tool_real_initialize     (GimpTool       *tool,
					     GDisplay       *gdisp);
static void   gimp_tool_real_control        (GimpTool       *tool,
					     ToolAction      tool_action,
					     GDisplay       *gdisp);
static void   gimp_tool_real_button_press   (GimpTool       *tool,
					     GdkEventButton *bevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_button_release (GimpTool       *tool,
					     GdkEventButton *bevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_motion         (GimpTool       *tool,
					     GdkEventMotion *mevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_arrow_key      (GimpTool       *tool,
					     GdkEventKey    *kevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_modifier_key   (GimpTool       *tool,
					     GdkEventKey    *kevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_cursor_update  (GimpTool       *tool,
					     GdkEventMotion *mevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_oper_update    (GimpTool       *tool,
					     GdkEventMotion *mevent,
					     GDisplay       *gdisp);


static guint gimp_tool_signals[LAST_SIGNAL] = { 0 };

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

  gimp_tool_signals[INITIALIZE] =
    g_signal_new ("initialize",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpToolClass, initialize),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER, 
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER); 

  gimp_tool_signals[CONTROL] =
    g_signal_new ("control",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpToolClass, control),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__INT_POINTER, 
		  G_TYPE_NONE, 2,
		  G_TYPE_INT,
		  G_TYPE_POINTER); 

  gimp_tool_signals[BUTTON_PRESS] =
    g_signal_new ("button_press",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpToolClass, button_press),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__POINTER_POINTER,
		  G_TYPE_NONE, 2,
		  G_TYPE_POINTER,
		  G_TYPE_POINTER);

  gimp_tool_signals[BUTTON_RELEASE] =
    g_signal_new ("button_release",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpToolClass, button_release),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__POINTER_POINTER, 
		  G_TYPE_NONE, 2,
		  G_TYPE_POINTER,
		  G_TYPE_POINTER); 

  gimp_tool_signals[MOTION] =
    g_signal_new ("motion",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpToolClass, motion),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__POINTER_POINTER, 
		  G_TYPE_NONE, 2,
		  G_TYPE_POINTER,
		  G_TYPE_POINTER); 

  gimp_tool_signals[ARROW_KEY] =
    g_signal_new ("arrow_key",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpToolClass, arrow_key),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__POINTER_POINTER, 
		  G_TYPE_NONE, 2,
		  G_TYPE_POINTER,
		  G_TYPE_POINTER); 

  gimp_tool_signals[MODIFIER_KEY] =
    g_signal_new ("modifier_key",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpToolClass, modifier_key),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__POINTER_POINTER, 
		  G_TYPE_NONE, 2,
		  G_TYPE_POINTER,
		  G_TYPE_POINTER); 

  gimp_tool_signals[CURSOR_UPDATE] =
    g_signal_new ("cursor_update",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpToolClass, cursor_update),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__POINTER_POINTER, 
		  G_TYPE_NONE, 2,
		  G_TYPE_POINTER,
		  G_TYPE_POINTER);

  gimp_tool_signals[OPER_UPDATE] =
    g_signal_new ("oper_update",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpToolClass, oper_update),
		  NULL, NULL,
		  gimp_cclosure_marshal_VOID__POINTER_POINTER, 
		  G_TYPE_NONE, 2,
		  G_TYPE_POINTER,
		  G_TYPE_POINTER); 

  klass->initialize     = gimp_tool_real_initialize;
  klass->control        = gimp_tool_real_control;
  klass->button_press   = gimp_tool_real_button_press;
  klass->button_release = gimp_tool_real_button_release;
  klass->motion         = gimp_tool_real_motion;
  klass->arrow_key      = gimp_tool_real_arrow_key;
  klass->modifier_key   = gimp_tool_real_modifier_key;
  klass->cursor_update  = gimp_tool_real_cursor_update;
  klass->oper_update    = gimp_tool_real_oper_update;
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
gimp_tool_initialize (GimpTool   *tool, 
		      GDisplay   *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_signal_emit (G_OBJECT (tool), gimp_tool_signals[INITIALIZE], 0,
                 gdisp);
}

void
gimp_tool_control (GimpTool   *tool, 
		   ToolAction  action, 
		   GDisplay   *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_signal_emit (G_OBJECT (tool), gimp_tool_signals[CONTROL], 0,
                 action, gdisp);
}

void
gimp_tool_button_press (GimpTool       *tool,
			GdkEventButton *bevent,
			GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_signal_emit (G_OBJECT (tool), gimp_tool_signals[BUTTON_PRESS], 0,
                 bevent, gdisp); 
}

void
gimp_tool_button_release (GimpTool       *tool,
			  GdkEventButton *bevent,
			  GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_signal_emit (G_OBJECT (tool), gimp_tool_signals[BUTTON_RELEASE], 0,
                 bevent, gdisp);
}

void
gimp_tool_motion (GimpTool       *tool,
		  GdkEventMotion *mevent,
		  GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_signal_emit (G_OBJECT (tool), gimp_tool_signals[MOTION], 0,
                 mevent, gdisp);
}

void
gimp_tool_arrow_key (GimpTool    *tool,
		     GdkEventKey *kevent,
		     GDisplay    *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_signal_emit (G_OBJECT (tool), gimp_tool_signals[ARROW_KEY], 0,
                 kevent, gdisp);
}

void
gimp_tool_modifier_key (GimpTool    *tool,
			GdkEventKey *kevent,
			GDisplay    *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_signal_emit (G_OBJECT (tool), gimp_tool_signals[MODIFIER_KEY], 0,
                 kevent, gdisp);  
}

void
gimp_tool_cursor_update (GimpTool       *tool,
			 GdkEventMotion *mevent,
			 GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_signal_emit (G_OBJECT (tool), gimp_tool_signals[CURSOR_UPDATE], 0,
                 mevent, gdisp);
}

void
gimp_tool_oper_update (GimpTool       *tool,
		       GdkEventMotion *mevent,
		       GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_signal_emit (G_OBJECT (tool), gimp_tool_signals[OPER_UPDATE], 0,
                 mevent, gdisp);
}


/*  standard member functions  */

static void
gimp_tool_real_initialize (GimpTool       *tool,
			   GDisplay       *gdisp)
{
}

static void
gimp_tool_real_control (GimpTool   *tool,
			ToolAction  tool_action,
			GDisplay   *gdisp)
{
}

static void
gimp_tool_real_button_press (GimpTool       *tool,
			     GdkEventButton *bevent,
			     GDisplay       *gdisp)
{
  tool->gdisp    = gdisp;
  tool->drawable = gimp_image_active_drawable (gdisp->gimage);

  tool->state = ACTIVE;
}

static void
gimp_tool_real_button_release (GimpTool       *tool,
			       GdkEventButton *bevent,
			       GDisplay       *gdisp)
{
  tool->state = INACTIVE;
}

static void
gimp_tool_real_motion (GimpTool       *tool,
		       GdkEventMotion *mevent,
		       GDisplay       *gdisp)
{
}

static void
gimp_tool_real_arrow_key (GimpTool    *tool,
			  GdkEventKey *kevent,
			  GDisplay    *gdisp)
{
}

static void
gimp_tool_real_modifier_key (GimpTool    *tool,
			     GdkEventKey *kevent,
			     GDisplay    *gdisp)
{
}

static void
gimp_tool_real_cursor_update (GimpTool       *tool,
			      GdkEventMotion *mevent,
			      GDisplay       *gdisp)
{
  gdisplay_install_tool_cursor (gdisp,
                                GDK_TOP_LEFT_ARROW,
				tool->tool_cursor,
				GIMP_CURSOR_MODIFIER_NONE);
}

static void
gimp_tool_real_oper_update (GimpTool       *tool,
			    GdkEventMotion *mevent,
			    GDisplay       *gdisp)
{
}





/*  Function definitions  */

void
gimp_tool_help_func (const gchar *help_data)
{
  gimp_standard_help_func (tool_manager_active_get_help_data (the_gimp));
}


#define STUB(x) void * x (void){g_message ("stub function %s called",#x); return NULL;}

STUB(clone_non_gui)
STUB(clone_non_gui_default)
STUB(convolve_non_gui)
STUB(convolve_non_gui_default)
