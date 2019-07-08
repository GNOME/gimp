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

#ifndef __GIMP_USER_INSTALL_H__
#define __GIMP_USER_INSTALL_H__


typedef struct _GimpUserInstall GimpUserInstall;

typedef void  (* GimpUserInstallLogFunc) (const gchar *message,
                                          gboolean     error,
                                          gpointer     user_data);


GimpUserInstall * gimp_user_install_new  (GObject          *gimp,
                                          gboolean          verbose);
gboolean          gimp_user_install_run  (GimpUserInstall  *install);
void              gimp_user_install_free (GimpUserInstall  *install);

void   gimp_user_install_set_log_handler (GimpUserInstall        *install,
                                          GimpUserInstallLogFunc  log,
                                          gpointer                user_data);


#endif /* __USER_INSTALL_H__ */
