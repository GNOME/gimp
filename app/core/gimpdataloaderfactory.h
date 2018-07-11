/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdataloaderfactory.h
 * Copyright (C) 2001-2018 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_DATA_LOADER_FACTORY_H__
#define __GIMP_DATA_LOADER_FACTORY_H__


#include "gimpdatafactory.h"


typedef GList * (* GimpDataLoadFunc) (GimpContext   *context,
                                      GFile         *file,
                                      GInputStream  *input,
                                      GError       **error);


#define GIMP_TYPE_DATA_LOADER_FACTORY            (gimp_data_loader_factory_get_type ())
#define GIMP_DATA_LOADER_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DATA_LOADER_FACTORY, GimpDataLoaderFactory))
#define GIMP_DATA_LOADER_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DATA_LOADER_FACTORY, GimpDataLoaderFactoryClass))
#define GIMP_IS_DATA_LOADER_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DATA_LOADER_FACTORY))
#define GIMP_IS_DATA_LOADER_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DATA_LOADER_FACTORY))
#define GIMP_DATA_LOADER_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DATA_LOADER_FACTORY, GimpDataLoaderFactoryClass))


typedef struct _GimpDataLoaderFactoryPrivate GimpDataLoaderFactoryPrivate;
typedef struct _GimpDataLoaderFactoryClass   GimpDataLoaderFactoryClass;

struct _GimpDataLoaderFactory
{
  GimpDataFactory               parent_instance;

  GimpDataLoaderFactoryPrivate *priv;
};

struct _GimpDataLoaderFactoryClass
{
  GimpDataFactoryClass  parent_class;
};


GType             gimp_data_loader_factory_get_type     (void) G_GNUC_CONST;

GimpDataFactory * gimp_data_loader_factory_new          (Gimp                    *gimp,
                                                         GType                    data_type,
                                                         const gchar             *path_property_name,
                                                         const gchar             *writable_property_name,
                                                         const gchar             *ext_property_name,
                                                         GimpDataNewFunc          new_func,
                                                         GimpDataGetStandardFunc  get_standard_func);

void              gimp_data_loader_factory_add_loader   (GimpDataFactory         *factory,
                                                         const gchar             *name,
                                                         GimpDataLoadFunc         load_func,
                                                         const gchar             *extension,
                                                         gboolean                 writable);
void              gimp_data_loader_factory_add_fallback (GimpDataFactory         *factory,
                                                         const gchar             *name,
                                                         GimpDataLoadFunc         load_func);


#endif  /*  __GIMP_DATA_LOADER_FACTORY_H__  */
