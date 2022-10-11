/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2019 Jehan
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

#include <appstream-glib.h>
#include <archive.h>
#include <archive_entry.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <glib.h>
#include <zlib.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpbrush-load.h"
#include "core/gimpbrush-private.h"
#include "core/gimpdrawable.h"
#include "core/gimpextension.h"
#include "core/gimpextensionmanager.h"
#include "core/gimpextension-error.h"
#include "core/gimpimage.h"
#include "core/gimplayer-new.h"
#include "core/gimpparamspecs.h"
#include "core/gimptempbuf.h"
#include "core/gimp-utils.h"

#include "pdb/gimpprocedure.h"

#include "file-data-gex.h"

#include "gimp-intl.h"


/*  local function prototypes  */


typedef struct
{
  GInputStream  *input;
  void          *buffer;
} GexReadData;

static int        file_gex_open_callback  (struct archive  *a,
                                           void            *client_data);
static la_ssize_t file_gex_read_callback  (struct archive  *a,
                                           void            *client_data,
                                           const void     **buffer);
static int        file_gex_close_callback (struct archive  *a,
                                           void            *client_data);

static gboolean   file_gex_validate_path  (const gchar     *path,
                                           const gchar     *file_name,
                                           gboolean         first,
                                           gchar          **plugin_id,
                                           GError         **error);
static gboolean   file_gex_validate       (GFile           *file,
                                           AsApp          **appstream,
                                           GError         **error);
static gchar *    file_gex_decompress     (GFile           *file,
                                           gchar           *plugin_id,
                                           GError         **error);

static int
file_gex_open_callback (struct archive  *a,
                        void            *client_data)
{
  /* File already opened when we start with libarchive. */
  GexReadData *data = client_data;

  data->buffer = g_malloc0 (2048);

  return ARCHIVE_OK;
}

static la_ssize_t
file_gex_read_callback (struct archive  *a,
                        void            *client_data,
                        const void     **buffer)
{
  GexReadData *data  = client_data;
  GError      *error = NULL;
  gssize       read_count;

  read_count = g_input_stream_read (data->input, data->buffer, 2048, NULL, &error);

  if (read_count == -1)
    {
      archive_set_error (a, 0, "%s: %s", G_STRFUNC, error->message);
      g_clear_error (&error);

      return ARCHIVE_FATAL;
    }

  *buffer = data->buffer;

  return read_count;
}

static int
file_gex_close_callback (struct archive  *a,
                         void            *client_data)
{
  /* Input allocated outside, let's also unref it outside. */
  GexReadData *data = client_data;

  g_free (data->buffer);

  return ARCHIVE_OK;
}

static gboolean
file_gex_validate_path (const gchar  *path,
                        const gchar  *file_name,
                        gboolean      first,
                        gchar       **plugin_id,
                        GError      **error)
{
  gchar    *dirname = g_path_get_dirname (path);
  gboolean  valid   = TRUE;

  if (g_path_is_absolute (path) || g_strcmp0 (dirname, "/") == 0)
    {
      *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                            _("Absolute path are forbidden in GIMP extension '%s': %s"),
                            file_name, path);
      g_free (dirname);
      return FALSE;
    }

  if (g_strcmp0 (dirname, ".") == 0)
    {
      if (first)
        {
          *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                _("File not allowed in root of GIMP extension '%s': %s"),
                                file_name, path);
          valid = FALSE;
        }
      else
        {
          if (*plugin_id)
            {
              if (g_strcmp0 (path, *plugin_id) != 0)
                {
                  *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                        _("File not in GIMP extension '%s' folder id '%s': %s"),
                                        file_name, *plugin_id, path);
                  valid = FALSE;
                }
            }
          else
            {
              *plugin_id = g_strdup (path);
            }
          }
    }
  else
    {
      valid = file_gex_validate_path (dirname, file_name, FALSE, plugin_id, error);
    }

  g_free (dirname);

  return valid;
}

/**
 * file_gex_validate:
 * @file:
 * @appstream:
 * @error:
 *
 * Validate the extension file with the following tests:
 * - No absolute path allowed.
 * - All files must be in a single folder which determines the extension
 *   ID.
 * - This folder must contain the AppStream metadata file which must be
 *   valid AppStream XML format.
 * - The extension ID resulting from the AppStream parsing must
 *   correspond to the extension ID resulting from the top folder.
 *
 * Returns: TRUE on success and allocates @appstream, FALSE otherwise
 *          with @error set.
 */
static gboolean
file_gex_validate (GFile   *file,
                   AsApp  **appstream,
                   GError **error)
{
  GInputStream *input;
  gboolean      success = FALSE;

  g_return_val_if_fail (error != NULL && *error == NULL, FALSE);
  g_return_val_if_fail (appstream != NULL && *appstream == NULL, FALSE);

  input = G_INPUT_STREAM (g_file_read (file, NULL, error));

  if (input)
    {
      struct archive        *a;
      struct archive_entry *entry;
      int                   r;
      GexReadData           user_data;

      user_data.input = input;
      if ((a = archive_read_new ()))
        {
          archive_read_support_format_zip (a);

          r = archive_read_open (a, &user_data, file_gex_open_callback,
                                 file_gex_read_callback, file_gex_close_callback);
          if (r == ARCHIVE_OK)
            {
              gchar  *appdata_path = NULL;
              GBytes *appdata      = NULL;
              gchar  *plugin_id    = NULL;

              while (archive_read_next_header (a, &entry) == ARCHIVE_OK &&
                     file_gex_validate_path (archive_entry_pathname (entry),
                                             gimp_file_get_utf8_name (file),
                                             TRUE, &plugin_id, error))
                {
                  if (plugin_id && ! appdata_path)
                    appdata_path = g_strdup_printf ("%s/%s.metainfo.xml", plugin_id, plugin_id);

                  if (appdata_path)
                    {
                      if (g_strcmp0 (appdata_path, archive_entry_pathname (entry)) == 0)
                        {
                          const void      *buffer;
                          GString         *appstring = g_string_new ("");
                          la_int64_t       offset;
                          size_t           size;

                          while (TRUE)
                            {
                              r = archive_read_data_block (a, &buffer, &size, &offset);

                              if (r == ARCHIVE_FATAL)
                                {
                                  *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                                        _("Fatal error when uncompressing GIMP extension '%s': %s"),
                                                        gimp_file_get_utf8_name (file),
                                                        archive_error_string (a));
                                  g_string_free (appstring, TRUE);
                                  break;
                                }
                              else if (r == ARCHIVE_EOF)
                                {
                                  appdata = g_string_free_to_bytes (appstring);
                                  break;
                                }

                              appstring = g_string_append_len (appstring, (const gchar *) buffer, size);
                            }
                          continue;
                        }
                    }
                  archive_read_data_skip (a);
                }

              if (! (*error))
                {
                  if (appdata)
                    {
                      *appstream = as_app_new ();

                      if (! as_app_parse_data (*appstream, appdata,
                                               AS_APP_PARSE_FLAG_USE_HEURISTICS,
                                               error))
                        {
                          g_clear_object (appstream);
                        }
                      else if (g_strcmp0 (as_app_get_id (*appstream), plugin_id) != 0)
                        {
                          *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                                _("GIMP extension '%s' directory (%s) different from AppStream id: %s"),
                                                gimp_file_get_utf8_name (file),
                                                plugin_id, as_app_get_id (*appstream));
                          g_clear_object (appstream);
                        }
                    }
                  else
                    {
                      *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                            _("GIMP extension '%s' requires an AppStream file: %s"),
                                            gimp_file_get_utf8_name (file),
                                            appdata_path);
                    }
                }
              if (appdata_path)
                g_free (appdata_path);
              if (appdata)
                g_bytes_unref (appdata);
              if (plugin_id)
                g_free (plugin_id);
            }
          else
            {
              *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                    _("Invalid GIMP extension '%s': %s"),
                                    gimp_file_get_utf8_name (file),
                                    archive_error_string (a));
            }

          archive_read_close (a);
          archive_read_free (a);
          if (! *error)
            success = TRUE;
        }
      else
        {
          *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                "%s: archive_read_new() failed.", G_STRFUNC);
        }

      g_object_unref (input);
    }
  else
    {
      g_prefix_error (error, _("Could not open '%s' for reading: "),
                      gimp_file_get_utf8_name (file));
    }

  return success;
}

static gchar *
file_gex_decompress (GFile   *file,
                     gchar   *plugin_id,
                     GError **error)
{
  GInputStream *input;
  GFile        *ext_dir = gimp_directory_file ("extensions", NULL);
  gchar        *plugin_dir = NULL;
  gboolean      success = FALSE;

  g_return_val_if_fail (error != NULL && *error == NULL, FALSE);
  g_return_val_if_fail (plugin_id != NULL, FALSE);

  input = G_INPUT_STREAM (g_file_read (file, NULL, error));

  if (input)
    {
      struct archive       *a;
      struct archive       *ext;
      struct archive_entry *entry;
      int                   r;
      GexReadData           user_data;
      const void           *buffer;

      user_data.input = input;
      if ((a = archive_read_new ()))
        {
          archive_read_support_format_zip (a);

          ext = archive_write_disk_new ();
          archive_write_disk_set_options (ext,
                                          ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
                                          ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS |
                                          ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                                          ARCHIVE_EXTRACT_SECURE_SYMLINKS | ARCHIVE_EXTRACT_NO_OVERWRITE);
          archive_write_disk_set_standard_lookup (ext);

          r = archive_read_open (a, &user_data, file_gex_open_callback,
                                 file_gex_read_callback, file_gex_close_callback);
          if (r == ARCHIVE_OK)
            {
              while (archive_read_next_header (a, &entry) == ARCHIVE_OK &&
                     /* Re-validate just in case the archive got swapped
                      * between validation and decompression. */
                     file_gex_validate_path (archive_entry_pathname (entry),
                                             gimp_file_get_utf8_name (file),
                                             TRUE, &plugin_id, error))
                {
                  gchar       *path;
                  size_t       size;
                  la_int64_t   offset;

                  path = g_build_filename (g_file_get_path (ext_dir), archive_entry_pathname (entry), NULL);

                  archive_entry_set_pathname (entry, path);
                  g_free (path);

                  r = archive_write_header (ext, entry);
                  if (r < ARCHIVE_WARN)
                    {
                      *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                            _("Fatal error when uncompressing GIMP extension '%s': %s"),
                                            gimp_file_get_utf8_name (file),
                                            archive_error_string (ext));
                      break;
                    }

                  if (archive_entry_size (entry) > 0)
                    {
                      while (TRUE)
                        {
                          r = archive_read_data_block (a, &buffer, &size, &offset);
                          if (r == ARCHIVE_EOF)
                            {
                              break;
                            }
                          else if (r < ARCHIVE_WARN)
                            {
                              *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                                    _("Fatal error when uncompressing GIMP extension '%s': %s"),
                                                    gimp_file_get_utf8_name (file),
                                                    archive_error_string (a));
                              break;
                            }

                          r = archive_write_data_block (ext, buffer, size, offset);
                          if (r == ARCHIVE_WARN)
                            {
                              g_printerr (_("Warning when uncompressing GIMP extension '%s': %s\n"),
                                          gimp_file_get_utf8_name (file),
                                          archive_error_string (ext));
                              break;
                            }
                          else if (r < ARCHIVE_OK)
                            {
                              *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                                    _("Fatal error when uncompressing GIMP extension '%s': %s"),
                                                    gimp_file_get_utf8_name (file),
                                                    archive_error_string (ext));
                              break;
                            }
                        }
                    }
                  if (*error)
                    break;

                  r = archive_write_finish_entry (ext);
                  if (r < ARCHIVE_OK)
                    {
                      *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                            _("Fatal error when uncompressing GIMP extension '%s': %s"),
                                            gimp_file_get_utf8_name (file),
                                            archive_error_string (ext));
                      break;
                    }
                }
            }
          else
            {
              *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                    _("Invalid GIMP extension '%s': %s"),
                                    gimp_file_get_utf8_name (file),
                                    archive_error_string (a));
            }

          archive_read_close (a);
          archive_read_free (a);
          archive_write_close(ext);
          archive_write_free(ext);

          if (! *error)
            success = TRUE;
        }
      else
        {
          *error = g_error_new (GIMP_EXTENSION_ERROR, GIMP_EXTENSION_FAILED,
                                "%s: archive_read_new() failed.", G_STRFUNC);
        }

      g_object_unref (input);
    }
  else
    {
      g_prefix_error (error, _("Could not open '%s' for reading: "),
                      gimp_file_get_utf8_name (file));
    }

  if (success)
    plugin_dir = g_build_filename (g_file_get_path (ext_dir), plugin_id, NULL);

  g_object_unref (ext_dir);

  return plugin_dir;
}

/*  public functions  */

GimpValueArray *
file_gex_load_invoker (GimpProcedure         *procedure,
                       Gimp                  *gimp,
                       GimpContext           *context,
                       GimpProgress          *progress,
                       const GimpValueArray  *args,
                       GError               **error)
{
  GimpValueArray *return_vals;
  GFile          *file;
  gchar          *ext_dir = NULL;
  AsApp          *appdata = NULL;
  gboolean        success = FALSE;

  gimp_set_busy (gimp);

  file = g_value_get_object (gimp_value_array_index (args, 1));

  success = file_gex_validate (file, &appdata, error);
  if (success)
    ext_dir = file_gex_decompress (file, (gchar *) as_app_get_id (appdata),
                                   error);

  if (ext_dir)
    {
      GimpExtension *extension;
      GError        *rm_error = NULL;

      extension = gimp_extension_new (ext_dir, TRUE);
      success   = gimp_extension_manager_install (gimp->extension_manager,
                                                  extension, error);

      if (! success)
        {
          GFile *file;

          g_object_unref (extension);

          file = g_file_new_for_path (ext_dir);

          if (! gimp_file_delete_recursive (file, &rm_error))
            {
              g_warning ("%s: %s\n", G_STRFUNC, rm_error->message);
              g_error_free (rm_error);
            }

          g_object_unref (file);
        }

      g_free (ext_dir);
    }
  else
    {
      success = FALSE;
    }

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);
  gimp_unset_busy (gimp);

  return return_vals;
}
