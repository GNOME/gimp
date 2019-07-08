/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-raw-placeholder.c -- raw file format plug-in that does nothing
 *                           except warning that there is no raw plug-in
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"

#include "file-raw-formats.h"


static void   query (void);
static void   run   (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals);


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

  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];
      gchar            *load_proc;
      gchar            *load_blurb;
      gchar            *load_help;

      load_proc  = g_strdup_printf (format->load_proc_format,  "raw-placeholder");
      load_blurb = g_strdup_printf (format->load_blurb_format, "raw-placeholder");
      load_help  = g_strdup_printf (format->load_help_format,  "raw-placeholder");

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

      g_free (load_proc);
      g_free (load_blurb);
      g_free (load_help);
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
  GimpPDBStatusType  status = GIMP_PDB_EXECUTION_ERROR;
  GError            *error  = NULL;
  gint               i;

  INIT_I18N ();

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
        load_proc = g_strdup_printf (format->load_proc_format, "raw-placeholder");

      if (load_proc && ! strcmp (name, load_proc))
        {
          g_set_error (&error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("There is no RAW loader installed to open '%s' files.\n"
                         "\n"
                         "GIMP currently supports these RAW loaders:\n"
                         "- darktable (http://www.darktable.org/), at least 1.7\n"
                         "- RawTherapee (http://rawtherapee.com/), at least 5.2\n"
                         "\n"
                         "Please install one of them in order to "
                         "load RAW files."),
                       gettext (format->file_type));
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
