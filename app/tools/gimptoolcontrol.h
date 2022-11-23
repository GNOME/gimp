/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_TOOL_CONTROL_H__
#define __LIGMA_TOOL_CONTROL_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_TOOL_CONTROL            (ligma_tool_control_get_type ())
#define LIGMA_TOOL_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_CONTROL, LigmaToolControl))
#define LIGMA_TOOL_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_CONTROL, LigmaToolControlClass))
#define LIGMA_IS_TOOL_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_CONTROL))
#define LIGMA_IS_TOOL_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_CONTROL))
#define LIGMA_TOOL_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_CONTROL, LigmaToolControlClass))


typedef struct _LigmaToolControlClass LigmaToolControlClass;


struct _LigmaToolControl
{
  LigmaObject           parent_instance;

  gboolean             active;             /*  state of tool activity          */
  gint                 paused_count;       /*  paused control count            */

  gboolean             preserve;           /*  Preserve this tool across       *
                                            *  drawable changes                */
  GSList              *preserve_stack;     /*  for push/pop preserve           */

  gboolean             scroll_lock;        /*  allow scrolling or not          */
  gboolean             handle_empty_image; /*  invoke the tool on images       *
                                            *  without active drawable         */
  LigmaDirtyMask        dirty_mask;         /*  if preserve is FALSE, stop      *
                                            *  the tool on these events        */
  LigmaToolAction       dirty_action;       /*  use this action to stop the     *
                                            *  tool when one of the dirty      *
                                            *  events occurs                   */
  LigmaMotionMode       motion_mode;        /*  how to process motion events    *
                                            *  before they go to the tool      */
  gboolean             auto_snap_to;       /*  snap to guides automatically    */
  gint                 snap_offset_x;
  gint                 snap_offset_y;
  gint                 snap_width;
  gint                 snap_height;

  LigmaCursorPrecision  precision;

  gboolean             wants_click;        /*  wants click detection           */
  gboolean             wants_double_click;
  gboolean             wants_triple_click;
  gboolean             wants_all_key_events;

  LigmaToolActiveModifiers  active_modifiers;

  gboolean             toggled;

  LigmaCursorType       cursor;
  LigmaToolCursorType   tool_cursor;
  LigmaCursorModifier   cursor_modifier;

  LigmaCursorType       toggle_cursor;
  LigmaToolCursorType   toggle_tool_cursor;
  LigmaCursorModifier   toggle_cursor_modifier;

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

struct _LigmaToolControlClass
{
  LigmaObjectClass parent_class;
};


GType          ligma_tool_control_get_type           (void) G_GNUC_CONST;

void           ligma_tool_control_activate           (LigmaToolControl *control);
void           ligma_tool_control_halt               (LigmaToolControl *control);
gboolean       ligma_tool_control_is_active          (LigmaToolControl *control);

void           ligma_tool_control_pause              (LigmaToolControl *control);
void           ligma_tool_control_resume             (LigmaToolControl *control);
gboolean       ligma_tool_control_is_paused          (LigmaToolControl *control);

void           ligma_tool_control_set_preserve       (LigmaToolControl *control,
                                                     gboolean         preserve);
gboolean       ligma_tool_control_get_preserve       (LigmaToolControl *control);

void           ligma_tool_control_push_preserve      (LigmaToolControl *control,
                                                     gboolean         preserve);
void           ligma_tool_control_pop_preserve       (LigmaToolControl *control);

void           ligma_tool_control_set_scroll_lock    (LigmaToolControl *control,
                                                     gboolean         scroll_lock);
gboolean       ligma_tool_control_get_scroll_lock    (LigmaToolControl *control);

void           ligma_tool_control_set_handle_empty_image
                                                    (LigmaToolControl *control,
                                                     gboolean         handle_empty);
gboolean       ligma_tool_control_get_handle_empty_image
                                                    (LigmaToolControl *control);

void           ligma_tool_control_set_dirty_mask     (LigmaToolControl *control,
                                                     LigmaDirtyMask    dirty_mask);
LigmaDirtyMask  ligma_tool_control_get_dirty_mask     (LigmaToolControl *control);

void           ligma_tool_control_set_dirty_action   (LigmaToolControl *control,
                                                     LigmaToolAction   action);
LigmaToolAction ligma_tool_control_get_dirty_action   (LigmaToolControl *control);

void           ligma_tool_control_set_motion_mode    (LigmaToolControl *control,
                                                     LigmaMotionMode   motion_mode);
LigmaMotionMode ligma_tool_control_get_motion_mode    (LigmaToolControl *control);

void           ligma_tool_control_set_snap_to        (LigmaToolControl *control,
                                                     gboolean         snap_to);
gboolean       ligma_tool_control_get_snap_to        (LigmaToolControl *control);

void           ligma_tool_control_set_wants_click    (LigmaToolControl *control,
                                                     gboolean         wants_click);
gboolean       ligma_tool_control_get_wants_click    (LigmaToolControl *control);

void           ligma_tool_control_set_wants_double_click
                                                    (LigmaToolControl *control,
                                                     gboolean         wants_double_click);
gboolean       ligma_tool_control_get_wants_double_click
                                                    (LigmaToolControl *control);

void           ligma_tool_control_set_wants_triple_click
                                                    (LigmaToolControl *control,
                                                     gboolean         wants_double_click);
gboolean       ligma_tool_control_get_wants_triple_click
                                                    (LigmaToolControl *control);

void           ligma_tool_control_set_wants_all_key_events
                                                    (LigmaToolControl *control,
                                                     gboolean         wants_key_events);
gboolean       ligma_tool_control_get_wants_all_key_events
                                                    (LigmaToolControl *control);

void           ligma_tool_control_set_active_modifiers
                                                    (LigmaToolControl *control,
                                                     LigmaToolActiveModifiers  active_modifiers);
LigmaToolActiveModifiers
               ligma_tool_control_get_active_modifiers
                                                    (LigmaToolControl *control);

void           ligma_tool_control_set_snap_offsets   (LigmaToolControl *control,
                                                     gint             offset_x,
                                                     gint             offset_y,
                                                     gint             width,
                                                     gint             height);
void           ligma_tool_control_get_snap_offsets   (LigmaToolControl *control,
                                                     gint            *offset_x,
                                                     gint            *offset_y,
                                                     gint            *width,
                                                     gint            *height);

void           ligma_tool_control_set_precision      (LigmaToolControl *control,
                                                     LigmaCursorPrecision  precision);
LigmaCursorPrecision
               ligma_tool_control_get_precision      (LigmaToolControl *control);

void           ligma_tool_control_set_toggled        (LigmaToolControl *control,
                                                     gboolean         toggled);
gboolean       ligma_tool_control_get_toggled        (LigmaToolControl *control);

void           ligma_tool_control_set_cursor         (LigmaToolControl *control,
                                                     LigmaCursorType   cursor);
void           ligma_tool_control_set_tool_cursor    (LigmaToolControl *control,
                                                     LigmaToolCursorType  cursor);
void           ligma_tool_control_set_cursor_modifier
                                                    (LigmaToolControl *control,
                                                     LigmaCursorModifier  modifier);
void           ligma_tool_control_set_toggle_cursor  (LigmaToolControl *control,
                                                     LigmaCursorType   cursor);
void           ligma_tool_control_set_toggle_tool_cursor
                                                    (LigmaToolControl *control,
                                                     LigmaToolCursorType  cursor);
void           ligma_tool_control_set_toggle_cursor_modifier
                                                    (LigmaToolControl *control,
                                                     LigmaCursorModifier  modifier);

LigmaCursorType
              ligma_tool_control_get_cursor          (LigmaToolControl *control);

LigmaToolCursorType
              ligma_tool_control_get_tool_cursor     (LigmaToolControl *control);

LigmaCursorModifier
              ligma_tool_control_get_cursor_modifier (LigmaToolControl *control);

void          ligma_tool_control_set_action_opacity  (LigmaToolControl *control,
                                                     const gchar     *action);
const gchar * ligma_tool_control_get_action_opacity  (LigmaToolControl *control);

void          ligma_tool_control_set_action_size     (LigmaToolControl *control,
                                                     const gchar     *action);
const gchar * ligma_tool_control_get_action_size     (LigmaToolControl *control);

void          ligma_tool_control_set_action_aspect   (LigmaToolControl *control,
                                                     const gchar     *action);
const gchar * ligma_tool_control_get_action_aspect   (LigmaToolControl *control);

void          ligma_tool_control_set_action_angle    (LigmaToolControl *control,
                                                     const gchar     *action);
const gchar * ligma_tool_control_get_action_angle    (LigmaToolControl *control);

void          ligma_tool_control_set_action_spacing  (LigmaToolControl *control,
                                                     const gchar     *action);
const gchar * ligma_tool_control_get_action_spacing  (LigmaToolControl *control);

void          ligma_tool_control_set_action_hardness (LigmaToolControl *control,
                                                     const gchar     *action);
const gchar * ligma_tool_control_get_action_hardness (LigmaToolControl *control);

void          ligma_tool_control_set_action_force    (LigmaToolControl *control,
                                                     const gchar     *action);
const gchar * ligma_tool_control_get_action_force    (LigmaToolControl *control);

void          ligma_tool_control_set_action_object_1 (LigmaToolControl *control,
                                                     const gchar     *action);
const gchar * ligma_tool_control_get_action_object_1 (LigmaToolControl *control);

void          ligma_tool_control_set_action_object_2 (LigmaToolControl *control,
                                                     const gchar     *action);
const gchar * ligma_tool_control_get_action_object_2 (LigmaToolControl *control);


void          ligma_tool_control_set_action_pixel_size (LigmaToolControl *control,
                                                       const gchar     *action);
const gchar * ligma_tool_control_get_action_pixel_size (LigmaToolControl *control);


#endif /* __LIGMA_TOOL_CONTROL_H__ */
