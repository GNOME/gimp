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
    { GIMP_PDB_IMAGE,  "image",        "Thumbnail image"               },
    { GIMP_PDB_INT32,  "image-width",  "Width of full-sized image"     },
    { GIMP_PDB_INT32,  "image-height", "Height of full-sized image"    }
  };

  /* check if darktable is installed
   * TODO: allow setting the location of the executable in preferences
   */
  gchar    *argv[]           = { "darktable", "--version", NULL };
  gchar    *darktable_stdout = NULL;
  gboolean  have_darktable   = FALSE;
  gint      i;

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    G_SPAWN_SEARCH_PATH,
                    NULL,
                    NULL,
                    &darktable_stdout,
                    NULL,
                    NULL,
                    NULL))
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

      g_free (darktable_stdout);
    }

  if (! have_darktable)
    return;

  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];

      gimp_install_procedure (format->load_proc,
                              format->load_blurb,
                              format->load_help,
                              "Tobias Ellinghaus",
                              "Tobias Ellinghaus",
                              "2016",
                              format->file_type,
                              NULL,
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (load_args),
                              G_N_ELEMENTS (load_return_vals),
                              load_args, load_return_vals);

      gimp_register_file_handler_mime (format->load_proc,
                                       format->mime_type);
      if (format->magic)
        gimp_register_magic_load_handler (format->load_proc,
                                          format->extensions,
                                          "",
                                          format->magic);
      else
        gimp_register_load_handler (format->load_proc,
                                    format->extensions,
                                    "");

      gimp_install_procedure (format->load_thumb_proc,
                              format->load_thumb_blurb,
                              format->load_thumb_help,
                              "Tobias Ellinghaus",
                              "Tobias Ellinghaus",
                              "2016",
                              NULL,
                              NULL,
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (thumb_args),
                              G_N_ELEMENTS (thumb_return_vals),
                              thumb_args, thumb_return_vals);

      gimp_register_thumbnail_loader (format->load_proc, format->load_thumb_proc);
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

  INIT_I18N ();

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
      else if (format->load_thumb_proc
               && ! strcmp (name, format->load_thumb_proc))
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
  GFile  *lua_file        = gimp_data_directory_file ("file-darktable",
                                                      "export-on-exit.lua",
                                                      NULL);
  gchar  *lua_script      = g_file_get_path (lua_file);
  gchar  *lua_quoted      = g_shell_quote (lua_script);
  gchar  *lua_cmd         = g_strdup_printf ("dofile(%s)", lua_quoted);
  gchar  *filename_out    = gimp_temp_name ("exr");
  gchar  *export_filename = g_strdup_printf ("lua/export_on_exit/export_filename=%s", filename_out);

  /* linear sRGB for now as GIMP uses that internally in many places anyway */
  gchar *argv[] =
    {
      "darktable",
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
                    G_SPAWN_STDOUT_TO_DEV_NULL |
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    G_SPAWN_SEARCH_PATH,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    error))
    {
      image_ID = gimp_file_load (run_mode, filename_out, filename_out);
      if (image_ID != -1)
        gimp_image_set_filename (image_ID, filename);
    }

  g_unlink (filename_out);
  g_free (lua_cmd);
  g_free (filename_out);
  g_free (export_filename);

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
  GFile  *lua_file         = gimp_data_directory_file ("file-darktable",
                                                       "get-size.lua",
                                                       NULL);
  gchar  *lua_script       = g_file_get_path (lua_file);
  gchar  *lua_quoted       = g_shell_quote (lua_script);
  gchar  *lua_cmd          = g_strdup_printf ("dofile(%s)", lua_quoted);
  gchar  *darktable_stdout = NULL;

  gchar *argv[] =
    {
      "darktable-cli",
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
                    G_SPAWN_SEARCH_PATH,
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

  return image_ID;
}
