/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <libgnomevfs/gnome-vfs.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define BUFSIZE 1024


/*  local function prototypes  */

static gboolean   copy_uri (const gchar  *src_uri,
                            const gchar  *dest_uri,
                            GError      **error);


/*  public functions  */

const gchar *
uri_backend_get_load_protocols (void)
{
  return "http:,https:,ftp:,sftp:,ssh:,smb:,dav:,davs:";
}

gboolean
uri_backend_load_image (const gchar  *uri,
                        const gchar  *tmpname,
                        GimpRunMode   run_mode,
                        GError      **error)
{
  gchar    *dest_uri;
  gboolean  success;

  dest_uri = g_filename_to_uri (tmpname, NULL, NULL);
  success = copy_uri (uri, dest_uri, error);
  g_free (dest_uri);

  gnome_vfs_shutdown ();

  return success;
}


/*  private functions  */

static gboolean
copy_uri (const gchar  *src_uri,
          const gchar  *dest_uri,
          GError      **error)
{
  GnomeVFSHandle   *read_handle;
  GnomeVFSHandle   *write_handle;
  GnomeVFSFileInfo *src_info;
  GnomeVFSFileSize  file_size  = 0;
  GnomeVFSFileSize  bytes_read = 0;
  guchar            buffer[BUFSIZE];
  GnomeVFSResult    result;
  gchar            *message;

  if (! gnome_vfs_init ())
    {
      g_set_error (error, 0, 0, "Could not initialize GnomeVFS");
      return FALSE;
    }

  gimp_progress_init (_("Connecting to server..."));

  src_info = gnome_vfs_file_info_new ();
  result = gnome_vfs_get_file_info (src_uri, src_info, 0);

  /*  ignore errors here, they will be noticed below  */
  if (result == GNOME_VFS_OK &&
      (src_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SIZE))
    {
      file_size = src_info->size;
    }

  gnome_vfs_file_info_unref (src_info);

  result = gnome_vfs_open (&read_handle, src_uri, GNOME_VFS_OPEN_READ);

  if (result != GNOME_VFS_OK)
    {
      g_set_error (error, 0, 0,
                   _("Could not open '%s' for reading: %s"),
                   src_uri, gnome_vfs_result_to_string (result));
      return FALSE;
    }

  result = gnome_vfs_create (&write_handle, dest_uri,
                             GNOME_VFS_OPEN_WRITE, FALSE, 0777);

  if (result != GNOME_VFS_OK)
    {
      g_set_error (error, 0, 0,
                   _("Could not open '%s' for writing: %s"),
                   src_uri, gnome_vfs_result_to_string (result));
      gnome_vfs_close (read_handle);
      return FALSE;
    }

  if (file_size > 0)
    {
      message = g_strdup_printf (_("Downloading %llu bytes of image data..."),
                                 file_size);
      gimp_progress_init (message);
      g_free (message);
    }
  else
    {
      gimp_progress_init (_("Downloaded 0 bytes of image data"));
    }

  while (TRUE)
    {
      GnomeVFSFileSize chunk_read;
      GnomeVFSFileSize chunk_written;

      result = gnome_vfs_read (read_handle, buffer, sizeof (buffer),
                               &chunk_read);

      if (chunk_read == 0)
        {
          if (result != GNOME_VFS_ERROR_EOF)
            {
              g_set_error (error, 0, 0,
                           _("Failed to read %d bytes from '%s': %s"),
                           sizeof (buffer), src_uri,
                           gnome_vfs_result_to_string (result));
              gnome_vfs_close (read_handle);
              gnome_vfs_close (write_handle);
              return FALSE;
            }
          else
            {
              gimp_progress_update (1.0);
              break;
            }
        }

      bytes_read += chunk_read;

      if (file_size > 0)
        {
          gimp_progress_update ((gdouble) bytes_read / (gdouble) file_size);
        }
      else
        {
          message = g_strdup_printf (_("Downloaded %llu bytes of image data"),
                                     bytes_read);
          gimp_progress_init (message);
          g_free (message);
        }

      result = gnome_vfs_write (write_handle, buffer, chunk_read,
                                &chunk_written);

      if (chunk_written < chunk_read)
        {
          g_set_error (error, 0, 0,
                       _("Failed to write %llu bytes to '%s': %s"),
                       chunk_read, dest_uri,
                       gnome_vfs_result_to_string (result));
          gnome_vfs_close (read_handle);
          gnome_vfs_close (write_handle);
          return FALSE;
        }
    }

  gnome_vfs_close (read_handle);
  gnome_vfs_close (write_handle);

  return TRUE;
}

