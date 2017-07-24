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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "file-formats.h"

#ifdef GDK_WINDOWING_QUARTZ
#include <CoreServices/CoreServices.h>
#endif

#ifdef GDK_WINDOWING_WIN32
#include <Windows.h>
#endif

#define LOAD_THUMB_PROC "file-darktable-load-thumb"

static gchar   *get_executable_path  (const gchar      *suffix,
                                      gboolean         *search_path);

static void     init                 (void);
static void     query                (void);
static void     run                  (const gchar      *name,
                                      gint              nparams,
                                      const GimpParam  *param,
                                      gint             *nreturn_vals,
                                      GimpParam       **return_vals);
static gint32   load_image           (const gchar      *filename,
                                      GimpRunMode       run_mode,
                                      GError          **error);

static gint32   load_thumbnail_image (const gchar      *filename,
                                      gint             thumb_size,
                                      gint             *width,
                                      gint             *height,
                                      GError          **error);

const GimpPlugInInfo PLUG_IN_INFO =
{
  init,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query proc */
  run,   /* run_proc */
};

MAIN ()

static gchar *
get_executable_path (const gchar *suffix,
                     gboolean    *search_path)
{
  /*
   * First check for the environment variable DARKTABLE_EXECUTABLE.
   * Next do platform specific checks (bundle lookup on Mac, registry stuff
   * on Windows).
   * Last resort is hoping for darktable to be in PATH.
   */

  /*
   * Look for env variable. That can be set directly or via an environ file.
   * We assume that just appendign the suffix to that value will work.
   * That means that on Windows there should be no ".exe"!
   */
  const gchar *dt_env = g_getenv ("DARKTABLE_EXECUTABLE");
  if (dt_env)
    return g_strconcat (dt_env, suffix, NULL);

#if defined (GDK_WINDOWING_QUARTZ)
  {
    OSStatus status;
    CFURLRef bundle_url = NULL;

    /* For macOS, attempt searching for a darktable app bundle first. */
    status = LSFindApplicationForInfo (kLSUnknownCreator,
                                      CFSTR ("org.darktable"),
                                      NULL, NULL, &bundle_url);

    if (status >= 0)
      {
        CFBundleRef bundle;
        CFURLRef exec_url, absolute_url;
        CFStringRef path;
        gchar *ret;
        CFIndex len;

        bundle = CFBundleCreate (kCFAllocatorDefault, bundle_url);
        CFRelease (bundle_url);

        exec_url = CFBundleCopyExecutableURL (bundle);
        absolute_url = CFURLCopyAbsoluteURL (exec_url);
        path = CFURLCopyFileSystemPath (absolute_url, kCFURLPOSIXPathStyle);

        /* This gets us the length in UTF16 characters, we multiply by 2
        * to make sure we have a buffer big enough to fit the UTF8 string.
        */
        len = CFStringGetLength (path);
        ret = g_malloc0 (len * 2 * sizeof (gchar));
        if (!CFStringGetCString (path, ret, 2 * len * sizeof (gchar),
                                kCFStringEncodingUTF8))
          ret = NULL;

        CFRelease (path);
        CFRelease (absolute_url);
        CFRelease (exec_url);
        CFRelease (bundle);

        if (ret)
          return ret;
      }
    /* else, app bundle was not found, try path search as last resort. */
  }
#elif defined (GDK_WINDOWING_WIN32)
  {
    /* Look for darktable in the Windows registry. */

    char *registry_key;
    const char *registry_key_base = "SOFTWARE\\Microsoft\\Windows\\"
                                    "CurrentVersion\\App Paths\\darktable";
    char path[MAX_PATH];
    DWORD buffer_size = sizeof (path);
    long status;

    if (suffix)
      registry_key = g_strconcat (registry_key_base, suffix, ".exe", NULL);
    else
      registry_key = g_strconcat (registry_key_base, ".exe", NULL);

    status = RegGetValue (HKEY_LOCAL_MACHINE, registry_key, "", RRF_RT_ANY,
                          NULL, (PVOID)&path, &buffer_size);

    g_free (registry_key);

    if (status == ERROR_SUCCESS)
      return g_strdup (path);
  }
#endif

  /* Finally, the last resort. */
  *search_path = TRUE;
  if (suffix)
    return g_strconcat ("darktable", suffix, NULL);
  return g_strdup ("darktable");
}

static void
init (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load." },
    { GIMP_PDB_STRING, "raw-filename", "The name entered" },
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Output image" }
  };

  static const GimpParamDef thumb_args[] =
  {
    { GIMP_PDB_STRING, "filename",     "The name of the file to load"  },
    { GIMP_PDB_INT32,  "thumb-size",   "Preferred thumbnail size"      }
  };

  static const GimpParamDef thumb_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Thumbnail image"               },
    { GIMP_PDB_INT32,  "image-width",  "Width of full-sized image"     },
    { GIMP_PDB_INT32,  "image-height", "Height of full-sized image"    }
  };

  /* check if darktable is installed
   */
  gboolean  search_path      = FALSE;
  gchar    *exec_path        = get_executable_path (NULL, &search_path);
  gchar    *argv[]           = { exec_path, "--version", NULL };
  gchar    *darktable_stdout = NULL;
  gchar    *darktable_stderr = NULL;
  gboolean  have_darktable   = FALSE;
  GError   *error            = NULL;
  gint      i;

  printf ("[%s] trying to call '%s'\n", __FILE__, exec_path);

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
      gint major, minor, patch;

      if (sscanf (darktable_stdout,
                  "this is darktable %d.%d.%d",
                  &major, &minor, &patch) == 3)
        {
          if (((major == 1 && minor >= 7) || major >= 2))
            {
              if (g_strstr_len (darktable_stdout, -1,
                                "Lua support enabled"))
                {
                  have_darktable = TRUE;
                }
            }
        }
    }
  else
    printf ("[%s] g_spawn_sync failed\n", __FILE__);

  if (error)
    {
      printf ("[%s] error: %s\n", __FILE__, error->message);
      g_error_free (error);
    }
  if (darktable_stdout && *darktable_stdout)
    printf ("[%s] stdout:\n%s\n", __FILE__, darktable_stdout);
  if (darktable_stderr && *darktable_stderr)
    printf ("[%s] stderr:\n%s\n", __FILE__, darktable_stderr);
  printf ("[%s] have_darktable: %d\n", __FILE__, have_darktable);

  g_free (darktable_stdout);
  g_free (darktable_stderr);
  g_free (exec_path);

  if (! have_darktable)
    return;

  gimp_install_procedure (LOAD_THUMB_PROC,
                          "Load thumbnail from a raw image via darktable",
                          "This plug-in loads a thumbnail from a raw image by calling darktable-cli.",
                          "Tobias Ellinghaus",
                          "Tobias Ellinghaus",
                          "2016",
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (thumb_args),
                          G_N_ELEMENTS (thumb_return_vals),
                          thumb_args, thumb_return_vals);

  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];
      gchar            *load_proc;
      gchar            *load_blurb;
      gchar            *load_help;

      load_proc  = g_strdup_printf (format->load_proc_format,  "darktable");
      load_blurb = g_strdup_printf (format->load_blurb_format, "darktable");
      load_help  = g_strdup_printf (format->load_help_format,  "darktable");

      gimp_install_procedure (load_proc,
                              load_blurb,
                              load_help,
                              "Tobias Ellinghaus",
                              "Tobias Ellinghaus",
                              "2016",
                              format->file_type,
                              NULL,
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (load_args),
                              G_N_ELEMENTS (load_return_vals),
                              load_args, load_return_vals);

      gimp_register_file_handler_mime (load_proc,
                                       format->mime_type);
      gimp_register_file_handler_raw (load_proc);
      gimp_register_magic_load_handler (load_proc,
                                        format->extensions,
                                        "",
                                        format->magic);

      gimp_register_thumbnail_loader (load_proc, LOAD_THUMB_PROC);

      g_free (load_proc);
      g_free (load_blurb);
      g_free (load_help);
    }
}

static void
query (void)
{
  /* query() is run only the first time for efficiency. Yet this plugin
   * is dependent on the presence of darktable which may be installed
   * or uninstalled between GIMP startups. Therefore we should move the
   * usual gimp_install_procedure() to init() so that the check is done
   * at every startup instead.
   */
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[6];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  gint               image_ID;
  GError            *error = NULL;
  gint               i;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  /* check if the format passed is actually supported & load */
  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format    = &file_formats[i];
      gchar            *load_proc = NULL;

      if (format->load_proc_format)
        load_proc = g_strdup_printf (format->load_proc_format, "darktable");

      if (load_proc && ! strcmp (name, load_proc))
        {
          image_ID = load_image (param[1].data.d_string, run_mode, &error);

          if (image_ID != -1)
            {
              *nreturn_vals = 2;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }

          break;
        }
      else if (! strcmp (name, LOAD_THUMB_PROC))
        {
          gint width  = 0;
          gint height = 0;

          image_ID = load_thumbnail_image (param[0].data.d_string,
                                           param[1].data.d_int32,
                                           &width,
                                           &height,
                                           &error);

          if (image_ID != -1)
            {
              *nreturn_vals = 6;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
              values[2].type         = GIMP_PDB_INT32;
              values[2].data.d_int32 = width;
              values[3].type         = GIMP_PDB_INT32;
              values[3].data.d_int32 = height;
              values[4].type         = GIMP_PDB_INT32;
              values[4].data.d_int32 = GIMP_RGB_IMAGE;
              values[5].type         = GIMP_PDB_INT32;
              values[5].data.d_int32 = 1; /* num_layers */
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }

          break;
        }
    }

  if (i == G_N_ELEMENTS (file_formats))
    status = GIMP_PDB_CALLING_ERROR;

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type           = GIMP_PDB_STRING;
      values[1].data.d_string  = error->message;
    }

  values[0].data.d_status = status;
}

static gint32
load_image (const gchar  *filename,
            GimpRunMode   run_mode,
            GError      **error)
{
  gint32  image_ID        = -1;
  GFile  *lua_file        = gimp_data_directory_file ("file-raw",
                                                      "file-darktable-export-on-exit.lua",
                                                      NULL);
  gchar  *lua_script      = g_file_get_path (lua_file);
  gchar  *lua_quoted      = g_shell_quote (lua_script);
  gchar  *lua_cmd         = g_strdup_printf ("dofile(%s)", lua_quoted);
  gchar  *filename_out    = gimp_temp_name ("exr");
  gchar  *export_filename = g_strdup_printf ("lua/export_on_exit/export_filename=%s", filename_out);

  gchar *darktable_stdout = NULL;

  /* linear sRGB for now as GIMP uses that internally in many places anyway */
  gboolean  search_path      = FALSE;
  gchar    *exec_path        = get_executable_path (NULL, &search_path);
  gchar *argv[] =
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
  g_free (lua_quoted);

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    /*G_SPAWN_STDOUT_TO_DEV_NULL |*/
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    NULL,
                    NULL,
                    &darktable_stdout,
                    NULL,
                    NULL,
                    error))
    {
      image_ID = gimp_file_load (run_mode, filename_out, filename_out);
      if (image_ID != -1)
        gimp_image_set_filename (image_ID, filename);
    }

  /*if (darktable_stdout) printf ("%s\n", darktable_stdout);*/
  g_free(darktable_stdout);

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
  gint32  image_ID         = -1;
  gchar  *filename_out     = gimp_temp_name ("jpg");
  gchar  *size             = g_strdup_printf ("%d", thumb_size);
  GFile  *lua_file         = gimp_data_directory_file ("file-raw",
                                                       "file-darktable-get-size.lua",
                                                       NULL);
  gchar  *lua_script       = g_file_get_path (lua_file);
  gchar  *lua_quoted       = g_shell_quote (lua_script);
  gchar  *lua_cmd          = g_strdup_printf ("dofile(%s)", lua_quoted);
  gchar  *darktable_stdout = NULL;

  gboolean  search_path      = FALSE;
  gchar    *exec_path        = get_executable_path ("-cli", &search_path);
  gchar *argv[] =
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
