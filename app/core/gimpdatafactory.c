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
#include "gimplist.h"

#include "gimp-intl.h"


#define WRITABLE_PATH_KEY "gimp-data-factory-writable-path"


static void    gimp_data_factory_class_init   (GimpDataFactoryClass   *klass);
static void    gimp_data_factory_init         (GimpDataFactory        *factory);

static void    gimp_data_factory_finalize     (GObject                *object);

static gint64  gimp_data_factory_get_memsize  (GimpObject             *object,
                                               gint64                 *gui_size);

static gchar * gimp_data_factory_get_save_dir (GimpDataFactory        *factory);
static void    gimp_data_factory_load_data    (const GimpDatafileData *file_data,
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
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

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
  factory->writable_property_name = NULL;
  factory->loader_entries         = NULL;
  factory->n_loader_entries       = 0;
  factory->data_new_func          = NULL;
  factory->data_get_standard_func = NULL;
}

static void
gimp_data_factory_finalize (GObject *object)
{
  GimpDataFactory *factory = GIMP_DATA_FACTORY (object);

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

  if (factory->writable_property_name)
    {
      g_free (factory->writable_property_name);
      factory->writable_property_name = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_data_factory_get_memsize (GimpObject *object,
                               gint64     *gui_size)
{
  GimpDataFactory *factory = GIMP_DATA_FACTORY (object);
  gint64           memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (factory->container),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

GimpDataFactory *
gimp_data_factory_new (Gimp                             *gimp,
                       GType                             data_type,
		       const gchar                      *path_property_name,
                       const gchar                      *writable_property_name,
		       const GimpDataFactoryLoaderEntry *loader_entries,
		       gint                              n_loader_entries,
		       GimpDataNewFunc                   new_func,
		       GimpDataGetStandardFunc           standard_func)
{
  GimpDataFactory *factory;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (data_type, GIMP_TYPE_DATA), NULL);
  g_return_val_if_fail (path_property_name != NULL, NULL);
  g_return_val_if_fail (writable_property_name != NULL, NULL);
  g_return_val_if_fail (loader_entries != NULL, NULL);
  g_return_val_if_fail (n_loader_entries > 0, NULL);

  factory = g_object_new (GIMP_TYPE_DATA_FACTORY, NULL);

  factory->gimp                   = gimp;
  factory->container              = gimp_list_new (data_type, TRUE);
  gimp_list_set_sort_func (GIMP_LIST (factory->container),
                           (GCompareFunc) gimp_data_name_compare);

  factory->path_property_name     = g_strdup (path_property_name);
  factory->writable_property_name = g_strdup (writable_property_name);

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
  gchar *path;
  gchar *writable_path;

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  gimp_container_freeze (factory->container);

  gimp_data_factory_data_free (factory);

  g_object_get (factory->gimp->config,
                factory->path_property_name,     &path,
                factory->writable_property_name, &writable_path,
                NULL);

  if (path && strlen (path))
    {
      GList *writable_list = NULL;
      gchar *tmp;

      tmp = gimp_config_path_expand (path, TRUE, NULL);
      g_free (path);
      path = tmp;

      if (writable_path)
        {
          tmp = gimp_config_path_expand (writable_path, TRUE, NULL);
          g_free (writable_path);
          writable_path = tmp;

          writable_list = gimp_path_parse (writable_path, 16, TRUE, NULL);

          g_object_set_data (G_OBJECT (factory), WRITABLE_PATH_KEY,
                             writable_list);
        }

      gimp_datafiles_read_directories (path,
                                       G_FILE_TEST_EXISTS,
				       gimp_data_factory_load_data,
				       factory);

      if (writable_path)
        {
          gimp_path_free (writable_list);
          g_object_set_data (G_OBJECT (factory), WRITABLE_PATH_KEY, NULL);
        }
    }

  g_free (path);
  g_free (writable_path);

  gimp_container_thaw (factory->container);
}

void
gimp_data_factory_data_save (GimpDataFactory *factory)
{
  GList *list;
  gchar *writable_dir;

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  if (gimp_container_num_children (factory->container) == 0)
    return;

  writable_dir = gimp_data_factory_get_save_dir (factory);

  if (! writable_dir)
    return;

  gimp_container_freeze (factory->container);

  for (list = GIMP_LIST (factory->container)->list;
       list;
       list = g_list_next (list))
    {
      GimpData *data = list->data;

      if (! data->filename)
        gimp_data_create_filename (data, writable_dir);

      if (data->dirty && data->writable)
        {
          GError *error = NULL;

          if (! gimp_data_save (data, &error))
            {
              /*  check if there actually was an error (no error
               *  means the data class does not implement save)
               */
              if (error)
                {
                  g_message (_("Warning: Failed to save data:\n\n%s"),
                             error->message);
                  g_clear_error (&error);
                }
            }
        }
    }

  gimp_container_thaw (factory->container);

  g_free (writable_dir);
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
  g_return_val_if_fail (*name != '\0', NULL);

  if (factory->data_new_func)
    {
      GimpBaseConfig *base_config;
      GimpData       *data;

      base_config = GIMP_BASE_CONFIG (factory->gimp->config);

      data = factory->data_new_func (name, base_config->stingy_memory_use);

      if (data)
        {
          gimp_container_add (factory->container, GIMP_OBJECT (data));
          g_object_unref (data);

          return data;
        }

      g_warning ("%s: factory->data_new_func() returned NULL", G_STRFUNC);
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

gboolean
gimp_data_factory_data_delete (GimpDataFactory  *factory,
                               GimpData         *data,
                               gboolean          delete_from_disk,
                               GError          **error)
{
  gboolean retval = TRUE;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (gimp_container_have (factory->container, GIMP_OBJECT (data)))
    {
      g_object_ref (data);

      gimp_container_remove (factory->container, GIMP_OBJECT (data));

      if (delete_from_disk && data->filename)
        retval = gimp_data_delete_from_disk (data, error);

      g_object_unref (data);
    }

  return retval;
}

GimpData *
gimp_data_factory_data_get_standard (GimpDataFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  if (factory->data_get_standard_func)
    return factory->data_get_standard_func ();

  return NULL;
}

gboolean
gimp_data_factory_data_save_single (GimpDataFactory *factory,
                                    GimpData        *data)
{
  GError *error = NULL;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);

  if (! data->dirty)
    return TRUE;

  if (! data->filename)
    {
      gchar *writable_dir;

      writable_dir = gimp_data_factory_get_save_dir (factory);

      if (! writable_dir)
        return FALSE;

      gimp_data_create_filename (data, writable_dir);

      g_free (writable_dir);
    }

  if (! data->writable)
    return FALSE;

  if (! gimp_data_save (data, &error))
    {
      /*  check if there actually was an error (no error
       *  means the data class does not implement save)
       */
      if (error)
        {
          g_message (_("Warning: Failed to save data:\n\n%s"),
                     error->message);
          g_clear_error (&error);
        }

      return FALSE;
    }

  return TRUE;
}


/*  private functions  */

static gchar *
gimp_data_factory_get_save_dir (GimpDataFactory *factory)
{
  gchar *path;
  gchar *writable_path;
  gchar *tmp;
  GList *path_list;
  GList *writable_list;
  GList *list;
  gchar *writable_dir = NULL;

  g_object_get (factory->gimp->config,
                factory->path_property_name,     &path,
                factory->writable_property_name, &writable_path,
                NULL);

  tmp = gimp_config_path_expand (path, TRUE, NULL);
  g_free (path);
  path = tmp;

  tmp = gimp_config_path_expand (writable_path, TRUE, NULL);
  g_free (writable_path);
  writable_path = tmp;

  path_list     = gimp_path_parse (path,          16, TRUE, NULL);
  writable_list = gimp_path_parse (writable_path, 16, TRUE, NULL);

  g_free (path);
  g_free (writable_path);

  for (list = writable_list; list; list = g_list_next (list))
    {
      GList *found;

      found = g_list_find_custom (path_list, list->data, (GCompareFunc) strcmp);

      if (found)
        {
          writable_dir = g_strdup (found->data);
          break;
        }
    }

  gimp_path_free (path_list);
  gimp_path_free (writable_list);

  return writable_dir;
}

static void
gimp_data_factory_load_data (const GimpDatafileData *file_data,
                             gpointer                user_data)
{
  GimpDataFactory *factory = GIMP_DATA_FACTORY (user_data);
  gint             i;

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
	  goto insert;
        }
    }

  return;

 insert:
  {
    GimpBaseConfig *base_config;
    GList          *data_list;
    GError         *error = NULL;

    base_config = GIMP_BASE_CONFIG (factory->gimp->config);

    data_list =
      (* factory->loader_entries[i].load_func) (file_data->filename,
                                                base_config->stingy_memory_use,
                                                &error);

    if (! data_list)
      {
	g_message (_("Warning: Failed to load data:\n\n%s"),
                   error->message);
        g_clear_error (&error);
      }
    else
      {
        GList    *writable_list;
        GList    *list;
        gboolean  writable;
        gboolean  deletable;

        writable_list = g_object_get_data (G_OBJECT (factory),
                                           WRITABLE_PATH_KEY);

        deletable = (g_list_length (data_list) == 1 &&
                     g_list_find_custom (writable_list, file_data->dirname,
                                         (GCompareFunc) strcmp) != NULL);

        writable = (deletable && factory->loader_entries[i].writable);

        for (list = data_list; list; list = g_list_next (list))
          {
            GimpData *data = list->data;

            gimp_data_set_filename (data, file_data->filename,
                                    writable, deletable);
            data->dirty = FALSE;

            gimp_container_add (factory->container, GIMP_OBJECT (data));
            g_object_unref (data);
          }

        g_list_free (data_list);
      }
  }
}
