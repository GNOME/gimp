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

#ifndef __HELP_H__
#define __HELP_H__


#define GIMP_HELP_DEFAULT_DOMAIN  "http://www.gimp.org/help"
#define GIMP_HELP_DEFAULT_ID      "gimp-main"
#define GIMP_HELP_DEFAULT_LOCALE  "en"

#define GIMP_HELP_PREFIX          "help"
#define GIMP_HELP_ENV_URI         "GIMP2_HELP_URI"

/*  #define GIMP_HELP_DEBUG  */


void  help_exit (void);


#endif /* ! __HELP_H__ */
