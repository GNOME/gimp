/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Author: Josh MacDonald. */

#include "config.h"

#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "uri-backend.h"

#include "libgimp/stdplugins-intl.h"


static void     query      (void);
static void     run        (const gchar      *name,
                            gint              nparams,
                            const GimpParam  *param,
                            gint             *nreturn_vals,
                            GimpParam       **return_vals);

static gint32   load_image (const gchar      *uri,
                            GimpRunMode       run_mode);


GimpPlugInInfo PLUG_IN_INFO =
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
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename", "The name entered"             }
  };

  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  GError *error = NULL;

  if (! uri_backend_init (&error))
    {
      g_message (error->message);
      g_clear_error (&error);

      return;
    }

  if (uri_backend_get_load_protocols ())
    {
      gimp_install_procedure ("file_uri_load",
                              "loads files given an URI",
                              "You need to have GNU Wget installed.",
                              "Spencer Kimball & Peter Mattis",
                              "Spencer Kimball & Peter Mattis",
                              "1995-1997",
                              N_("URI"),
                              NULL,
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (load_args),
                              G_N_ELEMENTS (load_return_vals),
                              load_args, load_return_vals);

      gimp_plugin_icon_register ("file_uri_load",
                                 GIMP_ICON_TYPE_STOCK_ID, GIMP_STOCK_WEB);
      gimp_register_load_handler ("file_uri_load",
                                  "",
                                  uri_backend_get_load_protocols ());
    }

  uri_backend_shutdown ();
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
  gint32             image_ID;
  GError            *error = NULL;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  if (! uri_backend_init (&error))
    {
      g_message (error->message);
      g_clear_error (&error);

      return;
    }

  if (! strcmp (name, "file_uri_load") && uri_backend_get_load_protocols ())
    {
      image_ID = load_image (param[2].data.d_string, run_mode);

      if (image_ID != -1)
        {
          status = GIMP_PDB_SUCCESS;

	  *nreturn_vals = 2;
	  values[1].type         = GIMP_PDB_IMAGE;
	  values[1].data.d_image = image_ID;
	}
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  uri_backend_shutdown ();

  values[0].data.d_status = status;
}

static gint32
load_image (const gchar *uri,
            GimpRunMode  run_mode)
{
  gchar    *basename;
  gchar    *tmpname    = NULL;
  gint32    image_ID   = -1;
  gboolean  name_image = FALSE;
  GError   *error      = NULL;

  basename = g_path_get_basename (uri);

  if (basename)
    {
      gchar *ext = strchr (basename, '.');

      if (ext && strlen (ext))
        {
          tmpname = gimp_temp_name (ext + 1);
          name_image = TRUE;
        }

      g_free (basename);
    }

  if (! tmpname)
    tmpname = gimp_temp_name ("xxx");

  if (uri_backend_load_image (uri, tmpname, run_mode, &error))
    {
      image_ID = gimp_file_load (run_mode, tmpname, tmpname);

      if (image_ID != -1)
        {
          if (name_image)
            gimp_image_set_filename (image_ID, uri);
          else
            gimp_image_set_filename (image_ID, "");
        }
    }
  else if (error)
    {
      g_message ("%s", error->message);
      g_clear_error (&error);
    }

  unlink (tmpname);
  g_free (tmpname);

  return image_ID;
}
