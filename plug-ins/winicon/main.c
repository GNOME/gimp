/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#include "config.h"

#include <string.h>
#include <stdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/* #define ICO_DBG */

#include "main.h"
#include "icoload.h"
#include "icosave.h"

#include "libgimp/stdplugins-intl.h"


static void   query (void);
static void   run   (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals);


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
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,   "raw_filename", "The name entered"             }
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" },
  };

  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw_filename", "The name entered" },
  };

  gimp_install_procedure ("file_ico_load",
                          "Loads files of Windows ICO file format",
                          "Loads files of Windows ICO file format",
                          "Christian Kreibich <christian@whoop.org>",
                          "Christian Kreibich <christian@whoop.org>",
                          "2002",
                          N_("Microsoft Windows icon"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime ("file_ico_load", "image/x-ico");
  gimp_register_magic_load_handler ("file_ico_load",
                                    "ico",
                                    "",
                                    "0,string,\\000\\001\\000\\000,0,string,\\000\\002\\000\\000");

  gimp_install_procedure ("file_ico_save",
                          "Saves files in Windows ICO file format",
                          "Saves files in Windows ICO file format",
                          "Christian Kreibich <christian@whoop.org>",
                          "Christian Kreibich <christian@whoop.org>",
                          "2002",
                          N_("Microsoft Windows icon"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime ("file_ico_save", "image/x-ico");
  gimp_register_save_handler ("file_ico_save", "ico", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  gint32             image_ID;
  gint32             drawable_ID;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "file_ico_load") == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != 3)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          image_ID = LoadICO (param[1].data.d_string);

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
        }
    }
  else if (strcmp (name, "file_ico_save") == 0)
    {
      gchar *file_name;

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;
      file_name   = param[3].data.d_string;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams < 5)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          status = SaveICO (file_name, image_ID);
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}


guint8 *
ico_alloc_map (gint  width,
               gint  height,
               gint  bpp,
               gint *length)
{
  gint    len = 0;
  guint8 *map = NULL;

  switch (bpp)
    {
    case 1:
      if ((width % 32) == 0)
        len = (width * height / 8);
      else
        len = 4 * ((width/32 + 1) * height);
      break;

    case 4:
      if ((width % 8) == 0)
        len = (width * height / 2);
      else
        len = 4 * ((width/8 + 1) * height);
      break;

    case 8:
      if ((width % 4) == 0)
        len = width * height;
      else
        len = 4 * ((width/4 + 1) * height);
      break;

    default:
      len = width * height * (bpp/8);
    }

  *length = len;
  map = g_new0 (guint8, len);

  return map;
}


void
ico_cleanup (MsIcon *ico)
{
  gint i;

  if (!ico)
    return;

  if (ico->fp)
    fclose (ico->fp);

  if (ico->icon_dir)
    g_free (ico->icon_dir);

  if (ico->icon_data)
    {
      for (i = 0; i < ico->icon_count; i++)
        {
          g_free (ico->icon_data[i].palette);
          g_free (ico->icon_data[i].xor_map);
          g_free (ico->icon_data[i].and_map);
        }
      g_free (ico->icon_data);
    }
}
