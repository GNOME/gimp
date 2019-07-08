/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/*
 * Desktop Entry Specification
 * http://standards.freedesktop.org/desktop-entry-spec/latest/
 */

#include "config.h"

#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-desktop-link-load"
#define PLUG_IN_BINARY "file-desktop-link"
#define PLUG_IN_ROLE   "gimp-file-desktop-link"


static void    query      (void);
static void    run        (const gchar      *name,
                           gint              nparams,
                           const GimpParam  *param,
                           gint             *nreturn_vals,
                           GimpParam       **return_vals);

static gint32  load_image (const gchar      *filename,
                           GimpRunMode       run_mode,
                           GError          **error);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name entered"             }
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Output image"                 }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Follows a link to an image in a .desktop file",
                          "Opens a .desktop file and if it is a link, it "
                          "asks GIMP to open the file the link points to.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          N_("Desktop Link"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_load_handler (LOAD_PROC, "desktop", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_EXECUTION_ERROR;
  GError            *error  = NULL;
  gint32             image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string, run_mode, &error);

      if (image_ID != -1)
        {
          status = GIMP_PDB_SUCCESS;

          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else if (error)
        {
          *nreturn_vals = 2;
          values[1].type          = GIMP_PDB_STRING;
          values[1].data.d_string = error->message;
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static gint32
load_image (const gchar  *filename,
            GimpRunMode   run_mode,
            GError      **load_error)
{
  GKeyFile *file     = g_key_file_new ();
  gchar    *group    = NULL;
  gchar    *value    = NULL;
  gint32    image_ID = -1;
  GError   *error    = NULL;

  if (! g_key_file_load_from_file (file, filename, G_KEY_FILE_NONE, &error))
    goto out;

  group = g_key_file_get_start_group (file);
  if (! group || strcmp (group, G_KEY_FILE_DESKTOP_GROUP) != 0)
    goto out;

  value = g_key_file_get_value (file,
                                group, G_KEY_FILE_DESKTOP_KEY_TYPE, &error);
  if (! value || strcmp (value, G_KEY_FILE_DESKTOP_TYPE_LINK) != 0)
    goto out;

  g_free (value);

  value = g_key_file_get_value (file,
                                group, G_KEY_FILE_DESKTOP_KEY_URL, &error);
  if (value)
    image_ID = gimp_file_load (run_mode, value, value);

 out:
  if (error)
    {
      g_set_error (load_error, error->domain, error->code,
                   _("Error loading desktop file '%s': %s"),
                   gimp_filename_to_utf8 (filename), error->message);
      g_error_free (error);
    }

  g_free (value);
  g_free (group);
  g_key_file_free (file);

  return image_ID;
}
