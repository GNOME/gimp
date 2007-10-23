/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
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


static void       gimp_tool_finalize            (GObject               *object);
static void       gimp_tool_set_property        (GObject               *object,
                                                 guint                  property_id,
                                                 const GValue          *value,
                                                 GParamSpec            *pspec);
static void       gimp_tool_get_property        (GObject               *object,
                                                 guint                  property_id,
                                                 GValue                *value,
                                                 GParamSpec            *pspec);

static gboolean   gimp_tool_real_has_display    (GimpTool              *tool,
                                                 GimpDisplay           *display);
static GimpDisplay * gimp_tool_real_has_image   (GimpTool              *tool,
                                                 GimpImage             *image);
static gboolean   gimp_tool_real_initialize     (GimpTool              *tool,
                                                 GimpDisplay           *display,
                                                 GError               **error);
static void       gimp_tool_real_control        (GimpTool              *tool,
                                                 GimpToolAction         action,
                                                 GimpDisplay           *display);
static void       gimp_tool_real_button_press   (GimpTool              *tool,
                                                 GimpCoords            *coords,
                                                 guint32                time,
                                                 GdkModifierType        state,
                                                 GimpDisplay           *display);
static void       gimp_tool_real_button_release (GimpTool              *tool,
                                                 GimpCoords            *coords,
                                                 guint32                time,
                                                 GdkModifierType        state,
                                                 GimpButtonReleaseType  release_type,
                                                 GimpDisplay           *display);
static void       gimp_tool_real_motion         (GimpTool              *tool,
                                                 GimpCoords            *coords,
                                                 guint32                time,
                                                 GdkModifierType        state,
                                                 GimpDisplay           *display);
static gboolean   gimp_tool_real_key_press      (GimpTool              *tool,
                                                 GdkEventKey           *kevent,
                                                 GimpDisplay           *display);
static void       gimp_tool_real_modifier_key   (GimpTool              *tool,
                                                 GdkModifierType        key,
                                                 gboolean               press,
                                                 GdkModifierType        state,
                                                 GimpDisplay           *display);
static void  gimp_tool_real_active_modifier_key (GimpTool              *tool,
                                                 GdkModifierType        key,
                                                 gboolean               press,
                                                 GdkModifierType        state,
                                                 GimpDisplay           *display);
static void       gimp_tool_real_oper_update    (GimpTool              *tool,
                                                 GimpCoords            *coords,
                                                 GdkModifierType        state,
                                                 gboolean               proximity,
                                                 GimpDisplay           *display);
static void       gimp_tool_real_cursor_update  (GimpTool              *tool,
                                                 GimpCoords            *coords,
                                                 GdkModifierType        state,
                                                 GimpDisplay           *display);

static void       gimp_tool_clear_status        (GimpTool              *tool);


G_DEFINE_TYPE (GimpTool, gimp_tool, GIMP_TYPE_OBJECT)

#define parent_class gimp_tool_parent_class

static gint global_tool_ID = 1;


static void
gimp_tool_class_init (GimpToolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_tool_finalize;
  object_class->set_property = gimp_tool_set_property;
  object_class->get_property = gimp_tool_get_property;

  klass->has_display         = gimp_tool_real_has_display;
  klass->has_image           = gimp_tool_real_has_image;
  klass->initialize          = gimp_tool_real_initialize;
  klass->control             = gimp_tool_real_control;
  klass->button_press        = gimp_tool_real_button_press;
  klass->button_release      = gimp_tool_real_button_release;
  klass->motion              = gimp_tool_real_motion;
  klass->key_press           = gimp_tool_real_key_press;
  klass->modifier_key        = gimp_tool_real_modifier_key;
  klass->active_modifier_key = gimp_tool_real_active_modifier_key;
  klass->oper_update         = gimp_tool_real_oper_update;
  klass->cursor_update       = gimp_tool_real_cursor_update;

  g_object_class_install_property (object_class, PROP_TOOL_INFO,
                                   g_param_spec_object ("tool-info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TOOL_INFO,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_tool_init (GimpTool *tool)
{
  tool->tool_info             = NULL;
  tool->ID                    = global_tool_ID++;
  tool->control               = g_object_new (GIMP_TYPE_TOOL_CONTROL, NULL);
  tool->display               = NULL;
  tool->drawable              = NULL;
  tool->focus_display         = NULL;
  tool->modifier_state        = 0;
  tool->active_modifier_state = 0;
  tool->button_press_state    = 0;
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

  if (tool->status_displays)
    {
      g_list_free (tool->status_displays);
      tool->status_displays = NULL;
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
gimp_tool_real_has_display (GimpTool    *tool,
                            GimpDisplay *display)
{
  return (display == tool->display ||
          g_list_find (tool->status_displays, display));
}

static GimpDisplay *
gimp_tool_real_has_image (GimpTool  *tool,
                          GimpImage *image)
{
  if (tool->display)
    {
      if (image && tool->display->image == image)
        return tool->display;

      /*  NULL image means any display  */
      if (! image)
        return tool->display;
    }

  return NULL;
}

static gboolean
gimp_tool_real_initialize (GimpTool     *tool,
                           GimpDisplay  *display,
                           GError      **error)
{
  return TRUE;
}

static void
gimp_tool_real_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      tool->display = NULL;
      break;
    }
}

static void
gimp_tool_real_button_press (GimpTool        *tool,
                             GimpCoords      *coords,
                             guint32          time,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  tool->display  = display;
  tool->drawable = gimp_image_get_active_drawable (display->image);

  gimp_tool_control_activate (tool->control);
}

static void
gimp_tool_real_button_release (GimpTool              *tool,
                               GimpCoords            *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  gimp_tool_control_halt (tool->control);
}

static void
gimp_tool_real_motion (GimpTool        *tool,
                       GimpCoords      *coords,
                       guint32          time,
                       GdkModifierType  state,
                       GimpDisplay     *display)
{
}

static gboolean
gimp_tool_real_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  return FALSE;
}

static void
gimp_tool_real_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
}

static void
gimp_tool_real_active_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
}

static void
gimp_tool_real_oper_update (GimpTool        *tool,
                            GimpCoords      *coords,
                            GdkModifierType  state,
                            gboolean         proximity,
                            GimpDisplay     *display)
{
}

static void
gimp_tool_real_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  gimp_tool_set_cursor (tool, display,
                        gimp_tool_control_get_cursor (tool->control),
                        gimp_tool_control_get_tool_cursor (tool->control),
                        gimp_tool_control_get_cursor_modifier (tool->control));
}


/*  public functions  */

GimpToolOptions *
gimp_tool_get_options (GimpTool *tool)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);
  g_return_val_if_fail (GIMP_IS_TOOL_INFO (tool->tool_info), NULL);

  return tool->tool_info->tool_options;
}

gboolean
gimp_tool_has_display (GimpTool    *tool,
                       GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  return GIMP_TOOL_GET_CLASS (tool)->has_display (tool, display);
}

GimpDisplay *
gimp_tool_has_image (GimpTool  *tool,
                     GimpImage *image)
{
  GimpDisplay *display;

  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);
  g_return_val_if_fail (image == NULL || GIMP_IS_IMAGE (image), NULL);

  display = GIMP_TOOL_GET_CLASS (tool)->has_image (tool, image);

  /*  check status displays last because they don't affect the tool
   *  itself (unlike tool->display or draw_tool->display)
   */
  if (! display && tool->status_displays)
    {
      GList *list;

      for (list = tool->status_displays; list; list = g_list_next (list))
        {
          GimpDisplay *status_display = list->data;

          if (status_display->image == image)
            return status_display;
        }

      /*  NULL image means any display  */
      if (! image)
        return tool->status_displays->data;
    }

  return display;
}

gboolean
gimp_tool_initialize (GimpTool    *tool,
                      GimpDisplay *display)
{
  GError *error = NULL;

  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  if (! GIMP_TOOL_GET_CLASS (tool)->initialize (tool, display, &error))
    {
      gimp_tool_message (tool, display, error->message);
      g_clear_error (&error);
      return FALSE;
    }

  return TRUE;
}

void
gimp_tool_control (GimpTool       *tool,
                   GimpToolAction  action,
                   GimpDisplay    *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      if (! gimp_tool_control_is_paused (tool->control))
        GIMP_TOOL_GET_CLASS (tool)->control (tool, action, display);

      gimp_tool_control_pause (tool->control);
      break;

    case GIMP_TOOL_ACTION_RESUME:
      if (gimp_tool_control_is_paused (tool->control))
        {
          gimp_tool_control_resume (tool->control);

          if (! gimp_tool_control_is_paused (tool->control))
            GIMP_TOOL_GET_CLASS (tool)->control (tool, action, display);
        }
      else
        {
          g_warning ("gimp_tool_control: unable to RESUME tool with "
                     "tool->control->paused_count == 0");
        }
      break;

    case GIMP_TOOL_ACTION_HALT:
      GIMP_TOOL_GET_CLASS (tool)->control (tool, action, display);

      if (gimp_tool_control_is_active (tool->control))
        gimp_tool_control_halt (tool->control);

      gimp_tool_clear_status (tool);
      break;
    }
}

void
gimp_tool_button_press (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  GIMP_TOOL_GET_CLASS (tool)->button_press (tool, coords, time, state,
                                            display);

  if (gimp_tool_control_is_active (tool->control))
    {
      tool->button_press_state    = state;
      tool->active_modifier_state = state;

      if (gimp_tool_control_get_wants_click (tool->control))
        {
          tool->in_click_distance   = TRUE;
          tool->got_motion_event    = FALSE;
          tool->button_press_coords = *coords;
          tool->button_press_time   = time;
        }
      else
        {
          tool->in_click_distance   = FALSE;
        }
    }
}

static gboolean
gimp_tool_check_click_distance (GimpTool    *tool,
                                GimpCoords  *coords,
                                guint32      time,
                                GimpDisplay *display)
{
  gint double_click_time;
  gint double_click_distance;

  if (! tool->in_click_distance)
    return FALSE;

  g_object_get (gtk_widget_get_settings (display->shell),
                "gtk-double-click-time",     &double_click_time,
                "gtk-double-click-distance", &double_click_distance,
                NULL);

  if ((time - tool->button_press_time) > double_click_time)
    {
      tool->in_click_distance = FALSE;
    }
  else
    {
      GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (display->shell);
      gdouble           dx;
      gdouble           dy;

      dx = SCALEX (shell, tool->button_press_coords.x - coords->x);
      dy = SCALEY (shell, tool->button_press_coords.y - coords->y);

      if ((SQR (dx) + SQR (dy)) > SQR (double_click_distance))
        {
          tool->in_click_distance = FALSE;
        }
    }

  return tool->in_click_distance;
}

void
gimp_tool_button_release (GimpTool        *tool,
                          GimpCoords      *coords,
                          guint32          time,
                          GdkModifierType  state,
                          GimpDisplay     *display)
{
  GimpButtonReleaseType release_type = GIMP_BUTTON_RELEASE_NORMAL;
  GimpCoords            my_coords;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  g_object_ref (tool);

  my_coords = *coords;

  if (state & GDK_BUTTON3_MASK)
    {
      release_type = GIMP_BUTTON_RELEASE_CANCEL;
    }
  else if (gimp_tool_control_get_wants_click (tool->control))
    {
      if (gimp_tool_check_click_distance (tool, coords, time, display))
        {
          release_type = GIMP_BUTTON_RELEASE_CLICK;
          my_coords    = tool->button_press_coords;

          /*  synthesize a motion event back to the recorded press
           *  coordinates
           */
          GIMP_TOOL_GET_CLASS (tool)->motion (tool, &my_coords, time,
                                              state & GDK_BUTTON1_MASK,
                                              display);
        }
      else if (! tool->got_motion_event)
        {
          release_type = GIMP_BUTTON_RELEASE_NO_MOTION;
        }
    }

  GIMP_TOOL_GET_CLASS (tool)->button_release (tool, &my_coords, time, state,
                                              release_type, display);

  if (tool->active_modifier_state != 0)
    gimp_tool_set_active_modifier_state (tool, 0, display);

  tool->button_press_state = 0;

  g_object_unref (tool);
}

void
gimp_tool_motion (GimpTool        *tool,
                  GimpCoords      *coords,
                  guint32          time,
                  GdkModifierType  state,
                  GimpDisplay     *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (gimp_tool_control_is_active (tool->control));

  tool->got_motion_event = TRUE;
  gimp_tool_check_click_distance (tool, coords, time, display);

  GIMP_TOOL_GET_CLASS (tool)->motion (tool, coords, time, state, display);
}

void
gimp_tool_set_focus_display (GimpTool    *tool,
                             GimpDisplay *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (display == NULL || GIMP_IS_DISPLAY (display));

#ifdef DEBUG_FOCUS
  g_printerr ("%s: tool: %p  display: %p  focus_display: %p\n",
              G_STRFUNC, tool, display, tool->focus_display);
#endif

  if (display != tool->focus_display)
    {
      if (tool->focus_display)
        {
          if (tool->active_modifier_state != 0)
            gimp_tool_set_active_modifier_state (tool, 0, tool->focus_display);

          if (tool->modifier_state != 0)
            gimp_tool_set_modifier_state (tool, 0, tool->focus_display);
        }

      tool->focus_display = display;
    }
}

gboolean
gimp_tool_key_press (GimpTool    *tool,
                     GdkEventKey *kevent,
                     GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (display == tool->focus_display, FALSE);

  return GIMP_TOOL_GET_CLASS (tool)->key_press (tool, kevent, display);
}

static void
gimp_tool_modifier_key (GimpTool        *tool,
                        GdkModifierType  key,
                        gboolean         press,
                        GdkModifierType  state,
                        GimpDisplay     *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (display == tool->focus_display);

  GIMP_TOOL_GET_CLASS (tool)->modifier_key (tool, key, press, state, display);
}

void
gimp_tool_set_modifier_state (GimpTool        *tool,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

#ifdef DEBUG_FOCUS
  g_printerr ("%s: tool: %p  display: %p  focus_display: %p\n",
              G_STRFUNC, tool, display, tool->focus_display);
#endif

  g_return_if_fail (display == tool->focus_display);

  if ((tool->modifier_state & GDK_SHIFT_MASK) != (state & GDK_SHIFT_MASK))
    {
      gimp_tool_modifier_key (tool, GDK_SHIFT_MASK,
                              (state & GDK_SHIFT_MASK) ? TRUE : FALSE, state,
                              display);
    }

  if ((tool->modifier_state & GDK_CONTROL_MASK) != (state & GDK_CONTROL_MASK))
    {
      gimp_tool_modifier_key (tool, GDK_CONTROL_MASK,
                              (state & GDK_CONTROL_MASK) ? TRUE : FALSE, state,
                              display);
    }

  if ((tool->modifier_state & GDK_MOD1_MASK) != (state & GDK_MOD1_MASK))
    {
      gimp_tool_modifier_key (tool, GDK_MOD1_MASK,
                              (state & GDK_MOD1_MASK) ? TRUE : FALSE, state,
                              display);
    }

  tool->modifier_state = state;
}

static void
gimp_tool_active_modifier_key (GimpTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (display == tool->focus_display);

  GIMP_TOOL_GET_CLASS (tool)->active_modifier_key (tool, key, press, state,
                                                   display);
}

void
gimp_tool_set_active_modifier_state (GimpTool        *tool,
                                     GdkModifierType  state,
                                     GimpDisplay     *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  g_return_if_fail (display == tool->focus_display);

  if ((tool->active_modifier_state & GDK_SHIFT_MASK) !=
      (state & GDK_SHIFT_MASK))
    {
      gboolean press = state & GDK_SHIFT_MASK;

#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: SHIFT %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      if (! press && (tool->button_press_state & GDK_SHIFT_MASK))
        {
          tool->button_press_state &= ~GDK_SHIFT_MASK;
        }
      else
        {
          gimp_tool_active_modifier_key (tool, GDK_SHIFT_MASK,
                                         press, state,
                                         display);
        }
    }

  if ((tool->active_modifier_state & GDK_CONTROL_MASK) !=
      (state & GDK_CONTROL_MASK))
    {
      gboolean press = state & GDK_CONTROL_MASK;

#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: CONTROL %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      if (! press && (tool->button_press_state & GDK_CONTROL_MASK))
        {
          tool->button_press_state &= ~GDK_CONTROL_MASK;
        }
      else
        {
          gimp_tool_active_modifier_key (tool, GDK_CONTROL_MASK,
                                         press, state,
                                         display);
        }
    }

  if ((tool->active_modifier_state & GDK_MOD1_MASK) !=
      (state & GDK_MOD1_MASK))
    {
      gboolean press = state & GDK_MOD1_MASK;

#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: ALT %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      if (! press && (tool->button_press_state & GDK_MOD1_MASK))
        {
          tool->button_press_state &= ~GDK_MOD1_MASK;
        }
      else
        {
          gimp_tool_active_modifier_key (tool, GDK_MOD1_MASK,
                                         press, state,
                                         display);
        }
    }

  tool->active_modifier_state = state;
}

void
gimp_tool_oper_update (GimpTool        *tool,
                       GimpCoords      *coords,
                       GdkModifierType  state,
                       gboolean         proximity,
                       GimpDisplay     *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  GIMP_TOOL_GET_CLASS (tool)->oper_update (tool, coords, state, proximity,
                                           display);
}

void
gimp_tool_cursor_update (GimpTool        *tool,
                         GimpCoords      *coords,
                         GdkModifierType  state,
                         GimpDisplay     *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  GIMP_TOOL_GET_CLASS (tool)->cursor_update (tool, coords, state, display);
}

void
gimp_tool_push_status (GimpTool    *tool,
                       GimpDisplay *display,
                       const gchar *format,
                       ...)
{
  GimpDisplayShell *shell;
  va_list           args;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  va_start (args, format);

  gimp_statusbar_push_valist (GIMP_STATUSBAR (shell->statusbar),
                              G_OBJECT_TYPE_NAME (tool),
                              format, args);

  va_end (args);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
gimp_tool_push_status_coords (GimpTool    *tool,
                              GimpDisplay *display,
                              const gchar *title,
                              gdouble      x,
                              const gchar *separator,
                              gdouble      y,
                              const gchar *help)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  shell = GIMP_DISPLAY_SHELL (display->shell);

  gimp_statusbar_push_coords (GIMP_STATUSBAR (shell->statusbar),
                              G_OBJECT_TYPE_NAME (tool),
                              title, x, separator, y, help);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
gimp_tool_push_status_length (GimpTool            *tool,
                              GimpDisplay         *display,
                              const gchar         *title,
                              GimpOrientationType  axis,
                              gdouble              value,
                              const gchar         *help)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  shell = GIMP_DISPLAY_SHELL (display->shell);

  gimp_statusbar_push_length (GIMP_STATUSBAR (shell->statusbar),
                              G_OBJECT_TYPE_NAME (tool),
                              title, axis, value, help);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
gimp_tool_replace_status (GimpTool    *tool,
                          GimpDisplay *display,
                          const gchar *format,
                          ...)
{
  GimpDisplayShell *shell;
  va_list           args;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  va_start (args, format);

  gimp_statusbar_replace_valist (GIMP_STATUSBAR (shell->statusbar),
                                 G_OBJECT_TYPE_NAME (tool),
                                 format, args);

  va_end (args);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
gimp_tool_pop_status (GimpTool    *tool,
                      GimpDisplay *display)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  shell = GIMP_DISPLAY_SHELL (display->shell);

  gimp_statusbar_pop (GIMP_STATUSBAR (shell->statusbar),
                      G_OBJECT_TYPE_NAME (tool));

  tool->status_displays = g_list_remove (tool->status_displays, display);
}

void
gimp_tool_message (GimpTool    *tool,
                   GimpDisplay *display,
                   const gchar *format,
                   ...)
{
  va_list args;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  va_start (args, format);

  gimp_message_valist (display->image->gimp, G_OBJECT (display),
                       GIMP_MESSAGE_WARNING, format, args);

  va_end (args);
}

void
gimp_tool_set_cursor (GimpTool           *tool,
                      GimpDisplay        *display,
                      GimpCursorType      cursor,
                      GimpToolCursorType  tool_cursor,
                      GimpCursorModifier  modifier)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  gimp_display_shell_set_cursor (GIMP_DISPLAY_SHELL (display->shell),
                                 cursor, tool_cursor, modifier);
}


/*  private functions  */

static void
gimp_tool_clear_status (GimpTool *tool)
{
  GList *list;

  g_return_if_fail (GIMP_IS_TOOL (tool));

  list = tool->status_displays;
  while (list)
    {
      GimpDisplay *display = list->data;

      /*  get next element early because we modify the list  */
      list = g_list_next (list);

      gimp_tool_pop_status (tool, display);
    }
}
