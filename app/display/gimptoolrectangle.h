/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolrectangle.h
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
 *
 * Based on LigmaRectangleTool
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

#ifndef __LIGMA_TOOL_RECTANGLE_H__
#define __LIGMA_TOOL_RECTANGLE_H__


#include "ligmatoolwidget.h"


typedef enum
{
  LIGMA_TOOL_RECTANGLE_DEAD,
  LIGMA_TOOL_RECTANGLE_CREATING,
  LIGMA_TOOL_RECTANGLE_MOVING,
  LIGMA_TOOL_RECTANGLE_RESIZING_UPPER_LEFT,
  LIGMA_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT,
  LIGMA_TOOL_RECTANGLE_RESIZING_LOWER_LEFT,
  LIGMA_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT,
  LIGMA_TOOL_RECTANGLE_RESIZING_LEFT,
  LIGMA_TOOL_RECTANGLE_RESIZING_RIGHT,
  LIGMA_TOOL_RECTANGLE_RESIZING_TOP,
  LIGMA_TOOL_RECTANGLE_RESIZING_BOTTOM,
  LIGMA_TOOL_RECTANGLE_AUTO_SHRINK,
  LIGMA_TOOL_RECTANGLE_EXECUTING,
  LIGMA_N_TOOL_RECTANGLE_FUNCTIONS
} LigmaRectangleFunction;


#define LIGMA_TYPE_TOOL_RECTANGLE            (ligma_tool_rectangle_get_type ())
#define LIGMA_TOOL_RECTANGLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_RECTANGLE, LigmaToolRectangle))
#define LIGMA_TOOL_RECTANGLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_RECTANGLE, LigmaToolRectangleClass))
#define LIGMA_IS_TOOL_RECTANGLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_RECTANGLE))
#define LIGMA_IS_TOOL_RECTANGLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_RECTANGLE))
#define LIGMA_TOOL_RECTANGLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_RECTANGLE, LigmaToolRectangleClass))


typedef struct _LigmaToolRectangle        LigmaToolRectangle;
typedef struct _LigmaToolRectanglePrivate LigmaToolRectanglePrivate;
typedef struct _LigmaToolRectangleClass   LigmaToolRectangleClass;

struct _LigmaToolRectangle
{
  LigmaToolWidget            parent_instance;

  LigmaToolRectanglePrivate *private;
};

struct _LigmaToolRectangleClass
{
  LigmaToolWidgetClass  parent_class;

  /*  signals  */

  gboolean (* change_complete) (LigmaToolRectangle *rectangle);
};


GType    ligma_tool_rectangle_get_type         (void) G_GNUC_CONST;

LigmaToolWidget * ligma_tool_rectangle_new      (LigmaDisplayShell       *shell);

LigmaRectangleFunction
         ligma_tool_rectangle_get_function     (LigmaToolRectangle      *rectangle);
void     ligma_tool_rectangle_set_function     (LigmaToolRectangle      *rectangle,
                                               LigmaRectangleFunction   function);

void     ligma_tool_rectangle_set_constraint   (LigmaToolRectangle      *rectangle,
                                               LigmaRectangleConstraint constraint);
LigmaRectangleConstraint
         ligma_tool_rectangle_get_constraint   (LigmaToolRectangle      *rectangle);

void     ligma_tool_rectangle_get_public_rect  (LigmaToolRectangle      *rectangle,
                                               gdouble                *pub_x1,
                                               gdouble                *pub_y1,
                                               gdouble                *pub_x2,
                                               gdouble                *pub_y2);

void     ligma_tool_rectangle_pending_size_set (LigmaToolRectangle      *rectangle,
                                               GObject                *object,
                                               const gchar            *width_property,
                                               const gchar            *height_property);

void     ligma_tool_rectangle_constraint_size_set
                                              (LigmaToolRectangle      *rectangle,
                                               GObject                *object,
                                               const gchar            *width_property,
                                               const gchar            *height_property);

gboolean ligma_tool_rectangle_rectangle_is_first
                                              (LigmaToolRectangle      *rectangle);
gboolean ligma_tool_rectangle_rectangle_is_new (LigmaToolRectangle      *rectangle);
gboolean ligma_tool_rectangle_point_in_rectangle
                                              (LigmaToolRectangle      *rectangle,
                                               gdouble                 x,
                                               gdouble                 y);

void     ligma_tool_rectangle_frame_item       (LigmaToolRectangle      *rectangle,
                                               LigmaItem               *item);
void     ligma_tool_rectangle_auto_shrink      (LigmaToolRectangle      *rectrectangle,
                                               gboolean                shrink_merged);


#endif /* __LIGMA_TOOL_RECTANGLE_H__ */
