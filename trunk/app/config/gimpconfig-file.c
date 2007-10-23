/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * File Utitility functions for GimpConfig.
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include <errno.h>
#include <sys/types.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "config-types.h"

#include "gimpconfig-file.h"

#include "gimp-intl.h"


gboolean
gimp_config_file_copy (const gchar  *source,
                       const gchar  *dest,
                       GError      **error)
{
  gchar        buffer[8192];
  FILE        *sfile;
  FILE        *dfile;
  struct stat  stat_buf;
  gint         nbytes;

  sfile = g_fopen (source, "rb");
  if (sfile == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (source), g_strerror (errno));
      return FALSE;
    }

  dfile = g_fopen (dest, "wb");
  if (dfile == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (dest), g_strerror (errno));
      fclose (sfile);
      return FALSE;
    }

  while ((nbytes = fread (buffer, 1, sizeof (buffer), sfile)) > 0)
    {
      if (fwrite (buffer, 1, nbytes, dfile) < nbytes)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Error writing '%s': %s"),
                       gimp_filename_to_utf8 (dest), g_strerror (errno));
          fclose (sfile);
          fclose (dfile);
          return FALSE;
        }
    }

  if (ferror (sfile))
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error reading '%s': %s"),
                   gimp_filename_to_utf8 (source), g_strerror (errno));
      fclose (sfile);
      fclose (dfile);
      return FALSE;
    }

  fclose (sfile);

  if (fclose (dfile) == EOF)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error writing '%s': %s"),
                   gimp_filename_to_utf8 (dest), g_strerror (errno));
      return FALSE;
    }

  if (g_stat (source, &stat_buf) == 0)
    {
      g_chmod (dest, stat_buf.st_mode);
    }

  return TRUE;
}

gboolean
gimp_config_file_backup_on_error (const gchar  *filename,
                                  const gchar  *name,
                                  GError      **error)
{
  gchar    *backup;
  gboolean  success;

  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  backup = g_strconcat (filename, "~", NULL);

  success = gimp_config_file_copy (filename, backup, error);

  if (success)
    g_message (_("There was an error parsing your '%s' file. "
                 "Default values will be used. A backup of your "
                 "configuration has been created at '%s'."),
               name, gimp_filename_to_utf8 (backup));

  g_free (backup);

  return success;
}
