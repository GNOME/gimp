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

#ifndef __GIMP_TOOL_CONTROL_H__
#define __GIMP_TOOL_CONTROL_H__

#include "tools-types.h"

#include "core/gimpobject.h"

#define GIMP_TYPE_TOOL_CONTROL            (gimp_tool_control_get_type ())
#define GIMP_TOOL_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_CONTROL, GimpToolControl))
#define GIMP_TOOL_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_CONTROL, GimpToolControlClass))
#define GIMP_IS_TOOL_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_CONTROL))
#define GIMP_IS_TOOL_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_CONTROL))
#define GIMP_TOOL_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_CONTROL, GimpToolControlClass))

typedef struct _GimpToolControlClass GimpToolControlClass;

struct _GimpToolControlClass {
  GimpObjectClass parent_class;
};

struct _GimpToolControl {
  GimpObject     parent_instance;

  GimpToolState  state;        /*  state of tool activity                     */
  gint           paused_count; /*  paused control count                       */

  gboolean       scroll_lock;        /*  allow scrolling or not               */
  gboolean       auto_snap_to;       /*  snap to guides automatically         */
  gboolean       preserve;           /*  Preserve this tool across drawable   *
                                      *  changes                              */
  gboolean       handle_empty_image; /*  invoke the tool on images without    *
                                      *  active drawable                      */
  GimpMotionMode motion_mode;        /*  how to process motion events before  *
                                      *  they are forwarded to the tool       */
  GdkCursorType      cursor;
  GimpToolCursorType tool_cursor;
  GimpCursorModifier cursor_modifier;

  GdkCursorType      toggle_cursor;
  GimpToolCursorType toggle_tool_cursor;
  GimpCursorModifier toggle_cursor_modifier;

  gboolean           toggled;
};

#endif /* __GIMP_TOOL_CONTROL_H__ */
