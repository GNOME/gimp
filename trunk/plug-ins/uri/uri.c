/* GIMP - The GNU Image Manipulation Program
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
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "uri-backend.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-uri-load"
#define SAVE_PROC      "file-uri-save"
#define PLUG_IN_BINARY "uri"


static void                query         (void);
static void                run           (const gchar      *name,
                                          gint              nparams,
                                          const GimpParam  *param,
                                          gint             *nreturn_vals,
                                          GimpParam       **return_vals);

static gint32              load_image    (const gchar      *uri,
                                          GimpRunMode       run_mode);
static GimpPDBStatusType   save_image    (const gchar      *uri,
                                          gint32            image_ID,
                                          gint32            drawable_ID,
                                          gint32            run_mode);

static gchar             * get_temp_name (const gchar      *uri,
                                          gboolean         *name_image);
static gboolean            valid_file    (const gchar      *filename);


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
    { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name entered"             }
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" }
  };

  GError *error = NULL;

  if (! uri_backend_init (PLUG_IN_BINARY, FALSE, 0, &error))
    {
      g_message (error->message);
      g_clear_error (&error);

      return;
    }

  if (uri_backend_get_load_protocols ())
    {
      gimp_install_procedure (LOAD_PROC,
                              "loads files given an URI",
                              uri_backend_get_load_help (),
                              "Spencer Kimball & Peter Mattis",
                              "Spencer Kimball & Peter Mattis",
                              "1995-1997",
                              N_("URI"),
                              NULL,
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (load_args),
                              G_N_ELEMENTS (load_return_vals),
                              load_args, load_return_vals);

      gimp_plugin_icon_register (LOAD_PROC, GIMP_ICON_TYPE_STOCK_ID,
                                 (const guint8 *) GIMP_STOCK_WEB);
      gimp_register_load_handler (LOAD_PROC,
                                  "", uri_backend_get_load_protocols ());
    }

  if (uri_backend_get_save_protocols ())
    {
      gimp_install_procedure (SAVE_PROC,
                              "saves files given an URI",
                              uri_backend_get_save_help (),
                              "Michael Natterer",
                              "Michael Natterer",
                              "2005",
                              N_("URI"),
                              "RGB*, GRAY*, INDEXED*",
                              GIMP_PLUGIN,
                              G_N_ELEMENTS (save_args), 0,
                              save_args, NULL);

      gimp_plugin_icon_register (SAVE_PROC, GIMP_ICON_TYPE_STOCK_ID,
                                 (const guint8 *) GIMP_STOCK_WEB);
      gimp_register_save_handler (SAVE_PROC,
                                  "", uri_backend_get_save_protocols ());
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

  if (! uri_backend_init (PLUG_IN_BINARY, TRUE, run_mode, &error))
    {
      g_message (error->message);
      g_clear_error (&error);

      return;
    }

  if (! strcmp (name, LOAD_PROC) && uri_backend_get_load_protocols ())
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
  else if (! strcmp (name, SAVE_PROC) && uri_backend_get_save_protocols ())
    {
      status = save_image (param[3].data.d_string,
                           param[1].data.d_int32,
                           param[2].data.d_int32,
                           run_mode);
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
  gchar    *tmpname    = NULL;
  gint32    image_ID   = -1;
  gboolean  name_image = FALSE;
  GError   *error      = NULL;

  tmpname = get_temp_name (uri, &name_image);

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

  g_unlink (tmpname);
  g_free (tmpname);

  return image_ID;
}

static GimpPDBStatusType
save_image (const gchar *uri,
            gint32       image_ID,
            gint32       drawable_ID,
            gint32       run_mode)
{
  gchar  *tmpname;
  GError *error = NULL;

  tmpname = get_temp_name (uri, NULL);

  if (! (gimp_file_save (run_mode,
                         image_ID,
                         drawable_ID,
                         tmpname,
                         tmpname) && valid_file (tmpname)))
    {
      g_unlink (tmpname);
      g_free (tmpname);

      return GIMP_PDB_EXECUTION_ERROR;
    }

  if (! uri_backend_save_image (uri, tmpname, run_mode, &error))
    {
      g_message ("%s", error->message);
      g_clear_error (&error);

      g_unlink (tmpname);
      g_free (tmpname);

      return GIMP_PDB_EXECUTION_ERROR;
    }

  g_unlink (tmpname);
  g_free (tmpname);

  return GIMP_PDB_SUCCESS;
}

static gchar *
get_temp_name (const gchar *uri,
               gboolean    *name_image)
{
  gchar *basename;
  gchar *tmpname = NULL;

  if (name_image)
    *name_image = FALSE;

  basename = g_path_get_basename (uri);

  if (basename)
    {
      gchar *ext = strchr (basename, '.');

      if (ext && strlen (ext))
        {
          tmpname = gimp_temp_name (ext + 1);

          if (name_image)
            *name_image = TRUE;
        }

      g_free (basename);
    }

  if (! tmpname)
    tmpname = gimp_temp_name ("xxx");

  return tmpname;
}

static gboolean
valid_file (const gchar *filename)
{
  struct stat buf;

  return g_stat (filename, &buf) == 0 && buf.st_size > 0;
}
