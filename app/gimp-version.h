/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __APP_GIMP_VERSION_H__
#define __APP_GIMP_VERSION_H__


void       gimp_version_show         (gboolean be_verbose);
gchar    * gimp_version              (gboolean be_verbose,
                                      gboolean localized);

gint       gimp_version_get_revision (void);

gboolean   gimp_version_check_update (void);


#endif /* __APP_GIMP_VERSION_H__ */
