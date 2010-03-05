/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenudock.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "gimpdialogfactory.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpmenudock.h"

#include "gimp-intl.h"


#define DEFAULT_MINIMAL_WIDTH  200

struct _GimpMenuDockPrivate
{
  gint make_sizeof_greater_than_zero;
};


static void   gimp_menu_dock_style_set               (GtkWidget      *widget,
                                                      GtkStyle       *prev_style);

static void   gimp_menu_dock_book_added              (GimpDock       *dock,
                                                      GimpDockbook   *dockbook);
static void   gimp_menu_dock_book_removed            (GimpDock       *dock,
                                                      GimpDockbook   *dockbook);

static void   gimp_menu_dock_dockbook_changed        (GimpDockbook   *dockbook,
                                                      GimpDockable   *dockable,
                                                      GimpMenuDock   *dock);



G_DEFINE_TYPE (GimpMenuDock, gimp_menu_dock, GIMP_TYPE_DOCK)

#define parent_class gimp_menu_dock_parent_class


static void
gimp_menu_dock_class_init (GimpMenuDockClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GimpDockClass  *dock_class   = GIMP_DOCK_CLASS (klass);

  widget_class->style_set   = gimp_menu_dock_style_set;

  dock_class->book_added    = gimp_menu_dock_book_added;
  dock_class->book_removed  = gimp_menu_dock_book_removed;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("minimal-width",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_MINIMAL_WIDTH,
                                                             GIMP_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (GimpMenuDockPrivate));
}

static void
gimp_menu_dock_init (GimpMenuDock *dock)
{
}

static void
gimp_menu_dock_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  gint minimal_width = -1;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "minimal-width", &minimal_width,
                        NULL);

  gtk_widget_set_size_request (widget, minimal_width, -1);
}

static void
gimp_menu_dock_book_added (GimpDock     *dock,
                           GimpDockbook *dockbook)
{
  g_signal_connect (dockbook, "dockable-added",
                    G_CALLBACK (gimp_menu_dock_dockbook_changed),
                    dock);
  g_signal_connect (dockbook, "dockable-removed",
                    G_CALLBACK (gimp_menu_dock_dockbook_changed),
                    dock);
  g_signal_connect (dockbook, "dockable-reordered",
                    G_CALLBACK (gimp_menu_dock_dockbook_changed),
                    dock);

  gimp_dock_invalidate_title (GIMP_DOCK (dock));

  GIMP_DOCK_CLASS (parent_class)->book_added (dock, dockbook);
}

static void
gimp_menu_dock_book_removed (GimpDock     *dock,
                             GimpDockbook *dockbook)
{
  g_signal_handlers_disconnect_by_func (dockbook,
                                        gimp_menu_dock_dockbook_changed,
                                        dock);

  gimp_dock_invalidate_title (GIMP_DOCK (dock));

  GIMP_DOCK_CLASS (parent_class)->book_removed (dock, dockbook);
}

GtkWidget *
gimp_menu_dock_new (void)
{
  return g_object_new (GIMP_TYPE_MENU_DOCK, NULL);
}

static void
gimp_menu_dock_dockbook_changed (GimpDockbook *dockbook,
                                 GimpDockable *dockable,
                                 GimpMenuDock *dock)
{
  gimp_dock_invalidate_title (GIMP_DOCK (dock));
}

