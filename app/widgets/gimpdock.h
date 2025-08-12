/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdock.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_DOCK            (gimp_dock_get_type ())
#define GIMP_DOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCK, GimpDock))
#define GIMP_DOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCK, GimpDockClass))
#define GIMP_IS_DOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCK))
#define GIMP_IS_DOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCK))
#define GIMP_DOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DOCK, GimpDockClass))


/* String used to separate dockables, e.g. "Tool Options, Layers" */
#define GIMP_DOCK_DOCKABLE_SEPARATOR C_("dock", ", ")

/* String used to separate books (GtkNotebooks) within a dock,
   e.g. "Tool Options, Layers - Brushes"
 */
#define GIMP_DOCK_BOOK_SEPARATOR C_("dock", " - ")

/* String used to separate dock columns,
   e.g. "Tool Options, Layers - Brushes | Gradients"
 */
#define GIMP_DOCK_COLUMN_SEPARATOR C_("dock", " | ")


typedef struct _GimpDockClass    GimpDockClass;
typedef struct _GimpDockPrivate  GimpDockPrivate;

/**
 * GimpDock:
 *
 * Contains a column of GimpDockbooks.
 */
struct _GimpDock
{
  GtkBox           parent_instance;

  GimpDockPrivate *p;
};

struct _GimpDockClass
{
  GtkBoxClass  parent_class;

  /*  virtual functions  */
  gchar * (* get_description)         (GimpDock       *dock,
                                       gboolean        complete);
  void    (* set_host_geometry_hints) (GimpDock       *dock,
                                       GtkWindow      *window);

  /*  signals  */
  void    (* book_added)              (GimpDock       *dock,
                                       GimpDockbook   *dockbook);
  void    (* book_removed)            (GimpDock       *dock,
                                       GimpDockbook   *dockbook);
  void    (* description_invalidated) (GimpDock       *dock);
  void    (* geometry_invalidated)    (GimpDock       *dock);
};


GType               gimp_dock_get_type                (void) G_GNUC_CONST;

gchar             * gimp_dock_get_description         (GimpDock       *dock,
                                                       gboolean        complete);
void                gimp_dock_set_host_geometry_hints (GimpDock       *dock,
                                                       GtkWindow      *window);
void                gimp_dock_invalidate_geometry     (GimpDock       *dock);
void                gimp_dock_update_with_context     (GimpDock       *dock,
                                                       GimpContext    *context);
GimpContext       * gimp_dock_get_context             (GimpDock       *dock);
GimpDialogFactory * gimp_dock_get_dialog_factory      (GimpDock       *dock);
GimpUIManager     * gimp_dock_get_ui_manager          (GimpDock       *dock);
GList             * gimp_dock_get_dockbooks           (GimpDock       *dock);
gint                gimp_dock_get_n_dockables         (GimpDock       *dock);
GtkWidget         * gimp_dock_get_main_vbox           (GimpDock       *dock);
GtkWidget         * gimp_dock_get_vbox                (GimpDock       *dock);

void                gimp_dock_set_id                  (GimpDock       *dock,
                                                       gint            ID);
gint                gimp_dock_get_id                  (GimpDock       *dock);

void                gimp_dock_add_book                (GimpDock       *dock,
                                                       GimpDockbook   *dockbook,
                                                       gint            index);
void                gimp_dock_remove_book             (GimpDock       *dock,
                                                       GimpDockbook   *dockbook);
