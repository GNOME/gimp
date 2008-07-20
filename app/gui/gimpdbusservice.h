/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDBusService
 * Copyright (C) 2007 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_DBUS_SERVICE_H__
#define __GIMP_DBUS_SERVICE_H__

G_BEGIN_DECLS


#define GIMP_DBUS_SERVICE_NAME       "org.gimp.GIMP.UI"
#define GIMP_DBUS_SERVICE_PATH       "/org/gimp/GIMP/UI"
#define GIMP_DBUS_SERVICE_INTERFACE  "org.gimp.GIMP.UI"


#define GIMP_TYPE_DBUS_SERVICE            (gimp_dbus_service_get_type ())
#define GIMP_DBUS_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DBUS_SERVICE, GimpDBusService))
#define GIMP_DBUS_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DBUS_SERVICE, GimpDBusServiceClass))
#define GIMP_IS_DBUS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DBUS_SERVICE))
#define GIMP_IS_DBUS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DBUS_SERVICE))
#define GIMP_DBUS_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DBUS_SERVICE, GimpDBusServiceClass))


typedef struct _GimpDBusService      GimpDBusService;
typedef struct _GimpDBusServiceClass GimpDBusServiceClass;

struct _GimpDBusService
{
  GObject  parent_instance;

  Gimp    *gimp;
};

struct _GimpDBusServiceClass
{
  GObjectClass  parent_class;
};


GType     gimp_dbus_service_get_type    (void) G_GNUC_CONST;

GObject * gimp_dbus_service_new         (Gimp            *gimp);

gboolean  gimp_dbus_service_open        (GimpDBusService  *service,
                                         const gchar      *uri,
                                         gboolean         *success,
                                         GError          **dbus_error);
gboolean  gimp_dbus_service_open_as_new (GimpDBusService  *service,
                                         const gchar      *uri,
                                         gboolean         *success,
                                         GError          **dbus_error);
gboolean  gimp_dbus_service_activate    (GimpDBusService  *service,
                                         GError          **dbus_error);


G_END_DECLS

#endif /* __GIMP_DBUS_SERVICE_H__ */
