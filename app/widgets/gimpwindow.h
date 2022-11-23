/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmawindow.h
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

#ifndef __LIGMA_WINDOW_H__
#define __LIGMA_WINDOW_H__


#define LIGMA_TYPE_WINDOW            (ligma_window_get_type ())
#define LIGMA_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_WINDOW, LigmaWindow))
#define LIGMA_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_WINDOW, LigmaWindowClass))
#define LIGMA_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_WINDOW))
#define LIGMA_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_WINDOW))
#define LIGMA_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_WINDOW, LigmaWindowClass))


typedef struct _LigmaWindowClass   LigmaWindowClass;
typedef struct _LigmaWindowPrivate LigmaWindowPrivate;

struct _LigmaWindow
{
  GtkApplicationWindow  parent_instance;

  LigmaWindowPrivate    *private;
};

struct _LigmaWindowClass
{
  GtkApplicationWindowClass  parent_class;

  void (* monitor_changed) (LigmaWindow *window,
                            GdkMonitor *monitor);
};


GType       ligma_window_get_type                 (void) G_GNUC_CONST;

void        ligma_window_set_primary_focus_widget (LigmaWindow *window,
                                                  GtkWidget  *primary_focus);
GtkWidget * ligma_window_get_primary_focus_widget (LigmaWindow *window);


#endif /* __LIGMA_WINDOW_H__ */
