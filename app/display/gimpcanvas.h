/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_CANVAS_H__
#define __LIGMA_CANVAS_H__


#include "widgets/ligmaoverlaybox.h"


#define LIGMA_CANVAS_EVENT_MASK (GDK_EXPOSURE_MASK            | \
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


#define LIGMA_TYPE_CANVAS            (ligma_canvas_get_type ())
#define LIGMA_CANVAS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS, LigmaCanvas))
#define LIGMA_CANVAS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS, LigmaCanvasClass))
#define LIGMA_IS_CANVAS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS))
#define LIGMA_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS))
#define LIGMA_CANVAS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS, LigmaCanvasClass))


typedef struct _LigmaCanvasClass LigmaCanvasClass;

struct _LigmaCanvas
{
  LigmaOverlayBox         parent_instance;

  LigmaDisplayConfig     *config;
  PangoLayout           *layout;

  LigmaCanvasPaddingMode  padding_mode;
  LigmaRGB                padding_color;
};

struct _LigmaCanvasClass
{
  LigmaOverlayBoxClass  parent_class;
};


GType         ligma_canvas_get_type     (void) G_GNUC_CONST;

GtkWidget   * ligma_canvas_new          (LigmaDisplayConfig     *config);

PangoLayout * ligma_canvas_get_layout   (LigmaCanvas            *canvas,
                                        const gchar           *format,
                                        ...) G_GNUC_PRINTF (2, 3);

void          ligma_canvas_set_padding  (LigmaCanvas            *canvas,
                                        LigmaCanvasPaddingMode  padding_mode,
                                        const LigmaRGB         *padding_color);


#endif /*  __LIGMA_CANVAS_H__  */
