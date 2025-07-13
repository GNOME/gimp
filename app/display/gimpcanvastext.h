/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvastext.h
 * Copyright (C) 2023 mr.fantastic <mrfantastic@firemail.cc>
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

#pragma once

#include "gimpcanvasitem.h"


#define GIMP_TYPE_CANVAS_TEXT            (gimp_canvas_text_get_type ())
#define GIMP_CANVAS_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_TEXT, GimpCanvasText))
#define GIMP_CANVAS_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_TEXT, GimpCanvasTextClass))
#define GIMP_IS_CANVAS_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((klass), GIMP_TYPE_CANVAS_TEXT))
#define GIMP_IS_CANVAS_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_TEXT))
#define GIMP_CANVAS_TEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_TEXT, GimpCanvasTextClass))


typedef struct _GimpCanvasText GimpCanvasText;
typedef struct _GimpCanvasTextClass GimpCanvasTextClass;

struct _GimpCanvasText
{
  GimpCanvasItem  parent_instance;
};

struct _GimpCanvasTextClass
{
  GimpCanvasItemClass  parent_class;
};


GType            gimp_canvas_text_get_type (void) G_GNUC_CONST;

GimpCanvasItem * gimp_canvas_text_new      (GimpDisplayShell *shell,
                                            gdouble           x,
                                            gdouble           y,
                                            gdouble           font_size,
                                            gchar            *text);
