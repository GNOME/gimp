/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpwindow.h
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

#ifndef __GIMP_WINDOW_H__
#define __GIMP_WINDOW_H__


#define GIMP_TYPE_WINDOW            (gimp_window_get_type ())
#define GIMP_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_WINDOW, GimpWindow))
#define GIMP_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_WINDOW, GimpWindowClass))
#define GIMP_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_WINDOW))
#define GIMP_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_WINDOW))
#define GIMP_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_WINDOW, GimpWindowClass))


typedef struct _GimpWindowClass   GimpWindowClass;
typedef struct _GimpWindowPrivate GimpWindowPrivate;

struct _GimpWindow
{
  GtkApplicationWindow  parent_instance;

  GimpWindowPrivate    *private;
};

struct _GimpWindowClass
{
  GtkApplicationWindowClass  parent_class;

  void (* monitor_changed) (GimpWindow *window,
                            GdkMonitor *monitor);
};


GType       gimp_window_get_type                 (void) G_GNUC_CONST;

void        gimp_window_set_primary_focus_widget (GimpWindow *window,
                                                  GtkWidget  *primary_focus);
GtkWidget * gimp_window_get_primary_focus_widget (GimpWindow *window);


#endif /* __GIMP_WINDOW_H__ */
