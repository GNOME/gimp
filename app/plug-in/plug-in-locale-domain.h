/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-locale-domain.h
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

#ifndef __PLUG_IN_LOCALE_DOMAIN_H__
#define __PLUG_IN_LOCALE_DOMAIN_H__


void          plug_in_locale_domain_exit     (Gimp         *gimp);

/* Add a locale domain */
void          plug_in_locale_domain_add      (Gimp         *gimp,
                                              const gchar  *prog_name,
                                              const gchar  *domain_name,
                                              const gchar  *domain_path);

/* Retrieve a plug-ins locale domain */
const gchar * plug_in_locale_domain          (Gimp         *gimp,
                                              const gchar  *prog_name,
                                              const gchar **locale_path);

/* Retrieve the locale domain of the standard plug-ins */
const gchar * plug_in_standard_locale_domain (void) G_GNUC_CONST;


#endif /* __PLUG_IN_LOCALE_DOMAIN_H__ */
