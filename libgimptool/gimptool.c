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

#include "libgimpproxy/gimpproxytypes.h"
#include "gimptooltypes.h"

/* this is ugly */

/*#include "core/gimpimage.h"
#include "core/gimpmarshal.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpstatusbar.h" */

#include "gimptool.h"
#include "gimptoolcontrol.h"


static void   gimp_tool_class_init          (GimpToolClass   *klass);
static void   gimp_tool_init                (GimpTool        *tool);

static void   gimp_tool_real_initialize     (GimpTool        *tool,
					     GimpDisplay     *gdisp);
static void   gimp_tool_real_control        (GimpTool        *tool,
					     GimpToolAction   action,
					     GimpDisplay     *gdisp);
void          gimp_tool_real_button_press   (GimpTool        *tool,
                                             GimpCoords      *coords,
                                             guint32          time,
					     GdkModifierType  state,
					     GimpDisplay     *gdisp);
void          gimp_tool_real_button_release (GimpTool        *tool,
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
  tool->ID       = global_tool_ID++;
  tool->control  = GIMP_TOOL_CONTROL (g_object_new (GIMP_TYPE_TOOL_CONTROL, 
                                                    NULL)); 
  tool->gdisp    = NULL;
  tool->drawable = NULL;
}

void
gimp_tool_initialize (GimpTool    *tool, 
		      GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  GIMP_TOOL_GET_CLASS (tool)->initialize (tool, gdisp);
}

void
gimp_tool_control (GimpTool       *tool, 
		   GimpToolAction  action, 
		   GimpDisplay    *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  switch (action)
    {
    case PAUSE:
      if (!gimp_tool_control_is_paused (tool->control))
        {
          GIMP_TOOL_GET_CLASS (tool)->control (tool, action, gdisp);
        }

      gimp_tool_control_pause (tool->control);
      break;

    case RESUME:
      if (gimp_tool_control_is_paused (tool->control))
        {
          gimp_tool_control_resume (tool->control);

          if (!gimp_tool_control_is_paused (tool->control))
            {
              GIMP_TOOL_GET_CLASS (tool)->control (tool, action, gdisp);
            }
        }
      else
        {
          g_warning ("gimp_tool_control(): "
                     "unable to RESUME tool with tool->paused_count == 0");
        }
      break;

    case HALT:
      GIMP_TOOL_GET_CLASS (tool)->control (tool, action, gdisp);

      gimp_tool_control_halt (tool->control);
      break;
    }
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

  /* FIXME */
  /*g_return_if_fail (GIMP_IS_DISPLAY (gdisp));*/

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
  
  /* FIXME */
  /*g_return_if_fail (GIMP_IS_DISPLAY (gdisp));*/

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
  /* FIXME */
  /*g_return_if_fail (GIMP_IS_DISPLAY (gdisp));*/

  GIMP_TOOL_GET_CLASS (tool)->motion (tool, coords, time, state, gdisp);
}

void
gimp_tool_arrow_key (GimpTool    *tool,
		     GdkEventKey *kevent,
		     GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  /* FIXME */
  /* g_return_if_fail (GIMP_IS_DISPLAY (gdisp)); */

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
  /* FIXME */
  /* g_return_if_fail (GIMP_IS_DISPLAY (gdisp)); */

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
  /* FIXME*/
  /*g_return_if_fail (GIMP_IS_DISPLAY (gdisp));*/

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
  /* FIXME */
  /* g_return_if_fail (GIMP_IS_DISPLAY (gdisp)); */

  GIMP_TOOL_GET_CLASS (tool)->cursor_update (tool, coords, state, gdisp);
}



/*  standard member functions  */

static void
gimp_tool_real_initialize (GimpTool    *tool,
			   GimpDisplay *gdisp)
{
}

static void
gimp_tool_real_control (GimpTool       *tool,
			GimpToolAction  action,
			GimpDisplay    *gdisp)
{
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
  if (gimp_tool_control_is_toggled (tool->control))
    {
      gimp_tool_set_cursor (tool, gdisp,
                            gimp_tool_control_get_toggle_cursor (tool->control),
                            gimp_tool_control_get_toggle_tool_cursor (tool->control),
                            gimp_tool_control_get_toggle_cursor_modifier (tool->control));
    }
  else
    {
      gimp_tool_set_cursor (tool, gdisp,
                            gimp_tool_control_get_cursor (tool->control),
                            gimp_tool_control_get_tool_cursor (tool->control),
                            gimp_tool_control_get_cursor_modifier (tool->control));
    }
}
