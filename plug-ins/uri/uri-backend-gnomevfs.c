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

static gchar    * get_protocols (void);
static gboolean   copy_uri      (const gchar  *src_uri,
                                 const gchar  *dest_uri,
                                 const gchar  *copying_format_str,
                                 const gchar  *copied_format_str,
                                 GError      **error);


/*  private variables  */

static gchar *supported_protocols = NULL;


/*  public functions  */

gboolean
uri_backend_init (GError **error)
{
  if (! gnome_vfs_init ())
    {
      g_set_error (error, 0, 0, "Could not initialize GnomeVFS");
      return FALSE;
    }

  return TRUE;
}

void
uri_backend_shutdown (void)
{
  gnome_vfs_shutdown ();
}

const gchar *
uri_backend_get_load_protocols (void)
{
  if (! supported_protocols)
    supported_protocols = get_protocols ();

  return supported_protocols;
}

const gchar *
uri_backend_get_save_protocols (void)
{
  if (! supported_protocols)
    supported_protocols = get_protocols ();

  return supported_protocols;
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
  success = copy_uri (uri, dest_uri,
                      _("Downloading %llu bytes of image data..."),
                      _("Downloaded %llu bytes of image data"),
                      error);
  g_free (dest_uri);

  return success;
}

gboolean
uri_backend_save_image (const gchar  *uri,
                        const gchar  *tmpname,
                        GimpRunMode   run_mode,
                        GError      **error)
{
  gchar    *src_uri;
  gboolean  success;

  src_uri = g_filename_to_uri (tmpname, NULL, NULL);
  success = copy_uri (src_uri, uri,
                      _("Uploading %llu bytes of image data..."),
                      _("Uploaded %llu bytes of image data"),
                      error);
  g_free (src_uri);

  return success;
}

/*  private functions  */

static gchar *
get_protocols (void)
{
  static const gchar *protocols[] =
  {
    "http:",
    "https:",
    "ftp:",
    "sftp:",
    "ssh:",
    "smb:",
    "dav:",
    "davs:"
  };

  GString *string = g_string_new (NULL);
  gint     i;

  for (i = 0; i < G_N_ELEMENTS (protocols); i++)
    {
      gchar       *uri;
      GnomeVFSURI *vfs_uri;

      uri = g_strdup_printf ("%s//foo/bar.xcf", protocols[i]);

      vfs_uri = gnome_vfs_uri_new (uri);

      if (vfs_uri)
        {
          if (string->len > 0)
            g_string_append_c (string, ',');

          g_string_append (string, protocols[i]);

          gnome_vfs_uri_unref (vfs_uri);
        }

      g_free (uri);
    }

  return g_string_free (string, FALSE);
}

static gboolean
copy_uri (const gchar  *src_uri,
          const gchar  *dest_uri,
          const gchar  *copying_format_str,
          const gchar  *copied_format_str,
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
                             GNOME_VFS_OPEN_WRITE, FALSE, 0644);

  if (result != GNOME_VFS_OK)
    {
      g_set_error (error, 0, 0,
                   _("Could not open '%s' for writing: %s"),
                   src_uri, gnome_vfs_result_to_string (result));
      gnome_vfs_close (read_handle);
      return FALSE;
    }

  if (file_size > 0)
    message = g_strdup_printf (copying_format_str, file_size);
  else
    message = g_strdup_printf (copied_format_str, (GnomeVFSFileSize) 0);

  gimp_progress_init (message);
  g_free (message);

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
          message = g_strdup_printf (copied_format_str, bytes_read);
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
