/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasitem.h
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

#ifndef __LIGMA_CANVAS_ITEM_H__
#define __LIGMA_CANVAS_ITEM_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_CANVAS_ITEM            (ligma_canvas_item_get_type ())
#define LIGMA_CANVAS_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_ITEM, LigmaCanvasItem))
#define LIGMA_CANVAS_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_ITEM, LigmaCanvasItemClass))
#define LIGMA_IS_CANVAS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_ITEM))
#define LIGMA_IS_CANVAS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_ITEM))
#define LIGMA_CANVAS_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_ITEM, LigmaCanvasItemClass))


typedef struct _LigmaCanvasItemPrivate LigmaCanvasItemPrivate;
typedef struct _LigmaCanvasItemClass   LigmaCanvasItemClass;

struct _LigmaCanvasItem
{
  LigmaObject             parent_instance;

  LigmaCanvasItemPrivate *private;
};

struct _LigmaCanvasItemClass
{
  LigmaObjectClass  parent_class;

  /*  signals  */
  void             (* update)      (LigmaCanvasItem   *item,
                                    cairo_region_t   *region);

  /*  virtual functions  */
  void             (* draw)        (LigmaCanvasItem   *item,
                                    cairo_t          *cr);
  cairo_region_t * (* get_extents) (LigmaCanvasItem   *item);

  void             (* stroke)      (LigmaCanvasItem   *item,
                                    cairo_t          *cr);
  void             (* fill)        (LigmaCanvasItem   *item,
                                    cairo_t          *cr);

  gboolean         (* hit)         (LigmaCanvasItem   *item,
                                    gdouble           x,
                                    gdouble           y);
};


GType            ligma_canvas_item_get_type         (void) G_GNUC_CONST;

LigmaDisplayShell * ligma_canvas_item_get_shell      (LigmaCanvasItem   *item);
LigmaImage      * ligma_canvas_item_get_image        (LigmaCanvasItem   *item);
GtkWidget      * ligma_canvas_item_get_canvas       (LigmaCanvasItem   *item);

void             ligma_canvas_item_draw             (LigmaCanvasItem   *item,
                                                    cairo_t          *cr);
cairo_region_t * ligma_canvas_item_get_extents      (LigmaCanvasItem   *item);

gboolean         ligma_canvas_item_hit              (LigmaCanvasItem   *item,
                                                    gdouble           x,
                                                    gdouble           y);

void             ligma_canvas_item_set_visible      (LigmaCanvasItem   *item,
                                                    gboolean          visible);
gboolean         ligma_canvas_item_get_visible      (LigmaCanvasItem   *item);

void             ligma_canvas_item_set_line_cap     (LigmaCanvasItem   *item,
                                                    cairo_line_cap_t  line_cap);

void             ligma_canvas_item_set_highlight    (LigmaCanvasItem   *item,
                                                    gboolean          highlight);
gboolean         ligma_canvas_item_get_highlight    (LigmaCanvasItem   *item);

void             ligma_canvas_item_begin_change     (LigmaCanvasItem   *item);
void             ligma_canvas_item_end_change       (LigmaCanvasItem   *item);

void             ligma_canvas_item_suspend_stroking (LigmaCanvasItem   *item);
void             ligma_canvas_item_resume_stroking  (LigmaCanvasItem   *item);

void             ligma_canvas_item_suspend_filling  (LigmaCanvasItem   *item);
void             ligma_canvas_item_resume_filling   (LigmaCanvasItem   *item);

void             ligma_canvas_item_transform        (LigmaCanvasItem   *item,
                                                    cairo_t          *cr);
void             ligma_canvas_item_transform_xy     (LigmaCanvasItem   *item,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gint             *tx,
                                                    gint             *ty);
void             ligma_canvas_item_transform_xy_f   (LigmaCanvasItem   *item,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble          *tx,
                                                    gdouble          *ty);
gdouble          ligma_canvas_item_transform_distance
                                                   (LigmaCanvasItem   *item,
                                                    gdouble           x1,
                                                    gdouble           y1,
                                                    gdouble           x2,
                                                    gdouble           y2);
gdouble          ligma_canvas_item_transform_distance_square
                                                   (LigmaCanvasItem   *item,
                                                    gdouble           x1,
                                                    gdouble           y1,
                                                    gdouble           x2,
                                                    gdouble           y2);
void             ligma_canvas_item_untransform_viewport
                                                   (LigmaCanvasItem   *item,
                                                    gint             *x,
                                                    gint             *y,
                                                    gint             *w,
                                                    gint             *h);


/*  protected  */

void             _ligma_canvas_item_update          (LigmaCanvasItem   *item,
                                                    cairo_region_t   *region);
gboolean         _ligma_canvas_item_needs_update    (LigmaCanvasItem   *item);
void             _ligma_canvas_item_stroke          (LigmaCanvasItem   *item,
                                                    cairo_t          *cr);
void             _ligma_canvas_item_fill            (LigmaCanvasItem   *item,
                                                    cairo_t          *cr);


#endif /* __LIGMA_CANVAS_ITEM_H__ */
