/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasarc.h
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

#ifndef __GIMP_CANVAS_ARC_H__
#define __GIMP_CANVAS_ARC_H__


#include "gimpcanvasitem.h"


#define GIMP_TYPE_CANVAS_ARC            (gimp_canvas_arc_get_type ())
#define GIMP_CANVAS_ARC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_ARC, GimpCanvasArc))
#define GIMP_CANVAS_ARC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_ARC, GimpCanvasArcClass))
#define GIMP_IS_CANVAS_ARC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_ARC))
#define GIMP_IS_CANVAS_ARC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_ARC))
#define GIMP_CANVAS_ARC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_ARC, GimpCanvasArcClass))


typedef struct _GimpCanvasArc      GimpCanvasArc;
typedef struct _GimpCanvasArcClass GimpCanvasArcClass;

struct _GimpCanvasArc
{
  GimpCanvasItem  parent_instance;
};

struct _GimpCanvasArcClass
{
  GimpCanvasItemClass  parent_class;
};


GType            gimp_canvas_arc_get_type (void) G_GNUC_CONST;

GimpCanvasItem * gimp_canvas_arc_new      (GimpDisplayShell *shell,
                                           gdouble          center_x,
                                           gdouble          center_y,
                                           gdouble          radius_x,
                                           gdouble          radius_y,
                                           gdouble          start_angle,
                                           gdouble          slice_angle,
                                           gboolean         filled);

void             gimp_canvas_arc_set      (GimpCanvasItem  *arc,
                                           gdouble          center_x,
                                           gdouble          center_y,
                                           gdouble          radius_x,
                                           gdouble          radius_y,
                                           gdouble          start_angle,
                                           gdouble          slice_angle);

#endif /* __GIMP_CANVAS_ARC_H__ */
