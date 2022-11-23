/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvashandle.h
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CANVAS_HANDLE_H__
#define __LIGMA_CANVAS_HANDLE_H__


#include "ligmacanvasitem.h"


#define LIGMA_CANVAS_HANDLE_SIZE_CIRCLE    13
#define LIGMA_CANVAS_HANDLE_SIZE_CROSS     15
#define LIGMA_CANVAS_HANDLE_SIZE_CROSSHAIR 43
#define LIGMA_CANVAS_HANDLE_SIZE_LARGE     25
#define LIGMA_CANVAS_HANDLE_SIZE_SMALL     7


#define LIGMA_TYPE_CANVAS_HANDLE            (ligma_canvas_handle_get_type ())
#define LIGMA_CANVAS_HANDLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_HANDLE, LigmaCanvasHandle))
#define LIGMA_CANVAS_HANDLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_HANDLE, LigmaCanvasHandleClass))
#define LIGMA_IS_CANVAS_HANDLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_HANDLE))
#define LIGMA_IS_CANVAS_HANDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_HANDLE))
#define LIGMA_CANVAS_HANDLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_HANDLE, LigmaCanvasHandleClass))


typedef struct _LigmaCanvasHandle      LigmaCanvasHandle;
typedef struct _LigmaCanvasHandleClass LigmaCanvasHandleClass;

struct _LigmaCanvasHandle
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasHandleClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_handle_get_type     (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_handle_new          (LigmaDisplayShell *shell,
                                                  LigmaHandleType    type,
                                                  LigmaHandleAnchor  anchor,
                                                  gdouble           x,
                                                  gdouble           y,
                                                  gint              width,
                                                  gint              height);

void             ligma_canvas_handle_get_position (LigmaCanvasItem   *handle,
                                                  gdouble          *x,
                                                  gdouble          *y);
void             ligma_canvas_handle_set_position (LigmaCanvasItem   *handle,
                                                  gdouble           x,
                                                  gdouble           y);

gint             ligma_canvas_handle_calc_size    (LigmaCanvasItem   *item,
                                                  gdouble           mouse_x,
                                                  gdouble           mouse_y,
                                                  gint              normal_size,
                                                  gint              hover_size);

void             ligma_canvas_handle_get_size     (LigmaCanvasItem   *handle,
                                                  gint             *width,
                                                  gint             *height);
void             ligma_canvas_handle_set_size     (LigmaCanvasItem   *handle,
                                                  gint              width,
                                                  gint              height);

void             ligma_canvas_handle_set_angles   (LigmaCanvasItem   *handle,
                                                  gdouble           start_handle,
                                                  gdouble           slice_handle);


#endif /* __LIGMA_CANVAS_HANDLE_H__ */
