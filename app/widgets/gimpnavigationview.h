/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpNavigationView Widget
 * Copyright (C) 2001-2002 Michael Natterer <mitch@gimp.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@gimp.org>
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

#ifndef __GIMP_NAVIGATION_VIEW_H__
#define __GIMP_NAVIGATION_VIEW_H__

#include "gimpview.h"


#define GIMP_TYPE_NAVIGATION_VIEW            (gimp_navigation_view_get_type ())
#define GIMP_NAVIGATION_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_NAVIGATION_VIEW, GimpNavigationView))
#define GIMP_NAVIGATION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_NAVIGATION_VIEW, GimpNavigationViewClass))
#define GIMP_IS_NAVIGATION_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_NAVIGATION_VIEW))
#define GIMP_IS_NAVIGATION_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_NAVIGATION_VIEW))
#define GIMP_NAVIGATION_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_NAVIGATION_VIEW, GimpNavigationViewClass))


typedef struct _GimpNavigationViewClass  GimpNavigationViewClass;

struct _GimpNavigationViewClass
{
  GimpViewClass  parent_class;

  void (* marker_changed) (GimpNavigationView *view,
                           gdouble             center_x,
                           gdouble             center_y,
                           gdouble             width,
                           gdouble             height);
  void (* zoom)           (GimpNavigationView *view,
                           GimpZoomType        direction);
  void (* scroll)         (GimpNavigationView *view,
                           GdkScrollDirection  direction);
};


GType   gimp_navigation_view_get_type     (void) G_GNUC_CONST;

void    gimp_navigation_view_set_marker   (GimpNavigationView *view,
                                           gdouble             center_x,
                                           gdouble             center_y,
                                           gdouble             width,
                                           gdouble             height,
                                           gboolean            flip_horizontally,
                                           gboolean            flip_vertically,
                                           gdouble             rotate_angle);
void    gimp_navigation_view_set_motion_offset
                                          (GimpNavigationView *view,
                                           gint                motion_offset_x,
                                           gint                motion_offset_y);
void    gimp_navigation_view_get_local_marker
                                          (GimpNavigationView *view,
                                           gint               *center_x,
                                           gint               *center_y,
                                           gint               *width,
                                           gint               *height);
void    gimp_navigation_view_grab_pointer (GimpNavigationView *view,
                                           GdkEvent           *event);


#endif /* __GIMP_NAVIGATION_VIEW_H__ */
