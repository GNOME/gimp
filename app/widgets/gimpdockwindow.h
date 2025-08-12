/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockwindow.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
 * Copyright (C)      2009 Martin Nordholts <martinn@src.gnome.org>
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

#pragma once

#include "widgets/gimpwindow.h"


#define GIMP_TYPE_DOCK_WINDOW            (gimp_dock_window_get_type ())
#define GIMP_DOCK_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCK_WINDOW, GimpDockWindow))
#define GIMP_DOCK_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCK_WINDOW, GimpDockWindowClass))
#define GIMP_IS_DOCK_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCK_WINDOW))
#define GIMP_IS_DOCK_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCK_WINDOW))
#define GIMP_DOCK_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DOCK_WINDOW, GimpDockWindowClass))


typedef struct _GimpDockWindowClass    GimpDockWindowClass;
typedef struct _GimpDockWindowPrivate  GimpDockWindowPrivate;

/**
 * GimpDockWindow:
 *
 * A top-level window containing GimpDocks.
 */
struct _GimpDockWindow
{
  GimpWindow  parent_instance;

  GimpDockWindowPrivate *p;
};

struct _GimpDockWindowClass
{
  GimpWindowClass  parent_class;
};


GType               gimp_dock_window_get_type               (void) G_GNUC_CONST;
GtkWidget         * gimp_dock_window_new                    (const gchar       *role,
                                                             const gchar       *ui_manager_name,
                                                             gboolean           allow_dockbook_absence,
                                                             GimpDialogFactory *factory,
                                                             GimpContext       *context);
gint                gimp_dock_window_get_id                 (GimpDockWindow    *dock_window);
void                gimp_dock_window_add_dock               (GimpDockWindow    *dock_window,
                                                             GimpDock          *dock,
                                                             gint               index);
void                gimp_dock_window_remove_dock            (GimpDockWindow    *dock_window,
                                                             GimpDock          *dock);
GimpContext       * gimp_dock_window_get_context            (GimpDockWindow    *dock);
gboolean            gimp_dock_window_get_auto_follow_active (GimpDockWindow    *menu_dock);
void                gimp_dock_window_set_auto_follow_active (GimpDockWindow    *menu_dock,
                                                             gboolean           show);
gboolean            gimp_dock_window_get_show_image_menu    (GimpDockWindow    *menu_dock);
void                gimp_dock_window_set_show_image_menu    (GimpDockWindow    *menu_dock,
                                                             gboolean           show);
void                gimp_dock_window_setup                  (GimpDockWindow    *dock_window,
                                                             GimpDockWindow    *template);
gboolean            gimp_dock_window_has_toolbox            (GimpDockWindow    *dock_window);

GimpDockWindow    * gimp_dock_window_from_dock              (GimpDock          *dock);
