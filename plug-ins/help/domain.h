/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2004 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

#ifndef __DOMAIN_H__
#define __DOMAIN_H__


typedef struct _HelpDomain HelpDomain;


void         domain_register (const gchar  *domain_name,
                              const gchar  *domain_uri,
                              const gchar  *domain_root);
HelpDomain * domain_lookup   (const gchar  *domain_name);

gchar      * domain_map      (HelpDomain   *domain,
                              GList        *help_locales,
                              const gchar  *help_id);
void         domain_exit     (void);


#endif /* ! __DOMAIN_H__ */
