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

#include <glib-object.h>

#include "core-types.h"

#include "gimpcontext.h"
#include "gimpdata.h"
#include "gimpdatafactory.h"
#include "gimpdatafiles.h"
#include "gimpdatalist.h"

#include "libgimp/gimpintl.h"


static void   gimp_data_factory_class_init (GimpDataFactoryClass *klass);
static void   gimp_data_factory_init       (GimpDataFactory      *factory);

static void   gimp_data_factory_finalize   (GObject              *object);

static void   gimp_data_factory_data_load_callback (const gchar *filename,
						    gpointer     callback_data);


static GimpObjectClass *parent_class = NULL;


GType
gimp_data_factory_get_type (void)
{
  static GType factory_type = 0;

  if (! factory_type)
    {
      static const GTypeInfo factory_info =
      {
        sizeof (GimpDataFactoryClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_data_factory_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpDataFactory),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_data_factory_init,
      };

      factory_type = g_type_register_static (GIMP_TYPE_OBJECT,
					     "GimpDataFactory",
					     &factory_info, 0);
    }

  return factory_type;
}

static void
gimp_data_factory_class_init (GimpDataFactoryClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_data_factory_finalize;
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
gimp_data_factory_finalize (GObject *object)
{
  GimpDataFactory *factory;

  factory = GIMP_DATA_FACTORY (object);

  if (factory->container)
    {
      g_object_unref (G_OBJECT (factory->container));
      factory->container = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpDataFactory *
gimp_data_factory_new (GType                              data_type,
		       const gchar                      **data_path,
		       const GimpDataFactoryLoaderEntry  *loader_entries,
		       gint                               n_loader_entries,
		       GimpDataNewFunc                    new_func,
		       GimpDataGetStandardFunc            standard_func)
{
  GimpDataFactory *factory;

  g_return_val_if_fail (g_type_is_a (data_type, GIMP_TYPE_DATA), NULL);
  g_return_val_if_fail (data_path != NULL, NULL);
  g_return_val_if_fail (loader_entries != NULL, NULL);
  g_return_val_if_fail (n_loader_entries > 0, NULL);

  factory = g_object_new (GIMP_TYPE_DATA_FACTORY, NULL);

  factory->container = gimp_data_list_new (data_type);

  factory->data_path              = data_path;

  factory->loader_entries         = loader_entries;
  factory->n_loader_entries       = n_loader_entries;

  factory->data_new_func          = new_func;
  factory->data_get_standard_func = standard_func;

  return factory;
}

void
gimp_data_factory_data_init (GimpDataFactory *factory,
			     gboolean         no_data /* FIXME */)
{
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

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  if (gimp_container_num_children (factory->container) == 0)
    return;

  list = GIMP_LIST (factory->container);

  gimp_container_freeze (factory->container);

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
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (factory->data_new_func)
    {
      GimpData *data;

      data = factory->data_new_func (name);

      gimp_container_add (factory->container, GIMP_OBJECT (data));
      g_object_unref (G_OBJECT (data));

      return data;
    }

  return NULL;
}

GimpData *
gimp_data_factory_data_get_standard (GimpDataFactory *factory)
{
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
          g_message (_("Trying legacy loader on\n"
		       "file '%s'\n" 
		       "with unknown extension."),
                     filename);
          goto insert;
        }
    }

  return;

 insert:
  {
    GimpData *data;

    data = (GimpData *) (* factory->loader_entries[i].load_func) (filename);

    if (! data)
      {
	g_message (_("Warning: Failed to load data from\n'%s'"), filename);
      }
    else
      {
	gimp_container_add (factory->container, GIMP_OBJECT (data));
	g_object_unref (G_OBJECT (data));
      }
  }
}
