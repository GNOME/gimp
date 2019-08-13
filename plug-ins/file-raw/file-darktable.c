/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-darktable.c -- raw file format plug-in that uses darktable
 * Copyright (C) 2012 Simon Budig <simon@gimp.org>
 * Copyright (C) 2016 Tobias Ellinghaus <me@houz.org>
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

#include <glib/gstdio.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"

#include "file-raw-formats.h"
#include "file-raw-utils.h"


#define LOAD_THUMB_PROC   "file-darktable-load-thumb"
#define REGISTRY_KEY_BASE "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\darktable"


typedef struct _Darktable      Darktable;
typedef struct _DarktableClass DarktableClass;

struct _Darktable
{
  GimpPlugIn      parent_instance;
};

struct _DarktableClass
{
  GimpPlugInClass parent_class;
};


#define DARKTABLE_TYPE  (darktable_get_type ())
#define DARKTABLE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DARKTABLE_TYPE, Darktable))

GType                   darktable_get_type         (void) G_GNUC_CONST;

static GList          * darktable_init_procedures  (GimpPlugIn           *plug_in);
static GimpProcedure  * darktable_create_procedure (GimpPlugIn           *plug_in,
                                                    const gchar          *name);

static GimpValueArray * darktable_load             (GimpProcedure        *procedure,
                                                    GimpRunMode           run_mode,
                                                    GFile                *file,
                                                    const GimpValueArray *args,
                                                    gpointer              run_data);
static GimpValueArray * darktable_load_thumb       (GimpProcedure        *procedure,
                                                    const GimpValueArray *args,
                                                    gpointer              run_data);

static gint32           load_image                 (const gchar          *filename,
                                                    GimpRunMode           run_mode,
                                                    GError              **error);
static gint32           load_thumbnail_image       (const gchar          *filename,
                                                    gint                  thumb_size,
                                                    gint                 *width,
                                                    gint                 *height,
                                                    GError              **error);


G_DEFINE_TYPE (Darktable, darktable, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (DARKTABLE_TYPE)


static void
darktable_class_init (DarktableClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->init_procedures  = darktable_init_procedures;
  plug_in_class->create_procedure = darktable_create_procedure;
}

static void
darktable_init (Darktable *darktable)
{
}

static GList *
darktable_init_procedures (GimpPlugIn *plug_in)
{
  /* check if darktable is installed
   */
  gboolean  search_path      = FALSE;
  gchar    *exec_path        = file_raw_get_executable_path ("darktable", NULL,
                                                             "DARKTABLE_EXECUTABLE",
                                                             "org.darktable",
                                                             REGISTRY_KEY_BASE,
                                                             &search_path);
  gchar    *argv[]           = { exec_path, "--version", NULL };
  gchar    *darktable_stdout = NULL;
  gchar    *darktable_stderr = NULL;
  gboolean  have_darktable   = FALSE;
  GError   *error            = NULL;
  gint      i;

  /* allow the user to have some insight into why darktable may fail. */
  gboolean  debug_prints     = g_getenv ("DARKTABLE_DEBUG") != NULL;

  if (debug_prints)
    g_printf ("[%s] trying to call '%s'\n", __FILE__, exec_path);

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    NULL,
                    NULL,
                    &darktable_stdout,
                    &darktable_stderr,
                    NULL,
                    &error))
    {
      GRegex     *regex;
      GMatchInfo *matches;
      gint        major;
      gint        minor;

      /* A default darktable would apparently output something like
       * "this is darktable 2.2.5", but this version string is
       * customizable. In the official Fedora package for instance, I
       * encountered a "this is darktable darktable-2.2.5-4.fc27".
       * Therefore make the version recognition a bit more flexible.
       */
      regex = g_regex_new ("this is darktable [^0-9]*([0-9]+)\\.([0-9]+)\\.([0-9]+)",
                           0, 0, NULL);
      if (g_regex_match (regex, darktable_stdout, 0, &matches))
        {
          gchar *match;

          match = g_match_info_fetch (matches, 1);
          major = g_ascii_strtoll (match, NULL, 10);
          g_free (match);

          match = g_match_info_fetch (matches, 2);
          minor = g_ascii_strtoll (match, NULL, 10);
          g_free (match);

          if (((major == 1 && minor >= 7) || major >= 2))
            {
              if (g_strstr_len (darktable_stdout, -1,
                                "Lua support enabled"))
                {
                  have_darktable = TRUE;
                }
            }
        }

      g_match_info_free (matches);
      g_regex_unref (regex);
    }
  else if (debug_prints)
    {
      g_printf ("[%s] g_spawn_sync failed\n", __FILE__);
    }

  if (debug_prints)
    {
      if (error)
        g_printf ("[%s] error: %s\n", __FILE__, error->message);

      if (darktable_stdout && *darktable_stdout)
        g_printf ("[%s] stdout:\n%s\n", __FILE__, darktable_stdout);

      if (darktable_stderr && *darktable_stderr)
        g_printf ("[%s] stderr:\n%s\n", __FILE__, darktable_stderr);

      g_printf ("[%s] have_darktable: %d\n", __FILE__, have_darktable);
    }

  g_clear_error (&error);

  g_free (darktable_stdout);
  g_free (darktable_stderr);
  g_free (exec_path);

  if (have_darktable)
    {
      GList *list = NULL;

      list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));

      for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
        {
          const FileFormat *format = &file_formats[i];
          gchar            *load_proc;

          load_proc = g_strdup_printf (format->load_proc_format, "darktable");

          list = g_list_append (list, load_proc);
        }

      return list;
    }

  return NULL;
}

static GimpProcedure *
darktable_create_procedure (GimpPlugIn  *plug_in,
                            const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name, GIMP_PLUGIN,
                                      darktable_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Load thumbnail from a raw image "
                                        "via darktable",
                                        "This plug-in loads a thumbnail "
                                        "from a raw image by calling "
                                        "darktable-cli.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Tobias Ellinghaus",
                                      "Tobias Ellinghaus",
                                      "2016");

      gimp_procedure_add_argument (procedure,
                                   gimp_param_spec_string ("filename",
                                                           "Filename",
                                                           "Name of the file "
                                                           "to load",
                                                           FALSE, TRUE, FALSE,
                                                           NULL,
                                                           GIMP_PARAM_READWRITE));
      gimp_procedure_add_argument (procedure,
                                   g_param_spec_int ("thumb-size",
                                                     "Thumb Size",
                                                     "Preferred thumbnail size",
                                                     16, 2014, 256,
                                                     GIMP_PARAM_READWRITE));

      gimp_procedure_add_return_value (procedure,
                                       gimp_param_spec_image_id ("image",
                                                                 "Image",
                                                                 "Thumbnail image",
                                                                 FALSE,
                                                                 GIMP_PARAM_READWRITE));
      gimp_procedure_add_return_value (procedure,
                                       g_param_spec_int ("image-width",
                                                         "Image width",
                                                         "Width of the "
                                                         "full-sized image",
                                                         1, GIMP_MAX_IMAGE_SIZE, 1,
                                                         GIMP_PARAM_READWRITE));
      gimp_procedure_add_return_value (procedure,
                                       g_param_spec_int ("image-height",
                                                         "Image height",
                                                         "Height of the "
                                                         "full-sized image",
                                                         1, GIMP_MAX_IMAGE_SIZE, 1,
                                                         GIMP_PARAM_READWRITE));
    }
  else
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
        {
          const FileFormat *format = &file_formats[i];
          gchar            *load_proc;
          gchar            *load_blurb;
          gchar            *load_help;

          load_proc = g_strdup_printf (format->load_proc_format, "darktable");

          if (strcmp (name, load_proc))
            {
              g_free (load_proc);
              continue;
            }

          load_blurb = g_strdup_printf (format->load_blurb_format, "darktable");
          load_help  = g_strdup_printf (format->load_help_format,  "darktable");

          procedure = gimp_load_procedure_new (plug_in, name, GIMP_PLUGIN,
                                               darktable_load,
                                               (gpointer) format, NULL);

          gimp_procedure_set_documentation (procedure,
                                            load_blurb, load_help, name);
          gimp_procedure_set_attribution (procedure,
                                          "Tobias Ellinghaus",
                                          "Tobias Ellinghaus",
                                          "2016");

          gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                              format->extensions);
          gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                          format->magic);

          gimp_load_procedure_set_handles_raw (GIMP_LOAD_PROCEDURE (procedure),
                                               TRUE);
          gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                    LOAD_THUMB_PROC);

          g_free (load_proc);
          g_free (load_blurb);
          g_free (load_help);

          break;
        }
    }

  return procedure;
}

static GimpValueArray *
darktable_load (GimpProcedure        *procedure,
                GimpRunMode           run_mode,
                GFile                *file,
                const GimpValueArray *args,
                gpointer              run_data)
{
  GimpValueArray *return_vals;
  gchar          *filename;
  gint32          image_id;
  GError         *error = NULL;

  INIT_I18N ();

  filename = g_file_get_path (file);

  image_id = load_image (filename, run_mode, &error);

  g_free (filename);

  if (image_id < 1)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  gimp_value_set_image_id (gimp_value_array_index (return_vals, 1),
                           image_id);

  return return_vals;
}

static GimpValueArray *
darktable_load_thumb (GimpProcedure        *procedure,
                      const GimpValueArray *args,
                      gpointer              run_data)
{
  GimpValueArray *return_vals;
  const gchar    *filename;
  gint            width;
  gint            height;
  gint32          image_id;
  GValue          value = G_VALUE_INIT;
  GError         *error = NULL;

  INIT_I18N ();

  filename = g_value_get_string (gimp_value_array_index (args, 0));
  width    = g_value_get_int    (gimp_value_array_index (args, 1));
  height   = width;

  image_id = load_thumbnail_image (filename, width, &width, &height, &error);

  if (image_id < 1)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  gimp_value_set_image_id (gimp_value_array_index (return_vals, 1), image_id);
  g_value_set_int         (gimp_value_array_index (return_vals, 2), width);
  g_value_set_int         (gimp_value_array_index (return_vals, 3), height);

  g_value_init (&value, GIMP_TYPE_IMAGE_TYPE);
  g_value_set_enum (&value, GIMP_RGB_IMAGE);
  gimp_value_array_append (return_vals, &value);
  g_value_unset (&value);

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, 1);
  gimp_value_array_append (return_vals, &value);
  g_value_unset (&value);

  return return_vals;
}

static gint32
load_image (const gchar  *filename,
            GimpRunMode   run_mode,
            GError      **error)
{
  gint32  image_ID           = -1;
  GFile  *lua_file           = gimp_data_directory_file ("file-raw",
                                                         "file-darktable-export-on-exit.lua",
                                                         NULL);
  gchar  *lua_script         = g_file_get_path (lua_file);
  gchar  *lua_script_escaped = g_strescape (lua_script, "");
  gchar  *lua_quoted         = g_shell_quote (lua_script_escaped);
  gchar  *lua_cmd            = g_strdup_printf ("dofile(%s)", lua_quoted);
  gchar  *filename_out       = gimp_temp_name ("exr");
  gchar  *export_filename    = g_strdup_printf ("lua/export_on_exit/export_filename=%s",
                                                filename_out);

  gchar *darktable_stdout    = NULL;
  gchar *darktable_stderr    = NULL;

  /* allow the user to have some insight into why darktable may fail. */
  gboolean  debug_prints     = g_getenv ("DARKTABLE_DEBUG") != NULL;

  /* linear sRGB for now as GIMP uses that internally in many places anyway */
  gboolean  search_path      = FALSE;
  gchar    *exec_path        = file_raw_get_executable_path ("darktable", NULL,
                                                             "DARKTABLE_EXECUTABLE",
                                                             "org.darktable",
                                                             REGISTRY_KEY_BASE,
                                                             &search_path);
  gchar    *argv[] =
    {
      exec_path,
      "--library", ":memory:",
      "--luacmd",  lua_cmd,
      "--conf",    "plugins/lighttable/export/icctype=3",
      "--conf",    export_filename,
      (gchar *) filename,
      NULL
    };

  g_object_unref (lua_file);
  g_free (lua_script);
  g_free (lua_script_escaped);
  g_free (lua_quoted);

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (debug_prints)
    {
      g_printf ("[%s] trying to call\n", __FILE__);
      for (gchar **iter = argv; *iter; iter++)
        g_printf ("    %s\n", *iter);
      g_printf ("\n");
    }

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    /*G_SPAWN_STDOUT_TO_DEV_NULL |*/
                    /*G_SPAWN_STDERR_TO_DEV_NULL |*/
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    NULL,
                    NULL,
                    &darktable_stdout,
                    &darktable_stderr,
                    NULL,
                    error))
    {
      image_ID = gimp_file_load (run_mode, filename_out, filename_out);
      if (image_ID != -1)
        gimp_image_set_filename (image_ID, filename);
    }

  if (debug_prints)
    {
      if (darktable_stdout && *darktable_stdout)
        g_printf ("%s\n", darktable_stdout);

      if (darktable_stderr && *darktable_stderr)
        g_printf ("%s\n", darktable_stderr);
    }

  g_free (darktable_stdout);
  g_free (darktable_stderr);

  g_unlink (filename_out);
  g_free (lua_cmd);
  g_free (filename_out);
  g_free (export_filename);
  g_free (exec_path);

  gimp_progress_update (1.0);

  return image_ID;
}

static gint32
load_thumbnail_image (const gchar   *filename,
                      gint           thumb_size,
                      gint          *width,
                      gint          *height,
                      GError       **error)
{
  gint32  image_ID           = -1;
  gchar  *filename_out       = gimp_temp_name ("jpg");
  gchar  *size               = g_strdup_printf ("%d", thumb_size);
  GFile  *lua_file           = gimp_data_directory_file ("file-raw",
                                                         "file-darktable-get-size.lua",
                                                         NULL);
  gchar  *lua_script         = g_file_get_path (lua_file);
  gchar  *lua_script_escaped = g_strescape (lua_script, "");
  gchar  *lua_quoted         = g_shell_quote (lua_script_escaped);
  gchar  *lua_cmd            = g_strdup_printf ("dofile(%s)", lua_quoted);
  gchar  *darktable_stdout   = NULL;

  gboolean  search_path      = FALSE;
  gchar    *exec_path        = file_raw_get_executable_path ("darktable", "-cli",
                                                             "DARKTABLE_EXECUTABLE",
                                                             "org.darktable",
                                                             REGISTRY_KEY_BASE,
                                                             &search_path);
  gchar    *argv[] =
    {
      exec_path,
      (gchar *) filename, filename_out,
      "--width",          size,
      "--height",         size,
      "--hq",             "false",
      "--core",
      "--conf",           "plugins/lighttable/export/icctype=3",
      "--luacmd",         lua_cmd,
      NULL
    };

  g_object_unref (lua_file);
  g_free (lua_script);
  g_free (lua_script_escaped);
  g_free (lua_quoted);

  gimp_progress_init_printf (_("Opening thumbnail for '%s'"),
                             gimp_filename_to_utf8 (filename));

  *width = *height = thumb_size;

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    NULL,
                    NULL,
                    &darktable_stdout,
                    NULL,
                    NULL,
                    error))
    {
      gimp_progress_update (0.5);

      image_ID = gimp_file_load (GIMP_RUN_NONINTERACTIVE,
                                 filename_out,
                                 filename_out);
      if (image_ID != -1)
        {
          /* the size reported by raw files isn't precise,
           * but it should be close enough to get an idea.
           */
          gchar *start_of_size = g_strstr_len (darktable_stdout,
                                               -1,
                                               "[dt4gimp]");
          if (start_of_size)
            sscanf (start_of_size, "[dt4gimp] %d %d", width, height);

          /* is this needed for thumbnails? */
          gimp_image_set_filename (image_ID, filename);
        }
    }

  gimp_progress_update (1.0);

  g_unlink (filename_out);
  g_free (filename_out);
  g_free (size);
  g_free (lua_cmd);
  g_free (darktable_stdout);
  g_free (exec_path);

  return image_ID;
}
