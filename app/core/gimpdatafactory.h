/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadatafactory.h
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

#ifndef __LIGMA_DATA_FACTORY_H__
#define __LIGMA_DATA_FACTORY_H__


#include "ligmaobject.h"


typedef LigmaData * (* LigmaDataNewFunc)         (LigmaContext     *context,
                                                const gchar     *name);
typedef LigmaData * (* LigmaDataGetStandardFunc) (LigmaContext     *context);

typedef void       (* LigmaDataForeachFunc)     (LigmaDataFactory *factory,
                                                LigmaData        *data,
                                                gpointer         user_data);


#define LIGMA_TYPE_DATA_FACTORY            (ligma_data_factory_get_type ())
#define LIGMA_DATA_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DATA_FACTORY, LigmaDataFactory))
#define LIGMA_DATA_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DATA_FACTORY, LigmaDataFactoryClass))
#define LIGMA_IS_DATA_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DATA_FACTORY))
#define LIGMA_IS_DATA_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DATA_FACTORY))
#define LIGMA_DATA_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DATA_FACTORY, LigmaDataFactoryClass))


typedef struct _LigmaDataFactoryPrivate LigmaDataFactoryPrivate;
typedef struct _LigmaDataFactoryClass   LigmaDataFactoryClass;

struct _LigmaDataFactory
{
  LigmaObject              parent_instance;

  LigmaDataFactoryPrivate *priv;
};

struct _LigmaDataFactoryClass
{
  LigmaObjectClass  parent_class;

  void           (* data_init)      (LigmaDataFactory *factory,
                                     LigmaContext     *context);
  void           (* data_refresh)   (LigmaDataFactory *factory,
                                     LigmaContext     *context);
  void           (* data_save)      (LigmaDataFactory *factory);

  void           (* data_cancel)    (LigmaDataFactory *factory);

  LigmaData     * (* data_duplicate) (LigmaDataFactory  *factory,
                                     LigmaData         *data);
  gboolean       (* data_delete)    (LigmaDataFactory  *factory,
                                     LigmaData         *data,
                                     gboolean          delete_from_disk,
                                     GError          **error);
};


GType           ligma_data_factory_get_type          (void) G_GNUC_CONST;

void            ligma_data_factory_data_init         (LigmaDataFactory  *factory,
                                                     LigmaContext      *context,
                                                     gboolean          no_data);
void            ligma_data_factory_data_clean        (LigmaDataFactory  *factory);
void            ligma_data_factory_data_refresh      (LigmaDataFactory  *factory,
                                                     LigmaContext      *context);
void            ligma_data_factory_data_save         (LigmaDataFactory  *factory);
void            ligma_data_factory_data_free         (LigmaDataFactory  *factory);

LigmaAsyncSet  * ligma_data_factory_get_async_set     (LigmaDataFactory  *factory);
gboolean        ligma_data_factory_data_wait         (LigmaDataFactory  *factory);
void            ligma_data_factory_data_cancel       (LigmaDataFactory  *factory);

gboolean        ligma_data_factory_has_data_new_func (LigmaDataFactory  *factory);
LigmaData      * ligma_data_factory_data_new          (LigmaDataFactory  *factory,
                                                     LigmaContext      *context,
                                                     const gchar      *name);
LigmaData      * ligma_data_factory_data_get_standard (LigmaDataFactory  *factory,
                                                     LigmaContext      *context);

LigmaData      * ligma_data_factory_data_duplicate    (LigmaDataFactory  *factory,
                                                     LigmaData         *data);
gboolean        ligma_data_factory_data_delete       (LigmaDataFactory  *factory,
                                                     LigmaData         *data,
                                                     gboolean          delete_from_disk,
                                                     GError          **error);

gboolean        ligma_data_factory_data_save_single  (LigmaDataFactory  *factory,
                                                     LigmaData         *data,
                                                     GError          **error);

void            ligma_data_factory_data_foreach      (LigmaDataFactory  *factory,
                                                     gboolean          skip_internal,
                                                     LigmaDataForeachFunc  callback,
                                                     gpointer          user_data);

Ligma          * ligma_data_factory_get_ligma          (LigmaDataFactory  *factory);
GType           ligma_data_factory_get_data_type     (LigmaDataFactory  *factory);
LigmaContainer * ligma_data_factory_get_container     (LigmaDataFactory  *factory);
LigmaContainer * ligma_data_factory_get_container_obsolete
                                                    (LigmaDataFactory  *factory);
GList         * ligma_data_factory_get_data_path     (LigmaDataFactory  *factory);
GList         * ligma_data_factory_get_data_path_writable
                                                    (LigmaDataFactory  *factory);
const GList   * ligma_data_factory_get_data_path_ext (LigmaDataFactory  *factory);



#endif  /*  __LIGMA_DATA_FACTORY_H__  */
