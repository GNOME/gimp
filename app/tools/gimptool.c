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

static void   gimp_tool_destroy             (GtkObject      *destroy);

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


GtkType
gimp_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpTool",
        sizeof (GimpTool),
        sizeof (GimpToolClass),
        (GtkClassInitFunc) gimp_tool_class_init,
        (GtkObjectInitFunc) gimp_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_OBJECT, &tool_info);
    }

  return tool_type;
}

static void
gimp_tool_class_init (GimpToolClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);

  gimp_tool_signals[INITIALIZE] =
    gtk_signal_new ("initialize",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       initialize),
		    gtk_marshal_NONE__POINTER, 
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[CONTROL] =
    gtk_signal_new ("control",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       control),
		    gtk_marshal_NONE__INT_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_INT,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[BUTTON_PRESS] =
    gtk_signal_new ("button_press",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       button_press),
		    gtk_marshal_NONE__POINTER_POINTER,
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER);

  gimp_tool_signals[BUTTON_RELEASE] =
    gtk_signal_new ("button_release",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       button_release),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[MOTION] =
    gtk_signal_new ("motion",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       motion),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[ARROW_KEY] =
    gtk_signal_new ("arrow_key",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       arrow_key),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[MODIFIER_KEY] =
    gtk_signal_new ("modifier_key",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       modifier_key),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[CURSOR_UPDATE] =
    gtk_signal_new ("cursor_update",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       cursor_update),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER);

  gimp_tool_signals[OPER_UPDATE] =
    gtk_signal_new ("oper_update",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       oper_update),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gtk_object_class_add_signals (object_class, gimp_tool_signals, LAST_SIGNAL);  

  object_class->destroy = gimp_tool_destroy;

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

static void
gimp_tool_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

void
gimp_tool_initialize (GimpTool   *tool, 
		      GDisplay   *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[INITIALIZE],
		   gdisp);
}

void
gimp_tool_control (GimpTool   *tool, 
		   ToolAction  action, 
		   GDisplay   *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[CONTROL],
		   action, gdisp);
}

void
gimp_tool_button_press (GimpTool       *tool,
			GdkEventButton *bevent,
			GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[BUTTON_PRESS],
		   bevent, gdisp); 
}

void
gimp_tool_button_release (GimpTool       *tool,
			  GdkEventButton *bevent,
			  GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[BUTTON_RELEASE],
		   bevent, gdisp);
}

void
gimp_tool_motion (GimpTool       *tool,
		  GdkEventMotion *mevent,
		  GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[MOTION],
		   mevent, gdisp);
}

void
gimp_tool_arrow_key (GimpTool    *tool,
		     GdkEventKey *kevent,
		     GDisplay    *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[ARROW_KEY],
		   kevent, gdisp);
}

void
gimp_tool_modifier_key (GimpTool    *tool,
			GdkEventKey *kevent,
			GDisplay    *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[MODIFIER_KEY],
		   kevent, gdisp);  
}

void
gimp_tool_cursor_update (GimpTool       *tool,
			 GdkEventMotion *mevent,
			 GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[CURSOR_UPDATE],
		   mevent, gdisp);
}

void
gimp_tool_oper_update (GimpTool       *tool,
		       GdkEventMotion *mevent,
		       GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[OPER_UPDATE],
		   mevent, gdisp);
}


const gchar *
gimp_tool_get_PDB_string (GimpTool *tool)
{
  GtkObject     *object;
  GimpToolClass *klass;

  g_return_val_if_fail (tool, NULL);
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);
  
  object = GTK_OBJECT (tool);

  klass = GIMP_TOOL_CLASS (object->klass);

  return klass->pdb_string;
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
