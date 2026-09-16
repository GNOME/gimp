/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenv.h
 * Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif

#ifndef __GIMP_ENV_H__
#define __GIMP_ENV_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#ifdef G_OS_WIN32
#  ifdef __GIMP_ENV_C__
#    define GIMPVAR extern __declspec(dllexport)
#  else  /* !__GIMP_ENV_C__ */
#    define GIMPVAR extern __declspec(dllimport)
#  endif /* !__GIMP_ENV_C__ */
#else  /* !G_OS_WIN32 */
#  define GIMPVAR extern
#endif

GIMPVAR const guint gimp_major_version;
GIMPVAR const guint gimp_minor_version;
GIMPVAR const guint gimp_micro_version;


const gchar * gimp_directory                   (void) G_GNUC_CONST;
const gchar * gimp_installation_directory      (void) G_GNUC_CONST;
const gchar * gimp_data_directory              (void) G_GNUC_CONST;
const gchar * gimp_locale_directory            (void) G_GNUC_CONST;
const gchar * gimp_sysconf_directory           (void) G_GNUC_CONST;
const gchar * gimp_plug_in_directory           (void) G_GNUC_CONST;
const gchar * gimp_cache_directory             (void) G_GNUC_CONST;
const gchar * gimp_temp_directory              (void) G_GNUC_CONST;

GFile       * gimp_directory_file              (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * gimp_installation_directory_file (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * gimp_data_directory_file         (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * gimp_locale_directory_file       (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * gimp_sysconf_directory_file      (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * gimp_plug_in_directory_file      (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;

GList       * gimp_parse_search_path           (const gchar  *path,
                                                gint          max_paths,
                                                gboolean      check,
                                                GList       **check_failed);
gchar       * gimp_create_search_path          (GList        *paths) G_GNUC_MALLOC;
void          gimp_free_paths                  (GList        *paths);
gchar       * gimp_get_user_writable_dir       (GList        *paths) G_GNUC_MALLOC;


/* Deprecated Functions */

GIMP_DEPRECATED_FOR(gimp_parse_search_path)
GList       * gimp_path_parse                  (const gchar  *path,
                                                gint          max_paths,
                                                gboolean      check,
                                                GList       **check_failed);
GIMP_DEPRECATED_FOR(gimp_create_search_path)
gchar       * gimp_path_to_str                 (GList        *path) G_GNUC_MALLOC;
GIMP_DEPRECATED_FOR(gimp_free_paths)
void          gimp_path_free                   (GList        *path);
GIMP_DEPRECATED_FOR(gimp_get_user_writable_dir)
gchar       * gimp_path_get_user_writable_dir  (GList        *path) G_GNUC_MALLOC;


G_END_DECLS

#endif  /*  __GIMP_ENV_H__  */
