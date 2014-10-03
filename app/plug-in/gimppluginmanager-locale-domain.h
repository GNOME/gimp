/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-locale-domain.h
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

#ifndef __GIMP_PLUG_IN_MANAGER_LOCALE_DOMAIN_H__
#define __GIMP_PLUG_IN_MANAGER_LOCALE_DOMAIN_H__


void          gimp_plug_in_manager_locale_domain_exit (GimpPlugInManager   *manager);

/* Add a locale domain */
void          gimp_plug_in_manager_add_locale_domain  (GimpPlugInManager   *manager,
                                                       GFile               *file,
                                                       const gchar         *domain_name,
                                                       const gchar         *domain_path);

/* Retrieve a plug-ins locale domain */
const gchar * gimp_plug_in_manager_get_locale_domain  (GimpPlugInManager   *manager,
                                                       GFile               *file,
                                                       const gchar        **locale_path);

/* Retrieve all locale domains */
gint          gimp_plug_in_manager_get_locale_domains (GimpPlugInManager   *manager,
                                                       gchar             ***locale_domains,
                                                       gchar             ***locale_paths);


#endif /* __GIMP_PLUG_IN_MANAGER_LOCALE_DOMAIN_H__ */
