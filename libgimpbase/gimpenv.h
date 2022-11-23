/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaenv.h
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

#if !defined (__LIGMA_BASE_H_INSIDE__) && !defined (LIGMA_BASE_COMPILATION)
#error "Only <libligmabase/ligmabase.h> can be included directly."
#endif

#ifndef __LIGMA_ENV_H__
#define __LIGMA_ENV_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#ifdef G_OS_WIN32
#  ifdef __LIGMA_ENV_C__
#    define LIGMAVAR extern __declspec(dllexport)
#  else  /* !__LIGMA_ENV_C__ */
#    define LIGMAVAR extern __declspec(dllimport)
#  endif /* !__LIGMA_ENV_C__ */
#else  /* !G_OS_WIN32 */
#  define LIGMAVAR extern
#endif

LIGMAVAR const guint ligma_major_version;
LIGMAVAR const guint ligma_minor_version;
LIGMAVAR const guint ligma_micro_version;


const gchar * ligma_directory                   (void) G_GNUC_CONST;
const gchar * ligma_installation_directory      (void) G_GNUC_CONST;
const gchar * ligma_data_directory              (void) G_GNUC_CONST;
const gchar * ligma_locale_directory            (void) G_GNUC_CONST;
const gchar * ligma_sysconf_directory           (void) G_GNUC_CONST;
const gchar * ligma_plug_in_directory           (void) G_GNUC_CONST;
const gchar * ligma_cache_directory             (void) G_GNUC_CONST;
const gchar * ligma_temp_directory              (void) G_GNUC_CONST;

GFile       * ligma_directory_file              (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * ligma_installation_directory_file (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * ligma_data_directory_file         (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * ligma_locale_directory_file       (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * ligma_sysconf_directory_file      (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * ligma_plug_in_directory_file      (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;

GList       * ligma_path_parse                  (const gchar  *path,
                                                gint          max_paths,
                                                gboolean      check,
                                                GList       **check_failed);
gchar       * ligma_path_to_str                 (GList        *path) G_GNUC_MALLOC;
void          ligma_path_free                   (GList        *path);

gchar       * ligma_path_get_user_writable_dir  (GList        *path) G_GNUC_MALLOC;


/*  should be considered private, don't use!  */
void          ligma_env_init                    (gboolean      plug_in);


G_END_DECLS

#endif  /*  __LIGMA_ENV_H__  */
