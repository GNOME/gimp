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

#ifndef __GIMP_TOOL_CONTROL_H__
#define __GIMP_TOOL_CONTROL_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_TOOL_CONTROL            (gimp_tool_control_get_type ())
#define GIMP_TOOL_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_CONTROL, GimpToolControl))
#define GIMP_TOOL_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_CONTROL, GimpToolControlClass))
#define GIMP_IS_TOOL_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_CONTROL))
#define GIMP_IS_TOOL_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_CONTROL))
#define GIMP_TOOL_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_CONTROL, GimpToolControlClass))


typedef struct _GimpToolControlClass GimpToolControlClass;


struct _GimpToolControl
{
  GimpObject         parent_instance;

  gboolean           active;             /*  state of tool activity          */
  gint               paused_count;       /*  paused control count            */

  gboolean           preserve;           /*  Preserve this tool across       *
                                          *  drawable changes                */
  gboolean           scroll_lock;        /*  allow scrolling or not          */
  gboolean           handle_empty_image; /*  invoke the tool on images       *
                                          *  without active drawable         */
  GimpDirtyMask      dirty_mask;         /*  if preserve is FALSE, cancel    *
                                          *  the tool on these events        */
  GimpMotionMode     motion_mode;        /*  how to process motion events    *
                                          *  before they go to the tool      */
  gboolean           auto_snap_to;       /*  snap to guides automatically    */
  gint               snap_offset_x;
  gint               snap_offset_y;
  gint               snap_width;
  gint               snap_height;

  gboolean           toggled;

  GimpCursorType     cursor;
  GimpToolCursorType tool_cursor;
  GimpCursorModifier cursor_modifier;

  GimpCursorType     toggle_cursor;
  GimpToolCursorType toggle_tool_cursor;
  GimpCursorModifier toggle_cursor_modifier;

  gchar             *action_value_1;
  gchar             *action_value_2;
  gchar             *action_value_3;
  gchar             *action_value_4;
  gchar             *action_object_1;
  gchar             *action_object_2;
};

struct _GimpToolControlClass
{
  GimpObjectClass parent_class;
};


GType          gimp_tool_control_get_type         (void) G_GNUC_CONST;

void           gimp_tool_control_activate         (GimpToolControl *control);
void           gimp_tool_control_halt             (GimpToolControl *control);
gboolean       gimp_tool_control_is_active        (GimpToolControl *control);

void           gimp_tool_control_pause            (GimpToolControl *control);
void           gimp_tool_control_resume           (GimpToolControl *control);
gboolean       gimp_tool_control_is_paused        (GimpToolControl *control);

void           gimp_tool_control_set_preserve     (GimpToolControl *control,
                                                   gboolean         preserve);
gboolean       gimp_tool_control_get_preserve     (GimpToolControl *control);

void           gimp_tool_control_set_scroll_lock  (GimpToolControl *control,
                                                   gboolean         scroll_lock);
gboolean       gimp_tool_control_get_scroll_lock  (GimpToolControl *control);

void     gimp_tool_control_set_handle_empty_image (GimpToolControl *control,
                                                   gboolean         handle_empty);
gboolean gimp_tool_control_get_handle_empty_image (GimpToolControl *control);

void           gimp_tool_control_set_dirty_mask   (GimpToolControl *control,
                                                   GimpDirtyMask    dirty_mask);
GimpDirtyMask  gimp_tool_control_get_dirty_mask   (GimpToolControl *control);

void           gimp_tool_control_set_motion_mode  (GimpToolControl *control,
                                                   GimpMotionMode   motion_mode);
GimpMotionMode gimp_tool_control_get_motion_mode  (GimpToolControl *control);

void           gimp_tool_control_set_snap_to      (GimpToolControl *control,
                                                   gboolean         snap_to);
gboolean       gimp_tool_control_get_snap_to      (GimpToolControl *control);

void           gimp_tool_control_set_snap_offsets (GimpToolControl *control,
                                                   gint             offset_x,
                                                   gint             offset_y,
                                                   gint             width,
                                                   gint             height);
void           gimp_tool_control_get_snap_offsets (GimpToolControl *control,
                                                   gint            *offset_x,
                                                   gint            *offset_y,
                                                   gint            *width,
                                                   gint            *height);

void           gimp_tool_control_set_toggled      (GimpToolControl *control,
                                                   gboolean         toggled);
gboolean       gimp_tool_control_get_toggled      (GimpToolControl *control);

void gimp_tool_control_set_cursor                 (GimpToolControl    *control,
                                                   GimpCursorType      cursor);
void gimp_tool_control_set_tool_cursor            (GimpToolControl    *control,
                                                   GimpToolCursorType  cursor);
void gimp_tool_control_set_cursor_modifier        (GimpToolControl    *control,
                                                   GimpCursorModifier  modifier);
void gimp_tool_control_set_toggle_cursor          (GimpToolControl    *control,
                                                   GimpCursorType      cursor);
void gimp_tool_control_set_toggle_tool_cursor     (GimpToolControl    *control,
                                                   GimpToolCursorType  cursor);
void gimp_tool_control_set_toggle_cursor_modifier (GimpToolControl    *control,
                                                   GimpCursorModifier  modifier);

GimpCursorType
              gimp_tool_control_get_cursor          (GimpToolControl *control);

GimpToolCursorType
              gimp_tool_control_get_tool_cursor     (GimpToolControl *control);

GimpCursorModifier
              gimp_tool_control_get_cursor_modifier (GimpToolControl *control);

void          gimp_tool_control_set_action_value_1  (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_value_1  (GimpToolControl *control);

void          gimp_tool_control_set_action_value_2  (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_value_2  (GimpToolControl *control);

void          gimp_tool_control_set_action_value_3  (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_value_3  (GimpToolControl *control);

void          gimp_tool_control_set_action_value_4  (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_value_4  (GimpToolControl *control);

void          gimp_tool_control_set_action_object_1 (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_object_1 (GimpToolControl *control);

void          gimp_tool_control_set_action_object_2 (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_object_2 (GimpToolControl *control);


#endif /* __GIMP_TOOL_CONTROL_H__ */
