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


#define LOAD_PROC      "file-darktable-load"
#define PLUG_IN_BINARY "file-darktable"


typedef struct _FileFormat FileFormat;

struct _FileFormat
{
  const gchar *file_type;
  const gchar *mime_type;
  const gchar *extensions;
  const gchar *magic;

  const gchar *load_proc;
  const gchar *load_blurb;
  const gchar *load_help;
};


static void     query      (void);
static void     run        (const gchar      *name,
                            gint              nparams,
                            const GimpParam  *param,
                            gint             *nreturn_vals,
                            GimpParam       **return_vals);
static gint32   load_image (const gchar      *filename,
                            GimpRunMode       run_mode,
                            GError          **error);


static const FileFormat file_formats[] =
{
  {
    N_("Canon CR2 raw"),
    "image/x-canon-cr2",
    "cr2",
    "0,string,\\x49\\x49\\x2a\\x00\\x10\\x00\\x00\\x00\\x43\\x52",

    "file-cr2-load",
    "Load files in the CR2 raw format via darktable",
    "This plug-in loads files in Canon's raw CR2 format by calling darktable."
  },

  {
    N_("Nikon NEF raw"),
    " image/x-nikon-nef ",
    "nef",
    "",

    "file-nef-load",
    "Load files in the NEF raw format via darktable",
    "This plug-in loads files in Nikon's raw NEF format by calling darktable."
  }
};


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

  gint i;

  // TODO: check if darktable is installed
  gboolean have_darktable = TRUE;

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
      gimp_register_magic_load_handler (format->load_proc,
                                        format->extensions,
                                        "",
                                        format->magic);
    }
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
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
  gchar  *filename_out    = gimp_temp_name ("exr");
  gchar  *export_filename = g_strdup_printf ("lua/export_on_exit/export_filename=%s", filename_out);
  gchar  *lua_cmd         = g_strdup_printf ("dofile(%s)", lua_quoted);
  gchar  *filename_in     = g_strdup (filename);

  /* linear sRGB for now as GIMP uses that internally in many places anyway */
  gchar *argv[] =
    {
      "darktable",
      "--library", ":memory:",
      "--luacmd",  lua_cmd,
      "--conf",    "plugins/lighttable/export/iccprofile=linear_rec709_rgb",
      "--conf",    export_filename,
      filename_in,
      NULL
    };

  g_object_unref (lua_file);
  g_free (lua_script);

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
  g_free (lua_quoted);
  g_free (lua_cmd);
  g_free (filename_in);
  g_free (filename_out);
  g_free (export_filename);

  gimp_progress_update (1.0);

  return image_ID;
}
