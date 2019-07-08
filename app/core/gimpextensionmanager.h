/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpextensionmanager.h
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#ifndef __GIMP_EXTENSION_MANAGER_H__
#define __GIMP_EXTENSION_MANAGER_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_EXTENSION_MANAGER            (gimp_extension_manager_get_type ())
#define GIMP_EXTENSION_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_EXTENSION_MANAGER, GimpExtensionManager))
#define GIMP_EXTENSION_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EXTENSION_MANAGER, GimpExtensionManagerClass))
#define GIMP_IS_EXTENSION_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_EXTENSION_MANAGER))
#define GIMP_IS_EXTENSION_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EXTENSION_MANAGER))
#define GIMP_EXTENSION_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_EXTENSION_MANAGER, GimpExtensionManagerClass))


typedef struct _GimpExtensionManagerClass   GimpExtensionManagerClass;
typedef struct _GimpExtensionManagerPrivate GimpExtensionManagerPrivate;

struct _GimpExtensionManager
{
  GimpObject                   parent_instance;

  GimpExtensionManagerPrivate *p;
};

struct _GimpExtensionManagerClass
{
  GimpObjectClass      parent_class;

  void              (* extension_installed) (GimpExtensionManager *manager,
                                             GimpExtension        *extension,
                                             gboolean              is_system_ext);
  void              (* extension_removed)   (GimpExtensionManager *manager,
                                             gchar                *extension_id);
};


GType                  gimp_extension_manager_get_type              (void) G_GNUC_CONST;

GimpExtensionManager * gimp_extension_manager_new                   (Gimp                 *gimp);

void                   gimp_extension_manager_initialize            (GimpExtensionManager *manager);
void                   gimp_extension_manager_exit                  (GimpExtensionManager *manager);

const GList          * gimp_extension_manager_get_system_extensions (GimpExtensionManager *manager);
const GList          * gimp_extension_manager_get_user_extensions   (GimpExtensionManager *manager);

gboolean               gimp_extension_manager_is_running            (GimpExtensionManager *manager,
                                                                     GimpExtension        *extension);
gboolean               gimp_extension_manager_can_run               (GimpExtensionManager *manager,
                                                                     GimpExtension        *extension);
gboolean               gimp_extension_manager_is_removed            (GimpExtensionManager *manager,
                                                                     GimpExtension        *extension);

gboolean               gimp_extension_manager_install               (GimpExtensionManager *manager,
                                                                     GimpExtension        *extension,
                                                                     GError              **error);
gboolean               gimp_extension_manager_remove                (GimpExtensionManager *manager,
                                                                     GimpExtension        *extension,
                                                                     GError              **error);
gboolean               gimp_extension_manager_undo_remove           (GimpExtensionManager *manager,
                                                                     GimpExtension        *extension,
                                                                     GError              **error);

#endif  /* __GIMP_EXTENSION_MANAGER_H__ */
