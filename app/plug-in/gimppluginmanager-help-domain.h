/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-help-domain.h
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

#ifndef __GIMP_PLUG_IN_MANAGER_HELP_DOMAIN_H__
#define __GIMP_PLUG_IN_MANAGER_HELP_DOMAIN_H__


void          gimp_plug_in_manager_help_domain_exit (GimpPlugInManager   *manager);

/* Add a help domain */
void          gimp_plug_in_manager_add_help_domain  (GimpPlugInManager   *manager,
                                                     GFile               *file,
                                                     const gchar         *domain_name,
                                                     const gchar         *domain_uri);

/* Retrieve a plug-ins help domain */
const gchar * gimp_plug_in_manager_get_help_domain  (GimpPlugInManager   *manager,
                                                     GFile               *file,
                                                     const gchar        **help_uri);

/* Retrieve all help domains */
gint          gimp_plug_in_manager_get_help_domains (GimpPlugInManager   *manager,
                                                     gchar             ***help_domains,
                                                     gchar             ***help_uris);


#endif /* __GIMP_PLUG_IN_MANAGER_HELP_DOMAIN_H__ */
