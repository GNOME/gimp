/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

#pragma once


struct _GimpHelpDomain
{
  gchar      *help_domain;
  gchar      *help_uri;
  GHashTable *help_locales;
};


GimpHelpDomain * gimp_help_domain_new           (const gchar       *domain_name,
                                                 const gchar       *domain_uri);
void             gimp_help_domain_free          (GimpHelpDomain    *domain);

GimpHelpLocale * gimp_help_domain_lookup_locale (GimpHelpDomain    *domain,
                                                 const gchar       *locale_id,
                                                 GimpHelpProgress  *progress);
gchar          * gimp_help_domain_map           (GimpHelpDomain    *domain,
                                                 GList             *help_locales,
                                                 const gchar       *help_id,
                                                 GimpHelpProgress  *progress,
                                                 GimpHelpLocale   **locale,
                                                 gboolean          *fatal_error);
void             gimp_help_domain_exit          (void);
