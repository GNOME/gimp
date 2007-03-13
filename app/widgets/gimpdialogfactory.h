/* GIMP - The GNU Image Manipulation Program
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


#include "core/gimpobject.h"

#define GIMP_DIALOG_VISIBILITY_KEY "gimp-dialog-visibility"

typedef enum
{
  GIMP_DIALOG_VISIBILITY_UNKNOWN = 0,
  GIMP_DIALOG_VISIBILITY_INVISIBLE,
  GIMP_DIALOG_VISIBILITY_VISIBLE
} GimpDialogVisibilityState;


typedef GtkWidget * (* GimpDialogNewFunc)     (GimpDialogFactory      *factory,
                                               GimpContext            *context,
                                               gint                    view_size);
typedef GtkWidget * (* GimpDialogConstructor) (GimpDialogFactory      *factory,
                                               GimpDialogFactoryEntry *entry,
                                               GimpContext            *context,
                                               gint                    view_size);


struct _GimpDialogFactoryEntry
{
  gchar             *identifier;
  gchar             *name;
  gchar             *blurb;
  gchar             *stock_id;
  gchar             *help_id;

  GimpDialogNewFunc  new_func;
  gint               view_size;

  gboolean           singleton;
  gboolean           session_managed;
  gboolean           remember_size;
  gboolean           remember_if_open;
};


#define GIMP_TYPE_DIALOG_FACTORY            (gimp_dialog_factory_get_type ())
#define GIMP_DIALOG_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DIALOG_FACTORY, GimpDialogFactory))
#define GIMP_DIALOG_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DIALOG_FACTORY, GimpDialogFactoryClass))
#define GIMP_IS_DIALOG_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DIALOG_FACTORY))
#define GIMP_IS_DIALOG_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DIALOG_FACTORY))
#define GIMP_DIALOG_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DIALOG_FACTORY, GimpDialogFactoryClass))


typedef struct _GimpDialogFactoryClass  GimpDialogFactoryClass;

struct _GimpDialogFactory
{
  GimpObject             parent_instance;

  GimpContext           *context;
  GimpMenuFactory       *menu_factory;

  /*< private >*/
  GimpDialogNewFunc      new_dock_func;
  GimpDialogConstructor  constructor;

  GList                 *registered_dialogs;
  GList                 *session_infos;

  GList                 *open_dialogs;
};

struct _GimpDialogFactoryClass
{
  GimpObjectClass  parent_class;

  GHashTable      *factories;
};


GType               gimp_dialog_factory_get_type  (void) G_GNUC_CONST;

GimpDialogFactory * gimp_dialog_factory_new       (const gchar       *name,
                                                   GimpContext       *context,
                                                   GimpMenuFactory   *menu_factory,
                                                   GimpDialogNewFunc  new_dock_func);

GimpDialogFactory * gimp_dialog_factory_from_name (const gchar       *name);

void        gimp_dialog_factory_set_constructor   (GimpDialogFactory *factory,
                                                   GimpDialogConstructor constructor);

void        gimp_dialog_factory_register_entry    (GimpDialogFactory *factory,
                                                   const gchar       *identifier,
                                                   const gchar       *name,
                                                   const gchar       *blurb,
                                                   const gchar       *stock_id,
                                                   const gchar       *help_id,
                                                   GimpDialogNewFunc  new_func,
                                                   gint               view_size,
                                                   gboolean           singleton,
                                                   gboolean           session_managed,
                                                   gboolean           remember_size,
                                                   gboolean           remember_if_open);
GimpDialogFactoryEntry * gimp_dialog_factory_find_entry
                                                  (GimpDialogFactory *factory,
                                                   const gchar       *identifier);

GimpSessionInfo * gimp_dialog_factory_find_session_info
                                                  (GimpDialogFactory *factory,
                                                   const gchar       *identifier);

GtkWidget * gimp_dialog_factory_dialog_new        (GimpDialogFactory *factory,
                                                   GdkScreen         *screen,
                                                   const gchar       *identifier,
                                                   gint               view_size,
                                                   gboolean           present);
GtkWidget * gimp_dialog_factory_dialog_raise      (GimpDialogFactory *factory,
                                                   GdkScreen         *screen,
                                                   const gchar       *identifiers,
                                                   gint               view_size);
GtkWidget * gimp_dialog_factory_dockable_new      (GimpDialogFactory *factory,
                                                   GimpDock          *dock,
                                                   const gchar       *identifier,
                                                   gint               view_size);
GtkWidget * gimp_dialog_factory_dock_new          (GimpDialogFactory *factory,
                                                   GdkScreen         *screen);

void        gimp_dialog_factory_add_dialog        (GimpDialogFactory *factory,
                                                   GtkWidget         *dialog);
void        gimp_dialog_factory_add_foreign       (GimpDialogFactory *factory,
                                                   const gchar       *identifier,
                                                   GtkWidget         *dialog);
void        gimp_dialog_factory_remove_dialog     (GimpDialogFactory *factory,
                                                   GtkWidget         *dialog);

void        gimp_dialog_factory_show_toolbox      (GimpDialogFactory *toolbox_factory);

void        gimp_dialog_factory_hide_dialog       (GtkWidget         *dialog);

void        gimp_dialog_factories_session_save    (GimpConfigWriter  *writer);
void        gimp_dialog_factories_session_restore (void);
void        gimp_dialog_factories_session_clear   (void);

void        gimp_dialog_factories_toggle          (void);
void        gimp_dialog_factories_set_busy        (void);
void        gimp_dialog_factories_unset_busy      (void);

GimpDialogFactory *
            gimp_dialog_factory_from_widget (GtkWidget               *dialog,
                                             GimpDialogFactoryEntry **entry);


#endif  /*  __GIMP_DIALOG_FACTORY_H__  */
