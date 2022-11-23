/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaDBusService
 * Copyright (C) 2007, 2008 Sven Neumann <sven@ligma.org>
 * Copyright (C) 2013       Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DBUS_SERVICE_H__
#define __LIGMA_DBUS_SERVICE_H__


#include "ligmadbusservice-generated.h"

/* service name and path should really be org.ligma.LIGMA and
 * /org/ligma/LIGMA and only the interface be called UI.
 */
#define LIGMA_DBUS_SERVICE_NAME   "org.ligma.LIGMA.UI"
#define LIGMA_DBUS_SERVICE_PATH   "/org/ligma/LIGMA/UI"
#define LIGMA_DBUS_INTERFACE_NAME "org.ligma.LIGMA.UI"
#define LIGMA_DBUS_INTERFACE_PATH "/org/ligma/LIGMA/UI"


#define LIGMA_TYPE_DBUS_SERVICE            (ligma_dbus_service_get_type ())
#define LIGMA_DBUS_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DBUS_SERVICE, LigmaDBusService))
#define LIGMA_DBUS_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DBUS_SERVICE, LigmaDBusServiceClass))
#define LIGMA_IS_DBUS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DBUS_SERVICE))
#define LIGMA_IS_DBUS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DBUS_SERVICE))
#define LIGMA_DBUS_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DBUS_SERVICE, LigmaDBusServiceClass))


typedef struct _LigmaDBusService      LigmaDBusService;
typedef struct _LigmaDBusServiceClass LigmaDBusServiceClass;

struct _LigmaDBusService
{
  LigmaDBusServiceUISkeleton  parent_instance;

  Ligma     *ligma;
  GQueue   *queue;
  GSource  *source;
  gboolean  timeout_source;
};

struct _LigmaDBusServiceClass
{
  LigmaDBusServiceUISkeletonClass  parent_class;
};


GType     ligma_dbus_service_get_type (void) G_GNUC_CONST;

GObject * ligma_dbus_service_new      (Ligma *ligma);


#endif /* __LIGMA_DBUS_SERVICE_H__ */
