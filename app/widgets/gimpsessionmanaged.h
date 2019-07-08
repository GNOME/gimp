/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessionmanaged.h
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#ifndef __GIMP_SESSION_MANAGED_H__
#define __GIMP_SESSION_MANAGED_H__


#define GIMP_TYPE_SESSION_MANAGED               (gimp_session_managed_get_type ())
#define GIMP_SESSION_MANAGED(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SESSION_MANAGED, GimpSessionManaged))
#define GIMP_IS_SESSION_MANAGED(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SESSION_MANAGED))
#define GIMP_SESSION_MANAGED_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_SESSION_MANAGED, GimpSessionManagedInterface))


typedef struct _GimpSessionManagedInterface GimpSessionManagedInterface;

struct _GimpSessionManagedInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  GList * (* get_aux_info) (GimpSessionManaged *session_managed);
  void    (* set_aux_info) (GimpSessionManaged *session_managed,
                            GList              *aux_info);
};


GType              gimp_session_managed_get_type     (void) G_GNUC_CONST;

GList            * gimp_session_managed_get_aux_info (GimpSessionManaged *session_managed);
void               gimp_session_managed_set_aux_info (GimpSessionManaged *session_managed,
                                                      GList              *aux_info);


#endif  /*  __GIMP_SESSION_MANAGED_H__  */
