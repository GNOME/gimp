/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphelp.h
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_HELP_H__
#define __GIMP_HELP_H__


/*  the main help function
 *
 *  there should be no need to use it directly
 */
void       gimp_help_show (Gimp         *gimp,
                           GimpProgress *progress,
                           const gchar  *help_domain,
                           const gchar  *help_id);


/*  checks if the help browser is available
 */
gboolean   gimp_help_browser_is_installed     (Gimp *gimp);

/*  checks if the user manual is installed locally
 */
gboolean   gimp_help_user_manual_is_installed (Gimp *gimp);

/*  the configuration changed with respect to the location
 *  of the user manual, invalidate the cached information
 */
void       gimp_help_user_manual_changed      (Gimp *gimp);


GList    * gimp_help_get_installed_languages  (void);

#endif /* __GIMP_HELP_H__ */
