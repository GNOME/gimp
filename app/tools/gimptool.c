/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis and others
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

#include "tools-types.h"

#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-cursor.h"
#include "display/gimpstatusbar.h"

#include "gimptool.h"
#include "gimptoolcontrol.h"


enum
{
  PROP_0,
  PROP_TOOL_INFO
};


static void     gimp_tool_class_init          (GimpToolClass   *klass);
static void     gimp_tool_init                (GimpTool        *tool);

static void     gimp_tool_finalize            (GObject         *object);
static void     gimp_tool_set_property        (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void     gimp_tool_get_property        (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);

static gboolean gimp_tool_real_initialize     (GimpTool        *tool,
                                               GimpDisplay     *gdisp);
static void     gimp_tool_real_control        (GimpTool        *tool,
                                               GimpToolAction   action,
                                               GimpDisplay     *gdisp);
static void     gimp_tool_real_button_press   (GimpTool        *tool,
                                               GimpCoords      *coords,
                                               guint32          time,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);
static void     gimp_tool_real_button_release (GimpTool        *tool,
                                               GimpCoords      *coords,
                                               guint32          time,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);
static void     gimp_tool_real_motion         (GimpTool        *tool,
                                               GimpCoords      *coords,
                                               guint32          time,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);
static gboolean gimp_tool_real_key_press      (GimpTool        *tool,
                                               GdkEventKey     *kevent,
                                               GimpDisplay     *gdisp);
static void     gimp_tool_real_modifier_key   (GimpTool        *tool,
                                               GdkModifierType  key,
                                               gboolean         press,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);
static void     gimp_tool_real_oper_update    (GimpTool        *tool,
                                               GimpCoords      *coords,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);
static void     gimp_tool_real_cursor_update  (GimpTool        *tool,
                                               GimpCoords      *coords,
                                               GdkModifierType  state,
                                               GimpDisplay     *gdisp);


static GimpObjectClass *parent_class = NULL;

static gint global_tool_ID = 1;


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
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_tool_finalize;
  object_class->set_property = gimp_tool_set_property;
  object_class->get_property = gimp_tool_get_property;

  klass->initialize     = gimp_tool_real_initialize;
  klass->control        = gimp_tool_real_control;
  klass->button_press   = gimp_tool_real_button_press;
  klass->button_release = gimp_tool_real_button_release;
  klass->motion         = gimp_tool_real_motion;
  klass->key_press      = gimp_tool_real_key_press;
  klass->modifier_key   = gimp_tool_real_modifier_key;
  klass->oper_update    = gimp_tool_real_oper_update;
  klass->cursor_update  = gimp_tool_real_cursor_update;

  g_object_class_install_property (object_class, PROP_TOOL_INFO,
                                   g_param_spec_object ("tool-info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TOOL_INFO,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

}

static void
gimp_tool_init (GimpTool *tool)
{
  tool->tool_info      = NULL;
  tool->ID             = global_tool_ID++;
  tool->control        = g_object_new (GIMP_TYPE_TOOL_CONTROL, NULL);
  tool->gdisp          = NULL;
  tool->drawable       = NULL;
  tool->focus_display  = NULL;
  tool->modifier_state = 0;
}

static void
gimp_tool_finalize (GObject *object)
{
  GimpTool *tool = GIMP_TOOL (object);

  if (tool->tool_info)
    {
      g_object_unref (tool->tool_info);
      tool->tool_info = NULL;
    }

  if (tool->control)
    {
      g_object_unref (tool->control);
      tool->control = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpTool *tool = GIMP_TOOL (object);

  switch (property_id)
    {
    case PROP_TOOL_INFO:
      tool->tool_info = GIMP_TOOL_INFO (g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpTool *tool = GIMP_TOOL (object);

  switch (property_id)
    {
    case PROP_TOOL_INFO:
      g_value_set_object (value, tool->tool_info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  standard member functions  */

static gboolean
gimp_tool_real_initialize (GimpTool    *tool,
                           GimpDisplay *gdisp)
{
  return TRUE;
}

static void
gimp_tool_real_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *gdisp)
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

  gimp_tool_control_activate (tool->control);
}

static void
gimp_tool_real_button_release (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
                               GdkModifierType  state,
                               GimpDisplay     *gdisp)
{
  gimp_tool_control_halt (tool->control);
}

static void
gimp_tool_real_motion (GimpTool        *tool,
                       GimpCoords      *coords,
                       guint32          time,
                       GdkModifierType  state,
                       GimpDisplay     *gdisp)
{
}

static gboolean
gimp_tool_real_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *gdisp)
{
  return FALSE;
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


/*  public functions  */

gboolean
gimp_tool_initialize (GimpTool    *tool,
                      GimpDisplay *gdisp)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), FALSE);

  return GIMP_TOOL_GET_CLASS (tool)->initialize (tool, gdisp);
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
      if (! gimp_tool_control_is_paused (tool->control))
        GIMP_TOOL_GET_CLASS (tool)->control (tool, action, gdisp);

      gimp_tool_control_pause (tool->control);
      break;

    case RESUME:
      if (gimp_tool_control_is_paused (tool->control))
        {
          gimp_tool_control_resume (tool->control);

          if (! gimp_tool_control_is_paused (tool->control))
            GIMP_TOOL_GET_CLASS (tool)->control (tool, action, gdisp);
        }
      else
        {
          g_warning ("gimp_tool_control: unable to RESUME tool with "
                     "tool->control->paused_count == 0");
        }
      break;

    case HALT:
      GIMP_TOOL_GET_CLASS (tool)->control (tool, action, gdisp);

      if (gimp_tool_control_is_active (tool->control))
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
  g_return_if_fail (gimp_tool_control_is_active (tool->control));

  GIMP_TOOL_GET_CLASS (tool)->motion (tool, coords, time, state, gdisp);
}

void
gimp_tool_set_focus_display (GimpTool    *tool,
                             GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (gdisp == NULL || GIMP_IS_DISPLAY (gdisp));

#ifdef DEBUG_FOCUS
  g_printerr ("gimp_tool_set_focus_display: gdisp: %p  focus_display: %p\n",
              gdisp, tool->focus_display);
#endif

  if (gdisp != tool->focus_display)
    {
      if (tool->focus_display)
        {
          if (tool->modifier_state != 0)
            gimp_tool_set_modifier_state (tool, 0, tool->focus_display);
        }

      tool->focus_display = gdisp;
    }
}

gboolean
gimp_tool_key_press (GimpTool    *tool,
                     GdkEventKey *kevent,
                     GimpDisplay *gdisp)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), FALSE);
  g_return_val_if_fail (gdisp == tool->focus_display, FALSE);

  return GIMP_TOOL_GET_CLASS (tool)->key_press (tool, kevent, gdisp);
}

static void
gimp_tool_modifier_key (GimpTool        *tool,
                        GdkModifierType  key,
                        gboolean         press,
                        GdkModifierType  state,
                        GimpDisplay     *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (gdisp == tool->focus_display);

  GIMP_TOOL_GET_CLASS (tool)->modifier_key (tool, key, press, state, gdisp);
}

void
gimp_tool_set_modifier_state (GimpTool        *tool,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

#ifdef DEBUG_FOCUS
  g_printerr ("gimp_tool_set_modifier_state: gdisp: %p  focus_display: %p\n",
              gdisp, tool->focus_display);
#endif

  g_return_if_fail (gdisp == tool->focus_display);

  if ((tool->modifier_state & GDK_SHIFT_MASK) != (state & GDK_SHIFT_MASK))
    {
      gimp_tool_modifier_key (tool, GDK_SHIFT_MASK,
                              (state & GDK_SHIFT_MASK) ? TRUE : FALSE, state,
                              gdisp);
    }

  if ((tool->modifier_state & GDK_CONTROL_MASK) != (state & GDK_CONTROL_MASK))
    {
      gimp_tool_modifier_key (tool, GDK_CONTROL_MASK,
                              (state & GDK_CONTROL_MASK) ? TRUE : FALSE, state,
                              gdisp);
    }

  if ((tool->modifier_state & GDK_MOD1_MASK) != (state & GDK_MOD1_MASK))
    {
      gimp_tool_modifier_key (tool, GDK_MOD1_MASK,
                              (state & GDK_MOD1_MASK) ? TRUE : FALSE, state,
                              gdisp);
    }

  tool->modifier_state = state;
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

void
gimp_tool_push_status (GimpTool    *tool,
                       const gchar *message)
{
  GimpStatusbar *statusbar;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (tool->gdisp));
  g_return_if_fail (message != NULL);

  statusbar =
    GIMP_STATUSBAR (GIMP_DISPLAY_SHELL (tool->gdisp->shell)->statusbar);

  gimp_statusbar_push (statusbar, G_OBJECT_TYPE_NAME (tool), message);
}

void
gimp_tool_push_status_coords (GimpTool    *tool,
                              const gchar *title,
                              gdouble      x,
                              const gchar *separator,
                              gdouble      y)
{
  GimpStatusbar *statusbar;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (tool->gdisp));
  g_return_if_fail (title != NULL);
  g_return_if_fail (separator != NULL);

  statusbar =
    GIMP_STATUSBAR (GIMP_DISPLAY_SHELL (tool->gdisp->shell)->statusbar);

  gimp_statusbar_push_coords (statusbar, G_OBJECT_TYPE_NAME (tool),
                              title, x, separator, y);
}

void
gimp_tool_push_status_length (GimpTool            *tool,
                              const gchar         *title,
                              GimpOrientationType  axis,
                              gdouble              value)
{
  GimpStatusbar *statusbar;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (tool->gdisp));
  g_return_if_fail (title != NULL);

  statusbar =
    GIMP_STATUSBAR (GIMP_DISPLAY_SHELL (tool->gdisp->shell)->statusbar);

  gimp_statusbar_push_length (statusbar, G_OBJECT_TYPE_NAME (tool),
                              title, axis, value);
}

void
gimp_tool_pop_status (GimpTool *tool)
{
  GimpStatusbar *statusbar;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (tool->gdisp));

  statusbar =
    GIMP_STATUSBAR (GIMP_DISPLAY_SHELL (tool->gdisp->shell)->statusbar);

  gimp_statusbar_pop (statusbar, G_OBJECT_TYPE_NAME (tool));
}

void
gimp_tool_set_cursor (GimpTool           *tool,
                      GimpDisplay        *gdisp,
                      GimpCursorType      cursor,
                      GimpToolCursorType  tool_cursor,
                      GimpCursorModifier  modifier)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  gimp_display_shell_set_cursor (GIMP_DISPLAY_SHELL (gdisp->shell),
                                 cursor, tool_cursor, modifier);
}
