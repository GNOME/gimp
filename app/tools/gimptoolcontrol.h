/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
  GimpObject           parent_instance;

  gboolean             active;             /*  state of tool activity          */
  gint                 paused_count;       /*  paused control count            */

  gboolean             preserve;           /*  Preserve this tool across       *
                                            *  drawable changes                */
  GSList              *preserve_stack;     /*  for push/pop preserve           */

  gboolean             scroll_lock;        /*  allow scrolling or not          */
  gboolean             handle_empty_image; /*  invoke the tool on images       *
                                            *  without active drawable         */
  GimpDirtyMask        dirty_mask;         /*  if preserve is FALSE, stop      *
                                            *  the tool on these events        */
  GimpToolAction       dirty_action;       /*  use this action to stop the     *
                                            *  tool when one of the dirty      *
                                            *  events occurs                   */
  GimpMotionMode       motion_mode;        /*  how to process motion events    *
                                            *  before they go to the tool      */
  gboolean             auto_snap_to;       /*  snap to guides automatically    */
  gint                 snap_offset_x;
  gint                 snap_offset_y;
  gint                 snap_width;
  gint                 snap_height;

  GimpCursorPrecision  precision;

  gboolean             wants_click;        /*  wants click detection           */
  gboolean             wants_double_click;
  gboolean             wants_triple_click;
  gboolean             wants_all_key_events;

  GimpToolActiveModifiers  active_modifiers;

  gboolean             toggled;

  GimpCursorType       cursor;
  GimpToolCursorType   tool_cursor;
  GimpCursorModifier   cursor_modifier;

  GimpCursorType       toggle_cursor;
  GimpToolCursorType   toggle_tool_cursor;
  GimpCursorModifier   toggle_cursor_modifier;

  gchar               *action_size;
  gchar               *action_aspect;
  gchar               *action_angle;

  gchar               *action_spacing;

  gchar               *action_opacity;
  gchar               *action_hardness;
  gchar               *action_force;

  gchar               *action_object_1;
  gchar               *action_object_2;

  /* Enum actions have a +-/min/max values which are not necessarily so
   * useful. They also have a variable value which works as a per mille
   * between the min and max, so it's hard to use, especially for
   * actions which have a huge max or which can have negative values.
   * For instance, aspect and angle can be negative. As for size and
   * spacing, their max value can be huge. Finally "size" can be
   * negative for the MyPaint brush tool, which uses a logarithmic
   * radius as base value.
   * For this reason, we also register specialized actions with clear
   * unit if needed.
   */
  gchar               *action_pixel_size;
};

struct _GimpToolControlClass
{
  GimpObjectClass parent_class;
};


GType          gimp_tool_control_get_type           (void) G_GNUC_CONST;

void           gimp_tool_control_activate           (GimpToolControl *control);
void           gimp_tool_control_halt               (GimpToolControl *control);
gboolean       gimp_tool_control_is_active          (GimpToolControl *control);

void           gimp_tool_control_pause              (GimpToolControl *control);
void           gimp_tool_control_resume             (GimpToolControl *control);
gboolean       gimp_tool_control_is_paused          (GimpToolControl *control);

void           gimp_tool_control_set_preserve       (GimpToolControl *control,
                                                     gboolean         preserve);
gboolean       gimp_tool_control_get_preserve       (GimpToolControl *control);

void           gimp_tool_control_push_preserve      (GimpToolControl *control,
                                                     gboolean         preserve);
void           gimp_tool_control_pop_preserve       (GimpToolControl *control);

void           gimp_tool_control_set_scroll_lock    (GimpToolControl *control,
                                                     gboolean         scroll_lock);
gboolean       gimp_tool_control_get_scroll_lock    (GimpToolControl *control);

void           gimp_tool_control_set_handle_empty_image
                                                    (GimpToolControl *control,
                                                     gboolean         handle_empty);
gboolean       gimp_tool_control_get_handle_empty_image
                                                    (GimpToolControl *control);

void           gimp_tool_control_set_dirty_mask     (GimpToolControl *control,
                                                     GimpDirtyMask    dirty_mask);
GimpDirtyMask  gimp_tool_control_get_dirty_mask     (GimpToolControl *control);

void           gimp_tool_control_set_dirty_action   (GimpToolControl *control,
                                                     GimpToolAction   action);
GimpToolAction gimp_tool_control_get_dirty_action   (GimpToolControl *control);

void           gimp_tool_control_set_motion_mode    (GimpToolControl *control,
                                                     GimpMotionMode   motion_mode);
GimpMotionMode gimp_tool_control_get_motion_mode    (GimpToolControl *control);

void           gimp_tool_control_set_snap_to        (GimpToolControl *control,
                                                     gboolean         snap_to);
gboolean       gimp_tool_control_get_snap_to        (GimpToolControl *control);

void           gimp_tool_control_set_wants_click    (GimpToolControl *control,
                                                     gboolean         wants_click);
gboolean       gimp_tool_control_get_wants_click    (GimpToolControl *control);

void           gimp_tool_control_set_wants_double_click
                                                    (GimpToolControl *control,
                                                     gboolean         wants_double_click);
gboolean       gimp_tool_control_get_wants_double_click
                                                    (GimpToolControl *control);

void           gimp_tool_control_set_wants_triple_click
                                                    (GimpToolControl *control,
                                                     gboolean         wants_double_click);
gboolean       gimp_tool_control_get_wants_triple_click
                                                    (GimpToolControl *control);

void           gimp_tool_control_set_wants_all_key_events
                                                    (GimpToolControl *control,
                                                     gboolean         wants_key_events);
gboolean       gimp_tool_control_get_wants_all_key_events
                                                    (GimpToolControl *control);

void           gimp_tool_control_set_active_modifiers
                                                    (GimpToolControl *control,
                                                     GimpToolActiveModifiers  active_modifiers);
GimpToolActiveModifiers
               gimp_tool_control_get_active_modifiers
                                                    (GimpToolControl *control);

void           gimp_tool_control_set_snap_offsets   (GimpToolControl *control,
                                                     gint             offset_x,
                                                     gint             offset_y,
                                                     gint             width,
                                                     gint             height);
void           gimp_tool_control_get_snap_offsets   (GimpToolControl *control,
                                                     gint            *offset_x,
                                                     gint            *offset_y,
                                                     gint            *width,
                                                     gint            *height);

void           gimp_tool_control_set_precision      (GimpToolControl *control,
                                                     GimpCursorPrecision  precision);
GimpCursorPrecision
               gimp_tool_control_get_precision      (GimpToolControl *control);

void           gimp_tool_control_set_toggled        (GimpToolControl *control,
                                                     gboolean         toggled);
gboolean       gimp_tool_control_get_toggled        (GimpToolControl *control);

void           gimp_tool_control_set_cursor         (GimpToolControl *control,
                                                     GimpCursorType   cursor);
void           gimp_tool_control_set_tool_cursor    (GimpToolControl *control,
                                                     GimpToolCursorType  cursor);
void           gimp_tool_control_set_cursor_modifier
                                                    (GimpToolControl *control,
                                                     GimpCursorModifier  modifier);
void           gimp_tool_control_set_toggle_cursor  (GimpToolControl *control,
                                                     GimpCursorType   cursor);
void           gimp_tool_control_set_toggle_tool_cursor
                                                    (GimpToolControl *control,
                                                     GimpToolCursorType  cursor);
void           gimp_tool_control_set_toggle_cursor_modifier
                                                    (GimpToolControl *control,
                                                     GimpCursorModifier  modifier);

GimpCursorType
              gimp_tool_control_get_cursor          (GimpToolControl *control);

GimpToolCursorType
              gimp_tool_control_get_tool_cursor     (GimpToolControl *control);

GimpCursorModifier
              gimp_tool_control_get_cursor_modifier (GimpToolControl *control);

void          gimp_tool_control_set_action_opacity  (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_opacity  (GimpToolControl *control);

void          gimp_tool_control_set_action_size     (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_size     (GimpToolControl *control);

void          gimp_tool_control_set_action_aspect   (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_aspect   (GimpToolControl *control);

void          gimp_tool_control_set_action_angle    (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_angle    (GimpToolControl *control);

void          gimp_tool_control_set_action_spacing  (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_spacing  (GimpToolControl *control);

void          gimp_tool_control_set_action_hardness (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_hardness (GimpToolControl *control);

void          gimp_tool_control_set_action_force    (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_force    (GimpToolControl *control);

void          gimp_tool_control_set_action_object_1 (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_object_1 (GimpToolControl *control);

void          gimp_tool_control_set_action_object_2 (GimpToolControl *control,
                                                     const gchar     *action);
const gchar * gimp_tool_control_get_action_object_2 (GimpToolControl *control);


void          gimp_tool_control_set_action_pixel_size (GimpToolControl *control,
                                                       const gchar     *action);
const gchar * gimp_tool_control_get_action_pixel_size (GimpToolControl *control);


#endif /* __GIMP_TOOL_CONTROL_H__ */
