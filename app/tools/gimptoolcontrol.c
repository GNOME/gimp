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

#include "tools-types.h"

#include "gimptoolcontrol.h"


static void gimp_tool_control_finalize (GObject *object);


G_DEFINE_TYPE (GimpToolControl, gimp_tool_control, GIMP_TYPE_OBJECT)

#define parent_class gimp_tool_control_parent_class


static void
gimp_tool_control_class_init (GimpToolControlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_tool_control_finalize;
}

static void
gimp_tool_control_init (GimpToolControl *control)
{
  control->active                 = FALSE;
  control->paused_count           = 0;

  control->preserve               = TRUE;
  control->scroll_lock            = FALSE;
  control->handle_empty_image     = FALSE;

  control->dirty_mask             = GIMP_DIRTY_NONE;
  control->motion_mode            = GIMP_MOTION_MODE_HINT;

  control->auto_snap_to           = TRUE;
  control->snap_offset_x          = 0;
  control->snap_offset_y          = 0;
  control->snap_width             = 0;
  control->snap_height            = 0;

  control->toggled                = FALSE;

  control->cursor                 = GIMP_CURSOR_MOUSE;
  control->tool_cursor            = GIMP_TOOL_CURSOR_NONE;
  control->cursor_modifier        = GIMP_CURSOR_MODIFIER_NONE;

  control->toggle_cursor          = -1;
  control->toggle_tool_cursor     = -1;
  control->toggle_cursor_modifier = -1;

  control->action_value_1         = NULL;
  control->action_value_2         = NULL;
  control->action_value_3         = NULL;
  control->action_value_4         = NULL;

  control->action_object_1        = NULL;
  control->action_object_2        = NULL;
}

static void
gimp_tool_control_finalize (GObject *object)
{
  GimpToolControl *control = GIMP_TOOL_CONTROL (object);

  g_free (control->action_value_1);
  g_free (control->action_value_2);
  g_free (control->action_value_3);
  g_free (control->action_value_4);
  g_free (control->action_object_1);
  g_free (control->action_object_2);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

void
gimp_tool_control_activate (GimpToolControl *control)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));
#ifdef GIMP_UNSTABLE
  g_return_if_fail (control->active == FALSE);
#endif

  control->active = TRUE;
}

void
gimp_tool_control_halt (GimpToolControl *control)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));
#ifdef GIMP_UNSTABLE
  g_return_if_fail (control->active == TRUE);
#endif

  control->active = FALSE;
}

gboolean
gimp_tool_control_is_active (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), FALSE);

  return control->active;
}

void
gimp_tool_control_pause (GimpToolControl *control)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->paused_count++;
}

void
gimp_tool_control_resume (GimpToolControl *control)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->paused_count--;
}

gboolean
gimp_tool_control_is_paused (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), FALSE);

  return control->paused_count > 0;
}

void
gimp_tool_control_set_preserve (GimpToolControl *control,
                                gboolean         preserve)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->preserve = preserve ? TRUE : FALSE;
}

gboolean
gimp_tool_control_get_preserve (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), FALSE);

  return control->preserve;
}

void
gimp_tool_control_set_scroll_lock (GimpToolControl *control,
                                   gboolean         scroll_lock)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->scroll_lock = scroll_lock ? TRUE : FALSE;
}

gboolean
gimp_tool_control_get_scroll_lock (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), FALSE);

  return control->scroll_lock;
}

void
gimp_tool_control_set_handle_empty_image (GimpToolControl *control,
                                          gboolean         handle_empty)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->handle_empty_image = handle_empty ? TRUE : FALSE;
}

gboolean
gimp_tool_control_get_handle_empty_image (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), FALSE);

  return control->handle_empty_image;
}

void
gimp_tool_control_set_dirty_mask (GimpToolControl *control,
                                  GimpDirtyMask    dirty_mask)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->dirty_mask = dirty_mask;
}

GimpDirtyMask
gimp_tool_control_get_dirty_mask (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), GIMP_DIRTY_NONE);

  return control->dirty_mask;
}

void
gimp_tool_control_set_motion_mode (GimpToolControl *control,
                                   GimpMotionMode   motion_mode)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->motion_mode = motion_mode;
}

GimpMotionMode
gimp_tool_control_get_motion_mode (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), GIMP_MOTION_MODE_HINT);

  return control->motion_mode;
}

void
gimp_tool_control_set_snap_to (GimpToolControl *control,
                               gboolean         snap_to)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->auto_snap_to = snap_to ? TRUE : FALSE;
}

gboolean
gimp_tool_control_get_snap_to (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), FALSE);

  return control->auto_snap_to;
}

void
gimp_tool_control_set_snap_offsets (GimpToolControl *control,
                                    gint             offset_x,
                                    gint             offset_y,
                                    gint             width,
                                    gint             height)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->snap_offset_x = offset_x;
  control->snap_offset_y = offset_y;
  control->snap_width    = width;
  control->snap_height   = height;
}

void
gimp_tool_control_get_snap_offsets (GimpToolControl *control,
                                    gint            *offset_x,
                                    gint            *offset_y,
                                    gint            *width,
                                    gint            *height)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  if (offset_x) *offset_x = control->snap_offset_x;
  if (offset_y) *offset_y = control->snap_offset_y;
  if (width)    *width    = control->snap_width;
  if (height)   *height   = control->snap_height;
}

void
gimp_tool_control_set_toggled (GimpToolControl *control,
                               gboolean         toggled)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->toggled = toggled ? TRUE : FALSE;
}

gboolean
gimp_tool_control_get_toggled (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), FALSE);

  return control->toggled;
}

void
gimp_tool_control_set_cursor (GimpToolControl *control,
                              GimpCursorType   cursor)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->cursor = cursor;
}

void
gimp_tool_control_set_tool_cursor (GimpToolControl    *control,
                                   GimpToolCursorType  cursor)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->tool_cursor = cursor;
}

void
gimp_tool_control_set_cursor_modifier (GimpToolControl    *control,
                                       GimpCursorModifier  modifier)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->cursor_modifier = modifier;
}

void
gimp_tool_control_set_toggle_cursor (GimpToolControl *control,
                                     GimpCursorType   cursor)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->toggle_cursor = cursor;
}

void
gimp_tool_control_set_toggle_tool_cursor (GimpToolControl    *control,
                                          GimpToolCursorType  cursor)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->toggle_tool_cursor = cursor;
}

void
gimp_tool_control_set_toggle_cursor_modifier (GimpToolControl    *control,
                                              GimpCursorModifier  modifier)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  control->toggle_cursor_modifier = modifier;
}

GimpCursorType
gimp_tool_control_get_cursor (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), FALSE);

  if (control->toggled && control->toggle_cursor != -1)
    return control->toggle_cursor;

  return control->cursor;
}

GimpToolCursorType
gimp_tool_control_get_tool_cursor (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), FALSE);

  if (control->toggled && control->toggle_tool_cursor != -1)
    return control->toggle_tool_cursor;

  return control->tool_cursor;
}

GimpCursorModifier
gimp_tool_control_get_cursor_modifier (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), FALSE);

  if (control->toggled && control->toggle_cursor_modifier != -1)
    return control->toggle_cursor_modifier;

  return control->cursor_modifier;
}

void
gimp_tool_control_set_action_value_1 (GimpToolControl *control,
                                      const gchar     *action)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  if (action != control->action_value_1)
    {
      g_free (control->action_value_1);
      control->action_value_1 = g_strdup (action);
    }
}

const gchar *
gimp_tool_control_get_action_value_1 (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), NULL);

  return control->action_value_1;
}

void
gimp_tool_control_set_action_value_2 (GimpToolControl *control,
                                      const gchar     *action)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  if (action != control->action_value_2)
    {
      g_free (control->action_value_2);
      control->action_value_2 = g_strdup (action);
    }
}

const gchar *
gimp_tool_control_get_action_value_2 (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), NULL);

  return control->action_value_2;
}

void
gimp_tool_control_set_action_value_3 (GimpToolControl *control,
                                      const gchar     *action)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  if (action != control->action_value_3)
    {
      g_free (control->action_value_3);
      control->action_value_3 = g_strdup (action);
    }
}

const gchar *
gimp_tool_control_get_action_value_3 (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), NULL);

  return control->action_value_3;
}

void
gimp_tool_control_set_action_value_4 (GimpToolControl *control,
                                      const gchar     *action)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  if (action != control->action_value_4)
    {
      g_free (control->action_value_4);
      control->action_value_4 = g_strdup (action);
    }
}

const gchar *
gimp_tool_control_get_action_value_4 (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), NULL);

  return control->action_value_4;
}

void
gimp_tool_control_set_action_object_1 (GimpToolControl *control,
                                       const gchar     *action)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  if (action != control->action_object_1)
    {
      g_free (control->action_object_1);
      control->action_object_1 = g_strdup (action);
    }
}

const gchar *
gimp_tool_control_get_action_object_1 (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), NULL);

  return control->action_object_1;
}

void
gimp_tool_control_set_action_object_2 (GimpToolControl *control,
                                       const gchar     *action)
{
  g_return_if_fail (GIMP_IS_TOOL_CONTROL (control));

  if (action != control->action_object_2)
    {
      g_free (control->action_object_2);
      control->action_object_2 = g_strdup (action);
    }
}

const gchar *
gimp_tool_control_get_action_object_2 (GimpToolControl *control)
{
  g_return_val_if_fail (GIMP_IS_TOOL_CONTROL (control), NULL);

  return control->action_object_2;
}
