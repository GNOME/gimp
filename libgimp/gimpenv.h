/* gimpenv.h
 *
 * Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMPENV_H__
#define __GIMPENV_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* gimp_directory() returns the user-specific GIMP settings
 * directory. If the environment variable GIMP_DIRECTORY exists, that
 * is used. If it is an absolute path, it is used as is.  If it is a
 * relative path, it is taken to be a subdirectory of the home
 * directory. If it is relative, and no home directory can be
 * determined, it is taken to be a subdirectory of
 * gimp_data_directory().
 *
 * The usual case is that no GIMP_DIRECTORY envvar exists, and then we
 * use the GIMPDIR subdirectory of the home directory. If no home
 * directory exists, we use a per-user subdirectory of
 * gimp_data_directory().
 *
 * In any case, we always return some non-empty string, whether it
 * corresponds to an existing directory or not.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 */
char *gimp_directory (void);

/* gimp_personal_rc_file() returns the name of a file in the
 * user-specific GIMP settings directory.
 *
 * The returned string is allocated dynamically and *SHOULD* be freed
 * with g_free() after use.
 */
char *gimp_personal_rc_file (char *basename);

/* gimp_data_directory returns the top directory for GIMP data.  If
 * the environment variable GIMP_DATADIR exists, that is used.  It
 * should be an absolute pathname.
 *
 * Otherwise, on Unix the compile-time defined directory is used.  On
 * Win32, the installation directory as deduced from the executable's
 * name is used.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 */
char *gimp_data_directory (void);

/* gimp_gtkrc returns the name of the GIMP's application-specific
 * gtkrc file.
 *
 * The returned string is allocated just once, and should *NOT* be
 * freed with g_free().
 */ 
char *gimp_gtkrc (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /*  __GIMPENV_H__  */
