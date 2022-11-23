/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoverlaychild.h
 * Copyright (C) 2009 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OVERLAY_CHILD_H__
#define __LIGMA_OVERLAY_CHILD_H__


typedef struct _LigmaOverlayChild LigmaOverlayChild;

struct _LigmaOverlayChild
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


LigmaOverlayChild * ligma_overlay_child_new                  (LigmaOverlayBox  *box,
                                                            GtkWidget       *widget,
                                                            gdouble          xalign,
                                                            gdouble          yalign,
                                                            gdouble          angle,
                                                            gdouble          opacity);
void               ligma_overlay_child_free                 (LigmaOverlayBox   *box,
                                                            LigmaOverlayChild *child);

LigmaOverlayChild * ligma_overlay_child_find                 (LigmaOverlayBox   *box,
                                                            GtkWidget        *widget);

void               ligma_overlay_child_realize              (LigmaOverlayBox   *box,
                                                            LigmaOverlayChild *child);
void               ligma_overlay_child_unrealize            (LigmaOverlayBox   *box,
                                                            LigmaOverlayChild *child);
void               ligma_overlay_child_get_preferred_width  (LigmaOverlayBox   *box,
                                                            LigmaOverlayChild *child,
                                                            gint             *minimum,
                                                            gint             *natural);
void               ligma_overlay_child_get_preferred_height (LigmaOverlayBox   *box,
                                                            LigmaOverlayChild *child,
                                                            gint             *minimum,
                                                            gint             *natural);
void               ligma_overlay_child_size_allocate        (LigmaOverlayBox   *box,
                                                            LigmaOverlayChild *child);
gboolean           ligma_overlay_child_draw                 (LigmaOverlayBox   *box,
                                                            LigmaOverlayChild *child,
                                                            cairo_t          *cr);
gboolean           ligma_overlay_child_damage               (LigmaOverlayBox   *box,
                                                            LigmaOverlayChild *child,
                                                            GdkEventExpose   *event);

void               ligma_overlay_child_invalidate           (LigmaOverlayBox   *box,
                                                            LigmaOverlayChild *child);
gboolean           ligma_overlay_child_pick                 (LigmaOverlayBox   *box,
                                                            LigmaOverlayChild *child,
                                                            gdouble           box_x,
                                                            gdouble           box_y);


#endif /* __LIGMA_OVERLAY_CHILD_H__ */
