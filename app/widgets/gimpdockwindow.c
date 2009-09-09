/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockwindow.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpdockwindow.h"
#include "gimpwindow.h"

#include "gimp-intl.h"


struct _GimpDockWindowPrivate
{
  gint dummy;
};


G_DEFINE_TYPE (GimpDockWindow, gimp_dock_window, GIMP_TYPE_WINDOW)

#define parent_class gimp_dock_window_parent_class

static void
gimp_dock_window_class_init (GimpDockWindowClass *klass)
{
  g_type_class_add_private (klass, sizeof (GimpDockWindowPrivate));
}

static void
gimp_dock_window_init (GimpDockWindow *dock_window)
{
  dock_window->p = G_TYPE_INSTANCE_GET_PRIVATE (dock_window,
                                                GIMP_TYPE_DOCK_WINDOW,
                                                GimpDockWindowPrivate);
}
