/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolwidget.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_TOOL_WIDGET_H__
#define __GIMP_TOOL_WIDGET_H__


#include "core/gimpobject.h"


#define GIMP_TOOL_WIDGET_RESPONSE_CONFIRM -1
#define GIMP_TOOL_WIDGET_RESPONSE_CANCEL  -2
#define GIMP_TOOL_WIDGET_RESPONSE_RESET   -3


#define GIMP_TYPE_TOOL_WIDGET            (gimp_tool_widget_get_type ())
#define GIMP_TOOL_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_WIDGET, GimpToolWidget))
#define GIMP_TOOL_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_WIDGET, GimpToolWidgetClass))
#define GIMP_IS_TOOL_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_WIDGET))
#define GIMP_IS_TOOL_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_WIDGET))
#define GIMP_TOOL_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_WIDGET, GimpToolWidgetClass))


typedef struct _GimpToolWidgetPrivate GimpToolWidgetPrivate;
typedef struct _GimpToolWidgetClass   GimpToolWidgetClass;

struct _GimpToolWidget
{
  GimpObject             parent_instance;

  GimpToolWidgetPrivate *private;
};

struct _GimpToolWidgetClass
{
  GimpObjectClass  parent_class;

  /*  signals  */
  void     (* changed)         (GimpToolWidget        *widget);
  void     (* response)        (GimpToolWidget        *widget,
                                gint                   response_id);
  void     (* snap_offsets)    (GimpToolWidget        *widget,
                                gint                   offset_x,
                                gint                   offset_y,
                                gint                   width,
                                gint                   height);
  void     (* status)          (GimpToolWidget        *widget,
                                const gchar           *status);
  void     (* status_coords)   (GimpToolWidget        *widget,
                                const gchar           *title,
                                gdouble                x,
                                const gchar           *separator,
                                gdouble                y,
                                const gchar           *help);
  void     (* message)         (GimpToolWidget        *widget,
                                const gchar           *message);
  void     (* focus_changed)   (GimpToolWidget        *widget);

  /*  virtual functions  */
  gint     (* button_press)    (GimpToolWidget        *widget,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonPressType    press_type);
  void     (* button_release)  (GimpToolWidget        *widget,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type);
  void     (* motion)          (GimpToolWidget        *widget,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state);

  GimpHit  (* hit)             (GimpToolWidget        *widget,
                                const GimpCoords      *coords,
                                GdkModifierType        state,
                                gboolean               proximity);
  void     (* hover)           (GimpToolWidget        *widget,
                                const GimpCoords      *coords,
                                GdkModifierType        state,
                                gboolean               proximity);
  void     (* leave_notify)    (GimpToolWidget        *widget);

  gboolean (* key_press)       (GimpToolWidget        *widget,
                                GdkEventKey           *kevent);
  gboolean (* key_release)     (GimpToolWidget        *widget,
                                GdkEventKey           *kevent);

  void     (* motion_modifier) (GimpToolWidget        *widget,
                                GdkModifierType        key,
                                gboolean               press,
                                GdkModifierType        state);
  void     (* hover_modifier)  (GimpToolWidget        *widget,
                                GdkModifierType        key,
                                gboolean               press,
                                GdkModifierType        state);

  gboolean (* get_cursor)      (GimpToolWidget        *widget,
                                const GimpCoords      *coords,
                                GdkModifierType        state,
                                GimpCursorType        *cursor,
                                GimpToolCursorType    *tool_cursor,
                                GimpCursorModifier    *modifier);

  GimpUIManager *
           (* get_popup)       (GimpToolWidget        *widget,
                                const GimpCoords      *coords,
                                GdkModifierType        state,
                                const gchar          **ui_path);

  gboolean update_on_scale;
  gboolean update_on_scroll;
  gboolean update_on_rotate;
};


GType              gimp_tool_widget_get_type          (void) G_GNUC_CONST;

GimpDisplayShell * gimp_tool_widget_get_shell         (GimpToolWidget  *widget);
GimpCanvasItem   * gimp_tool_widget_get_item          (GimpToolWidget  *widget);

void               gimp_tool_widget_set_visible       (GimpToolWidget  *widget,
                                                       gboolean         visible);
gboolean           gimp_tool_widget_get_visible       (GimpToolWidget  *widget);

void               gimp_tool_widget_set_focus         (GimpToolWidget  *widget,
                                                       gboolean         focus);
gboolean           gimp_tool_widget_get_focus         (GimpToolWidget  *widget);

/*  for subclasses, to notify the handling tool
 */
void               gimp_tool_widget_changed           (GimpToolWidget  *widget);

void               gimp_tool_widget_response          (GimpToolWidget  *widget,
                                                       gint             response_id);

void               gimp_tool_widget_set_snap_offsets  (GimpToolWidget  *widget,
                                                       gint             offset_x,
                                                       gint             offset_y,
                                                       gint             width,
                                                       gint             height);
void               gimp_tool_widget_get_snap_offsets  (GimpToolWidget  *widget,
                                                       gint            *offset_x,
                                                       gint            *offset_y,
                                                       gint            *width,
                                                       gint            *height);

void               gimp_tool_widget_set_status        (GimpToolWidget  *widget,
                                                       const gchar     *status);
void               gimp_tool_widget_set_status_coords (GimpToolWidget  *widget,
                                                       const gchar     *title,
                                                       gdouble          x,
                                                       const gchar     *separator,
                                                       gdouble          y,
                                                       const gchar     *help);

void               gimp_tool_widget_message           (GimpToolWidget  *widget,
                                                       const gchar     *format,
                                                       ...) G_GNUC_PRINTF (2, 3);
void               gimp_tool_widget_message_literal   (GimpToolWidget  *widget,
                                                       const gchar     *message);

/*  for subclasses, to add and manage their items
 */
void               gimp_tool_widget_add_item         (GimpToolWidget  *widget,
                                                      GimpCanvasItem  *item);
void               gimp_tool_widget_remove_item      (GimpToolWidget  *widget,
                                                      GimpCanvasItem  *item);

GimpCanvasGroup  * gimp_tool_widget_add_group        (GimpToolWidget  *widget);
GimpCanvasGroup  * gimp_tool_widget_add_stroke_group (GimpToolWidget  *widget);
GimpCanvasGroup  * gimp_tool_widget_add_fill_group   (GimpToolWidget  *widget);

void               gimp_tool_widget_push_group       (GimpToolWidget  *widget,
                                                      GimpCanvasGroup *group);
void               gimp_tool_widget_pop_group        (GimpToolWidget  *widget);

/*  for subclasses, convenience functions to add specific items
 */
GimpCanvasItem * gimp_tool_widget_add_line      (GimpToolWidget       *widget,
                                                 gdouble               x1,
                                                 gdouble               y1,
                                                 gdouble               x2,
                                                 gdouble               y2);
GimpCanvasItem * gimp_tool_widget_add_rectangle (GimpToolWidget       *widget,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               width,
                                                 gdouble               height,
                                                 gboolean              filled);
GimpCanvasItem * gimp_tool_widget_add_arc       (GimpToolWidget       *widget,
                                                 gdouble               center_x,
                                                 gdouble               center_y,
                                                 gdouble               radius_x,
                                                 gdouble               radius_y,
                                                 gdouble               start_angle,
                                                 gdouble               slice_angle,
                                                 gboolean              filled);
GimpCanvasItem * gimp_tool_widget_add_limit     (GimpToolWidget       *widget,
                                                 GimpLimitType         type,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               radius,
                                                 gdouble               aspect_ratio,
                                                 gdouble               angle,
                                                 gboolean              dashed);
GimpCanvasItem * gimp_tool_widget_add_polygon   (GimpToolWidget       *widget,
                                                 GimpMatrix3          *transform,
                                                 const GimpVector2    *points,
                                                 gint                  n_points,
                                                 gboolean              filled);
GimpCanvasItem * gimp_tool_widget_add_polygon_from_coords
                                                (GimpToolWidget       *widget,
                                                 GimpMatrix3          *transform,
                                                 const GimpCoords     *points,
                                                 gint                  n_points,
                                                 gboolean              filled);
GimpCanvasItem * gimp_tool_widget_add_path      (GimpToolWidget       *widget,
                                                 const GimpBezierDesc *desc);

GimpCanvasItem * gimp_tool_widget_add_handle    (GimpToolWidget       *widget,
                                                 GimpHandleType        type,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gint                  width,
                                                 gint                  height,
                                                 GimpHandleAnchor      anchor);
GimpCanvasItem * gimp_tool_widget_add_corner    (GimpToolWidget       *widget,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               width,
                                                 gdouble               height,
                                                 GimpHandleAnchor      anchor,
                                                 gint                  corner_width,
                                                 gint                  corner_height,
                                                 gboolean              outside);

GimpCanvasItem * gimp_tool_widget_add_rectangle_guides
                                                (GimpToolWidget       *widget,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               width,
                                                 gdouble               height,
                                                 GimpGuidesType        type);
GimpCanvasItem * gimp_tool_widget_add_transform_guides
                                                (GimpToolWidget       *widget,
                                                 const GimpMatrix3    *transform,
                                                 gdouble               x1,
                                                 gdouble               y1,
                                                 gdouble               x2,
                                                 gdouble               y2,
                                                 GimpGuidesType        type,
                                                 gint                  n_guides,
                                                 gboolean              clip);

/*  for tools, to be called from the respective GimpTool method
 *  implementations
 */
gint       gimp_tool_widget_button_press    (GimpToolWidget        *widget,
                                             const GimpCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             GimpButtonPressType    press_type);
void       gimp_tool_widget_button_release  (GimpToolWidget        *widget,
                                             const GimpCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             GimpButtonReleaseType  release_type);
void       gimp_tool_widget_motion          (GimpToolWidget        *widget,
                                             const GimpCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state);

GimpHit    gimp_tool_widget_hit             (GimpToolWidget        *widget,
                                             const GimpCoords      *coords,
                                             GdkModifierType        state,
                                             gboolean               proximity);
void       gimp_tool_widget_hover           (GimpToolWidget        *widget,
                                             const GimpCoords      *coords,
                                             GdkModifierType        state,
                                             gboolean               proximity);
void       gimp_tool_widget_leave_notify    (GimpToolWidget        *widget);

gboolean   gimp_tool_widget_key_press       (GimpToolWidget        *widget,
                                             GdkEventKey           *kevent);
gboolean   gimp_tool_widget_key_release     (GimpToolWidget        *widget,
                                             GdkEventKey           *kevent);

void       gimp_tool_widget_motion_modifier (GimpToolWidget        *widget,
                                             GdkModifierType        key,
                                             gboolean               press,
                                             GdkModifierType        state);
void       gimp_tool_widget_hover_modifier  (GimpToolWidget        *widget,
                                             GdkModifierType        key,
                                             gboolean               press,
                                             GdkModifierType        state);

gboolean   gimp_tool_widget_get_cursor      (GimpToolWidget        *widget,
                                             const GimpCoords      *coords,
                                             GdkModifierType        state,
                                             GimpCursorType        *cursor,
                                             GimpToolCursorType    *tool_cursor,
                                             GimpCursorModifier    *modifier);

GimpUIManager *
           gimp_tool_widget_get_popup       (GimpToolWidget        *widget,
                                             const GimpCoords      *coords,
                                             GdkModifierType        state,
                                             const gchar          **ui_path);

#endif /* __GIMP_TOOL_WIDGET_H__ */
