/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasitem.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CANVAS_ITEM_H__
#define __GIMP_CANVAS_ITEM_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_CANVAS_ITEM            (gimp_canvas_item_get_type ())
#define GIMP_CANVAS_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_ITEM, GimpCanvasItem))
#define GIMP_CANVAS_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_ITEM, GimpCanvasItemClass))
#define GIMP_IS_CANVAS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_ITEM))
#define GIMP_IS_CANVAS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_ITEM))
#define GIMP_CANVAS_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_ITEM, GimpCanvasItemClass))


typedef struct _GimpCanvasItemPrivate GimpCanvasItemPrivate;
typedef struct _GimpCanvasItemClass   GimpCanvasItemClass;

struct _GimpCanvasItem
{
  GimpObject             parent_instance;

  GimpCanvasItemPrivate *private;
};

struct _GimpCanvasItemClass
{
  GimpObjectClass  parent_class;

  /*  signals  */
  void             (* update)      (GimpCanvasItem   *item,
                                    cairo_region_t   *region);

  /*  virtual functions  */
  void             (* draw)        (GimpCanvasItem   *item,
                                    cairo_t          *cr);
  cairo_region_t * (* get_extents) (GimpCanvasItem   *item);

  void             (* stroke)      (GimpCanvasItem   *item,
                                    cairo_t          *cr);
  void             (* fill)        (GimpCanvasItem   *item,
                                    cairo_t          *cr);

  gboolean         (* hit)         (GimpCanvasItem   *item,
                                    gdouble           x,
                                    gdouble           y);
};


GType            gimp_canvas_item_get_type         (void) G_GNUC_CONST;

GimpDisplayShell * gimp_canvas_item_get_shell      (GimpCanvasItem   *item);
GimpImage      * gimp_canvas_item_get_image        (GimpCanvasItem   *item);
GtkWidget      * gimp_canvas_item_get_canvas       (GimpCanvasItem   *item);

void             gimp_canvas_item_draw             (GimpCanvasItem   *item,
                                                    cairo_t          *cr);
cairo_region_t * gimp_canvas_item_get_extents      (GimpCanvasItem   *item);

gboolean         gimp_canvas_item_hit              (GimpCanvasItem   *item,
                                                    gdouble           x,
                                                    gdouble           y);

void             gimp_canvas_item_set_visible      (GimpCanvasItem   *item,
                                                    gboolean          visible);
gboolean         gimp_canvas_item_get_visible      (GimpCanvasItem   *item);

void             gimp_canvas_item_set_line_cap     (GimpCanvasItem   *item,
                                                    cairo_line_cap_t  line_cap);

void             gimp_canvas_item_set_highlight    (GimpCanvasItem   *item,
                                                    gboolean          highlight);
gboolean         gimp_canvas_item_get_highlight    (GimpCanvasItem   *item);

void             gimp_canvas_item_begin_change     (GimpCanvasItem   *item);
void             gimp_canvas_item_end_change       (GimpCanvasItem   *item);

void             gimp_canvas_item_suspend_stroking (GimpCanvasItem   *item);
void             gimp_canvas_item_resume_stroking  (GimpCanvasItem   *item);

void             gimp_canvas_item_suspend_filling  (GimpCanvasItem   *item);
void             gimp_canvas_item_resume_filling   (GimpCanvasItem   *item);

void             gimp_canvas_item_transform        (GimpCanvasItem   *item,
                                                    cairo_t          *cr);
void             gimp_canvas_item_transform_xy     (GimpCanvasItem   *item,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gint             *tx,
                                                    gint             *ty);
void             gimp_canvas_item_transform_xy_f   (GimpCanvasItem   *item,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble          *tx,
                                                    gdouble          *ty);
gdouble          gimp_canvas_item_transform_distance
                                                   (GimpCanvasItem   *item,
                                                    gdouble           x1,
                                                    gdouble           y1,
                                                    gdouble           x2,
                                                    gdouble           y2);
gdouble          gimp_canvas_item_transform_distance_square
                                                   (GimpCanvasItem   *item,
                                                    gdouble           x1,
                                                    gdouble           y1,
                                                    gdouble           x2,
                                                    gdouble           y2);
void             gimp_canvas_item_untransform_viewport
                                                   (GimpCanvasItem   *item,
                                                    gint             *x,
                                                    gint             *y,
                                                    gint             *w,
                                                    gint             *h);


/*  protected  */

void             _gimp_canvas_item_update          (GimpCanvasItem   *item,
                                                    cairo_region_t   *region);
gboolean         _gimp_canvas_item_needs_update    (GimpCanvasItem   *item);
void             _gimp_canvas_item_stroke          (GimpCanvasItem   *item,
                                                    cairo_t          *cr);
void             _gimp_canvas_item_fill            (GimpCanvasItem   *item,
                                                    cairo_t          *cr);


#endif /* __GIMP_CANVAS_ITEM_H__ */
