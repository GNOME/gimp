/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdialogfactory.c
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

static void   gimp_dialog_factory_get_window_info     (GtkWidget       *window, 
						       GimpSessionInfo *info);
static void   gimp_dialog_factory_set_window_geometry (GtkWidget       *window,
						       GimpSessionInfo *info,
						       gboolean         set_size);


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
  factory->new_dock_func      = NULL;
  factory->registered_dialogs = NULL;
  factory->session_infos      = NULL;
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
gimp_dialog_factory_new (const gchar       *name,
			 GimpContext       *context,
			 GtkItemFactory    *item_factory,
			 GimpDialogNewFunc  new_dock_func)
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

  factory->context       = context;
  factory->item_factory  = item_factory;
  factory->new_dock_func = new_dock_func;

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
			      GimpDialogNewFunc  new_func,
			      gboolean           singleton,
			      gboolean           session_managed)
{
  GimpDialogFactoryEntry *entry;

  g_return_if_fail (factory != NULL);
  g_return_if_fail (GIMP_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (identifier != NULL);

  entry = g_new0 (GimpDialogFactoryEntry, 1);

  entry->identifier      = g_strdup (identifier);
  entry->new_func        = new_func;
  entry->singleton       = singleton ? TRUE : FALSE;
  entry->session_managed = session_managed ? TRUE : FALSE;

  factory->registered_dialogs = g_list_prepend (factory->registered_dialogs,
						entry);
}

GimpDialogFactoryEntry *
gimp_dialog_factory_find_entry (GimpDialogFactory *factory,
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
	  return entry;
	}
    }

  return NULL;
}

GimpSessionInfo *
gimp_dialog_factory_find_session_info (GimpDialogFactory *factory,
				       const gchar       *identifier)
{
  GList *list;

  g_return_val_if_fail (factory != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  for (list = factory->session_infos; list; list = g_list_next (list))
    {
      GimpSessionInfo *info;

      info = (GimpSessionInfo *) list->data;

      if (info->toplevel_entry &&
	  ! strcmp (identifier, info->toplevel_entry->identifier))
	{
	  return info;
	}
    }

  return NULL;
}

GtkWidget *
gimp_dialog_factory_dialog_new (GimpDialogFactory *factory,
				const gchar       *identifier)
{
  GimpDialogFactoryEntry *entry;
  GtkWidget              *dialog = NULL;

  g_return_val_if_fail (factory != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  entry = gimp_dialog_factory_find_entry (factory, identifier);

  if (! entry)
    {
      g_warning ("%s(): no entry entry registered for \"%s\"",
		 G_GNUC_FUNCTION, identifier);
      return NULL;
    }

  if (! entry->new_func)
    {
      g_warning ("%s(): entry for \"%s\" has no constructor",
		 G_GNUC_FUNCTION, identifier);
      return NULL;
    }

  if (entry->singleton)
    {
      GimpSessionInfo *info;

      info = gimp_dialog_factory_find_session_info (factory, identifier);

      if (info)
	dialog = info->widget;
    }

  if (! dialog)
    {
      dialog = entry->new_func (factory);

      if (dialog)
	{
	  gtk_object_set_data (GTK_OBJECT (dialog),
			       "gimp-dialog-factory",
			       factory);
	  gtk_object_set_data (GTK_OBJECT (dialog),
			       "gimp-dialog-factory-entry",
			       entry);

	  if (GTK_WIDGET_TOPLEVEL (dialog))
	    {
	      gimp_dialog_factory_add_toplevel (factory, dialog);
	    }
	}
    }

  if (dialog && GTK_WIDGET_TOPLEVEL (dialog))
    {
      if (! GTK_WIDGET_VISIBLE (dialog))
	gtk_widget_show (dialog);
      else if (dialog->window)
	gdk_window_raise (dialog->window);
    }

  return dialog;
}

GtkWidget *
gimp_dialog_factory_dock_new (GimpDialogFactory *factory)
{
  GtkWidget *dock;

  g_return_val_if_fail (factory != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (factory->new_dock_func != NULL, NULL);

  dock = factory->new_dock_func (factory);

  if (dock)
    gimp_dialog_factory_add_toplevel (factory, dock);

  return dock;
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
      g_print ("%s: registering toplevel \"%s\"\n",
	       G_GNUC_FUNCTION, entry->identifier);

      for (list = factory->session_infos; list; list = g_list_next (list))
	{
	  info = (GimpSessionInfo *) list->data;

	  if (info->toplevel_entry == entry)
	    {
	      if (entry->singleton)
		{
		  if (info->widget)
		    {
		      g_warning ("%s(): singleton dialog \"%s\"created twice",
				 G_GNUC_FUNCTION, entry->identifier);
		      return;
		    }
		}
	      else if (info->widget)
		{
		  continue;
		}

	      info->widget = toplevel;

	      if (entry->session_managed)
		{
		  gimp_dialog_factory_set_window_geometry (info->widget,
							   info, FALSE);
		}

	      break;
	    }
	}

      if (! list) /*  didn't find a session info  */
	{
	  info = g_new0 (GimpSessionInfo, 1);

	  info->widget         = toplevel;
	  info->toplevel_entry = entry;

	  factory->session_infos = g_list_append (factory->session_infos, info);
	}
    }
  else /*  toplevel is a dock  */
    {
      g_print ("%s: registering dock\n", G_GNUC_FUNCTION);

      for (list = factory->session_infos; list; list = g_list_next (list))
	{
	  info = (GimpSessionInfo *) list->data;

	  if (! info->widget) /*  take the first empty slot  */
	    {
	      info->widget = toplevel;

	      gimp_dialog_factory_set_window_geometry (info->widget,
						       info, FALSE);

	      break;
	    }
	}

      if (! list) /*  didn't find a session info  */
	{
	  info = g_new0 (GimpSessionInfo, 1);

	  info->widget = toplevel;

	  factory->session_infos = g_list_append (factory->session_infos, info);
	}
    }

  factory->open_dialogs = g_list_prepend (factory->open_dialogs, toplevel);

  gtk_signal_connect_object_while_alive (GTK_OBJECT (toplevel), "destroy",
					 GTK_SIGNAL_FUNC (gimp_dialog_factory_remove_toplevel),
					 GTK_OBJECT (factory));
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

      /*  we keep session info entries for all dialogs created by the
       *  factory but don't save them if they don't want to be managed
       */
      if (info->toplevel_entry && ! info->toplevel_entry->session_managed)
	continue;

      if (info->widget)
	gimp_dialog_factory_get_window_info (info->widget, info);

      fprintf (fp, "(session-info \"%s\" \"%s\"\n",
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

      info->open = FALSE;

      if (info->toplevel_entry)
	{
	  gimp_dialog_factory_dialog_new (factory,
					  info->toplevel_entry->identifier);
	}
      else
	{
	  GimpDock *dock;
	  GList    *books;

	  dock = GIMP_DOCK (gimp_dialog_factory_dock_new (factory));

	  for (books = info->sub_dialogs; books; books = g_list_next (books))
	    {
	      GimpDockbook *dockbook;
	      GList        *pages;

	      dockbook = GIMP_DOCKBOOK (gimp_dockbook_new ());

	      gimp_dock_add_book (dock, dockbook, -1);

	      for (pages = books->data; pages; pages = g_list_next (pages))
		{
		  GtkWidget *dockable;
		  gchar     *identifier;

		  identifier = (gchar *) pages->data;

		  dockable = gimp_dialog_factory_dialog_new (factory,
							     identifier);

		  if (dockable)
		    gimp_dockbook_add (dockbook, GIMP_DOCKABLE (dockable), -1);

		  g_free (identifier);
		}

	      g_list_free (books->data);
	    }

	  g_list_free (info->sub_dialogs);
	  info->sub_dialogs = NULL;

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


/*  private functions  */

static void
gimp_dialog_factory_get_window_info (GtkWidget       *window,
				     GimpSessionInfo *info)
{
  g_return_if_fail (window != NULL);
  g_return_if_fail (GTK_IS_WIDGET (window));
  g_return_if_fail (GTK_WIDGET_TOPLEVEL (window));
  g_return_if_fail (info != NULL);

  if (window->window)
    {
      gdk_window_get_root_origin (window->window, &info->x, &info->y);
      gdk_window_get_size (window->window, &info->width, &info->height);
    }

  info->open = GTK_WIDGET_VISIBLE (window);
}

static void
gimp_dialog_factory_set_window_geometry (GtkWidget       *window,
					 GimpSessionInfo *info,
					 gboolean         set_size)
{
  static gint screen_width  = 0;
  static gint screen_height = 0;
  
  g_return_if_fail (window != NULL);
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (GTK_WIDGET_TOPLEVEL (window));
  g_return_if_fail (info != NULL);
  
  if (screen_width == 0 || screen_height == 0)
    {
      screen_width  = gdk_screen_width ();
      screen_height = gdk_screen_height ();
    }

  info->x = CLAMP (info->x, 0, screen_width  - 32);
  info->y = CLAMP (info->y, 0, screen_height - 32);

  gtk_widget_set_uposition (window, info->x, info->y);

  if (set_size)
    {
      gtk_window_set_default_size (GTK_WINDOW (window),
				   info->width, info->height);
    }
}
