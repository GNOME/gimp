/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-rawtherapee.c -- raw file format plug-in that uses RawTherapee
 * Copyright (C) 2012 Simon Budig <simon@gimp.org>
 * Copyright (C) 2016 Tobias Ellinghaus <me@houz.org>
 * Copyright (C) 2017 Alberto Griggio <alberto.griggio@gmail.com>
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


#define LOAD_THUMB_PROC "file-rawtherapee-load-thumb"
#define REGISTRY_KEY_BASE "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\rawtherapee"


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
                                      GError          **error);

const GimpPlugInInfo PLUG_IN_INFO =
{
  init,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query proc */
  run,   /* run_proc */
};

MAIN ()


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
    { GIMP_PDB_IMAGE,  "image",        "Thumbnail image"               }
  };

  /* check if rawtherapee is installed
   * TODO: allow setting the location of the executable in preferences
   */
  gboolean  search_path      = FALSE;
  gchar    *exec_path        = file_raw_get_executable_path ("rawtherapee", NULL,
                                                             "RAWTHERAPEE_EXECUTABLE",
                                                             "com.rawtherapee.rawtherapee",
                                                             REGISTRY_KEY_BASE,
                                                             &search_path);
#ifdef G_OS_WIN32
  /* Issue #2716 - Prevent RT from opening a console window */
  gchar    *argv[]             = { exec_path, "-v", "-w", NULL };
#else
  gchar    *argv[]             = { exec_path, "-v", NULL };
#endif
  gchar    *rawtherapee_stdout = NULL;
  gboolean  have_rawtherapee   = FALSE;
  gint      i;

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    (search_path ? G_SPAWN_SEARCH_PATH : 0) |
                    G_SPAWN_STDERR_TO_DEV_NULL,
                    NULL,
                    NULL,
                    &rawtherapee_stdout,
                    NULL,
                    NULL,
                    NULL))
    {
      gint rtmajor = 0;
      gint rtminor = 0;

      if (sscanf (rawtherapee_stdout,
                  "RawTherapee, version %d.%d",
                  &rtmajor, &rtminor) == 2 &&
          ((rtmajor == 5 && rtminor >= 2) || rtmajor >= 6))
        {
          have_rawtherapee = TRUE;
        }

      g_free (rawtherapee_stdout);
    }

  g_free (exec_path);

  if (! have_rawtherapee)
    return;

  gimp_install_procedure (LOAD_THUMB_PROC,
                          "Load thumbnail from a raw image via rawtherapee",
                          "This plug-in loads a thumbnail from a raw image by calling rawtherapee-cli.",
                          "Alberto Griggio",
                          "Alberto Griggio",
                          "2017",
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

      load_proc  = g_strdup_printf (format->load_proc_format,  "rawtherapee");
      load_blurb = g_strdup_printf (format->load_blurb_format, "rawtherapee");
      load_help  = g_strdup_printf (format->load_help_format,  "rawtherapee");

      gimp_install_procedure (load_proc,
                              load_blurb,
                              load_help,
                              "Alberto Griggio",
                              "Alberto Griggio",
                              "2017",
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
   * is dependent on the presence of rawtherapee which may be installed
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
        load_proc = g_strdup_printf (format->load_proc_format, "rawtherapee");

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
          image_ID = load_thumbnail_image (param[0].data.d_string,
                                           param[1].data.d_int32,
                                           &error);

          if (image_ID != -1)
            {
              *nreturn_vals = 4;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
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
  gint32    image_ID           = -1;
  gchar    *filename_out       = gimp_temp_name ("tif");
  gchar    *rawtherapee_stdout = NULL;

  gboolean  search_path        = FALSE;
  gchar    *exec_path          = file_raw_get_executable_path ("rawtherapee", NULL,
                                                               "RAWTHERAPEE_EXECUTABLE",
                                                               "com.rawtherapee.rawtherapee",
                                                               REGISTRY_KEY_BASE,
                                                               &search_path);

  /* linear sRGB for now as GIMP uses that internally in many places anyway */
  gchar *argv[] =
    {
      exec_path,
      "-gimp",
      (gchar *) filename,
      filename_out,
      NULL
    };

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
                    &rawtherapee_stdout,
                    NULL,
                    NULL,
                    error))
    {
      image_ID = gimp_file_load (run_mode, filename_out, filename_out);
      if (image_ID != -1)
        gimp_image_set_filename (image_ID, filename);
    }

  /*if (rawtherapee_stdout) printf ("%s\n", rawtherapee_stdout);*/
  g_free (rawtherapee_stdout);
  g_free (exec_path);

  g_unlink (filename_out);
  g_free (filename_out);

  gimp_progress_update (1.0);

  return image_ID;
}

static gint32
load_thumbnail_image (const gchar   *filename,
                      gint           thumb_size,
                      GError       **error)
{
  gint32  image_ID         = -1;
  gchar  *filename_out     = gimp_temp_name ("jpg");
  gchar  *thumb_pp3        = gimp_temp_name ("pp3");
  FILE   *thumb_pp3_f      = fopen (thumb_pp3, "w");
  gchar  *rawtherapee_stdout = NULL;
  const char *pp3_content =
    "[Version]\n"
    "AppVersion=5.0\n"
    "Version=326\n"
    "\n"
    "[Resize]\n"
    "Enabled=true\n"
    "AppliesTo=Cropped area\n"
    "Method=Lanczos\n"
    "Width=%d\n"
    "Height=%d\n"
    "\n"
    "[Sharpening]\n"
    "Enabled=false\n"
    "\n"
    "[SharpenEdge]\n"
    "Enabled=false\n"
    "\n"
    "[SharpenMicro]\n"
    "Enabled=false\n"
    "\n"
    "[Defringing]\n"
    "Enabled=false\n"
    "\n"
    "[Directional Pyramid Equalizer]\n"
    "Enabled=false\n"
    "\n"
    "[PostResizeSharpening]\n"
    "Enabled=false\n"
    "\n"
    "[Directional Pyramid Denoising]\n"
    "Enabled=false\n"
    "\n"
    "[Impulse Denoising]\n"
    "Enabled=false\n"
    "\n"
    "[Wavelet]\n"
    "Enabled=false\n"
    "\n"
    "[RAW Bayer]\n"
    "Method=fast\n"
    "\n"
    "[RAW X-Trans]\n"
    "Method=fast\n";


  gboolean  search_path = FALSE;
  gchar    *exec_path   = file_raw_get_executable_path ("rawtherapee", "-cli",
                                                        "RAWTHERAPEE_EXECUTABLE",
                                                        "com.rawtherapee.rawtherapee",
                                                        REGISTRY_KEY_BASE,
                                                        &search_path);
  gchar *argv[] =
    {
      exec_path,
      "-o", filename_out,
      "-d",
      "-s",
      "-j",
      "-p", thumb_pp3,
      "-f",
      "-c", (char *) filename,
      NULL
    };

  if (thumb_pp3_f)
    {
      if (fprintf (thumb_pp3_f, pp3_content, thumb_size, thumb_size) < 0)
        {
          fclose (thumb_pp3_f);
          thumb_pp3_f = NULL;
        }
    }

  gimp_progress_init_printf (_("Opening thumbnail for '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (thumb_pp3_f &&
      g_spawn_sync (NULL,
                    argv,
                    NULL,
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    NULL,
                    NULL,
                    &rawtherapee_stdout,
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
          /* is this needed for thumbnails? */
          gimp_image_set_filename (image_ID, filename);
        }
    }

  gimp_progress_update (1.0);

  if (thumb_pp3_f)
    fclose (thumb_pp3_f);

  g_unlink (thumb_pp3);
  g_free (filename_out);
  g_free (thumb_pp3);
  g_free (rawtherapee_stdout);
  g_free (exec_path);

  return image_ID;
}
