/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactory.h
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

#ifndef __GIMP_DATA_FACTORY_H__
#define __GIMP_DATA_FACTORY_H__


#include "gimpobject.h"


typedef GimpData * (* GimpDataNewFunc)         (const gchar *name);
typedef GimpData * (* GimpDataLoadFunc)        (const gchar *filename);
typedef GimpData * (* GimpDataGetStandardFunc) (void);


typedef struct _GimpDataFactoryLoaderEntry GimpDataFactoryLoaderEntry;

struct _GimpDataFactoryLoaderEntry
{
  GimpDataLoadFunc  load_func;
  const gchar      *extension;
};


#define GIMP_TYPE_DATA_FACTORY            (gimp_data_factory_get_type ())
#define GIMP_DATA_FACTORY(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_DATA_FACTORY, GimpDataFactory))
#define GIMP_DATA_FACTORY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DATA_FACTORY, GimpDataFactoryClass))
#define GIMP_IS_DATA_FACTORY(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DATA_FACTORY))
#define GIMP_IS_DATA_FACTORY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DATA_FACTORY))


typedef struct _GimpDataFactoryClass  GimpDataFactoryClass;

struct _GimpDataFactory
{
  GimpObject                         parent_instance;

  GimpContainer                     *container;

  const gchar                      **data_path;

  const GimpDataFactoryLoaderEntry  *loader_entries;
  gint                               n_loader_entries;

  GimpDataNewFunc                    data_new_func;
  GimpDataGetStandardFunc            data_get_standard_func;
};

struct _GimpDataFactoryClass
{
  GimpObjectClass  parent_class;
};


GtkType           gimp_data_factory_get_type (void);
GimpDataFactory * gimp_data_factory_new      (GtkType                            data_type,
					      const gchar                      **data_path,
					      const GimpDataFactoryLoaderEntry  *loader_entries,
					      gint                               n_loader_entries,
					      GimpDataNewFunc                    new_func,
					      GimpDataGetStandardFunc            standard_func);

void       gimp_data_factory_data_init         (GimpDataFactory *factory,
						gboolean         no_data);
void       gimp_data_factory_data_save         (GimpDataFactory *factory);
void       gimp_data_factory_data_free         (GimpDataFactory *factory);

GimpData * gimp_data_factory_data_new          (GimpDataFactory *factory,
						const gchar     *name);
GimpData * gimp_data_factory_data_duplicate    (GimpDataFactory *factory,
						GimpData        *data,
						const gchar     *name);
GimpData * gimp_data_factory_data_get_standard (GimpDataFactory *factory);


#endif  /*  __GIMP_DATA_FACTORY_H__  */
