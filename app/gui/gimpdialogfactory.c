/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "gimpdialogfactory.h"


typedef struct _GimpDialogFactoryEntry GimpDialogFactoryEntry;

struct _GimpDialogFactoryEntry
{
  gchar             *identifier;
  GimpDialogNewFunc  new_func;
};


static void   gimp_dialog_factory_class_init (GimpDialogFactoryClass *klass);
static void   gimp_dialog_factory_init       (GimpDialogFactory      *factory);

static void   gimp_dialog_factory_destroy    (GtkObject              *object);


static GimpObjectClass *parent_class = NULL;


GtkType
gimp_dialog_factory_get_type (void)
{
  static guint factory_type = 0;

  if (! factory_type)
    {
      GtkTypeInfo factory_info =
      {
	"GimpDialogFactory",
	sizeof (GimpDialogFactory),
	sizeof (GimpDialogFactoryClass),
	(GtkClassInitFunc) gimp_dialog_factory_class_init,
	(GtkObjectInitFunc) gimp_dialog_factory_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      factory_type = gtk_type_unique (GIMP_TYPE_OBJECT, &factory_info);
    }

  return factory_type;
}

static void
gimp_dialog_factory_class_init (GimpDialogFactoryClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_VBOX);

  object_class->destroy = gimp_dialog_factory_destroy;
}

static void
gimp_dialog_factory_init (GimpDialogFactory *factory)
{
  factory->item_factory       = NULL;
  factory->registered_dialogs = NULL;
}

static void
gimp_dialog_factory_destroy (GtkObject *object)
{
  GimpDialogFactory *factory;
  GList             *list;

  factory = GIMP_DIALOG_FACTORY (object);

  for (list = factory->registered_dialogs; list; list = g_list_next (list))
    {
      GimpDialogFactoryEntry *entry;

      entry = (GimpDialogFactoryEntry *) list->data;

      g_free (entry->identifier);
      g_free (entry);
    }

  g_list_free (factory->registered_dialogs);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GimpDialogFactory *
gimp_dialog_factory_new (GtkItemFactory *item_factory)
{
  GimpDialogFactory *factory;

  g_return_val_if_fail (item_factory != NULL, NULL);
  g_return_val_if_fail (GTK_IS_ITEM_FACTORY (item_factory), NULL);

  factory = gtk_type_new (GIMP_TYPE_DIALOG_FACTORY);

  factory->item_factory = item_factory;

  return factory;
}

void
gimp_dialog_factory_register (GimpDialogFactory *factory,
			      const gchar       *identifier,
			      GimpDialogNewFunc  new_func)
{
  GimpDialogFactoryEntry *entry;

  g_return_if_fail (factory != NULL);
  g_return_if_fail (GIMP_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (new_func != NULL);

  entry = g_new0 (GimpDialogFactoryEntry, 1);

  entry->identifier = g_strdup (identifier);
  entry->new_func   = new_func;

  factory->registered_dialogs = g_list_prepend (factory->registered_dialogs,
						entry);
}

GimpDockable *
gimp_dialog_factory_dialog_new (GimpDialogFactory *factory,
				const gchar       *identifier)
{
  GList *list;

  g_return_val_if_fail (factory != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  for (list = factory->registered_dialogs; list; list = g_list_next (list))
    {
      GimpDialogFactoryEntry *entry;

      entry = (GimpDialogFactoryEntry *) list->data;

      if (! strcmp (identifier, entry->identifier))
	{
	  return entry->new_func ();
	}
    }

  return NULL;
}
