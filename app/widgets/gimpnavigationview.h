/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpNavigationView Widget
 * Copyright (C) 2001-2002 Michael Natterer <mitch@gimp.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@gimp.org>
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

struct _GimpNavigationView
{
  GimpView     parent_instance;

  /*  values in image coordinates  */
  gdouble      x;
  gdouble      y;
  gdouble      width;
  gdouble      height;

  /*  values in view coordinates  */
  gint         p_x;
  gint         p_y;
  gint         p_width;
  gint         p_height;

  gint         motion_offset_x;
  gint         motion_offset_y;
  gboolean     has_grab;

  GdkGC       *gc;
};

struct _GimpNavigationViewClass
{
  GimpViewClass  parent_class;

  void (* marker_changed) (GimpNavigationView *view,
                           gdouble             x,
                           gdouble             y);
  void (* zoom)           (GimpNavigationView *view,
                           GimpZoomType        direction);
  void (* scroll)         (GimpNavigationView *view,
                           GdkScrollDirection  direction);
};


GType   gimp_navigation_view_get_type     (void) G_GNUC_CONST;

void    gimp_navigation_view_set_marker   (GimpNavigationView *view,
                                           gdouble             x,
                                           gdouble             y,
                                           gdouble             width,
                                           gdouble             height);
void    gimp_navigation_view_grab_pointer (GimpNavigationView *view);


#endif /* __GIMP_NAVIGATION_VIEW_H__ */
