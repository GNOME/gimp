/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamenudock.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_MENU_DOCK_H__
#define __LIGMA_MENU_DOCK_H__


#include "ligmadock.h"


#define LIGMA_TYPE_MENU_DOCK            (ligma_menu_dock_get_type ())
#define LIGMA_MENU_DOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MENU_DOCK, LigmaMenuDock))
#define LIGMA_MENU_DOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MENU_DOCK, LigmaMenuDockClass))
#define LIGMA_IS_MENU_DOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MENU_DOCK))
#define LIGMA_IS_MENU_DOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MENU_DOCK))
#define LIGMA_MENU_DOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MENU_DOCK, LigmaMenuDockClass))

typedef struct _LigmaMenuDockPrivate LigmaMenuDockPrivate;
typedef struct _LigmaMenuDockClass   LigmaMenuDockClass;

struct _LigmaMenuDock
{
  LigmaDock             parent_instance;

  LigmaMenuDockPrivate *p;
};

struct _LigmaMenuDockClass
{
  LigmaDockClass  parent_class;
};


GType       ligma_menu_dock_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_menu_dock_new      (void);



#endif /* __LIGMA_MENU_DOCK_H__ */
