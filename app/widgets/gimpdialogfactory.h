/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdialogfactory.h
 * Copyright (C) 2001 Michael Natterer
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


#include <stdio.h>

#include "gimpobject.h"


typedef GtkWidget * (* GimpDialogNewFunc) (GimpDialogFactory *factory,
					   GimpContext       *context);


typedef struct _GimpDialogFactoryEntry GimpDialogFactoryEntry;

struct _GimpDialogFactoryEntry
{
  gchar             *identifier;
  GimpDialogNewFunc  new_func;
  gboolean           singleton;
  gboolean           session_managed;
  gboolean           remember_size;
};


typedef struct _GimpSessionInfo GimpSessionInfo;

struct _GimpSessionInfo
{
  gint       x;
  gint       y;
  gint       width;
  gint       height;

  GtkWidget *widget;

  /*  only valid while restoring and saving the session  */
  gboolean   open;

  /*  only one of these is valid  */
  GimpDialogFactoryEntry *toplevel_entry;
  GList                  *sub_dialogs;  /*  GList of GLists of entries  */
};


#define GIMP_TYPE_DIALOG_FACTORY            (gimp_dialog_factory_get_type ())
#define GIMP_DIALOG_FACTORY(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_DIALOG_FACTORY, GimpDialogFactory))
#define GIMP_DIALOG_FACTORY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DIALOG_FACTORY, GimpDialogFactoryClass))
#define GIMP_IS_DIALOG_FACTORY(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DIALOG_FACTORY))
#define GIMP_IS_DIALOG_FACTORY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DIALOG_FACTORY))


typedef struct _GimpDialogFactoryClass  GimpDialogFactoryClass;

struct _GimpDialogFactory
{
  GimpObject         parent_instance;

  GimpContext       *context;
  GtkItemFactory    *item_factory;

  /*< private >*/
  GimpDialogNewFunc  new_dock_func;

  GList             *registered_dialogs;
  GList             *session_infos;

  GList             *open_dialogs;
};

struct _GimpDialogFactoryClass
{
  GimpObjectClass  parent_class;

  GHashTable      *factories;
};


GtkType             gimp_dialog_factory_get_type  (void);
GimpDialogFactory * gimp_dialog_factory_new       (const gchar      *name,
						   GimpContext      *context,
						   GtkItemFactory   *item_factory,
						   GimpDialogNewFunc  new_dock_func);

GimpDialogFactory * gimp_dialog_factory_from_name       (const gchar       *name);

GimpDialogFactoryEntry * gimp_dialog_factory_find_entry (GimpDialogFactory *factory,
							 const gchar       *identifier);
GimpSessionInfo * gimp_dialog_factory_find_session_info (GimpDialogFactory *factory,
							 const gchar       *identifier);

void        gimp_dialog_factory_register         (GimpDialogFactory *factory,
						  const gchar       *identifier,
						  GimpDialogNewFunc  new_func,
						  gboolean           singleton,
						  gboolean           session_managed,
						  gboolean           remember_size);

GtkWidget * gimp_dialog_factory_dialog_new       (GimpDialogFactory *factory,
						  const gchar       *identifier);
GtkWidget * gimp_dialog_factory_dockable_new     (GimpDialogFactory *factory,
						  GimpDock          *dock,
						  const gchar       *identifier);
GtkWidget * gimp_dialog_factory_dock_new         (GimpDialogFactory *factory);

void        gimp_dialog_factory_add_toplevel     (GimpDialogFactory *factory,
						  GtkWidget         *toplevel);
void        gimp_dialog_factory_remove_toplevel  (GimpDialogFactory *factory,
						  GtkWidget         *toplevel);

void        gimp_dialog_factories_session_save    (FILE             *file);
void        gimp_dialog_factories_session_restore (void);


#endif  /*  __GIMP_DIALOG_FACTORY_H__  */
