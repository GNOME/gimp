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

#ifndef __GIMP_CANVAS_H__
#define __GIMP_CANVAS_H__


#include <gtk/gtkdrawingarea.h>


#define GIMP_TYPE_CANVAS            (gimp_canvas_get_type ())
#define GIMP_CANVAS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS, GimpCanvas))
#define GIMP_CANVAS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS, GimpCanvasClass))
#define GIMP_IS_CANVAS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS))
#define GIMP_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS))
#define GIMP_CANVAS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS, GimpCanvasClass))


typedef struct _GimpCanvasClass GimpCanvasClass;

struct _GimpCanvas
{
  GtkDrawingArea  parent_instance;

  /*  GC for rendering the image  */
  GdkGC          *render_gc;

  /*  GCs for rendering guides    */
  struct
  {
    GdkGC        *normal_hgc;
    GdkGC        *active_hgc;
    GdkGC        *normal_vgc;
    GdkGC        *active_vgc;
  } guides;

  /*  GC for the grid             */
  GdkGC          *grid_gc;

  /*  GC for the vectors          */
  GdkGC          *vectors_gc;
};

struct _GimpCanvasClass
{
  GtkDrawingAreaClass  parent_class;
};


GType        gimp_canvas_get_type    (void) G_GNUC_CONST;

GtkWidget  * gimp_canvas_new         (void);
GdkGC      * gimp_canvas_grid_gc_new (GimpCanvas *canvas,
                                      GimpGrid   *grid);
void         gimp_canvas_draw_cursor (GimpCanvas *canvas,
                                      gint        x,
                                      gint        y);


#endif /*  __GIMP_CANVAS_H__  */
