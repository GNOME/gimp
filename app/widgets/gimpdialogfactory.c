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
#include "gimpdock.h"
#include "gimpdockbook.h"
#include "gimpdockable.h"

#include "gimpcontext.h"


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

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);

  object_class->destroy = gimp_dialog_factory_destroy;

  klass->factories = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
gimp_dialog_factory_init (GimpDialogFactory *factory)
{
  factory->item_factory       = NULL;
  factory->registered_dialogs = NULL;
  factory->open_dialogs       = NULL;
}

static void
gimp_dialog_factory_destroy (GtkObject *object)
{
  GimpDialogFactory *factory;
  GList             *list;

  factory = GIMP_DIALOG_FACTORY (object);

  g_hash_table_remove (GIMP_DIALOG_FACTORY_CLASS (object->klass)->factories,
		       GIMP_OBJECT (factory)->name);

  for (list = factory->registered_dialogs; list; list = g_list_next (list))
    {
      GimpDialogFactoryEntry *entry;

      entry = (GimpDialogFactoryEntry *) list->data;

      g_free (entry->identifier);
      g_free (entry);
    }

  g_list_free (factory->registered_dialogs);
  g_list_free (factory->open_dialogs);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GimpDialogFactory *
gimp_dialog_factory_new (const gchar    *name,
			 GimpContext    *context,
			 GtkItemFactory *item_factory)
{
  GimpDialogFactoryClass *factory_class;
  GimpDialogFactory      *factory;

  g_return_val_if_fail (name != NULL, NULL);

  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  g_return_val_if_fail (! item_factory || GTK_IS_ITEM_FACTORY (item_factory),
			NULL);

  factory_class =
    GIMP_DIALOG_FACTORY_CLASS (gtk_type_class (GIMP_TYPE_DIALOG_FACTORY));

  if (g_hash_table_lookup (factory_class->factories, name))
    {
      g_warning ("%s(): dialog factory \"%s\" already exists",
		 G_GNUC_FUNCTION, name);
      return NULL;
    }

  factory = gtk_type_new (GIMP_TYPE_DIALOG_FACTORY);

  gimp_object_set_name (GIMP_OBJECT (factory), name);

  g_hash_table_insert (factory_class->factories,
		       GIMP_OBJECT (factory)->name, factory);

  factory->context      = context;
  factory->item_factory = item_factory;

  return factory;
}

GimpDialogFactory *
gimp_dialog_factory_from_name (const gchar *name)
{
  GimpDialogFactoryClass *factory_class;
  GimpDialogFactory      *factory;

  g_return_val_if_fail (name != NULL, NULL);

  factory_class =
    GIMP_DIALOG_FACTORY_CLASS (gtk_type_class (GIMP_TYPE_DIALOG_FACTORY));

  factory = (GimpDialogFactory *)
    g_hash_table_lookup (factory_class->factories, name);

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

GtkWidget *
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
	  GtkWidget *dialog;

	  dialog = entry->new_func (factory);

	  gtk_object_set_data (GTK_OBJECT (dialog), "gimp-dialog-factory",
			       factory);
	  gtk_object_set_data (GTK_OBJECT (dialog), "gimp-dialog-factory-entry",
			       entry);

	  return dialog;
	}
    }

  return NULL;
}

void
gimp_dialog_factory_add_toplevel (GimpDialogFactory *factory,
				  GtkWidget         *toplevel)
{
  GimpDialogFactory      *toplevel_factory;
  GimpDialogFactoryEntry *entry;
  GimpSessionInfo        *info;
  GList                  *list;

  g_return_if_fail (factory != NULL);
  g_return_if_fail (GIMP_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (toplevel != NULL);
  g_return_if_fail (GTK_IS_WIDGET (toplevel));

  if (g_list_find (factory->open_dialogs, toplevel))
    {
      g_warning ("%s(): dialog already registered", G_GNUC_FUNCTION);
      return;
    }

  toplevel_factory = gtk_object_get_data (GTK_OBJECT (toplevel),
					  "gimp-dialog-factory");

  if (toplevel_factory && toplevel_factory != factory)
    {
      g_warning ("%s(): dialog was created by a different GimpDialogFactory",
		 G_GNUC_FUNCTION);
      return;
    }

  entry = gtk_object_get_data (GTK_OBJECT (toplevel),
			       "gimp-dialog-factory-entry");

  if (entry) /*  toplevel was created by this factory  */
    {
      for (list = factory->session_infos; list; list = g_list_next (list))
	{
	  info = (GimpSessionInfo *) list->data;

	  if (info->toplevel_entry == entry)
	    {
	      info->open   = TRUE;
	      info->widget = toplevel;

	      break;
	    }
	}

      if (! list) /*  didn't find a session info  */
	{
	  info = g_new0 (GimpSessionInfo, 1);

	  info->open           = TRUE;
	  info->widget         = toplevel;
	  info->toplevel_entry = entry;

	  factory->session_infos = g_list_append (factory->session_infos, info);
	}
    }
  else /*  toplevel is a dock  */
    {
      for (list = factory->session_infos; list; list = g_list_next (list))
	{
	  info = (GimpSessionInfo *) list->data;

	  if (! info->widget) /*  take the first empty slot  */
	    {
	      info->open   = TRUE;
	      info->widget = toplevel;

	      info->x = CLAMP (info->x, 0, gdk_screen_width ()  - 32);
	      info->y = CLAMP (info->y, 0, gdk_screen_height () - 32);

	      if (info->x && info->y)
		gtk_widget_set_uposition (toplevel, info->x, info->y);

	      break;
	    }
	}

      if (! list) /*  didn't find a session info  */
	{
	  info = g_new0 (GimpSessionInfo, 1);

	  info->open   = TRUE;
	  info->widget = toplevel;

	  factory->session_infos = g_list_append (factory->session_infos, info);
	}
    }

  factory->open_dialogs = g_list_prepend (factory->open_dialogs, toplevel);
}

void
gimp_dialog_factory_remove_toplevel (GimpDialogFactory *factory,
				     GtkWidget         *toplevel)
{
  GimpDialogFactory *toplevel_factory;
  GimpSessionInfo   *session_info;
  GList             *list;

  g_return_if_fail (factory != NULL);
  g_return_if_fail (GIMP_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (toplevel != NULL);
  g_return_if_fail (GTK_IS_WIDGET (toplevel));

  if (! g_list_find (factory->open_dialogs, toplevel))
    {
      g_warning ("%s(): dialog not registered", G_GNUC_FUNCTION);
      return;
    }

  factory->open_dialogs = g_list_remove (factory->open_dialogs, toplevel);

  toplevel_factory = gtk_object_get_data (GTK_OBJECT (toplevel),
					  "gimp-dialog-factory");

  if (toplevel_factory && toplevel_factory != factory)
    {
      g_warning ("%s(): dialog was created by a different GimpDialogFactory",
		 G_GNUC_FUNCTION);
      return;
    }

  for (list = factory->session_infos; list; list = g_list_next (list))
    {
      session_info = (GimpSessionInfo *) list->data;

      if (session_info->widget == toplevel)
	{
	  session_info->open   = FALSE;
	  session_info->widget = NULL;

	  break;
	}
    }
}

static void
gimp_dialog_factories_session_save_foreach (gchar             *name,
					    GimpDialogFactory *factory,
					    FILE              *fp)
{
  GList *list;

  for (list = factory->session_infos; list; list = g_list_next (list))
    {
      GimpSessionInfo *info;

      info = (GimpSessionInfo *) list->data;

      if (info->widget)
	{
	  gdk_window_get_root_origin (info->widget->window,
				      &info->x, &info->y);
	  gdk_window_get_size (info->widget->window,
			       &info->width, &info->height);

	  info->open = GTK_WIDGET_VISIBLE (info->widget);
	}

      fprintf (fp, "(new-session-info \"%s\" \"%s\"\n",
	       name,
	       info->toplevel_entry ? info->toplevel_entry->identifier : "dock");
      fprintf (fp, "    (position %d %d)\n", info->x, info->y);
      fprintf (fp, "    (size %d %d)", info->width, info->height);

      if (info->open)
	fprintf (fp, "\n    (open-on-exit)");

      if (! info->toplevel_entry && info->widget)
	{
	  GimpDock *dock;
	  GList    *books;

	  dock = GIMP_DOCK (info->widget);

	  fprintf (fp, "\n    (dock ");

	  for (books = dock->dockbooks; books; books = g_list_next (books))
	    {
	      GimpDockbook *dockbook;
	      GList        *pages;

	      dockbook = (GimpDockbook *) books->data;

	      fprintf (fp, "(");

	      for (pages = gtk_container_children (GTK_CONTAINER (dockbook));
		   pages;
		   pages = g_list_next (pages))
		{
		  GimpDockable           *dockable;
		  GimpDialogFactoryEntry *entry;

		  dockable = (GimpDockable *) pages->data;

		  entry = gtk_object_get_data (GTK_OBJECT (dockable),
					       "gimp-dialog-factory-entry");

		  if (entry)
		    fprintf (fp, "\"%s\"%s",
			     entry->identifier, pages->next ? " ": "");
		}

	      fprintf (fp, ")%s", books->next ? "\n          " : "");
	    }

	  fprintf (fp, ")");
	}

      fprintf (fp, ")\n\n");
    }
}

void
gimp_dialog_factories_session_save (FILE *file)
{
  GimpDialogFactoryClass *factory_class;

  g_return_if_fail (file != NULL);

  factory_class =
    GIMP_DIALOG_FACTORY_CLASS (gtk_type_class (GIMP_TYPE_DIALOG_FACTORY));

  g_hash_table_foreach (factory_class->factories,
			(GHFunc) gimp_dialog_factories_session_save_foreach,
			file);
}

static void
gimp_dialog_factories_session_restore_foreach (gchar             *name,
					       GimpDialogFactory *factory,
					       gpointer           data)
{
  GList *list;

  for (list = factory->session_infos; list; list = g_list_next (list))
    {
      GimpSessionInfo *info;

      info = (GimpSessionInfo *) list->data;

      if (! info->open)
	continue;

      if (info->toplevel_entry)
	{
	}
      else
	{
	  GimpDock *dock;
	  GList    *books;

	  dock = GIMP_DOCK (gimp_dock_new (factory));

	  while ((books = info->sub_dialogs))
	    {
	      GimpDockbook *dockbook;
	      GList        *pages;

	      dockbook = GIMP_DOCKBOOK (gimp_dockbook_new ());

	      gimp_dock_add_book (dock, dockbook, -1);

	      while ((pages = (GList *) books->data))
		{
		  GtkWidget *dockable;
		  gchar     *identifier;

		  identifier = (gchar *) pages->data;

		  dockable = gimp_dialog_factory_dialog_new (factory,
							     identifier);

		  if (dockable)
		    gimp_dockbook_add (dockbook, GIMP_DOCKABLE (dockable), -1);

		  books->data = g_list_remove (books->data, identifier);
		  g_free (identifier);
		}

	      info->sub_dialogs = g_list_remove (info->sub_dialogs,
						 info->sub_dialogs->data);
	    }

	  gtk_widget_show (GTK_WIDGET (dock));
	}
    }
}

void
gimp_dialog_factories_session_restore (void)
{
  GimpDialogFactoryClass *factory_class;

  factory_class =
    GIMP_DIALOG_FACTORY_CLASS (gtk_type_class (GIMP_TYPE_DIALOG_FACTORY));

  g_hash_table_foreach (factory_class->factories,
			(GHFunc) gimp_dialog_factories_session_restore_foreach,
			NULL);
}
