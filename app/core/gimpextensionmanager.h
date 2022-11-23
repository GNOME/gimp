/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmaextensionmanager.h
 * Copyright (C) 2018 Jehan <jehan@ligma.org>
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

#ifndef __LIGMA_EXTENSION_MANAGER_H__
#define __LIGMA_EXTENSION_MANAGER_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_EXTENSION_MANAGER            (ligma_extension_manager_get_type ())
#define LIGMA_EXTENSION_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_EXTENSION_MANAGER, LigmaExtensionManager))
#define LIGMA_EXTENSION_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_EXTENSION_MANAGER, LigmaExtensionManagerClass))
#define LIGMA_IS_EXTENSION_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_EXTENSION_MANAGER))
#define LIGMA_IS_EXTENSION_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_EXTENSION_MANAGER))
#define LIGMA_EXTENSION_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_EXTENSION_MANAGER, LigmaExtensionManagerClass))


typedef struct _LigmaExtensionManagerClass   LigmaExtensionManagerClass;
typedef struct _LigmaExtensionManagerPrivate LigmaExtensionManagerPrivate;

struct _LigmaExtensionManager
{
  LigmaObject                   parent_instance;

  LigmaExtensionManagerPrivate *p;
};

struct _LigmaExtensionManagerClass
{
  LigmaObjectClass      parent_class;

  void              (* extension_installed) (LigmaExtensionManager *manager,
                                             LigmaExtension        *extension,
                                             gboolean              is_system_ext);
  void              (* extension_removed)   (LigmaExtensionManager *manager,
                                             gchar                *extension_id);
};


GType                  ligma_extension_manager_get_type              (void) G_GNUC_CONST;

LigmaExtensionManager * ligma_extension_manager_new                   (Ligma                 *ligma);

void                   ligma_extension_manager_initialize            (LigmaExtensionManager *manager);
void                   ligma_extension_manager_exit                  (LigmaExtensionManager *manager);

const GList          * ligma_extension_manager_get_system_extensions (LigmaExtensionManager *manager);
const GList          * ligma_extension_manager_get_user_extensions   (LigmaExtensionManager *manager);

gboolean               ligma_extension_manager_is_running            (LigmaExtensionManager *manager,
                                                                     LigmaExtension        *extension);
gboolean               ligma_extension_manager_can_run               (LigmaExtensionManager *manager,
                                                                     LigmaExtension        *extension);
gboolean               ligma_extension_manager_is_removed            (LigmaExtensionManager *manager,
                                                                     LigmaExtension        *extension);

gboolean               ligma_extension_manager_install               (LigmaExtensionManager *manager,
                                                                     LigmaExtension        *extension,
                                                                     GError              **error);
gboolean               ligma_extension_manager_remove                (LigmaExtensionManager *manager,
                                                                     LigmaExtension        *extension,
                                                                     GError              **error);
gboolean               ligma_extension_manager_undo_remove           (LigmaExtensionManager *manager,
                                                                     LigmaExtension        *extension,
                                                                     GError              **error);

#endif  /* __LIGMA_EXTENSION_MANAGER_H__ */
