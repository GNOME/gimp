/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef  __GIMP_RECTANGLE_TOOL_H__
#define  __GIMP_RECTANGLE_TOOL_H__


typedef enum
{
  GIMP_RECTANGLE_TOOL_PROP_0,
  GIMP_RECTANGLE_TOOL_PROP_X1,
  GIMP_RECTANGLE_TOOL_PROP_Y1,
  GIMP_RECTANGLE_TOOL_PROP_X2,
  GIMP_RECTANGLE_TOOL_PROP_Y2,
  GIMP_RECTANGLE_TOOL_PROP_CONSTRAINT,
  GIMP_RECTANGLE_TOOL_PROP_PRECISION,
  GIMP_RECTANGLE_TOOL_PROP_NARROW_MODE,
  GIMP_RECTANGLE_TOOL_PROP_LAST = GIMP_RECTANGLE_TOOL_PROP_NARROW_MODE
} GimpRectangleToolProp;


typedef enum
{
  GIMP_RECTANGLE_TOOL_INACTIVE,
  GIMP_RECTANGLE_TOOL_DEAD,
  GIMP_RECTANGLE_TOOL_CREATING,
  GIMP_RECTANGLE_TOOL_MOVING,
  GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT,
  GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT,
  GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT,
  GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT,
  GIMP_RECTANGLE_TOOL_RESIZING_LEFT,
  GIMP_RECTANGLE_TOOL_RESIZING_RIGHT,
  GIMP_RECTANGLE_TOOL_RESIZING_TOP,
  GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM,
  GIMP_RECTANGLE_TOOL_AUTO_SHRINK,
  GIMP_RECTANGLE_TOOL_EXECUTING
} GimpRectangleFunction;


#define GIMP_TYPE_RECTANGLE_TOOL               (gimp_rectangle_tool_interface_get_type ())
#define GIMP_IS_RECTANGLE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_RECTANGLE_TOOL))
#define GIMP_RECTANGLE_TOOL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RECTANGLE_TOOL, GimpRectangleTool))
#define GIMP_RECTANGLE_TOOL_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_RECTANGLE_TOOL, GimpRectangleToolInterface))

#define GIMP_RECTANGLE_TOOL_GET_OPTIONS(t)     (GIMP_RECTANGLE_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpRectangleTool          GimpRectangleTool;
typedef struct _GimpRectangleToolInterface GimpRectangleToolInterface;

struct _GimpRectangleToolInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  gboolean (* execute)           (GimpRectangleTool *rect_tool,
                                  gint               x,
                                  gint               y,
                                  gint               w,
                                  gint               h);
  void     (* cancel)            (GimpRectangleTool *rect_tool);

  /*  signals  */
  gboolean (* rectangle_change_complete) (GimpRectangleTool *rect_tool);
};


GType       gimp_rectangle_tool_interface_get_type  (void) G_GNUC_CONST;

void        gimp_rectangle_tool_constructor         (GObject                 *object);

void        gimp_rectangle_tool_init                (GimpRectangleTool       *rect_tool);
void        gimp_rectangle_tool_control             (GimpTool                *tool,
                                                     GimpToolAction           action,
                                                     GimpDisplay             *display);
void        gimp_rectangle_tool_button_press        (GimpTool                *tool,
                                                     const GimpCoords        *coords,
                                                     guint32                  time,
                                                     GdkModifierType          state,
                                                     GimpDisplay             *display);
void        gimp_rectangle_tool_button_release      (GimpTool                *tool,
                                                     const GimpCoords        *coords,
                                                     guint32                  time,
                                                     GdkModifierType          state,
                                                     GimpButtonReleaseType    release_type,
                                                     GimpDisplay             *display);
void        gimp_rectangle_tool_motion              (GimpTool                *tool,
                                                     const GimpCoords        *coords,
                                                     guint32                  time,
                                                     GdkModifierType          state,
                                                     GimpDisplay             *display);
gboolean    gimp_rectangle_tool_key_press           (GimpTool                *tool,
                                                     GdkEventKey             *kevent,
                                                     GimpDisplay             *display);
void        gimp_rectangle_tool_active_modifier_key (GimpTool                *tool,
                                                     GdkModifierType          key,
                                                     gboolean                 press,
                                                     GdkModifierType          state,
                                                     GimpDisplay             *display);
void        gimp_rectangle_tool_oper_update         (GimpTool                *tool,
                                                     const GimpCoords        *coords,
                                                     GdkModifierType          state,
                                                     gboolean                 proximity,
                                                     GimpDisplay             *display);
void        gimp_rectangle_tool_cursor_update       (GimpTool                *tool,
                                                     const GimpCoords        *coords,
                                                     GdkModifierType          state,
                                                     GimpDisplay             *display);
void        gimp_rectangle_tool_draw                (GimpDrawTool            *draw,
                                                     GimpCanvasGroup         *stroke_group);
gboolean    gimp_rectangle_tool_execute             (GimpRectangleTool       *rect_tool);
void        gimp_rectangle_tool_cancel              (GimpRectangleTool       *rect_tool);
void        gimp_rectangle_tool_set_constraint      (GimpRectangleTool       *rectangle,
                                                     GimpRectangleConstraint  constraint);
GimpRectangleConstraint gimp_rectangle_tool_get_constraint
                                                    (GimpRectangleTool       *rectangle);
GimpRectangleFunction gimp_rectangle_tool_get_function (GimpRectangleTool    *rectangle);
void        gimp_rectangle_tool_set_function        (GimpRectangleTool       *rectangle,
                                                     GimpRectangleFunction    function);
void        gimp_rectangle_tool_pending_size_set    (GimpRectangleTool       *rectangle,
                                                     GObject                 *object,
                                                     const gchar             *width_property,
                                                     const gchar             *height_property);
void        gimp_rectangle_tool_constraint_size_set (GimpRectangleTool       *rectangle,
                                                     GObject                 *object,
                                                     const gchar             *width_property,
                                                     const gchar             *height_property);
gboolean    gimp_rectangle_tool_rectangle_is_new    (GimpRectangleTool       *rect_tool);
gboolean    gimp_rectangle_tool_point_in_rectangle  (GimpRectangleTool       *rect_tool,
                                                     gdouble                  x,
                                                     gdouble                  y);
void        gimp_rectangle_tool_frame_item          (GimpRectangleTool       *rect_tool,
                                                     GimpItem                *item);


/*  convenience functions  */

void        gimp_rectangle_tool_install_properties  (GObjectClass *klass);
void        gimp_rectangle_tool_set_property        (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
void        gimp_rectangle_tool_get_property        (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);


#endif  /*  __GIMP_RECTANGLE_TOOL_H__  */
