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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpbaseconfig.h"
#include "config/gimpconfig-path.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdata.h"
#include "gimpdatafactory.h"
#include "gimpdatalist.h"

#include "gimp-intl.h"


static void    gimp_data_factory_class_init    (GimpDataFactoryClass *klass);
static void    gimp_data_factory_init          (GimpDataFactory      *factory);

static void    gimp_data_factory_finalize      (GObject              *object);

static gsize   gimp_data_factory_get_memsize   (GimpObject           *object,
                                                gsize                *gui_size);

static void    gimp_data_factory_load_data   (const GimpDatafileData *file_data,
                                              gpointer                user_data);


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
  GObjectClass    *object_class;
  GimpObjectClass *gimp_object_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize         = gimp_data_factory_finalize;

  gimp_object_class->get_memsize = gimp_data_factory_get_memsize;
}

static void
gimp_data_factory_init (GimpDataFactory *factory)
{
  factory->gimp                   = NULL;
  factory->container              = NULL;
  factory->path_property_name     = NULL;
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
      g_object_unref (factory->container);
      factory->container = NULL;
    }

  if (factory->path_property_name)
    {
      g_free (factory->path_property_name);
      factory->path_property_name = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_data_factory_get_memsize (GimpObject *object,
                               gsize      *gui_size)
{
  GimpDataFactory *factory;
  gsize            memsize = 0;

  factory = GIMP_DATA_FACTORY (object);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (factory->container),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

GimpDataFactory *
gimp_data_factory_new (Gimp                             *gimp,
                       GType                             data_type,
		       const gchar                      *path_property_name,
		       const GimpDataFactoryLoaderEntry *loader_entries,
		       gint                              n_loader_entries,
		       GimpDataNewFunc                   new_func,
		       GimpDataGetStandardFunc           standard_func)
{
  GimpDataFactory *factory;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (data_type, GIMP_TYPE_DATA), NULL);
  g_return_val_if_fail (path_property_name != NULL, NULL);
  g_return_val_if_fail (loader_entries != NULL, NULL);
  g_return_val_if_fail (n_loader_entries > 0, NULL);

  factory = g_object_new (GIMP_TYPE_DATA_FACTORY, NULL);

  factory->gimp                   = gimp;
  factory->container              = gimp_data_list_new (data_type);

  factory->path_property_name     = g_strdup (path_property_name);

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
  gchar *path = NULL;

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  gimp_container_freeze (factory->container);

  gimp_data_factory_data_free (factory);

  g_object_get (factory->gimp->config,
                factory->path_property_name, &path,
                NULL);

  if (path && strlen (path))
    {
      gchar    *tmp;

      tmp = gimp_config_path_expand (path, TRUE, NULL);
      g_free (path);
      path = tmp;

      gimp_datafiles_read_directories (path,
                                       G_FILE_TEST_EXISTS,
				       gimp_data_factory_load_data,
				       factory);
    }

  g_free (path);

  gimp_container_thaw (factory->container);  
}

void
gimp_data_factory_data_save (GimpDataFactory *factory)
{
  gchar *path = NULL;

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  if (gimp_container_num_children (factory->container) == 0)
    return;

  g_object_get (factory->gimp->config,
                factory->path_property_name, &path,
                NULL);

  if (path && strlen (path))
    {
      GimpList *gimp_list;
      GList    *list;
      gchar    *tmp;

      tmp = gimp_config_path_expand (path, TRUE, NULL);
      g_free (path);
      path = tmp;

      gimp_list = GIMP_LIST (factory->container);

      gimp_container_freeze (factory->container);

      for (list = gimp_list->list; list; list = g_list_next (list))
        {
          GimpData *data;

          data = GIMP_DATA (list->data);

          if (! data->filename)
            gimp_data_create_filename (data, GIMP_OBJECT (data)->name, path);

          if (data->dirty)
            {
              GError *error = NULL;

              if (! gimp_data_save (data, &error))
                {
                  /*  check if there actually was an error (no error
                   *  means the data class does not implement save)
                   */
                  if (error)
                    {
                      g_message (_("Warning: Failed to save data:\n%s"),
                                 error->message);
                      g_clear_error (&error);
                    }
                }
            }
        }

      gimp_container_thaw (factory->container);
    }

  g_free (path);
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

  if (list->list)
    {
      if (GIMP_DATA (list->list->data)->internal)
        {
          /*  if there are internal objects in the list, skip them  */

          GList *glist;

          for (glist = list->list; glist; glist = g_list_next (glist))
            {
              if (glist->next && ! GIMP_DATA (glist->next->data)->internal)
                {
                  while (glist->next)
                    {
                      gimp_container_remove (factory->container,
                                             GIMP_OBJECT (glist->next->data));
                   }

                  break;
                }
            }
        }
      else
        {
          /*  otherwise delete everything  */

          while (list->list)
            {
              gimp_container_remove (factory->container,
                                     GIMP_OBJECT (list->list->data));
            }
        }
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
      GimpBaseConfig *base_config;
      GimpData       *data;

      base_config = GIMP_BASE_CONFIG (factory->gimp->config);

      data = factory->data_new_func (name, base_config->stingy_memory_use);

      gimp_container_add (factory->container, GIMP_OBJECT (data));
      g_object_unref (data);

      return data;
    }

  return NULL;
}

GimpData *
gimp_data_factory_data_duplicate (GimpDataFactory *factory,
                                  GimpData        *data)
{
  GimpBaseConfig *base_config;
  GimpData       *new_data;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  base_config = GIMP_BASE_CONFIG (factory->gimp->config);

  new_data = gimp_data_duplicate (data, base_config->stingy_memory_use);

  if (new_data)
    {
      const gchar *name;
      gchar       *ext;
      gint         copy_len;
      gint         number;
      gchar       *new_name;

      name = gimp_object_get_name (GIMP_OBJECT (data));

      ext      = strrchr (name, '#');
      copy_len = strlen (_("copy"));

      if ((strlen (name) >= copy_len                                 &&
           strcmp (&name[strlen (name) - copy_len], _("copy")) == 0) ||
          (ext && (number = atoi (ext + 1)) > 0                      &&
           ((gint) (log10 (number) + 1)) == strlen (ext + 1)))
        {
          /* don't have redundant "copy"s */
          new_name = g_strdup (name);
        }
      else
        {
          new_name = g_strdup_printf (_("%s copy"), name);
        }

      gimp_object_set_name (GIMP_OBJECT (new_data), new_name);

      g_free (new_name);

      gimp_container_add (factory->container, GIMP_OBJECT (new_data));
      g_object_unref (new_data);
    }

  return new_data;
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
gimp_data_factory_load_data (const GimpDatafileData *file_data,
                             gpointer                user_data)
{
  GimpDataFactory *factory;
  gint             i;

  factory = GIMP_DATA_FACTORY (user_data);

  for (i = 0; i < factory->n_loader_entries; i++)
    {
      if (factory->loader_entries[i].extension)
        {
          if (gimp_datafiles_check_extension (file_data->filename,
					      factory->loader_entries[i].extension))
	    {
              goto insert;
            }
        }
      else
        {
          g_message (_("Trying legacy loader on file '%s' "
		       "with unknown extension."),
                     file_data->filename);
          goto insert;
        }
    }

  return;

 insert:
  {
    GimpBaseConfig *base_config;
    GimpData       *data;
    GError         *error = NULL;

    base_config = GIMP_BASE_CONFIG (factory->gimp->config);

    data = (GimpData *)
      (* factory->loader_entries[i].load_func) (file_data->filename,
                                                base_config->stingy_memory_use,
                                                &error);

    if (! data)
      {
	g_message (_("Warning: Failed to load data:\n%s"),
                   error->message);
        g_clear_error (&error);
      }
    else
      {
	gimp_container_add (factory->container, GIMP_OBJECT (data));
	g_object_unref (data);
      }
  }
}
