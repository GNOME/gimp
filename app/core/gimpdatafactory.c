/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactory.c
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

#include "config.h"

#include <gtk/gtk.h>

#include "core-types.h"

#include "datafiles.h"
#include "gimpdata.h"
#include "gimpdatalist.h"
#include "gimpdatafactory.h"
#include "gimpcontext.h"
#include "gimpmarshal.h"

#include "libgimp/gimpintl.h"


static void   gimp_data_factory_class_init (GimpDataFactoryClass *klass);
static void   gimp_data_factory_init       (GimpDataFactory      *factory);
static void   gimp_data_factory_destroy    (GtkObject            *object);

static void   gimp_data_factory_data_load_callback (const gchar *filename,
						    gpointer     callback_data);


static GimpObjectClass *parent_class = NULL;


GtkType
gimp_data_factory_get_type (void)
{
  static guint factory_type = 0;

  if (! factory_type)
    {
      GtkTypeInfo factory_info =
      {
	"GimpDataFactory",
	sizeof (GimpDataFactory),
	sizeof (GimpDataFactoryClass),
	(GtkClassInitFunc) gimp_data_factory_class_init,
	(GtkObjectInitFunc) gimp_data_factory_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      factory_type = gtk_type_unique (GIMP_TYPE_OBJECT, &factory_info);
    }

  return factory_type;
}

static void
gimp_data_factory_class_init (GimpDataFactoryClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);

  object_class->destroy = gimp_data_factory_destroy;
}

static void
gimp_data_factory_init (GimpDataFactory *factory)
{
  factory->container              = NULL;
  factory->data_path              = NULL;
  factory->loader_entries         = NULL;
  factory->n_loader_entries       = 0;
  factory->data_new_func          = NULL;
  factory->data_get_standard_func = NULL;
}

static void
gimp_data_factory_destroy (GtkObject *object)
{
  GimpDataFactory *factory;

  factory = GIMP_DATA_FACTORY (object);

  if (factory->container)
    {
      gtk_object_unref (GTK_OBJECT (factory->container));
      factory->container = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GimpDataFactory *
gimp_data_factory_new (GtkType                            data_type,
		       const gchar                      **data_path,
		       const GimpDataFactoryLoaderEntry  *loader_entries,
		       gint                               n_loader_entries,
		       GimpDataNewFunc                    new_func,
		       GimpDataGetStandardFunc            standard_func)
{
  GimpDataFactory *factory;

  g_return_val_if_fail (gtk_type_is_a (data_type, GIMP_TYPE_DATA), NULL);
  g_return_val_if_fail (data_path != NULL, NULL);
  g_return_val_if_fail (loader_entries != NULL, NULL);
  g_return_val_if_fail (n_loader_entries > 0, NULL);

  factory = gtk_type_new (GIMP_TYPE_DATA_FACTORY);

  factory->container = gimp_data_list_new (data_type);

  gtk_object_ref (GTK_OBJECT (factory->container));
  gtk_object_sink (GTK_OBJECT (factory->container));

  factory->data_path              = data_path;

  factory->loader_entries         = loader_entries;
  factory->n_loader_entries       = n_loader_entries;

  factory->data_new_func          = new_func;
  factory->data_get_standard_func = standard_func;

  return factory;
}

void
gimp_data_factory_data_init (GimpDataFactory *factory,
			     gboolean         no_data)
{
  g_return_if_fail (factory != NULL);
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  gimp_container_freeze (factory->container);

  if (gimp_container_num_children (factory->container) > 0)
    gimp_data_factory_data_free (factory);

  if (factory->data_path && *factory->data_path)
    {
      gimp_datafiles_read_directories (*factory->data_path, 0,
				       gimp_data_factory_data_load_callback,
				       factory);
    }

  gimp_container_thaw (factory->container);  
}

void
gimp_data_factory_data_save (GimpDataFactory *factory)
{
  GimpList *gimp_list;
  GList    *list;

  g_return_if_fail (factory != NULL);
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  if (gimp_container_num_children (factory->container) == 0)
    return;

  if (! (factory->data_path && *factory->data_path))
    return;

  gimp_list = GIMP_LIST (factory->container);

  gimp_container_freeze (factory->container);

  for (list = gimp_list->list; list; list = g_list_next (list))
    {
      GimpData *data;

      data = GIMP_DATA (list->data);

      if (! data->filename)
	gimp_data_create_filename (data,
				   GIMP_OBJECT (data)->name,
				   *factory->data_path);

      if (data->dirty)
	gimp_data_save (data);
    }

  gimp_container_thaw (factory->container);
}

void
gimp_data_factory_data_free (GimpDataFactory *factory)
{
  GimpList *list;

  g_return_if_fail (factory != NULL);
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  if (gimp_container_num_children (factory->container) == 0)
    return;

  list = GIMP_LIST (factory->container);

  gimp_container_freeze (factory->container);

  gimp_data_factory_data_save (factory);

  while (list->list)
    {
      gimp_container_remove (factory->container,
			     GIMP_OBJECT (list->list->data));
    }

  gimp_container_thaw (factory->container);
}

GimpData *
gimp_data_factory_data_new (GimpDataFactory *factory,
			    const gchar     *name)
{
  g_return_val_if_fail (factory != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (factory->data_new_func)
    {
      GimpData *data;

      data = factory->data_new_func (name);

      gimp_container_add (factory->container, GIMP_OBJECT (data));

      return data;
    }

  return NULL;
}

GimpData *
gimp_data_factory_data_get_standard (GimpDataFactory *factory)
{
  g_return_val_if_fail (factory != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  if (factory->data_get_standard_func)
    return factory->data_get_standard_func ();

  return NULL;
}

static void
gimp_data_factory_data_load_callback (const gchar *filename,
				      gpointer     callback_data)
{
  GimpDataFactory *factory;
  gint             i;

  factory = (GimpDataFactory *) callback_data;

  for (i = 0; i < factory->n_loader_entries; i++)
    {
      if (factory->loader_entries[i].extension)
        {
          if (gimp_datafiles_check_extension (filename,
					      factory->loader_entries[i].extension))
	    {
              goto insert;
            }
        }
      else
        {
          g_warning ("%s(): trying legacy loader on file with unknown "
                     "extension: %s",
                     G_GNUC_FUNCTION, filename);
          goto insert;
        }
    }

  return;

 insert:
  {
    GimpData *data;

    data = (GimpData *) (* factory->loader_entries[i].load_func) (filename);

    if (! data)
      g_message (_("Warning: Failed to load data from\n\"%s\""), filename);
    else
      gimp_container_add (factory->container, GIMP_OBJECT (data));
  }
}
