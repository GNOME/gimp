/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdialogfactory.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_DIALOG_FACTORY_H__
#define __GIMP_DIALOG_FACTORY_H__


#include "gimpobject.h"


typedef GimpDockable * (* GimpDialogNewFunc) (GimpDialogFactory *factory);


#define GIMP_TYPE_DIALOG_FACTORY            (gimp_dialog_factory_get_type ())
#define GIMP_DIALOG_FACTORY(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_DIALOG_FACTORY, GimpDialogFactory))
#define GIMP_DIALOG_FACTORY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DIALOG_FACTORY, GimpDialogFactoryClass))
#define GIMP_IS_DIALOG_FACTORY(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DIALOG_FACTORY))
#define GIMP_IS_DIALOG_FACTORY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DIALOG_FACTORY))


typedef struct _GimpDialogFactoryClass  GimpDialogFactoryClass;

struct _GimpDialogFactory
{
  GimpObject      parent_instance;

  GimpContext    *context;
  GtkItemFactory *item_factory;

  /*< private >*/
  GList          *registered_dialogs;

  GList          *open_docks;
};

struct _GimpDialogFactoryClass
{
  GimpObjectClass  parent_class;
};


GtkType             gimp_dialog_factory_get_type (void);
GimpDialogFactory * gimp_dialog_factory_new      (GimpContext       *context,
						  GtkItemFactory    *item_factory);

void           gimp_dialog_factory_register      (GimpDialogFactory *factory,
						  const gchar       *identifier,
						  GimpDialogNewFunc  new_func);
GimpDockable * gimp_dialog_factory_dialog_new    (GimpDialogFactory *factory,
						  const gchar       *identifier);

void           gimp_dialog_factory_add_dock      (GimpDialogFactory *factory,
						  GimpDock          *dock);
void           gimp_dialog_factory_remove_dock   (GimpDialogFactory *factory,
						  GimpDock          *dock);


#endif  /*  __GIMP_DIALOG_FACTORY_H__  */
