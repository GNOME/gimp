/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadataloaderfactory.h
 * Copyright (C) 2001-2018 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DATA_LOADER_FACTORY_H__
#define __LIGMA_DATA_LOADER_FACTORY_H__


#include "ligmadatafactory.h"


typedef GList * (* LigmaDataLoadFunc) (LigmaContext   *context,
                                      GFile         *file,
                                      GInputStream  *input,
                                      GError       **error);


#define LIGMA_TYPE_DATA_LOADER_FACTORY            (ligma_data_loader_factory_get_type ())
#define LIGMA_DATA_LOADER_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DATA_LOADER_FACTORY, LigmaDataLoaderFactory))
#define LIGMA_DATA_LOADER_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DATA_LOADER_FACTORY, LigmaDataLoaderFactoryClass))
#define LIGMA_IS_DATA_LOADER_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DATA_LOADER_FACTORY))
#define LIGMA_IS_DATA_LOADER_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DATA_LOADER_FACTORY))
#define LIGMA_DATA_LOADER_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DATA_LOADER_FACTORY, LigmaDataLoaderFactoryClass))


typedef struct _LigmaDataLoaderFactoryPrivate LigmaDataLoaderFactoryPrivate;
typedef struct _LigmaDataLoaderFactoryClass   LigmaDataLoaderFactoryClass;

struct _LigmaDataLoaderFactory
{
  LigmaDataFactory               parent_instance;

  LigmaDataLoaderFactoryPrivate *priv;
};

struct _LigmaDataLoaderFactoryClass
{
  LigmaDataFactoryClass  parent_class;
};


GType             ligma_data_loader_factory_get_type     (void) G_GNUC_CONST;

LigmaDataFactory * ligma_data_loader_factory_new          (Ligma                    *ligma,
                                                         GType                    data_type,
                                                         const gchar             *path_property_name,
                                                         const gchar             *writable_property_name,
                                                         const gchar             *ext_property_name,
                                                         LigmaDataNewFunc          new_func,
                                                         LigmaDataGetStandardFunc  get_standard_func);

void              ligma_data_loader_factory_add_loader   (LigmaDataFactory         *factory,
                                                         const gchar             *name,
                                                         LigmaDataLoadFunc         load_func,
                                                         const gchar             *extension,
                                                         gboolean                 writable);
void              ligma_data_loader_factory_add_fallback (LigmaDataFactory         *factory,
                                                         const gchar             *name,
                                                         LigmaDataLoadFunc         load_func);


#endif  /*  __LIGMA_DATA_LOADER_FACTORY_H__  */
