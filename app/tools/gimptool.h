/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_TOOL_H__
#define __GIMP_TOOL_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_TOOL            (gimp_tool_get_type ())
#define GIMP_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL, GimpTool))
#define GIMP_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL, GimpToolClass))
#define GIMP_IS_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL))
#define GIMP_IS_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL))
#define GIMP_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL, GimpToolClass))

#define GIMP_TOOL_GET_OPTIONS(t)  (gimp_tool_get_options (GIMP_TOOL (t)))


typedef struct _GimpToolClass GimpToolClass;

struct _GimpTool
{
  GimpObject       parent_instance;

  GimpToolInfo    *tool_info;

  gchar           *label;
  gchar           *undo_desc;
  gchar           *icon_name;
  gchar           *help_id;

  gint             ID;          /*  unique tool ID                         */

  GimpToolControl *control;

  GimpDisplay     *display;     /*  pointer to currently active display    */
  GList           *drawables;   /*  list of the tool's current drawables   */

  /*  private state of gimp_tool_set_focus_display() and
   *  gimp_tool_set_[active_]modifier_state()
   */
  GimpDisplay     *focus_display;
  GdkModifierType  modifier_state;
  GdkModifierType  button_press_state;
  GdkModifierType  active_modifier_state;

  /*  private state for synthesizing button_release() events
   */
  GimpCoords       last_pointer_coords;
  guint32          last_pointer_time;
  GdkModifierType  last_pointer_state;

  /*  private state for click detection
   */
  gboolean         in_click_distance;
  gboolean         got_motion_event;
  GimpCoords       button_press_coords;
  guint32          button_press_time;

  /*  private list of displays which have a status message from this tool
   */
  GList           *status_displays;

  /*  on-canvas progress  */
  GimpCanvasItem  *progress;
  GimpDisplay     *progress_display;
  GtkWidget       *progress_grab_widget;
  gboolean         progress_cancelable;
};

struct _GimpToolClass
{
  GimpObjectClass  parent_class;

  /*  virtual functions  */

  gboolean        (* has_display)         (GimpTool              *tool,
                                           GimpDisplay           *display);
  GimpDisplay   * (* has_image)           (GimpTool              *tool,
                                           GimpImage             *image);

  gboolean        (* initialize)          (GimpTool              *tool,
                                           GimpDisplay           *display,
                                           GError               **error);
  void            (* control)             (GimpTool              *tool,
                                           GimpToolAction         action,
                                           GimpDisplay           *display);

  void            (* button_press)        (GimpTool              *tool,
                                           const GimpCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           GimpButtonPressType    press_type,
                                           GimpDisplay           *display);
  void            (* button_release)      (GimpTool              *tool,
                                           const GimpCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           GimpButtonReleaseType  release_type,
                                           GimpDisplay           *display);
  void            (* motion)              (GimpTool              *tool,
                                           const GimpCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           GimpDisplay           *display);

  gboolean        (* key_press)           (GimpTool              *tool,
                                           GdkEventKey           *kevent,
                                           GimpDisplay           *display);
  gboolean        (* key_release)         (GimpTool              *tool,
                                           GdkEventKey           *kevent,
                                           GimpDisplay           *display);
  void            (* modifier_key)        (GimpTool              *tool,
                                           GdkModifierType        key,
                                           gboolean               press,
                                           GdkModifierType        state,
                                           GimpDisplay           *display);
  void            (* active_modifier_key) (GimpTool              *tool,
                                           GdkModifierType        key,
                                           gboolean               press,
                                           GdkModifierType        state,
                                           GimpDisplay           *display);

  void            (* oper_update)         (GimpTool              *tool,
                                           const GimpCoords      *coords,
                                           GdkModifierType        state,
                                           gboolean               proximity,
                                           GimpDisplay           *display);
  void            (* cursor_update)       (GimpTool              *tool,
                                           const GimpCoords      *coords,
                                           GdkModifierType        state,
                                           GimpDisplay           *display);

  const gchar   * (* can_undo)            (GimpTool              *tool,
                                           GimpDisplay           *display);
  const gchar   * (* can_redo)            (GimpTool              *tool,
                                           GimpDisplay           *display);
  gboolean        (* undo)                (GimpTool              *tool,
                                           GimpDisplay           *display);
  gboolean        (* redo)                (GimpTool              *tool,
                                           GimpDisplay           *display);

  GimpUIManager * (* get_popup)           (GimpTool              *tool,
                                           const GimpCoords      *coords,
                                           GdkModifierType        state,
                                           GimpDisplay           *display,
                                           const gchar          **ui_path);

  void            (* options_notify)      (GimpTool              *tool,
                                           GimpToolOptions       *options,
                                           const GParamSpec      *pspec);
};


GType             gimp_tool_get_type            (void) G_GNUC_CONST;

GimpToolOptions * gimp_tool_get_options         (GimpTool            *tool);

void              gimp_tool_set_label           (GimpTool            *tool,
                                                 const gchar         *label);
const gchar     * gimp_tool_get_label           (GimpTool            *tool);

void              gimp_tool_set_undo_desc       (GimpTool            *tool,
                                                 const gchar         *undo_desc);
const gchar     * gimp_tool_get_undo_desc       (GimpTool            *tool);

void              gimp_tool_set_icon_name       (GimpTool            *tool,
                                                 const gchar         *icon_name);
const gchar     * gimp_tool_get_icon_name       (GimpTool            *tool);

void              gimp_tool_set_help_id         (GimpTool            *tool,
                                                 const gchar         *help_id);
const gchar     * gimp_tool_get_help_id         (GimpTool            *tool);

gboolean          gimp_tool_has_display         (GimpTool            *tool,
                                                 GimpDisplay         *display);
GimpDisplay     * gimp_tool_has_image           (GimpTool            *tool,
                                                 GimpImage           *image);

gboolean          gimp_tool_initialize          (GimpTool            *tool,
                                                 GimpDisplay         *display);
void              gimp_tool_control             (GimpTool            *tool,
                                                 GimpToolAction       action,
                                                 GimpDisplay         *display);

void              gimp_tool_button_press        (GimpTool            *tool,
                                                 const GimpCoords    *coords,
                                                 guint32              time,
                                                 GdkModifierType      state,
                                                 GimpButtonPressType  press_type,
                                                 GimpDisplay         *display);
void              gimp_tool_button_release      (GimpTool            *tool,
                                                 const GimpCoords    *coords,
                                                 guint32              time,
                                                 GdkModifierType      state,
                                                 GimpDisplay         *display);
void              gimp_tool_motion              (GimpTool            *tool,
                                                 const GimpCoords    *coords,
                                                 guint32              time,
                                                 GdkModifierType      state,
                                                 GimpDisplay         *display);

gboolean          gimp_tool_key_press           (GimpTool            *tool,
                                                 GdkEventKey         *kevent,
                                                 GimpDisplay         *display);
gboolean          gimp_tool_key_release         (GimpTool            *tool,
                                                 GdkEventKey         *kevent,
                                                 GimpDisplay         *display);

void              gimp_tool_set_focus_display   (GimpTool            *tool,
                                                 GimpDisplay         *display);
void              gimp_tool_set_modifier_state  (GimpTool            *tool,
                                                 GdkModifierType      state,
                                                 GimpDisplay         *display);
void        gimp_tool_set_active_modifier_state (GimpTool            *tool,
                                                 GdkModifierType      state,
                                                 GimpDisplay         *display);

void              gimp_tool_oper_update         (GimpTool            *tool,
                                                 const GimpCoords    *coords,
                                                 GdkModifierType      state,
                                                 gboolean             proximity,
                                                 GimpDisplay         *display);
void              gimp_tool_cursor_update       (GimpTool            *tool,
                                                 const GimpCoords    *coords,
                                                 GdkModifierType      state,
                                                 GimpDisplay         *display);

const gchar     * gimp_tool_can_undo            (GimpTool            *tool,
                                                 GimpDisplay         *display);
const gchar     * gimp_tool_can_redo            (GimpTool            *tool,
                                                 GimpDisplay         *display);
gboolean          gimp_tool_undo                (GimpTool            *tool,
                                                 GimpDisplay         *display);
gboolean          gimp_tool_redo                (GimpTool            *tool,
                                                 GimpDisplay         *display);

GimpUIManager   * gimp_tool_get_popup           (GimpTool            *tool,
                                                 const GimpCoords    *coords,
                                                 GdkModifierType      state,
                                                 GimpDisplay         *display,
                                                 const gchar        **ui_path);

void              gimp_tool_push_status         (GimpTool            *tool,
                                                 GimpDisplay         *display,
                                                 const gchar         *format,
                                                 ...) G_GNUC_PRINTF(3,4);
void              gimp_tool_push_status_coords  (GimpTool            *tool,
                                                 GimpDisplay         *display,
                                                 GimpCursorPrecision  precision,
                                                 const gchar         *title,
                                                 gdouble              x,
                                                 const gchar         *separator,
                                                 gdouble              y,
                                                 const gchar         *help);
void              gimp_tool_push_status_length  (GimpTool            *tool,
                                                 GimpDisplay         *display,
                                                 const gchar         *title,
                                                 GimpOrientationType  axis,
                                                 gdouble              value,
                                                 const gchar         *help);
void              gimp_tool_replace_status      (GimpTool            *tool,
                                                 GimpDisplay         *display,
                                                 const gchar         *format,
                                                 ...) G_GNUC_PRINTF(3,4);
void              gimp_tool_pop_status          (GimpTool            *tool,
                                                 GimpDisplay         *display);

void              gimp_tool_message             (GimpTool            *tool,
                                                 GimpDisplay         *display,
                                                 const gchar         *format,
                                                 ...) G_GNUC_PRINTF(3,4);
void              gimp_tool_message_literal     (GimpTool            *tool,
                                                 GimpDisplay         *display,
                                                 const gchar         *message);

void              gimp_tool_set_cursor          (GimpTool            *tool,
                                                 GimpDisplay         *display,
                                                 GimpCursorType       cursor,
                                                 GimpToolCursorType   tool_cursor,
                                                 GimpCursorModifier   modifier);


#endif  /*  __GIMP_TOOL_H__  */
