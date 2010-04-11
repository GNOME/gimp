/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactory.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_DATA_FACTORY_H__
#define __GIMP_DATA_FACTORY_H__


#include "gimpobject.h"


typedef GimpData * (* GimpDataNewFunc)         (GimpContext  *context,
                                                const gchar  *name);
typedef GList    * (* GimpDataLoadFunc)        (GimpContext  *context,
                                                const gchar  *filename,
                                                GError      **error);
typedef GimpData * (* GimpDataGetStandardFunc) (GimpContext  *context);


typedef struct _GimpDataFactoryLoaderEntry GimpDataFactoryLoaderEntry;

struct _GimpDataFactoryLoaderEntry
{
  GimpDataLoadFunc  load_func;
  const gchar      *extension;
  gboolean          writable;
};


#define GIMP_TYPE_DATA_FACTORY            (gimp_data_factory_get_type ())
#define GIMP_DATA_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DATA_FACTORY, GimpDataFactory))
#define GIMP_DATA_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DATA_FACTORY, GimpDataFactoryClass))
#define GIMP_IS_DATA_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DATA_FACTORY))
#define GIMP_IS_DATA_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DATA_FACTORY))
#define GIMP_DATA_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DATA_FACTORY, GimpDataFactoryClass))


typedef struct _GimpDataFactoryClass  GimpDataFactoryClass;
typedef struct _GimpDataFactoryPriv   GimpDataFactoryPriv;

struct _GimpDataFactory
{
  GimpObject           parent_instance;

  GimpDataFactoryPriv *priv;
};

struct _GimpDataFactoryClass
{
  GimpObjectClass  parent_class;
};


GType             gimp_data_factory_get_type (void) G_GNUC_CONST;

GimpDataFactory * gimp_data_factory_new      (Gimp                             *gimp,
                                              GType                             data_type,
                                              const gchar                      *path_property_name,
                                              const gchar                      *writable_property_name,
                                              const GimpDataFactoryLoaderEntry *loader_entries,
                                              gint                              n_loader_entries,
                                              GimpDataNewFunc                   new_func,
                                              GimpDataGetStandardFunc           get_standard_func);

void            gimp_data_factory_data_init         (GimpDataFactory  *factory,
                                                     GimpContext      *context,
                                                     gboolean          no_data);
void            gimp_data_factory_data_refresh      (GimpDataFactory  *factory,
                                                     GimpContext      *context);
void            gimp_data_factory_data_save         (GimpDataFactory  *factory);
void            gimp_data_factory_data_free         (GimpDataFactory  *factory);

GimpData      * gimp_data_factory_data_new          (GimpDataFactory  *factory,
                                                     GimpContext      *context,
                                                     const gchar      *name);
GimpData      * gimp_data_factory_data_duplicate    (GimpDataFactory  *factory,
                                                     GimpData         *data);
gboolean        gimp_data_factory_data_delete       (GimpDataFactory  *factory,
                                                     GimpData         *data,
                                                     gboolean          delete_from_disk,
                                                     GError          **error);
GimpData      * gimp_data_factory_data_get_standard (GimpDataFactory  *factory,
                                                     GimpContext      *context);
gboolean        gimp_data_factory_data_save_single  (GimpDataFactory  *factory,
                                                     GimpData         *data,
                                                     GError          **error);
GimpContainer * gimp_data_factory_get_container     (GimpDataFactory  *factory);
GimpContainer * gimp_data_factory_get_container_obsolete
                                                    (GimpDataFactory  *factory);
Gimp          * gimp_data_factory_get_gimp          (GimpDataFactory  *factory);
gboolean        gimp_data_factory_has_data_new_func (GimpDataFactory  *factory);


#endif  /*  __GIMP_DATA_FACTORY_H__  */
