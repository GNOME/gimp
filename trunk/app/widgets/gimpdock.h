/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdock.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_DOCK_H__
#define __GIMP_DOCK_H__


#include <gtk/gtkwindow.h>


#define GIMP_TYPE_DOCK            (gimp_dock_get_type ())
#define GIMP_DOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCK, GimpDock))
#define GIMP_DOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCK, GimpDockClass))
#define GIMP_IS_DOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCK))
#define GIMP_IS_DOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCK))
#define GIMP_DOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DOCK, GimpDockClass))


typedef struct _GimpDockClass GimpDockClass;

struct _GimpDock
{
  GtkWindow          parent_instance;

  GimpDialogFactory *dialog_factory;
  GimpContext       *context;

  GtkWidget         *main_vbox;
  GtkWidget         *vbox;

  GList             *dockbooks;
};

struct _GimpDockClass
{
  GtkWindowClass parent_class;

  /*  virtual functions  */
  void    (* setup)        (GimpDock       *dock,
                            const GimpDock *template);
  void    (* set_aux_info) (GimpDock       *dock,
                            GList          *aux_info);
  GList * (* get_aux_info) (GimpDock       *dock);

  /*  signals  */
  void    (* book_added)   (GimpDock       *dock,
                            GimpDockbook   *dockbook);
  void    (* book_removed) (GimpDock       *dock,
                            GimpDockbook   *dockbook);
};


GType   gimp_dock_get_type     (void) G_GNUC_CONST;

void    gimp_dock_setup        (GimpDock       *dock,
                                const GimpDock *template);
void    gimp_dock_set_aux_info (GimpDock       *dock,
                                GList          *aux_info);
GList * gimp_dock_get_aux_info (GimpDock       *dock);

void    gimp_dock_add          (GimpDock       *dock,
                                GimpDockable   *dockable,
                                gint            book,
                                gint            index);
void    gimp_dock_remove       (GimpDock       *dock,
                                GimpDockable   *dockable);

void    gimp_dock_add_book     (GimpDock       *dock,
                                GimpDockbook   *dockbook,
                                gint            index);
void    gimp_dock_remove_book  (GimpDock       *dock,
                                GimpDockbook   *dockbook);


#endif /* __GIMP_DOCK_H__ */
