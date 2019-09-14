/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * File Utitility functions for GimpConfig.
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include <errno.h>
#include <sys/types.h>

#include <gio/gio.h>
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
gimp_config_file_copy (const gchar         *source,
                       const gchar         *dest,
                       const gchar         *old_options_pattern,
                       GRegexEvalCallback   update_callback,
                       GError             **error)
{
  gchar        buffer[8192];
  FILE        *sfile;
  FILE        *dfile;
  GStatBuf     stat_buf;
  gint         nbytes;
  gint         unwritten_len = 0;
  GRegex      *old_options_regexp = NULL;

  if (old_options_pattern && update_callback)
    {
      old_options_regexp = g_regex_new (old_options_pattern, 0, 0, error);

      /* error set by g_regex_new. */
      if (! old_options_regexp)
        return FALSE;
    }

  sfile = g_fopen (source, "rb");
  if (sfile == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (source), g_strerror (errno));
      if (old_options_regexp)
        g_regex_unref (old_options_regexp);
      return FALSE;
    }

  dfile = g_fopen (dest, "wb");
  if (dfile == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (dest), g_strerror (errno));
      fclose (sfile);
      if (old_options_regexp)
        g_regex_unref (old_options_regexp);
      return FALSE;
    }

  while ((nbytes = fread (buffer + unwritten_len, 1,
                          sizeof (buffer) - unwritten_len, sfile)) > 0 || unwritten_len)
    {
      size_t read_len = nbytes + unwritten_len;
      size_t write_len;
      gchar* eol = NULL;
      gchar* write_bytes = NULL;

      if (old_options_regexp && update_callback)
        {
          eol = g_strrstr_len (buffer, read_len, "\n");
          if (eol)
            {
              *eol = '\0';
              read_len = strlen (buffer) + 1;
              *eol++ = '\n';
            }
          else if (! feof (sfile))
            {
              gchar format[256];

              /* We are in unlikely case where a single config line is
               * longer than the buffer!
               */

              g_snprintf (format, sizeof (format),
                          _("Error parsing '%%s': line longer than %s characters."),
                          G_GINT64_FORMAT);

              g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                           format,
                           gimp_filename_to_utf8 (source),
                           (gint64) sizeof (buffer));

              fclose (sfile);
              fclose (dfile);
              g_regex_unref (old_options_regexp);
              return FALSE;
            }

          write_bytes = g_regex_replace_eval (old_options_regexp, buffer,
                                              read_len, 0, 0, update_callback,
                                              NULL, error);
          if (write_bytes == NULL)
            {
              /* error already set. */
              fclose (sfile);
              fclose (dfile);
              g_regex_unref (old_options_regexp);
              return FALSE;
            }
          write_len = strlen (write_bytes);
        }
      else
        {
          write_bytes = buffer;
          write_len = read_len;
        }

      if (fwrite (write_bytes, 1, write_len, dfile) < write_len)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Error writing '%s': %s"),
                       gimp_filename_to_utf8 (dest), g_strerror (errno));
          if (old_options_regexp && update_callback)
            {
              g_free (write_bytes);
              g_regex_unref (old_options_regexp);
            }
          fclose (sfile);
          fclose (dfile);
          return FALSE;
        }

      if (old_options_regexp && update_callback)
        {
          g_free (write_bytes);

          if (eol)
            {
              unwritten_len = nbytes + unwritten_len - read_len;
              memmove (buffer, eol, unwritten_len);
            }
          else
            /* EOF */
            break;
        }
    }

  if (ferror (sfile))
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error reading '%s': %s"),
                   gimp_filename_to_utf8 (source), g_strerror (errno));
      fclose (sfile);
      fclose (dfile);
      if (old_options_regexp)
        g_regex_unref (old_options_regexp);
      return FALSE;
    }

  fclose (sfile);

  if (fclose (dfile) == EOF)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error writing '%s': %s"),
                   gimp_filename_to_utf8 (dest), g_strerror (errno));
      if (old_options_regexp)
        g_regex_unref (old_options_regexp);
      return FALSE;
    }

  if (g_stat (source, &stat_buf) == 0)
    {
      g_chmod (dest, stat_buf.st_mode);
    }

  if (old_options_regexp)
    g_regex_unref (old_options_regexp);
  return TRUE;
}

gboolean
gimp_config_file_backup_on_error (GFile        *file,
                                  const gchar  *name,
                                  GError      **error)
{
  gchar    *path;
  gchar    *backup;
  gboolean  success;

  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  path   = g_file_get_path (file);
  backup = g_strconcat (path, "~", NULL);

  success = gimp_config_file_copy (path, backup, NULL, NULL, error);

  if (success)
    g_message (_("There was an error parsing your '%s' file. "
                 "Default values will be used. A backup of your "
                 "configuration has been created at '%s'."),
               name, gimp_filename_to_utf8 (backup));

  g_free (backup);
  g_free (path);

  return success;
}
