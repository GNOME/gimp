/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenudock.h
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

#ifndef __GIMP_MENU_DOCK_H__
#define __GIMP_MENU_DOCK_H__


#include "gimpimagedock.h"


#define GIMP_TYPE_MENU_DOCK            (gimp_menu_dock_get_type ())
#define GIMP_MENU_DOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MENU_DOCK, GimpMenuDock))
#define GIMP_MENU_DOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MENU_DOCK, GimpMenuDockClass))
#define GIMP_IS_MENU_DOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MENU_DOCK))
#define GIMP_IS_MENU_DOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MENU_DOCK))
#define GIMP_MENU_DOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MENU_DOCK, GimpMenuDockClass))


typedef struct _GimpMenuDockClass  GimpMenuDockClass;

struct _GimpMenuDock
{
  GimpImageDock  parent_instance;

  GimpContainer *image_container;
  GimpContainer *display_container;

  gboolean       show_image_menu;
  gboolean       auto_follow_active;

  GtkWidget     *image_combo;
  GtkWidget     *auto_button;

  guint          update_title_idle_id;
};

struct _GimpMenuDockClass
{
  GimpImageDockClass  parent_class;
};


GType       gimp_menu_dock_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_menu_dock_new      (GimpDialogFactory *dialog_factory,
                                     GimpContainer     *image_container,
                                     GimpContainer     *display_container);

void        gimp_menu_dock_set_auto_follow_active (GimpMenuDock *menu_dock,
                                                   gboolean      show);
void        gimp_menu_dock_set_show_image_menu    (GimpMenuDock *menu_dock,
                                                   gboolean      show);


#endif /* __GIMP_MENU_DOCK_H__ */
