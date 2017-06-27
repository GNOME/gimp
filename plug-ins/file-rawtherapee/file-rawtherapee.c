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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

//#include "libgimp/stdplugins-intl.h"

#include "file-formats.h"


#define LOAD_THUMB_PROC "file-rawtherapee-load-thumb"


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
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query proc */
  run,   /* run_proc */
};

MAIN ()


static void
query (void)
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
  gchar    *argv[]           = { "rawtherapee", "-v", NULL };
  gchar    *rawtherapee_stdout = NULL;
  gboolean  have_rawtherapee   = FALSE;
  gint      i;

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    G_SPAWN_SEARCH_PATH,
                    NULL,
                    NULL,
                    &rawtherapee_stdout,
                    NULL,
                    NULL,
                    NULL))
    {
      char *rtversion = NULL;

      if (sscanf (rawtherapee_stdout,
                  "RawTherapee, version %ms",
                  &rtversion) == 1)
        {
          have_rawtherapee = TRUE;
          free(rtversion);
        }

      g_free (rawtherapee_stdout);
    }

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

      gimp_install_procedure (format->load_proc,
                              format->load_blurb,
                              format->load_help,
                              "Alberto Griggio",
                              "Alberto Griggio",
                              "2017",
                              format->file_type,
                              NULL,
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (load_args),
                              G_N_ELEMENTS (load_return_vals),
                              load_args, load_return_vals);

      gimp_register_file_handler_mime (format->load_proc,
                                       format->mime_type);
      gimp_register_file_handler_raw (format->load_proc);
      gimp_register_magic_load_handler (format->load_proc,
                                        format->extensions,
                                        "",
                                        format->magic);

      gimp_register_thumbnail_loader (format->load_proc, LOAD_THUMB_PROC);
    }
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

//  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  /* check if the format passed is actually supported & load */
  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];

      if (format->load_proc && ! strcmp (name, format->load_proc))
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
  gint32  image_ID        = -1;
  gchar  *filename_out    = gimp_temp_name ("tif");

  gchar *rawtherapee_stdout = NULL;

  /* linear sRGB for now as GIMP uses that internally in many places anyway */
  gchar *argv[] =
    {
      "rawtherapee",
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
//                     G_SPAWN_STDOUT_TO_DEV_NULL |
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    G_SPAWN_SEARCH_PATH,
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

// if (rawtherapee_stdout) printf ("%s\n", rawtherapee_stdout);
  g_free(rawtherapee_stdout);

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
  gchar  *size             = g_strdup_printf ("%d", thumb_size);
  FILE   *thumb_pp3_f      = fopen(thumb_pp3, "w");
  gboolean pp3_ok          = FALSE;
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
 

  gchar *argv[] =
    {
      "rawtherapee-cli",
      "-o", filename_out,
      "-d",
      "-s",
      "-j",
      "-p", thumb_pp3,
      "-f",
      "-c", (char *) filename,
      NULL
    };

  if (thumb_pp3_f) {
    if (fprintf(thumb_pp3_f, pp3_content, thumb_size, thumb_size) < 0) {
      fclose(thumb_pp3_f);
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
                    G_SPAWN_SEARCH_PATH,
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

  if (thumb_pp3_f) {
    fclose(thumb_pp3_f);
  }
  g_unlink (thumb_pp3);
  g_free (filename_out);
  g_free (thumb_pp3);
  g_free (rawtherapee_stdout);

  return image_ID;
}
