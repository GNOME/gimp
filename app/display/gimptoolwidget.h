/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolwidget.h
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TOOL_WIDGET_H__
#define __LIGMA_TOOL_WIDGET_H__


#include "core/ligmaobject.h"


#define LIGMA_TOOL_WIDGET_RESPONSE_CONFIRM -1
#define LIGMA_TOOL_WIDGET_RESPONSE_CANCEL  -2
#define LIGMA_TOOL_WIDGET_RESPONSE_RESET   -3


#define LIGMA_TYPE_TOOL_WIDGET            (ligma_tool_widget_get_type ())
#define LIGMA_TOOL_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_WIDGET, LigmaToolWidget))
#define LIGMA_TOOL_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_WIDGET, LigmaToolWidgetClass))
#define LIGMA_IS_TOOL_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_WIDGET))
#define LIGMA_IS_TOOL_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_WIDGET))
#define LIGMA_TOOL_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_WIDGET, LigmaToolWidgetClass))


typedef struct _LigmaToolWidgetPrivate LigmaToolWidgetPrivate;
typedef struct _LigmaToolWidgetClass   LigmaToolWidgetClass;

struct _LigmaToolWidget
{
  LigmaObject             parent_instance;

  LigmaToolWidgetPrivate *private;
};

struct _LigmaToolWidgetClass
{
  LigmaObjectClass  parent_class;

  /*  signals  */
  void     (* changed)         (LigmaToolWidget        *widget);
  void     (* response)        (LigmaToolWidget        *widget,
                                gint                   response_id);
  void     (* snap_offsets)    (LigmaToolWidget        *widget,
                                gint                   offset_x,
                                gint                   offset_y,
                                gint                   width,
                                gint                   height);
  void     (* status)          (LigmaToolWidget        *widget,
                                const gchar           *status);
  void     (* status_coords)   (LigmaToolWidget        *widget,
                                const gchar           *title,
                                gdouble                x,
                                const gchar           *separator,
                                gdouble                y,
                                const gchar           *help);
  void     (* message)         (LigmaToolWidget        *widget,
                                const gchar           *message);
  void     (* focus_changed)   (LigmaToolWidget        *widget);

  /*  virtual functions  */
  gint     (* button_press)    (LigmaToolWidget        *widget,
                                const LigmaCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                LigmaButtonPressType    press_type);
  void     (* button_release)  (LigmaToolWidget        *widget,
                                const LigmaCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                LigmaButtonReleaseType  release_type);
  void     (* motion)          (LigmaToolWidget        *widget,
                                const LigmaCoords      *coords,
                                guint32                time,
                                GdkModifierType        state);

  LigmaHit  (* hit)             (LigmaToolWidget        *widget,
                                const LigmaCoords      *coords,
                                GdkModifierType        state,
                                gboolean               proximity);
  void     (* hover)           (LigmaToolWidget        *widget,
                                const LigmaCoords      *coords,
                                GdkModifierType        state,
                                gboolean               proximity);
  void     (* leave_notify)    (LigmaToolWidget        *widget);

  gboolean (* key_press)       (LigmaToolWidget        *widget,
                                GdkEventKey           *kevent);
  gboolean (* key_release)     (LigmaToolWidget        *widget,
                                GdkEventKey           *kevent);

  void     (* motion_modifier) (LigmaToolWidget        *widget,
                                GdkModifierType        key,
                                gboolean               press,
                                GdkModifierType        state);
  void     (* hover_modifier)  (LigmaToolWidget        *widget,
                                GdkModifierType        key,
                                gboolean               press,
                                GdkModifierType        state);

  gboolean (* get_cursor)      (LigmaToolWidget        *widget,
                                const LigmaCoords      *coords,
                                GdkModifierType        state,
                                LigmaCursorType        *cursor,
                                LigmaToolCursorType    *tool_cursor,
                                LigmaCursorModifier    *modifier);

  LigmaUIManager *
           (* get_popup)       (LigmaToolWidget        *widget,
                                const LigmaCoords      *coords,
                                GdkModifierType        state,
                                const gchar          **ui_path);

  gboolean update_on_scale;
  gboolean update_on_scroll;
  gboolean update_on_rotate;
};


GType              ligma_tool_widget_get_type          (void) G_GNUC_CONST;

LigmaDisplayShell * ligma_tool_widget_get_shell         (LigmaToolWidget  *widget);
LigmaCanvasItem   * ligma_tool_widget_get_item          (LigmaToolWidget  *widget);

void               ligma_tool_widget_set_visible       (LigmaToolWidget  *widget,
                                                       gboolean         visible);
gboolean           ligma_tool_widget_get_visible       (LigmaToolWidget  *widget);

void               ligma_tool_widget_set_focus         (LigmaToolWidget  *widget,
                                                       gboolean         focus);
gboolean           ligma_tool_widget_get_focus         (LigmaToolWidget  *widget);

/*  for subclasses, to notify the handling tool
 */
void               ligma_tool_widget_changed           (LigmaToolWidget  *widget);

void               ligma_tool_widget_response          (LigmaToolWidget  *widget,
                                                       gint             response_id);

void               ligma_tool_widget_set_snap_offsets  (LigmaToolWidget  *widget,
                                                       gint             offset_x,
                                                       gint             offset_y,
                                                       gint             width,
                                                       gint             height);
void               ligma_tool_widget_get_snap_offsets  (LigmaToolWidget  *widget,
                                                       gint            *offset_x,
                                                       gint            *offset_y,
                                                       gint            *width,
                                                       gint            *height);

void               ligma_tool_widget_set_status        (LigmaToolWidget  *widget,
                                                       const gchar     *status);
void               ligma_tool_widget_set_status_coords (LigmaToolWidget  *widget,
                                                       const gchar     *title,
                                                       gdouble          x,
                                                       const gchar     *separator,
                                                       gdouble          y,
                                                       const gchar     *help);

void               ligma_tool_widget_message           (LigmaToolWidget  *widget,
                                                       const gchar     *format,
                                                       ...) G_GNUC_PRINTF (2, 3);
void               ligma_tool_widget_message_literal   (LigmaToolWidget  *widget,
                                                       const gchar     *message);

/*  for subclasses, to add and manage their items
 */
void               ligma_tool_widget_add_item         (LigmaToolWidget  *widget,
                                                      LigmaCanvasItem  *item);
void               ligma_tool_widget_remove_item      (LigmaToolWidget  *widget,
                                                      LigmaCanvasItem  *item);

LigmaCanvasGroup  * ligma_tool_widget_add_group        (LigmaToolWidget  *widget);
LigmaCanvasGroup  * ligma_tool_widget_add_stroke_group (LigmaToolWidget  *widget);
LigmaCanvasGroup  * ligma_tool_widget_add_fill_group   (LigmaToolWidget  *widget);

void               ligma_tool_widget_push_group       (LigmaToolWidget  *widget,
                                                      LigmaCanvasGroup *group);
void               ligma_tool_widget_pop_group        (LigmaToolWidget  *widget);

/*  for subclasses, convenience functions to add specific items
 */
LigmaCanvasItem * ligma_tool_widget_add_line      (LigmaToolWidget       *widget,
                                                 gdouble               x1,
                                                 gdouble               y1,
                                                 gdouble               x2,
                                                 gdouble               y2);
LigmaCanvasItem * ligma_tool_widget_add_rectangle (LigmaToolWidget       *widget,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               width,
                                                 gdouble               height,
                                                 gboolean              filled);
LigmaCanvasItem * ligma_tool_widget_add_arc       (LigmaToolWidget       *widget,
                                                 gdouble               center_x,
                                                 gdouble               center_y,
                                                 gdouble               radius_x,
                                                 gdouble               radius_y,
                                                 gdouble               start_angle,
                                                 gdouble               slice_angle,
                                                 gboolean              filled);
LigmaCanvasItem * ligma_tool_widget_add_limit     (LigmaToolWidget       *widget,
                                                 LigmaLimitType         type,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               radius,
                                                 gdouble               aspect_ratio,
                                                 gdouble               angle,
                                                 gboolean              dashed);
LigmaCanvasItem * ligma_tool_widget_add_polygon   (LigmaToolWidget       *widget,
                                                 LigmaMatrix3          *transform,
                                                 const LigmaVector2    *points,
                                                 gint                  n_points,
                                                 gboolean              filled);
LigmaCanvasItem * ligma_tool_widget_add_polygon_from_coords
                                                (LigmaToolWidget       *widget,
                                                 LigmaMatrix3          *transform,
                                                 const LigmaCoords     *points,
                                                 gint                  n_points,
                                                 gboolean              filled);
LigmaCanvasItem * ligma_tool_widget_add_path      (LigmaToolWidget       *widget,
                                                 const LigmaBezierDesc *desc);

LigmaCanvasItem * ligma_tool_widget_add_handle    (LigmaToolWidget       *widget,
                                                 LigmaHandleType        type,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gint                  width,
                                                 gint                  height,
                                                 LigmaHandleAnchor      anchor);
LigmaCanvasItem * ligma_tool_widget_add_corner    (LigmaToolWidget       *widget,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               width,
                                                 gdouble               height,
                                                 LigmaHandleAnchor      anchor,
                                                 gint                  corner_width,
                                                 gint                  corner_height,
                                                 gboolean              outside);

LigmaCanvasItem * ligma_tool_widget_add_rectangle_guides
                                                (LigmaToolWidget       *widget,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               width,
                                                 gdouble               height,
                                                 LigmaGuidesType        type);
LigmaCanvasItem * ligma_tool_widget_add_transform_guides
                                                (LigmaToolWidget       *widget,
                                                 const LigmaMatrix3    *transform,
                                                 gdouble               x1,
                                                 gdouble               y1,
                                                 gdouble               x2,
                                                 gdouble               y2,
                                                 LigmaGuidesType        type,
                                                 gint                  n_guides,
                                                 gboolean              clip);

/*  for tools, to be called from the respective LigmaTool method
 *  implementations
 */
gint       ligma_tool_widget_button_press    (LigmaToolWidget        *widget,
                                             const LigmaCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             LigmaButtonPressType    press_type);
void       ligma_tool_widget_button_release  (LigmaToolWidget        *widget,
                                             const LigmaCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             LigmaButtonReleaseType  release_type);
void       ligma_tool_widget_motion          (LigmaToolWidget        *widget,
                                             const LigmaCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state);

LigmaHit    ligma_tool_widget_hit             (LigmaToolWidget        *widget,
                                             const LigmaCoords      *coords,
                                             GdkModifierType        state,
                                             gboolean               proximity);
void       ligma_tool_widget_hover           (LigmaToolWidget        *widget,
                                             const LigmaCoords      *coords,
                                             GdkModifierType        state,
                                             gboolean               proximity);
void       ligma_tool_widget_leave_notify    (LigmaToolWidget        *widget);

gboolean   ligma_tool_widget_key_press       (LigmaToolWidget        *widget,
                                             GdkEventKey           *kevent);
gboolean   ligma_tool_widget_key_release     (LigmaToolWidget        *widget,
                                             GdkEventKey           *kevent);

void       ligma_tool_widget_motion_modifier (LigmaToolWidget        *widget,
                                             GdkModifierType        key,
                                             gboolean               press,
                                             GdkModifierType        state);
void       ligma_tool_widget_hover_modifier  (LigmaToolWidget        *widget,
                                             GdkModifierType        key,
                                             gboolean               press,
                                             GdkModifierType        state);

gboolean   ligma_tool_widget_get_cursor      (LigmaToolWidget        *widget,
                                             const LigmaCoords      *coords,
                                             GdkModifierType        state,
                                             LigmaCursorType        *cursor,
                                             LigmaToolCursorType    *tool_cursor,
                                             LigmaCursorModifier    *modifier);

LigmaUIManager *
           ligma_tool_widget_get_popup       (LigmaToolWidget        *widget,
                                             const LigmaCoords      *coords,
                                             GdkModifierType        state,
                                             const gchar          **ui_path);

#endif /* __LIGMA_TOOL_WIDGET_H__ */
