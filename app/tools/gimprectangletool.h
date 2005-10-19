/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef  __GIMP_RECTANGLE_TOOL_H__
#define  __GIMP_RECTANGLE_TOOL_H__

#include "gimptool.h"


typedef enum
{
  GIMP_RECTANGLE_TOOL_PROP_0,
  GIMP_RECTANGLE_TOOL_PROP_CONTROLS,
  GIMP_RECTANGLE_TOOL_PROP_DIMENSIONS_ENTRY,
  GIMP_RECTANGLE_TOOL_PROP_STARTX,
  GIMP_RECTANGLE_TOOL_PROP_STARTY,
  GIMP_RECTANGLE_TOOL_PROP_LASTX,
  GIMP_RECTANGLE_TOOL_PROP_LASTY,
  GIMP_RECTANGLE_TOOL_PROP_PRESSX,
  GIMP_RECTANGLE_TOOL_PROP_PRESSY,
  GIMP_RECTANGLE_TOOL_PROP_X1,
  GIMP_RECTANGLE_TOOL_PROP_Y1,
  GIMP_RECTANGLE_TOOL_PROP_X2,
  GIMP_RECTANGLE_TOOL_PROP_Y2,
  GIMP_RECTANGLE_TOOL_PROP_FUNCTION,
  GIMP_RECTANGLE_TOOL_PROP_DX1,
  GIMP_RECTANGLE_TOOL_PROP_DY1,
  GIMP_RECTANGLE_TOOL_PROP_DX2,
  GIMP_RECTANGLE_TOOL_PROP_DY2,
  GIMP_RECTANGLE_TOOL_PROP_DCW,
  GIMP_RECTANGLE_TOOL_PROP_DCH,
  GIMP_RECTANGLE_TOOL_PROP_ORIGX,
  GIMP_RECTANGLE_TOOL_PROP_ORIGY,
  GIMP_RECTANGLE_TOOL_PROP_SIZEW,
  GIMP_RECTANGLE_TOOL_PROP_SIZEH,
  GIMP_RECTANGLE_TOOL_PROP_LAST = GIMP_RECTANGLE_TOOL_PROP_SIZEH
} GimpRectangleToolProp;

#define GIMP_TYPE_RECTANGLE_TOOL               (gimp_rectangle_tool_interface_get_type ())
#define GIMP_IS_RECTANGLE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_RECTANGLE_TOOL))
#define GIMP_RECTANGLE_TOOL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RECTANGLE_TOOL, GimpRectangleTool))
#define GIMP_RECTANGLE_TOOL_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_RECTANGLE_TOOL, GimpRectangleToolInterface))


/*  possible functions  */
enum
{
  RECT_CREATING,
  RECT_MOVING,
  RECT_RESIZING_UPPER_LEFT,
  RECT_RESIZING_UPPER_RIGHT,
  RECT_RESIZING_LOWER_LEFT,
  RECT_RESIZING_LOWER_RIGHT,
  RECT_RESIZING_LEFT,
  RECT_RESIZING_RIGHT,
  RECT_RESIZING_TOP,
  RECT_RESIZING_BOTTOM,
  RECT_EXECUTING
};


typedef struct _GimpRectangleTool          GimpRectangleTool;
typedef struct _GimpRectangleToolInterface GimpRectangleToolInterface;

struct _GimpRectangleToolInterface
{
  GTypeInterface base_iface;

  gboolean (* execute)     (GimpRectangleTool *rect_tool,
                            gint               x,
                            gint               y,
                            gint               w,
                            gint               h);
};


GType       gimp_rectangle_tool_interface_get_type  (void) G_GNUC_CONST;

void        gimp_rectangle_tool_set_controls        (GimpRectangleTool *tool,
                                                     GtkWidget         *controls);
GtkWidget * gimp_rectangle_tool_get_controls        (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_entry           (GimpRectangleTool *tool,
                                                     GtkWidget         *entry);
GtkWidget * gimp_rectangle_tool_get_entry           (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_startx          (GimpRectangleTool *tool,
                                                     gint               startx);
gint        gimp_rectangle_tool_get_startx          (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_starty          (GimpRectangleTool *tool,
                                                     gint               starty);
gint        gimp_rectangle_tool_get_starty          (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_lastx           (GimpRectangleTool *tool,
                                                     gint               lastx);
gint        gimp_rectangle_tool_get_lastx           (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_lasty           (GimpRectangleTool *tool,
                                                     gint               lasty);
gint        gimp_rectangle_tool_get_lasty           (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_pressx          (GimpRectangleTool *tool,
                                                     gint               pressx);
gint        gimp_rectangle_tool_get_pressx          (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_pressy          (GimpRectangleTool *tool,
                                                     gint               pressy);
gint        gimp_rectangle_tool_get_pressy          (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_x1              (GimpRectangleTool *tool,
                                                     gint               x1);
gint        gimp_rectangle_tool_get_x1              (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_y1              (GimpRectangleTool *tool,
                                                     gint               y1);
gint        gimp_rectangle_tool_get_y1              (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_x2              (GimpRectangleTool *tool,
                                                     gint               x2);
gint        gimp_rectangle_tool_get_x2              (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_y2              (GimpRectangleTool *tool,
                                                     gint               y2);
gint        gimp_rectangle_tool_get_y2              (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_function        (GimpRectangleTool *tool,
                                                     guint              function);
guint       gimp_rectangle_tool_get_function        (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_dx1             (GimpRectangleTool *tool,
                                                     gint               dx1);
gint        gimp_rectangle_tool_get_dx1             (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_dy1             (GimpRectangleTool *tool,
                                                     gint               dy1);
gint        gimp_rectangle_tool_get_dy1             (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_dx2             (GimpRectangleTool *tool,
                                                     gint               dx2);
gint        gimp_rectangle_tool_get_dx2             (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_dy2             (GimpRectangleTool *tool,
                                                     gint               dy2);
gint        gimp_rectangle_tool_get_dy2             (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_dcw             (GimpRectangleTool *tool,
                                                     gint               dcw);
gint        gimp_rectangle_tool_get_dcw             (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_dch             (GimpRectangleTool *tool,
                                                     gint               dch);
gint        gimp_rectangle_tool_get_dch             (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_origx           (GimpRectangleTool *tool,
                                                     gdouble            origx);
gdouble     gimp_rectangle_tool_get_origx           (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_origy           (GimpRectangleTool *tool,
                                                     gdouble            origy);
gdouble     gimp_rectangle_tool_get_origy           (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_sizew           (GimpRectangleTool *tool,
                                                     gdouble            sizew);
gdouble     gimp_rectangle_tool_get_sizew           (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_sizeh           (GimpRectangleTool *tool,
                                                     gdouble            sizeh);
gdouble     gimp_rectangle_tool_get_sizeh           (GimpRectangleTool *tool);

void        gimp_rectangle_tool_constructor         (GObject           *object);
void        gimp_rectangle_tool_dispose             (GObject           *object);
gboolean    gimp_rectangle_tool_initialize          (GimpTool          *tool,
                                                     GimpDisplay       *gdisp);
void        gimp_rectangle_tool_button_press        (GimpTool          *tool,
                                                     GimpCoords        *coords,
                                                     guint32            time,
                                                     GdkModifierType    state,
                                                     GimpDisplay       *gdisp);
void        gimp_rectangle_tool_button_release      (GimpTool          *tool,
                                                     GimpCoords        *coords,
                                                     guint32            time,
                                                     GdkModifierType    state,
                                                     GimpDisplay       *gdisp);
void        gimp_rectangle_tool_motion              (GimpTool          *tool,
                                                     GimpCoords        *coords,
                                                     guint32            time,
                                                     GdkModifierType    state,
                                                     GimpDisplay       *gdisp);
gboolean    gimp_rectangle_tool_key_press           (GimpTool          *tool,
                                                     GdkEventKey       *kevent,
                                                     GimpDisplay       *gdisp);
void        gimp_rectangle_tool_modifier_key        (GimpTool          *tool,
                                                     GdkModifierType    key,
                                                     gboolean           press,
                                                     GdkModifierType    state,
                                                     GimpDisplay       *gdisp);
void        gimp_rectangle_tool_oper_update         (GimpTool          *tool,
                                                     GimpCoords        *coords,
                                                     GdkModifierType    state,
                                                     GimpDisplay       *gdisp);
void        gimp_rectangle_tool_cursor_update       (GimpTool          *tool,
                                                     GimpCoords        *coords,
                                                     GdkModifierType    state,
                                                     GimpDisplay       *gdisp);
void        gimp_rectangle_tool_draw                (GimpDrawTool      *draw);
gboolean    gimp_rectangle_tool_execute             (GimpRectangleTool *rect_tool,
                                                     gint               x,
                                                     gint               y,
                                                     gint               w,
                                                     gint               h);
void        gimp_rectangle_tool_response            (GtkWidget         *widget,
                                                     gint               response_id,
                                                     GimpRectangleTool *rectangle);
void        gimp_rectangle_tool_configure           (GimpRectangleTool *rectangle);


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
