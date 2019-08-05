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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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


static void   query (void);
static void   run   (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};


MAIN()

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
    { GIMP_PDB_INT32,    "preset",        "preset (Default=0, Picture=1, Photo=2, Drawing=3, Icon=4, Text=5)" },
    { GIMP_PDB_INT32,    "lossless",      "Use lossless encoding (0/1)" },
    { GIMP_PDB_FLOAT,    "quality",       "Quality of the image (0 <= quality <= 100)" },
    { GIMP_PDB_FLOAT,    "alpha-quality", "Quality of the image's alpha channel (0 <= alpha-quality <= 100)" },
    { GIMP_PDB_INT32,    "animation",     "Use layers for animation (0/1)" },
    { GIMP_PDB_INT32,    "anim-loop",     "Loop animation infinitely (0/1)" },
    { GIMP_PDB_INT32,    "minimize-size", "Minimize animation size (0/1)" },
    { GIMP_PDB_INT32,    "kf-distance",   "Maximum distance between key-frames (>=0)" },
    { GIMP_PDB_INT32,    "exif",          "Toggle saving exif data (0/1)" },
    { GIMP_PDB_INT32,    "iptc",          "Toggle saving iptc data (0/1)" },
    { GIMP_PDB_INT32,    "xmp",           "Toggle saving xmp data (0/1)" },
    { GIMP_PDB_INT32,    "delay",         "Delay to use when timestamps are not available or forced" },
    { GIMP_PDB_INT32,    "force-delay",   "Force delay on all frames" }
  };

  gimp_install_procedure (LOAD_PROC,
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

  gimp_register_file_handler_mime (LOAD_PROC, "image/webp");
  gimp_register_load_handler (LOAD_PROC, "webp", "");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "webp",
                                    "",
                                    "8,string,WEBP");

  gimp_install_procedure (SAVE_PROC,
                          "Saves files in the WebP image format",
                          "Saves files in the WebP image format",
                          "Nathan Osman, Ben Touchette",
                          "(C) 2015-2016 Nathan Osman, (C) 2016 Ben Touchette",
                          "2015,2016",
                          N_("WebP image"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_arguments),
                          0,
                          save_arguments,
                          NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/webp");
  gimp_register_save_handler (SAVE_PROC, "webp", "");
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

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (! strcmp (name, LOAD_PROC))
    {
      image_ID = load_image (param[1].data.d_string, FALSE, &error);

      if (image_ID != -1)
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
  else if (! strcmp (name, SAVE_PROC))
    {
      GimpMetadata          *metadata = NULL;
      GimpMetadataSaveFlags  metadata_flags;
      WebPSaveParams         params;
      GimpExportReturn       export = GIMP_EXPORT_CANCEL;

      if (run_mode == GIMP_RUN_INTERACTIVE ||
          run_mode == GIMP_RUN_WITH_LAST_VALS)
        gimp_ui_init (PLUG_IN_BINARY, FALSE);

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /* Default settings */
      params.preset        = WEBP_PRESET_DEFAULT;
      params.lossless      = FALSE;
      params.animation     = FALSE;
      params.loop          = TRUE;
      params.minimize_size = TRUE;
      params.kf_distance   = 50;
      params.quality       = 90.0f;
      params.alpha_quality = 100.0f;
      params.exif          = FALSE;
      params.iptc          = FALSE;
      params.xmp           = FALSE;
      params.delay         = 200;
      params.force_delay   = FALSE;

      /* Override the defaults with preferences. */
      metadata = gimp_image_metadata_save_prepare (image_ID,
                                                   "image/webp",
                                                   &metadata_flags);
      params.exif    = (metadata_flags & GIMP_METADATA_SAVE_EXIF) != 0;
      params.xmp     = (metadata_flags & GIMP_METADATA_SAVE_XMP) != 0;
      params.iptc    = (metadata_flags & GIMP_METADATA_SAVE_IPTC) != 0;
      params.profile = (metadata_flags & GIMP_METADATA_SAVE_COLOR_PROFILE) != 0;

      switch (run_mode)
        {
        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly override with session data  */
          gimp_get_data (SAVE_PROC, &params);
          break;

        case GIMP_RUN_INTERACTIVE:
          /*  Possibly override with session data  */
          gimp_get_data (SAVE_PROC, &params);

          if (! save_dialog (&params, image_ID))
            {
              status = GIMP_PDB_CANCEL;
            }
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != 18)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              if (param[5].data.d_int32 < WEBP_PRESET_DEFAULT ||
                  param[5].data.d_int32 > WEBP_PRESET_TEXT)
                params.preset = WEBP_PRESET_DEFAULT;
              else
                params.preset = param[5].data.d_int32;

              params.lossless      = param[6].data.d_int32;
              params.quality       = param[7].data.d_float;
              params.alpha_quality = param[8].data.d_float;
              params.animation     = param[9].data.d_int32;
              params.loop          = param[10].data.d_int32;
              params.minimize_size = param[11].data.d_int32;
              params.kf_distance   = param[12].data.d_int32;
              params.exif          = param[13].data.d_int32;
              params.iptc          = param[14].data.d_int32;
              params.xmp           = param[15].data.d_int32;
              params.delay         = param[16].data.d_int32;
              params.force_delay   = param[17].data.d_int32;
            }
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS && (run_mode == GIMP_RUN_INTERACTIVE ||
                                         run_mode == GIMP_RUN_WITH_LAST_VALS))
        {
          GimpExportCapabilities capabilities =
            GIMP_EXPORT_CAN_HANDLE_RGB     |
            GIMP_EXPORT_CAN_HANDLE_GRAY    |
            GIMP_EXPORT_CAN_HANDLE_INDEXED |
            GIMP_EXPORT_CAN_HANDLE_ALPHA;

          if (params.animation)
            capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION;

          export = gimp_export_image (&image_ID, &drawable_ID, "WebP",
                                      capabilities);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              status = GIMP_PDB_CANCEL;
            }
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (! save_image (param[3].data.d_string,
                            image_ID,
                            drawable_ID,
                            metadata, metadata_flags,
                            &params,
                            &error))
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }


      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);

      if (metadata)
        g_object_unref (metadata);

      if (status == GIMP_PDB_SUCCESS)
        {
          /* save parameters for later */
          gimp_set_data (SAVE_PROC, &params, sizeof (params));
        }
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
