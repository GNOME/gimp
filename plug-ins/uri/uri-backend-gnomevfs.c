/* GIMP - The GNU Image Manipulation Program
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

#ifdef HAVE_GNOMEUI
#include <libgnomeui/gnome-authentication-manager.h>
#endif
#ifdef HAVE_GNOME_KEYRING
#include <gnome-keyring.h>
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "uri-backend.h"

#include "libgimp/stdplugins-intl.h"


#define BUFSIZE 4096


/*  local function prototypes  */

static gchar    * get_protocols (void);
static gboolean   copy_uri      (const gchar  *src_uri,
                                 const gchar  *dest_uri,
                                 const gchar  *copying_format_str,
                                 const gchar  *copied_format_str,
                                 GError      **error);

#ifdef HAVE_GNOME_KEYRING
static void vfs_async_fill_authentication_callback (gconstpointer in,
                                                    size_t        in_size,
                                                    gpointer      out,
                                                    size_t        out_size,
                                                    gpointer      user_data,
                                                    GnomeVFSModuleCallbackResponse response,
                                                    gpointer      response_data);
static void vfs_fill_authentication_callback       (gconstpointer in,
                                                    size_t        in_size,
                                                    gpointer      out,
                                                    size_t        out_size,
                                                    gpointer      user_data);
#endif /* HAVE_GNOME_KEYRING */


/*  private variables  */

static gchar *supported_protocols = NULL;


/*  public functions  */

gboolean
uri_backend_init (const gchar  *plugin_name,
                  gboolean      run,
                  GimpRunMode   run_mode,
                  GError      **error)
{
  if (! gnome_vfs_init ())
    {
      g_set_error (error, 0, 0, "Could not initialize GnomeVFS");
      return FALSE;
    }

#ifdef HAVE_GNOMEUI
  if (run)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          gimp_ui_init (plugin_name, FALSE);
          gnome_authentication_manager_init ();
        }
      else
        {
#ifdef HAVE_GNOME_KEYRING
          gnome_vfs_async_module_callback_set_default
            (GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
             vfs_async_fill_authentication_callback,
             GINT_TO_POINTER (0),
             NULL);

          gnome_vfs_module_callback_set_default
            (GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
             vfs_fill_authentication_callback,
             GINT_TO_POINTER (0),
             NULL);
#endif /* HAVE_GNOME_KEYRING */
        }
    }
#endif /* HAVE_GNOMEUI */

  return TRUE;
}

void
uri_backend_shutdown (void)
{
  gnome_vfs_shutdown ();
}

const gchar *
uri_backend_get_load_help (void)
{
  return "Loads a file using the GnomeVFS library";
}

const gchar *
uri_backend_get_save_help (void)
{
  return "Saves a file using the GnomeVFS library";
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
                      _("Downloading %s of image data..."),
                      _("Downloaded %s of image data"),
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
                      _("Uploading %s of image data..."),
                      _("Uploaded %s of image data"),
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
  gchar            *memsize;
  GTimeVal          last_time = { 0, 0 };

  gimp_progress_init (_("Connecting to server"));

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
                   dest_uri, gnome_vfs_result_to_string (result));
      gnome_vfs_close (read_handle);
      return FALSE;
    }

  memsize = gimp_memsize_to_string (file_size);

  gimp_progress_init_printf (file_size > 0 ?
                             copying_format_str : copied_format_str,
                             memsize);

  g_free (memsize);

  while (TRUE)
    {
      GnomeVFSFileSize  chunk_read;
      GnomeVFSFileSize  chunk_written;
      GTimeVal          now;

      result = gnome_vfs_read (read_handle, buffer, sizeof (buffer),
                               &chunk_read);

      if (chunk_read == 0)
        {
          if (result != GNOME_VFS_ERROR_EOF)
            {
              memsize = gimp_memsize_to_string (sizeof (buffer));
              g_set_error (error, 0, 0,
                           _("Failed to read %s from '%s': %s"),
                           memsize, src_uri,
                           gnome_vfs_result_to_string (result));
              g_free (memsize);

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

      /*  update the progress only up to 10 times a second  */

      g_get_current_time (&now);

      if (((now.tv_sec - last_time.tv_sec) * 1000 +
           (now.tv_usec - last_time.tv_usec) / 1000) > 100)
        {
          if (file_size > 0)
            {
              gimp_progress_update ((gdouble) bytes_read / (gdouble) file_size);
            }
          else
            {
              memsize = gimp_memsize_to_string (bytes_read);

              gimp_progress_set_text_printf (copied_format_str, memsize);
              gimp_progress_pulse ();

              g_free (memsize);
            }

          last_time = now;
        }

      result = gnome_vfs_write (write_handle, buffer, chunk_read,
                                &chunk_written);

      if (chunk_written < chunk_read)
        {
          memsize = gimp_memsize_to_string (chunk_read);
          g_set_error (error, 0, 0,
                       _("Failed to write %s to '%s': %s"),
                       memsize, dest_uri,
                       gnome_vfs_result_to_string (result));
          g_free (memsize);

          gnome_vfs_close (read_handle);
          gnome_vfs_close (write_handle);
          return FALSE;
        }
    }

  gnome_vfs_close (read_handle);
  gnome_vfs_close (write_handle);

  return TRUE;
}

#ifdef HAVE_GNOME_KEYRING

/* gnome-keyring code copied from
 * libgnomeui/libgnomeui/gnome-authentication-manager.c CVS version 1.13
 */

typedef struct
{
  const GnomeVFSModuleCallbackFillAuthenticationIn *in_args;
  GnomeVFSModuleCallbackFillAuthenticationOut	   *out_args;

  GnomeVFSModuleCallbackResponse                    response;
  gpointer                                          response_data;
} FillCallbackInfo;

static void
fill_auth_callback (GnomeKeyringResult result,
		    GList             *list,
		    gpointer           data)
{
  FillCallbackInfo                *info = data;
  GnomeKeyringNetworkPasswordData *pwd_data;;

  if (result != GNOME_KEYRING_RESULT_OK || list == NULL)
    {
      info->out_args->valid = FALSE;
    }
  else
    {
      /* We use the first result, which is the least specific match */
      pwd_data = list->data;

      info->out_args->valid    = TRUE;
      info->out_args->username = g_strdup (pwd_data->user);
      info->out_args->domain   = g_strdup (pwd_data->domain);
      info->out_args->password = g_strdup (pwd_data->password);
    }

  info->response (info->response_data);
}

static void /* GnomeVFSAsyncModuleCallback */
vfs_async_fill_authentication_callback (gconstpointer in,
                                        size_t in_size,
					gpointer out,
                                        size_t out_size,
					gpointer user_data,
					GnomeVFSModuleCallbackResponse response,
					gpointer response_data)
{
  GnomeVFSModuleCallbackFillAuthenticationIn  *in_real;
  GnomeVFSModuleCallbackFillAuthenticationOut *out_real;
  gpointer                                     request;
  FillCallbackInfo                            *info;

  g_return_if_fail
    (sizeof (GnomeVFSModuleCallbackFillAuthenticationIn) == in_size &&
     sizeof (GnomeVFSModuleCallbackFillAuthenticationOut) == out_size);

  g_return_if_fail (in != NULL);
  g_return_if_fail (out != NULL);

  in_real  = (GnomeVFSModuleCallbackFillAuthenticationIn *)in;
  out_real = (GnomeVFSModuleCallbackFillAuthenticationOut *)out;

  info = g_new (FillCallbackInfo, 1);

  info->in_args       = in_real;
  info->out_args      = out_real;
  info->response      = response;
  info->response_data = response_data;

  request = gnome_keyring_find_network_password (in_real->username,
                                                 in_real->domain,
                                                 in_real->server,
                                                 in_real->object,
                                                 in_real->protocol,
                                                 in_real->authtype,
                                                 in_real->port,
                                                 fill_auth_callback,
                                                 info, g_free);
}

static void /* GnomeVFSModuleCallback */
vfs_fill_authentication_callback (gconstpointer in,
                                  size_t        in_size,
				  gpointer      out,
                                  size_t        out_size,
				  gpointer      user_data)
{
  GnomeVFSModuleCallbackFillAuthenticationIn  *in_real;
  GnomeVFSModuleCallbackFillAuthenticationOut *out_real;
  GnomeKeyringNetworkPasswordData             *pwd_data;
  GList                                       *list;
  GnomeKeyringResult                           result;

  g_return_if_fail
    (sizeof (GnomeVFSModuleCallbackFillAuthenticationIn) == in_size &&
     sizeof (GnomeVFSModuleCallbackFillAuthenticationOut) == out_size);

  g_return_if_fail (in != NULL);
  g_return_if_fail (out != NULL);

  in_real  = (GnomeVFSModuleCallbackFillAuthenticationIn *)in;
  out_real = (GnomeVFSModuleCallbackFillAuthenticationOut *)out;

  result = gnome_keyring_find_network_password_sync (in_real->username,
                                                     in_real->domain,
                                                     in_real->server,
                                                     in_real->object,
                                                     in_real->protocol,
                                                     in_real->authtype,
                                                     in_real->port,
                                                     &list);

  if (result != GNOME_KEYRING_RESULT_OK || list == NULL)
    {
      out_real->valid = FALSE;
    }
  else
    {
      /* We use the first result, which is the least specific match */
      pwd_data = list->data;

      out_real->valid    = TRUE;
      out_real->username = g_strdup (pwd_data->user);
      out_real->domain   = g_strdup (pwd_data->domain);
      out_real->password = g_strdup (pwd_data->password);

      gnome_keyring_network_password_list_free (list);
    }
}

#endif /* HAVE_GNOME_KEYRING */
