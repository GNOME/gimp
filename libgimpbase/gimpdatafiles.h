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

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif

#ifndef __GIMP_DATAFILES_H__
#define __GIMP_DATAFILES_H__

#include <time.h>

G_BEGIN_DECLS


/**
 * GimpDatafileData:
 * @filename: the data file's full path.
 * @dirname:  the folder the data file is in.
 * @basename: the data file's basename.
 * @atime:    the last time the file was accessed for reading.
 * @mtime:    the last time the file was modified.
 * @ctime:    the time the file was created.
 *
 * This structure is passed to the #GimpDatafileLoaderFunc given to
 * gimp_datafiles_read_directories() for each file encountered in the
 * data path.
 **/
struct _GimpDatafileData
{
  const gchar *filename;
  const gchar *dirname;
  const gchar *basename;

  time_t       atime;
  time_t       mtime;
  time_t       ctime;
};


GIMP_DEPRECATED
gboolean   gimp_datafiles_check_extension  (const gchar            *filename,
                                            const gchar            *extension);

GIMP_DEPRECATED_FOR(GFileEnumerator)
void       gimp_datafiles_read_directories (const gchar            *path_str,
                                            GFileTest               flags,
                                            GimpDatafileLoaderFunc  loader_func,
                                            gpointer                user_data);


G_END_DECLS

#endif  /*  __GIMP_DATAFILES_H__ */
