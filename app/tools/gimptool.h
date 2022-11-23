/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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

#ifndef __LIGMA_TOOL_H__
#define __LIGMA_TOOL_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_TOOL            (ligma_tool_get_type ())
#define LIGMA_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL, LigmaTool))
#define LIGMA_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL, LigmaToolClass))
#define LIGMA_IS_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL))
#define LIGMA_IS_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL))
#define LIGMA_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL, LigmaToolClass))

#define LIGMA_TOOL_GET_OPTIONS(t)  (ligma_tool_get_options (LIGMA_TOOL (t)))


typedef struct _LigmaToolClass LigmaToolClass;

struct _LigmaTool
{
  LigmaObject       parent_instance;

  LigmaToolInfo    *tool_info;

  gchar           *label;
  gchar           *undo_desc;
  gchar           *icon_name;
  gchar           *help_id;

  gint             ID;          /*  unique tool ID                         */

  LigmaToolControl *control;

  LigmaDisplay     *display;     /*  pointer to currently active display    */
  GList           *drawables;   /*  list of the tool's current drawables   */

  /*  private state of ligma_tool_set_focus_display() and
   *  ligma_tool_set_[active_]modifier_state()
   */
  LigmaDisplay     *focus_display;
  GdkModifierType  modifier_state;
  GdkModifierType  button_press_state;
  GdkModifierType  active_modifier_state;

  /*  private state for synthesizing button_release() events
   */
  LigmaCoords       last_pointer_coords;
  guint32          last_pointer_time;
  GdkModifierType  last_pointer_state;

  /*  private state for click detection
   */
  gboolean         in_click_distance;
  gboolean         got_motion_event;
  LigmaCoords       button_press_coords;
  guint32          button_press_time;

  /*  private list of displays which have a status message from this tool
   */
  GList           *status_displays;

  /*  on-canvas progress  */
  LigmaCanvasItem  *progress;
  LigmaDisplay     *progress_display;
  GtkWidget       *progress_grab_widget;
  gboolean         progress_cancelable;
};

struct _LigmaToolClass
{
  LigmaObjectClass  parent_class;

  /*  virtual functions  */

  gboolean        (* has_display)         (LigmaTool              *tool,
                                           LigmaDisplay           *display);
  LigmaDisplay   * (* has_image)           (LigmaTool              *tool,
                                           LigmaImage             *image);

  gboolean        (* initialize)          (LigmaTool              *tool,
                                           LigmaDisplay           *display,
                                           GError               **error);
  void            (* control)             (LigmaTool              *tool,
                                           LigmaToolAction         action,
                                           LigmaDisplay           *display);

  void            (* button_press)        (LigmaTool              *tool,
                                           const LigmaCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           LigmaButtonPressType    press_type,
                                           LigmaDisplay           *display);
  void            (* button_release)      (LigmaTool              *tool,
                                           const LigmaCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           LigmaButtonReleaseType  release_type,
                                           LigmaDisplay           *display);
  void            (* motion)              (LigmaTool              *tool,
                                           const LigmaCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           LigmaDisplay           *display);

  gboolean        (* key_press)           (LigmaTool              *tool,
                                           GdkEventKey           *kevent,
                                           LigmaDisplay           *display);
  gboolean        (* key_release)         (LigmaTool              *tool,
                                           GdkEventKey           *kevent,
                                           LigmaDisplay           *display);
  void            (* modifier_key)        (LigmaTool              *tool,
                                           GdkModifierType        key,
                                           gboolean               press,
                                           GdkModifierType        state,
                                           LigmaDisplay           *display);
  void            (* active_modifier_key) (LigmaTool              *tool,
                                           GdkModifierType        key,
                                           gboolean               press,
                                           GdkModifierType        state,
                                           LigmaDisplay           *display);

  void            (* oper_update)         (LigmaTool              *tool,
                                           const LigmaCoords      *coords,
                                           GdkModifierType        state,
                                           gboolean               proximity,
                                           LigmaDisplay           *display);
  void            (* cursor_update)       (LigmaTool              *tool,
                                           const LigmaCoords      *coords,
                                           GdkModifierType        state,
                                           LigmaDisplay           *display);

  const gchar   * (* can_undo)            (LigmaTool              *tool,
                                           LigmaDisplay           *display);
  const gchar   * (* can_redo)            (LigmaTool              *tool,
                                           LigmaDisplay           *display);
  gboolean        (* undo)                (LigmaTool              *tool,
                                           LigmaDisplay           *display);
  gboolean        (* redo)                (LigmaTool              *tool,
                                           LigmaDisplay           *display);

  LigmaUIManager * (* get_popup)           (LigmaTool              *tool,
                                           const LigmaCoords      *coords,
                                           GdkModifierType        state,
                                           LigmaDisplay           *display,
                                           const gchar          **ui_path);

  void            (* options_notify)      (LigmaTool              *tool,
                                           LigmaToolOptions       *options,
                                           const GParamSpec      *pspec);
};


GType             ligma_tool_get_type            (void) G_GNUC_CONST;

LigmaToolOptions * ligma_tool_get_options         (LigmaTool            *tool);

void              ligma_tool_set_label           (LigmaTool            *tool,
                                                 const gchar         *label);
const gchar     * ligma_tool_get_label           (LigmaTool            *tool);

void              ligma_tool_set_undo_desc       (LigmaTool            *tool,
                                                 const gchar         *undo_desc);
const gchar     * ligma_tool_get_undo_desc       (LigmaTool            *tool);

void              ligma_tool_set_icon_name       (LigmaTool            *tool,
                                                 const gchar         *icon_name);
const gchar     * ligma_tool_get_icon_name       (LigmaTool            *tool);

void              ligma_tool_set_help_id         (LigmaTool            *tool,
                                                 const gchar         *help_id);
const gchar     * ligma_tool_get_help_id         (LigmaTool            *tool);

gboolean          ligma_tool_has_display         (LigmaTool            *tool,
                                                 LigmaDisplay         *display);
LigmaDisplay     * ligma_tool_has_image           (LigmaTool            *tool,
                                                 LigmaImage           *image);

gboolean          ligma_tool_initialize          (LigmaTool            *tool,
                                                 LigmaDisplay         *display);
void              ligma_tool_control             (LigmaTool            *tool,
                                                 LigmaToolAction       action,
                                                 LigmaDisplay         *display);

void              ligma_tool_button_press        (LigmaTool            *tool,
                                                 const LigmaCoords    *coords,
                                                 guint32              time,
                                                 GdkModifierType      state,
                                                 LigmaButtonPressType  press_type,
                                                 LigmaDisplay         *display);
void              ligma_tool_button_release      (LigmaTool            *tool,
                                                 const LigmaCoords    *coords,
                                                 guint32              time,
                                                 GdkModifierType      state,
                                                 LigmaDisplay         *display);
void              ligma_tool_motion              (LigmaTool            *tool,
                                                 const LigmaCoords    *coords,
                                                 guint32              time,
                                                 GdkModifierType      state,
                                                 LigmaDisplay         *display);

gboolean          ligma_tool_key_press           (LigmaTool            *tool,
                                                 GdkEventKey         *kevent,
                                                 LigmaDisplay         *display);
gboolean          ligma_tool_key_release         (LigmaTool            *tool,
                                                 GdkEventKey         *kevent,
                                                 LigmaDisplay         *display);

void              ligma_tool_set_focus_display   (LigmaTool            *tool,
                                                 LigmaDisplay         *display);
void              ligma_tool_set_modifier_state  (LigmaTool            *tool,
                                                 GdkModifierType      state,
                                                 LigmaDisplay         *display);
void        ligma_tool_set_active_modifier_state (LigmaTool            *tool,
                                                 GdkModifierType      state,
                                                 LigmaDisplay         *display);

void              ligma_tool_oper_update         (LigmaTool            *tool,
                                                 const LigmaCoords    *coords,
                                                 GdkModifierType      state,
                                                 gboolean             proximity,
                                                 LigmaDisplay         *display);
void              ligma_tool_cursor_update       (LigmaTool            *tool,
                                                 const LigmaCoords    *coords,
                                                 GdkModifierType      state,
                                                 LigmaDisplay         *display);

const gchar     * ligma_tool_can_undo            (LigmaTool            *tool,
                                                 LigmaDisplay         *display);
const gchar     * ligma_tool_can_redo            (LigmaTool            *tool,
                                                 LigmaDisplay         *display);
gboolean          ligma_tool_undo                (LigmaTool            *tool,
                                                 LigmaDisplay         *display);
gboolean          ligma_tool_redo                (LigmaTool            *tool,
                                                 LigmaDisplay         *display);

LigmaUIManager   * ligma_tool_get_popup           (LigmaTool            *tool,
                                                 const LigmaCoords    *coords,
                                                 GdkModifierType      state,
                                                 LigmaDisplay         *display,
                                                 const gchar        **ui_path);

void              ligma_tool_push_status         (LigmaTool            *tool,
                                                 LigmaDisplay         *display,
                                                 const gchar         *format,
                                                 ...) G_GNUC_PRINTF(3,4);
void              ligma_tool_push_status_coords  (LigmaTool            *tool,
                                                 LigmaDisplay         *display,
                                                 LigmaCursorPrecision  precision,
                                                 const gchar         *title,
                                                 gdouble              x,
                                                 const gchar         *separator,
                                                 gdouble              y,
                                                 const gchar         *help);
void              ligma_tool_push_status_length  (LigmaTool            *tool,
                                                 LigmaDisplay         *display,
                                                 const gchar         *title,
                                                 LigmaOrientationType  axis,
                                                 gdouble              value,
                                                 const gchar         *help);
void              ligma_tool_replace_status      (LigmaTool            *tool,
                                                 LigmaDisplay         *display,
                                                 const gchar         *format,
                                                 ...) G_GNUC_PRINTF(3,4);
void              ligma_tool_pop_status          (LigmaTool            *tool,
                                                 LigmaDisplay         *display);

void              ligma_tool_message             (LigmaTool            *tool,
                                                 LigmaDisplay         *display,
                                                 const gchar         *format,
                                                 ...) G_GNUC_PRINTF(3,4);
void              ligma_tool_message_literal     (LigmaTool            *tool,
                                                 LigmaDisplay         *display,
                                                 const gchar         *message);

void              ligma_tool_set_cursor          (LigmaTool            *tool,
                                                 LigmaDisplay         *display,
                                                 LigmaCursorType       cursor,
                                                 LigmaToolCursorType   tool_cursor,
                                                 LigmaCursorModifier   modifier);


#endif  /*  __LIGMA_TOOL_H__  */
