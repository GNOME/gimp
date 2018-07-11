/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolrectangle.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *
 * Based on GimpRectangleTool
 * Copyright (C) 2007 Martin Nordholts
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

#ifndef __GIMP_TOOL_RECTANGLE_H__
#define __GIMP_TOOL_RECTANGLE_H__


#include "gimptoolwidget.h"


typedef enum
{
  GIMP_TOOL_RECTANGLE_DEAD,
  GIMP_TOOL_RECTANGLE_CREATING,
  GIMP_TOOL_RECTANGLE_MOVING,
  GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT,
  GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT,
  GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT,
  GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT,
  GIMP_TOOL_RECTANGLE_RESIZING_LEFT,
  GIMP_TOOL_RECTANGLE_RESIZING_RIGHT,
  GIMP_TOOL_RECTANGLE_RESIZING_TOP,
  GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM,
  GIMP_TOOL_RECTANGLE_AUTO_SHRINK,
  GIMP_TOOL_RECTANGLE_EXECUTING,
  GIMP_N_TOOL_RECTANGLE_FUNCTIONS
} GimpRectangleFunction;


#define GIMP_TYPE_TOOL_RECTANGLE            (gimp_tool_rectangle_get_type ())
#define GIMP_TOOL_RECTANGLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_RECTANGLE, GimpToolRectangle))
#define GIMP_TOOL_RECTANGLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_RECTANGLE, GimpToolRectangleClass))
#define GIMP_IS_TOOL_RECTANGLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_RECTANGLE))
#define GIMP_IS_TOOL_RECTANGLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_RECTANGLE))
#define GIMP_TOOL_RECTANGLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_RECTANGLE, GimpToolRectangleClass))


typedef struct _GimpToolRectangle        GimpToolRectangle;
typedef struct _GimpToolRectanglePrivate GimpToolRectanglePrivate;
typedef struct _GimpToolRectangleClass   GimpToolRectangleClass;

struct _GimpToolRectangle
{
  GimpToolWidget            parent_instance;

  GimpToolRectanglePrivate *private;
};

struct _GimpToolRectangleClass
{
  GimpToolWidgetClass  parent_class;

  /*  signals  */

  gboolean (* change_complete) (GimpToolRectangle *rectangle);
};


GType    gimp_tool_rectangle_get_type         (void) G_GNUC_CONST;

GimpToolWidget * gimp_tool_rectangle_new      (GimpDisplayShell       *shell);

GimpRectangleFunction
         gimp_tool_rectangle_get_function     (GimpToolRectangle      *rectangle);
void     gimp_tool_rectangle_set_function     (GimpToolRectangle      *rectangle,
                                               GimpRectangleFunction   function);

void     gimp_tool_rectangle_set_constraint   (GimpToolRectangle      *rectangle,
                                               GimpRectangleConstraint constraint);
GimpRectangleConstraint
         gimp_tool_rectangle_get_constraint   (GimpToolRectangle      *rectangle);

void     gimp_tool_rectangle_get_public_rect  (GimpToolRectangle      *rectangle,
                                               gdouble                *pub_x1,
                                               gdouble                *pub_y1,
                                               gdouble                *pub_x2,
                                               gdouble                *pub_y2);

void     gimp_tool_rectangle_pending_size_set (GimpToolRectangle      *rectangle,
                                               GObject                *object,
                                               const gchar            *width_property,
                                               const gchar            *height_property);

void     gimp_tool_rectangle_constraint_size_set
                                              (GimpToolRectangle      *rectangle,
                                               GObject                *object,
                                               const gchar            *width_property,
                                               const gchar            *height_property);

gboolean gimp_tool_rectangle_rectangle_is_first
                                              (GimpToolRectangle      *rectangle);
gboolean gimp_tool_rectangle_rectangle_is_new (GimpToolRectangle      *rectangle);
gboolean gimp_tool_rectangle_point_in_rectangle
                                              (GimpToolRectangle      *rectangle,
                                               gdouble                 x,
                                               gdouble                 y);

void     gimp_tool_rectangle_frame_item       (GimpToolRectangle      *rectangle,
                                               GimpItem               *item);
void     gimp_tool_rectangle_auto_shrink      (GimpToolRectangle      *rectrectangle,
                                               gboolean                shrink_merged);


#endif /* __GIMP_TOOL_RECTANGLE_H__ */
