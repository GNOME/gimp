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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "widgets/gimpoverlaybox.h"


#define GIMP_CANVAS_EVENT_MASK (GDK_EXPOSURE_MASK            | \
                                GDK_POINTER_MOTION_MASK      | \
                                GDK_BUTTON_PRESS_MASK        | \
                                GDK_BUTTON_RELEASE_MASK      | \
                                GDK_SCROLL_MASK              | \
                                GDK_SMOOTH_SCROLL_MASK       | \
                                GDK_TOUCHPAD_GESTURE_MASK    | \
                                GDK_STRUCTURE_MASK           | \
                                GDK_ENTER_NOTIFY_MASK        | \
                                GDK_LEAVE_NOTIFY_MASK        | \
                                GDK_FOCUS_CHANGE_MASK        | \
                                GDK_KEY_PRESS_MASK           | \
                                GDK_KEY_RELEASE_MASK         | \
                                GDK_PROXIMITY_OUT_MASK)


#define GIMP_TYPE_CANVAS            (gimp_canvas_get_type ())
#define GIMP_CANVAS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS, GimpCanvas))
#define GIMP_CANVAS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS, GimpCanvasClass))
#define GIMP_IS_CANVAS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS))
#define GIMP_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS))
#define GIMP_CANVAS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS, GimpCanvasClass))


typedef struct _GimpCanvasClass GimpCanvasClass;

struct _GimpCanvas
{
  GimpOverlayBox         parent_instance;

  GimpDisplayConfig     *config;
  PangoLayout           *layout;

  GimpCanvasPaddingMode  padding_mode;
  GeglColor             *padding_color;
};

struct _GimpCanvasClass
{
  GimpOverlayBoxClass  parent_class;
};


GType         gimp_canvas_get_type     (void) G_GNUC_CONST;

GtkWidget   * gimp_canvas_new          (GimpDisplayConfig     *config);

PangoLayout * gimp_canvas_get_layout   (GimpCanvas            *canvas,
                                        const gchar           *format,
                                        ...) G_GNUC_PRINTF (2, 3);

void          gimp_canvas_set_padding  (GimpCanvas            *canvas,
                                        GimpCanvasPaddingMode  padding_mode,
                                        GeglColor             *padding_color);
