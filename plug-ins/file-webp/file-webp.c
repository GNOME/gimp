/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-webp - WebP file format plug-in for the GIMP
 * Copyright (C) 2015  Nathan Osman
 * Copyright (C) 2016  Ben Touchette
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <webp/encode.h>

#include "file-webp-dialog.h"
#include "file-webp-load.h"
#include "file-webp-save.h"
#include "file-webp.h"

#include "libgimp/stdplugins-intl.h"


const char BINARY_NAME[]    = "file-webp";
const char LOAD_PROCEDURE[] = "file-webp-load";
const char SAVE_PROCEDURE[] = "file-webp-save";

/* Predeclare our entrypoints. */
static void   query (void);
static void   run   (const gchar *,
                     gint,
                     const GimpParam *,
                     gint *,
                     GimpParam **);

/* Declare our plugin entry points. */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};

MAIN()

/* This function registers our load and save handlers. */
static void
query (void)
{
  static const GimpParamDef load_arguments[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name entered" }
  };

  static const GimpParamDef load_return_values[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  static const GimpParamDef save_arguments[] =
  {
    { GIMP_PDB_INT32,    "run-mode",      "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",         "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",      "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",      "The name of the file to save the image to" },
    { GIMP_PDB_STRING,   "raw-filename",  "The name entered" },
    { GIMP_PDB_STRING,   "preset",        "Name of preset to use" },
    { GIMP_PDB_INT32,    "lossless",      "Use lossless encoding (0/1)" },
    { GIMP_PDB_FLOAT,    "quality",       "Quality of the image (0 <= quality <= 100)" },
    { GIMP_PDB_FLOAT,    "alpha-quality", "Quality of the image's alpha channel (0 <= alpha-quality <= 100)" },
    { GIMP_PDB_INT32,    "animation",     "Use layers for animation (0/1)" },
    { GIMP_PDB_INT32,    "anim-loop",     "Loop animation infinitely (0/1)" }
  };

  gimp_install_procedure (LOAD_PROCEDURE,
                          "Loads images in the WebP file format",
                          "Loads images in the WebP file format",
                          "Nathan Osman, Ben Touchette",
                          "(C) 2015-2016 Nathan Osman, (C) 2016 Ben Touchette",
                          "2015,2016",
                          N_("WebP image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_arguments),
                          G_N_ELEMENTS (load_return_values),
                          load_arguments,
                          load_return_values);

  gimp_register_file_handler_mime (LOAD_PROCEDURE, "image/webp");
  gimp_register_load_handler (LOAD_PROCEDURE, "webp", "");
  gimp_register_magic_load_handler (LOAD_PROCEDURE,
                                    "webp",
                                    "",
                                    "8,string,WEBP");

  gimp_install_procedure (SAVE_PROCEDURE,
                          "Saves files in the WebP image format",
                          "Saves files in the WebP image format",
                          "Nathan Osman, Ben Touchette",
                          "(C) 2015-2016 Nathan Osman, (C) 2016 Ben Touchette",
                          "2015,2016",
                          N_("WebP image"),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_arguments),
                          0,
                          save_arguments,
                          NULL);

  gimp_register_file_handler_mime (SAVE_PROCEDURE, "image/webp");
  gimp_register_save_handler (SAVE_PROCEDURE, "webp", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  gint32            drawable_ID;
  GError           *error = NULL;

  /* Determine the current run mode */
  run_mode = param[0].data.d_int32;

  /* Fill in the return values */
  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  /* Determine which procedure is being invoked */
  if (! strcmp (name, LOAD_PROCEDURE))
    {
      /* No need to determine whether the plugin is being invoked
       * interactively here since we don't need a UI for loading
       */
      image_ID = load_image (param[1].data.d_string, FALSE, &error);

      if(image_ID != -1)
        {
          /* Return the new image that was loaded */
          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (! strcmp (name, SAVE_PROCEDURE))
    {
      WebPSaveParams    params;
      GimpExportReturn  export_ret = GIMP_EXPORT_CANCEL;
      gint32           *layers;
      gint32            n_layers;

      /* Initialize the parameters to their defaults */
      params.preset        = "default";
      params.lossless      = FALSE;
      params.animation     = FALSE;
      params.loop          = TRUE;
      params.quality       = 90.0f;
      params.alpha_quality = 100.0f;

      /* Load the image and drawable IDs */
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      layers = gimp_image_get_layers (image_ID, &n_layers);

      /* What happens next depends on the run mode */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:

          gimp_ui_init (BINARY_NAME, FALSE);

          /* Attempt to export the image */
          export_ret = gimp_export_image (&image_ID,
                                          &drawable_ID,
                                          "WEBP",
                                          GIMP_EXPORT_CAN_HANDLE_RGB |
                                          GIMP_EXPORT_CAN_HANDLE_ALPHA);

          /* Return immediately if canceled */
          if (export_ret == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }

          /* Display the dialog */
          if (save_dialog (&params, image_ID, n_layers) != GTK_RESPONSE_OK)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }

          break;

        case GIMP_RUN_NONINTERACTIVE:

          /* Ensure the correct number of parameters were supplied */
          if (nparams != 10)
            {
              status = GIMP_PDB_CALLING_ERROR;
              break;
            }

          /* Load the parameters */
          params.preset        = param[5].data.d_string;
          params.lossless      = param[6].data.d_int32;
          params.quality       = param[7].data.d_float;
          params.alpha_quality = param[8].data.d_float;
          params.animation     = param[9].data.d_int32;
          params.loop          = param[10].data.d_int32;

          break;
        }

      /* Attempt to save the image */
      if (! save_image (param[3].data.d_string,
                        n_layers, layers,
                        image_ID,
                        drawable_ID,
                        &params,
                        &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      g_free (layers);
    }

  /* If an error was supplied, include it in the return values */
  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}
