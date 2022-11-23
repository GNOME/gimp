/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-darktable.c -- raw file format plug-in that uses darktable
 * Copyright (C) 2012 Simon Budig <simon@ligma.org>
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

#include <libligma/ligma.h>

#include "libligma/stdplugins-intl.h"

#include "file-raw-formats.h"
#include "file-raw-utils.h"


#define LOAD_THUMB_PROC   "file-darktable-load-thumb"
#define REGISTRY_KEY_BASE "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\darktable"


typedef struct _Darktable      Darktable;
typedef struct _DarktableClass DarktableClass;

struct _Darktable
{
  LigmaPlugIn      parent_instance;
};

struct _DarktableClass
{
  LigmaPlugInClass parent_class;
};


#define DARKTABLE_TYPE  (darktable_get_type ())
#define DARKTABLE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DARKTABLE_TYPE, Darktable))

GType                   darktable_get_type         (void) G_GNUC_CONST;

static GList          * darktable_init_procedures  (LigmaPlugIn           *plug_in);
static LigmaProcedure  * darktable_create_procedure (LigmaPlugIn           *plug_in,
                                                    const gchar          *name);

static LigmaValueArray * darktable_load             (LigmaProcedure        *procedure,
                                                    LigmaRunMode           run_mode,
                                                    GFile                *file,
                                                    const LigmaValueArray *args,
                                                    gpointer              run_data);
static LigmaValueArray * darktable_load_thumb       (LigmaProcedure        *procedure,
                                                    GFile                *file,
                                                    gint                  size,
                                                    const LigmaValueArray *args,
                                                    gpointer              run_data);

static LigmaImage      * load_image                 (GFile                *file,
                                                    LigmaRunMode           run_mode,
                                                    GError              **error);
static LigmaImage      * load_thumbnail_image       (GFile                *file,
                                                    gint                  thumb_size,
                                                    gint                 *width,
                                                    gint                 *height,
                                                    GError              **error);


G_DEFINE_TYPE (Darktable, darktable, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (DARKTABLE_TYPE)
DEFINE_STD_SET_I18N


static void
darktable_class_init (DarktableClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->init_procedures  = darktable_init_procedures;
  plug_in_class->create_procedure = darktable_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
darktable_init (Darktable *darktable)
{
}

static GList *
darktable_init_procedures (LigmaPlugIn *plug_in)
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

static LigmaProcedure *
darktable_create_procedure (LigmaPlugIn  *plug_in,
                            const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = ligma_thumbnail_procedure_new (plug_in, name,
                                                LIGMA_PDB_PROC_TYPE_PLUGIN,
                                                darktable_load_thumb, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Load thumbnail from a raw image "
                                        "via darktable",
                                        "This plug-in loads a thumbnail "
                                        "from a raw image by calling "
                                        "darktable-cli.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Tobias Ellinghaus",
                                      "Tobias Ellinghaus",
                                      "2016");
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

          procedure = ligma_load_procedure_new (plug_in, name,
                                               LIGMA_PDB_PROC_TYPE_PLUGIN,
                                               darktable_load,
                                               (gpointer) format, NULL);

          ligma_procedure_set_documentation (procedure,
                                            load_blurb, load_help, name);
          ligma_procedure_set_attribution (procedure,
                                          "Tobias Ellinghaus",
                                          "Tobias Ellinghaus",
                                          "2016");

          ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                              format->extensions);
          ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                          format->magic);

          ligma_load_procedure_set_handles_raw (LIGMA_LOAD_PROCEDURE (procedure),
                                               TRUE);
          ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                    LOAD_THUMB_PROC);

          g_free (load_proc);
          g_free (load_blurb);
          g_free (load_help);

          break;
        }
    }

  return procedure;
}

static LigmaValueArray *
darktable_load (LigmaProcedure        *procedure,
                LigmaRunMode           run_mode,
                GFile                *file,
                const LigmaValueArray *args,
                gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  image = load_image (file, run_mode, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
darktable_load_thumb (LigmaProcedure        *procedure,
                      GFile                *file,
                      gint                  size,
                      const LigmaValueArray *args,
                      gpointer              run_data)
{
  LigmaValueArray *return_vals;
  gint            width;
  gint            height;
  LigmaImage      *image = NULL;
  GError         *error = NULL;

  width  = size;
  height = size;

  image = load_thumbnail_image (file, width, &width, &height, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);
  LIGMA_VALUES_SET_INT   (return_vals, 2, width);
  LIGMA_VALUES_SET_INT   (return_vals, 3, height);
  LIGMA_VALUES_SET_ENUM  (return_vals, 4, LIGMA_RGB_IMAGE);
  LIGMA_VALUES_SET_INT   (return_vals, 5, 1);

  ligma_value_array_truncate (return_vals, 6);

  return return_vals;
}

static LigmaImage *
load_image (GFile        *file,
            LigmaRunMode   run_mode,
            GError      **error)
{
  LigmaImage *image              = NULL;
  GFile     *lua_file           = ligma_data_directory_file ("file-raw",
                                                            "file-darktable-export-on-exit.lua",
                                                            NULL);
  gchar     *lua_script_escaped = g_strescape (g_file_peek_path (lua_file), "");
  gchar     *lua_quoted         = g_shell_quote (lua_script_escaped);
  gchar     *lua_cmd            = g_strdup_printf ("dofile(%s)", lua_quoted);
  GFile     *file_out           = ligma_temp_file ("exr");
  gchar     *export_filename    = g_strdup_printf ("lua/export_on_exit/export_filename=%s",
                                                   g_file_peek_path (file_out));

  gchar *darktable_stdout    = NULL;
  gchar *darktable_stderr    = NULL;

  /* allow the user to have some insight into why darktable may fail. */
  gboolean  debug_prints     = g_getenv ("DARKTABLE_DEBUG") != NULL;

  /* linear sRGB for now as LIGMA uses that internally in many places anyway */
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
      (gchar *) g_file_peek_path (file),
      NULL
    };

  g_object_unref (lua_file);
  g_free (lua_script_escaped);
  g_free (lua_quoted);

  ligma_progress_init_printf (_("Opening '%s'"),
                             ligma_file_get_utf8_name (file));

  if (debug_prints)
    {
      gchar **environ_ = g_get_environ ();
      gint    i;

      g_printf ("[%s] trying to call\n", __FILE__);
      for (gchar **iter = argv; *iter; iter++)
        g_printf ("    %s\n", *iter);
      g_printf ("\n");

      g_printf ("## Environment ##\n");
      for (i = 0; environ_[i]; i++)
        g_printf ("- %s\n", environ_[i]);
      g_strfreev (environ_) ;
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
      image = ligma_file_load (run_mode, file_out);
      if (image)
        ligma_image_set_file (image, file);
    }

  if (debug_prints)
    {
      if (darktable_stdout && *darktable_stdout)
        g_printf ("\n## stdout ##\n%s\n", darktable_stdout);

      if (darktable_stderr && *darktable_stderr)
        g_printf ("\n## stderr ##\n%s\n", darktable_stderr);
    }

  g_free (darktable_stdout);
  g_free (darktable_stderr);

  g_file_delete (file_out, NULL, NULL);
  g_free (lua_cmd);
  g_free (export_filename);
  g_free (exec_path);

  ligma_progress_update (1.0);

  return image;
}

static LigmaImage *
load_thumbnail_image (GFile   *file,
                      gint     thumb_size,
                      gint    *width,
                      gint    *height,
                      GError **error)
{
  LigmaImage *image           = NULL;

  GFile  *file_out           = ligma_temp_file ("jpg");
  gchar  *size               = g_strdup_printf ("%d", thumb_size);
  GFile  *lua_file           = ligma_data_directory_file ("file-raw",
                                                         "file-darktable-get-size.lua",
                                                         NULL);
  gchar  *lua_script_escaped = g_strescape (g_file_peek_path (lua_file), "");
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
      (gchar *) g_file_peek_path (file),
      (gchar *) g_file_peek_path (file_out),
      "--width",          size,
      "--height",         size,
      "--hq",             "false",
      "--core",
      "--conf",           "plugins/lighttable/export/icctype=3",
      "--luacmd",         lua_cmd,
      NULL
    };

  g_object_unref (lua_file);
  g_free (lua_script_escaped);
  g_free (lua_quoted);

  ligma_progress_init_printf (_("Opening thumbnail for '%s'"),
                             ligma_file_get_utf8_name (file));

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
      ligma_progress_update (0.5);

      image = ligma_file_load (LIGMA_RUN_NONINTERACTIVE, file_out);
      if (image)
        {
          /* the size reported by raw files isn't precise,
           * but it should be close enough to get an idea.
           */
          gchar *start_of_size = g_strstr_len (darktable_stdout,
                                               -1,
                                               "[dt4ligma]");
          if (start_of_size)
            sscanf (start_of_size, "[dt4ligma] %d %d", width, height);

          /* is this needed for thumbnails? */
          ligma_image_set_file (image, file);
        }
    }

  ligma_progress_update (1.0);

  g_file_delete (file_out, NULL, NULL);
  g_free (size);
  g_free (lua_cmd);
  g_free (darktable_stdout);
  g_free (exec_path);

  return image;
}
