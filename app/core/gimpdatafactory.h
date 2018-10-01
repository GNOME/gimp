/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactory.h
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

#ifndef __GIMP_DATA_FACTORY_H__
#define __GIMP_DATA_FACTORY_H__


#include "gimpobject.h"


typedef GimpData * (* GimpDataNewFunc)         (GimpContext     *context,
                                                const gchar     *name);
typedef GimpData * (* GimpDataGetStandardFunc) (GimpContext     *context);

typedef void       (* GimpDataForeachFunc)     (GimpDataFactory *factory,
                                                GimpData        *data,
                                                gpointer         user_data);


#define GIMP_TYPE_DATA_FACTORY            (gimp_data_factory_get_type ())
#define GIMP_DATA_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DATA_FACTORY, GimpDataFactory))
#define GIMP_DATA_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DATA_FACTORY, GimpDataFactoryClass))
#define GIMP_IS_DATA_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DATA_FACTORY))
#define GIMP_IS_DATA_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DATA_FACTORY))
#define GIMP_DATA_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DATA_FACTORY, GimpDataFactoryClass))


typedef struct _GimpDataFactoryPrivate GimpDataFactoryPrivate;
typedef struct _GimpDataFactoryClass   GimpDataFactoryClass;

struct _GimpDataFactory
{
  GimpObject              parent_instance;

  GimpDataFactoryPrivate *priv;
};

struct _GimpDataFactoryClass
{
  GimpObjectClass  parent_class;

  void           (* data_init)      (GimpDataFactory *factory,
                                     GimpContext     *context);
  void           (* data_refresh)   (GimpDataFactory *factory,
                                     GimpContext     *context);
  void           (* data_save)      (GimpDataFactory *factory);

  void           (* data_cancel)    (GimpDataFactory *factory);

  GimpData     * (* data_duplicate) (GimpDataFactory  *factory,
                                     GimpData         *data);
  gboolean       (* data_delete)    (GimpDataFactory  *factory,
                                     GimpData         *data,
                                     gboolean          delete_from_disk,
                                     GError          **error);
};


GType           gimp_data_factory_get_type          (void) G_GNUC_CONST;

void            gimp_data_factory_data_init         (GimpDataFactory  *factory,
                                                     GimpContext      *context,
                                                     gboolean          no_data);
void            gimp_data_factory_data_clean        (GimpDataFactory  *factory);
void            gimp_data_factory_data_refresh      (GimpDataFactory  *factory,
                                                     GimpContext      *context);
void            gimp_data_factory_data_save         (GimpDataFactory  *factory);
void            gimp_data_factory_data_free         (GimpDataFactory  *factory);

GimpAsyncSet  * gimp_data_factory_get_async_set     (GimpDataFactory  *factory);
gboolean        gimp_data_factory_data_wait         (GimpDataFactory  *factory);
void            gimp_data_factory_data_cancel       (GimpDataFactory  *factory);

gboolean        gimp_data_factory_has_data_new_func (GimpDataFactory  *factory);
GimpData      * gimp_data_factory_data_new          (GimpDataFactory  *factory,
                                                     GimpContext      *context,
                                                     const gchar      *name);
GimpData      * gimp_data_factory_data_get_standard (GimpDataFactory  *factory,
                                                     GimpContext      *context);

GimpData      * gimp_data_factory_data_duplicate    (GimpDataFactory  *factory,
                                                     GimpData         *data);
gboolean        gimp_data_factory_data_delete       (GimpDataFactory  *factory,
                                                     GimpData         *data,
                                                     gboolean          delete_from_disk,
                                                     GError          **error);

gboolean        gimp_data_factory_data_save_single  (GimpDataFactory  *factory,
                                                     GimpData         *data,
                                                     GError          **error);

void            gimp_data_factory_data_foreach      (GimpDataFactory  *factory,
                                                     gboolean          skip_internal,
                                                     GimpDataForeachFunc  callback,
                                                     gpointer          user_data);

Gimp          * gimp_data_factory_get_gimp          (GimpDataFactory  *factory);
GType           gimp_data_factory_get_data_type     (GimpDataFactory  *factory);
GimpContainer * gimp_data_factory_get_container     (GimpDataFactory  *factory);
GimpContainer * gimp_data_factory_get_container_obsolete
                                                    (GimpDataFactory  *factory);
GList         * gimp_data_factory_get_data_path     (GimpDataFactory  *factory);
GList         * gimp_data_factory_get_data_path_writable
                                                    (GimpDataFactory  *factory);
const GList   * gimp_data_factory_get_data_path_ext (GimpDataFactory  *factory);



#endif  /*  __GIMP_DATA_FACTORY_H__  */
