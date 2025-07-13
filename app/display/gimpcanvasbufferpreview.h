/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasbufferpreview.h
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
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


#define GIMP_TYPE_CANVAS_BUFFER_PREVIEW            (gimp_canvas_buffer_preview_get_type ())
#define GIMP_CANVAS_BUFFER_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_BUFFER_PREVIEW, GimpCanvasBufferPreview))
#define GIMP_CANVAS_BUFFER_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_BUFFER_PREVIEW, GimpCanvasBufferPreviewClass))
#define GIMP_IS_CANVAS_BUFFER_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_BUFFER_PREVIEW))
#define GIMP_IS_CANVAS_BUFFER_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_BUFFER_PREVIEW))
#define GIMP_CANVAS_BUFFER_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_BUFFER_PREVIEW, GimpCanvasBufferPreviewClass))


typedef struct _GimpCanvasBufferPreview      GimpCanvasBufferPreview;
typedef struct _GimpCanvasBufferPreviewClass GimpCanvasBufferPreviewClass;

struct _GimpCanvasBufferPreview
{
  GimpCanvasItem  parent_instance;
};

struct _GimpCanvasBufferPreviewClass
{
  GimpCanvasItemClass  parent_class;
};


GType            gimp_canvas_buffer_preview_get_type (void) G_GNUC_CONST;

GimpCanvasItem * gimp_canvas_buffer_preview_new      (GimpDisplayShell  *shell,
                                                      GeglBuffer        *buffer);
