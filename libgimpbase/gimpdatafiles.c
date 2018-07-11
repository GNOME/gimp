/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Datafiles module copyight (C) 1996 Federico Mena Quintero
 * federico@nuclecu.unam.mx
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

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#include "gimpbasetypes.h"

#include "gimpdatafiles.h"
#include "gimpenv.h"


/**
 * SECTION: gimpdatafiles
 * @title: gimpdatafiles
 * @short_description: Functions to handle GIMP data files.
 *
 * Functions to handle GIMP data files.
 **/


static inline gboolean   is_script (const gchar *filename);


/*  public functions  */

gboolean
gimp_datafiles_check_extension (const gchar *filename,
                                const gchar *extension)
{
  gint name_len;
  gint ext_len;

  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (extension != NULL, FALSE);

  name_len = strlen (filename);
  ext_len  = strlen (extension);

  if (! (name_len && ext_len && (name_len > ext_len)))
    return FALSE;

  return (g_ascii_strcasecmp (&filename[name_len - ext_len], extension) == 0);
}

void
gimp_datafiles_read_directories (const gchar            *path_str,
                                 GFileTest               flags,
                                 GimpDatafileLoaderFunc  loader_func,
                                 gpointer                user_data)
{
  gchar *local_path;
  GList *path;
  GList *list;

  g_return_if_fail (path_str != NULL);
  g_return_if_fail (loader_func != NULL);

  local_path = g_strdup (path_str);

  path = gimp_path_parse (local_path, 256, TRUE, NULL);

  for (list = path; list; list = g_list_next (list))
    {
      const gchar *dirname = list->data;
      GDir        *dir;

      dir = g_dir_open (dirname, 0, NULL);

      if (dir)
        {
          const gchar *dir_ent;

          while ((dir_ent = g_dir_read_name (dir)))
            {
              gchar     *filename;
              GFile     *file;
              GFileInfo *info;

              filename = g_build_filename (dirname, dir_ent, NULL);
              file = g_file_new_for_path (filename);

              info = g_file_query_info (file,
                                        G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                        G_FILE_ATTRIBUTE_STANDARD_TYPE      ","
                                        G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE ","
                                        "time::*",
                                        G_FILE_QUERY_INFO_NONE,
                                        NULL, NULL);

              if (info)
                {
                  GimpDatafileData  file_data;
                  GFileType         file_type;

                  file_data.filename = filename;
                  file_data.dirname  = dirname;
                  file_data.basename = dir_ent;

                  file_data.atime =
                    g_file_info_get_attribute_uint64 (info,
                                                      G_FILE_ATTRIBUTE_TIME_ACCESS);

                  file_data.mtime =
                    g_file_info_get_attribute_uint64 (info,
                                                      G_FILE_ATTRIBUTE_TIME_MODIFIED);

                  file_data.ctime =
                    g_file_info_get_attribute_uint64 (info,
                                                      G_FILE_ATTRIBUTE_TIME_CREATED);

                  file_type = g_file_info_get_file_type (info);

                  if (g_file_info_get_is_hidden (info))
                    {
                      /* do nothing */
                    }
                  else if (flags & G_FILE_TEST_EXISTS)
                    {
                      (* loader_func) (&file_data, user_data);
                    }
                  else if ((flags & G_FILE_TEST_IS_REGULAR) &&
                           (file_type == G_FILE_TYPE_REGULAR))
                    {
                      (* loader_func) (&file_data, user_data);
                    }
                  else if ((flags & G_FILE_TEST_IS_DIR) &&
                           (file_type == G_FILE_TYPE_DIRECTORY))
                    {
                      (* loader_func) (&file_data, user_data);
                    }
                  else if ((flags & G_FILE_TEST_IS_SYMLINK) &&
                           (file_type == G_FILE_TYPE_SYMBOLIC_LINK))
                    {
                      (* loader_func) (&file_data, user_data);
                    }
                  else if ((flags & G_FILE_TEST_IS_EXECUTABLE) &&
                           (g_file_info_get_attribute_boolean (info,
                                                               G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE) ||
                            ((file_type == G_FILE_TYPE_REGULAR) &&
                             is_script (filename))))
                    {
                      (* loader_func) (&file_data, user_data);
                    }

                  g_object_unref (info);
                }

              g_object_unref (file);
              g_free (filename);
            }

          g_dir_close (dir);
        }
    }

  gimp_path_free (path);
  g_free (local_path);
}


/*  private functions  */

static inline gboolean
is_script (const gchar *filename)
{
#ifdef G_OS_WIN32
  /* On Windows there is no concept like the Unix executable flag.
   * There is a weak emulation provided by the MS C Runtime using file
   * extensions (com, exe, cmd, bat). This needs to be extended to treat
   * scripts (Python, Perl, ...) as executables, too. We use the PATHEXT
   * variable, which is also used by cmd.exe.
   */
  static gchar **exts = NULL;

  const gchar   *ext = strrchr (filename, '.');
  gchar         *pathext;
  gint           i;

  if (exts == NULL)
    {
      pathext = (gchar *) g_getenv ("PATHEXT");
      if (pathext != NULL)
        {
          exts = g_strsplit (pathext, G_SEARCHPATH_SEPARATOR_S, 100);
        }
      else
        {
          exts = g_new (gchar *, 1);
          exts[0] = NULL;
        }
    }

  i = 0;
  while (exts[i] != NULL)
    {
      if (g_ascii_strcasecmp (ext, exts[i]) == 0)
        return TRUE;
      i++;
    }
#endif /* G_OS_WIN32 */

  return FALSE;
}
