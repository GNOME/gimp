/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoverlaychild.h
 * Copyright (C) 2009 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_OVERLAY_CHILD_H__
#define __GIMP_OVERLAY_CHILD_H__


typedef struct _GimpOverlayChild GimpOverlayChild;

struct _GimpOverlayChild
{
  GtkWidget      *widget;
  GdkWindow      *window;

  gboolean        has_position;
  gdouble         xalign;
  gdouble         yalign;
  gdouble         x;
  gdouble         y;

  gdouble         angle;
  gdouble         opacity;

  /* updated in size_allocate */
  cairo_matrix_t  matrix;
};


GimpOverlayChild * gimp_overlay_child_new           (GimpOverlayBox  *box,
                                                     GtkWidget       *widget,
                                                     gdouble          xalign,
                                                     gdouble          yalign,
                                                     gdouble          angle,
                                                     gdouble          opacity);
void               gimp_overlay_child_free          (GimpOverlayBox   *box,
                                                     GimpOverlayChild *child);

GimpOverlayChild * gimp_overlay_child_find          (GimpOverlayBox   *box,
                                                     GtkWidget        *widget);

void               gimp_overlay_child_realize       (GimpOverlayBox   *box,
                                                     GimpOverlayChild *child);
void               gimp_overlay_child_unrealize     (GimpOverlayBox   *box,
                                                     GimpOverlayChild *child);
void               gimp_overlay_child_size_request  (GimpOverlayBox   *box,
                                                     GimpOverlayChild *child);
void               gimp_overlay_child_size_allocate (GimpOverlayBox   *box,
                                                     GimpOverlayChild *child);
gboolean           gimp_overlay_child_draw          (GimpOverlayBox   *box,
                                                     GimpOverlayChild *child,
                                                     cairo_t          *cr);
gboolean           gimp_overlay_child_damage        (GimpOverlayBox   *box,
                                                     GimpOverlayChild *child,
                                                     GdkEventExpose   *event);

void               gimp_overlay_child_invalidate    (GimpOverlayBox   *box,
                                                     GimpOverlayChild *child);
gboolean           gimp_overlay_child_pick          (GimpOverlayBox   *box,
                                                     GimpOverlayChild *child,
                                                     gdouble           box_x,
                                                     gdouble           box_y);


#endif /* __GIMP_OVERLAY_CHILD_H__ */
