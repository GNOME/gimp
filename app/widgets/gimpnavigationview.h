/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaNavigationView Widget
 * Copyright (C) 2001-2002 Michael Natterer <mitch@ligma.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@ligma.org>
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

#ifndef __LIGMA_NAVIGATION_VIEW_H__
#define __LIGMA_NAVIGATION_VIEW_H__

#include "ligmaview.h"


#define LIGMA_TYPE_NAVIGATION_VIEW            (ligma_navigation_view_get_type ())
#define LIGMA_NAVIGATION_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_NAVIGATION_VIEW, LigmaNavigationView))
#define LIGMA_NAVIGATION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_NAVIGATION_VIEW, LigmaNavigationViewClass))
#define LIGMA_IS_NAVIGATION_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_NAVIGATION_VIEW))
#define LIGMA_IS_NAVIGATION_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_NAVIGATION_VIEW))
#define LIGMA_NAVIGATION_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_NAVIGATION_VIEW, LigmaNavigationViewClass))


typedef struct _LigmaNavigationViewClass  LigmaNavigationViewClass;

struct _LigmaNavigationViewClass
{
  LigmaViewClass  parent_class;

  void (* marker_changed) (LigmaNavigationView *view,
                           gdouble             center_x,
                           gdouble             center_y,
                           gdouble             width,
                           gdouble             height);
  void (* zoom)           (LigmaNavigationView *view,
                           LigmaZoomType        direction,
                           gdouble             delta);
  void (* scroll)         (LigmaNavigationView *view,
                           GdkEventScroll     *sevent);
};


GType   ligma_navigation_view_get_type     (void) G_GNUC_CONST;

void    ligma_navigation_view_set_marker   (LigmaNavigationView *view,
                                           gdouble             center_x,
                                           gdouble             center_y,
                                           gdouble             width,
                                           gdouble             height,
                                           gboolean            flip_horizontally,
                                           gboolean            flip_vertically,
                                           gdouble             rotate_angle);
void    ligma_navigation_view_set_canvas   (LigmaNavigationView *view,
                                           gboolean            visible,
                                           gdouble             x,
                                           gdouble             y,
                                           gdouble             width,
                                           gdouble             height);
void    ligma_navigation_view_set_motion_offset
                                          (LigmaNavigationView *view,
                                           gint                motion_offset_x,
                                           gint                motion_offset_y);
void    ligma_navigation_view_get_local_marker
                                          (LigmaNavigationView *view,
                                           gint               *center_x,
                                           gint               *center_y,
                                           gint               *width,
                                           gint               *height);
void    ligma_navigation_view_grab_pointer (LigmaNavigationView *view,
                                           GdkEvent           *event);


#endif /* __LIGMA_NAVIGATION_VIEW_H__ */
