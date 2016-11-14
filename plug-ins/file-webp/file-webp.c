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


static void       query              (void);
static void       run                (const gchar      *name,
                                      gint              nparams,
                                      const GimpParam  *param,
                                      gint             *nreturn_vals,
                                      GimpParam       **return_vals);

static WebPPreset get_preset_from_id (gint              id);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};


MAIN()

static void
set_default_params (WebPSaveParams* params)
{
  params->preset        = WEBP_PRESET_DEFAULT;
  params->lossless      = FALSE;
  params->animation     = FALSE;
  params->loop          = TRUE;
  params->quality       = 90.0f;
  params->alpha_quality = 100.0f;
  params->exif          = TRUE;
  params->iptc          = TRUE;
  params->xmp           = TRUE;
  params->delay         = 200;
  params->force_delay   = FALSE;
}

static void
query (void)
{
  gchar *preset_param_desc;
  gint   i;

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

  static GimpParamDef save_arguments[] =
  {
    { GIMP_PDB_INT32,    "run-mode",      "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",         "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",      "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",      "The name of the file to save the image to" },
    { GIMP_PDB_STRING,   "raw-filename",  "The name entered" },
    { GIMP_PDB_INT32,    "preset",        NULL },
    { GIMP_PDB_INT32,    "lossless",      "Use lossless encoding (0/1)" },
    { GIMP_PDB_FLOAT,    "quality",       "Quality of the image (0 <= quality <= 100)" },
    { GIMP_PDB_FLOAT,    "alpha-quality", "Quality of the image's alpha channel (0 <= alpha-quality <= 100)" },
    { GIMP_PDB_INT32,    "animation",     "Use layers for animation (0/1)" },
    { GIMP_PDB_INT32,    "anim-loop",     "Loop animation infinitely (0/1)" },
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

  /*
   * "preset" values in the PDB save proc are internal IDs, not
   * necessarily corresponding to the constants from libwebp (though at
   * time of writing, they are the same).
   * Generate the "preset" parameter description from webp_presets, so
   * that we don't have to edit multiple places if new presets are added
   * in the future.
   */
  preset_param_desc = g_strdup_printf ("WebP encoder preset (%s=0",
                                       webp_presets[0].label);
  for (i = 1; i < G_N_ELEMENTS (webp_presets); ++i)
    {
      gchar *preset_param;
      gchar *tmp;

      preset_param = g_strdup_printf (", %s=%d%s", webp_presets[i].label, i,
                                      i == G_N_ELEMENTS (webp_presets) - 1 ?  ")" : "");

      tmp = preset_param_desc;
      preset_param_desc = g_strconcat (preset_param_desc, preset_param,
                                       NULL);
      g_free (preset_param);
      g_free (tmp);
    }
  save_arguments[5].description = preset_param_desc;
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
  g_free (preset_param_desc);
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
      WebPSaveParams    params;
      GimpExportReturn  export = GIMP_EXPORT_CANCEL;
      gint32           *layers = NULL;
      gint32            n_layers;

      if (run_mode == GIMP_RUN_INTERACTIVE ||
          run_mode == GIMP_RUN_WITH_LAST_VALS)
        gimp_ui_init (PLUG_IN_BINARY, FALSE);

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      switch (run_mode)
        {
        case GIMP_RUN_WITH_LAST_VALS:
        case GIMP_RUN_INTERACTIVE:
          /* Default settings */
          set_default_params (&params);
          /*  Possibly override with session data  */
          gimp_get_data (SAVE_PROC, &params);

          export = gimp_export_image (&image_ID, &drawable_ID, "WebP",
                                      GIMP_EXPORT_CAN_HANDLE_RGB     |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                      GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              status = GIMP_PDB_CANCEL;
            }

          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != 16)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              params.preset        = get_preset_from_id (param[5].data.d_int32);
              params.lossless      = param[6].data.d_int32;
              params.quality       = param[7].data.d_float;
              params.alpha_quality = param[8].data.d_float;
              params.animation     = param[9].data.d_int32;
              params.loop          = param[10].data.d_int32;
              params.exif          = param[11].data.d_int32;
              params.iptc          = param[12].data.d_int32;
              params.xmp           = param[13].data.d_int32;
              params.delay         = param[14].data.d_int32;
              params.force_delay   = param[15].data.d_int32;
            }
          break;

        default:
          break;
        }


      if (status == GIMP_PDB_SUCCESS)
        {
          layers = gimp_image_get_layers (image_ID, &n_layers);
          if (run_mode == GIMP_RUN_INTERACTIVE)
            {
              if (! save_dialog (&params, image_ID, n_layers))
                {
                  status = GIMP_PDB_CANCEL;
                }
            }
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (! save_image (param[3].data.d_string,
                            n_layers, layers,
                            image_ID,
                            drawable_ID,
                            &params,
                            &error))
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }


      g_free (layers);

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);

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

static WebPPreset
get_preset_from_id (gint id)
{
  if (id >= 0 && id < G_N_ELEMENTS (webp_presets))
    return webp_presets[id].preset;
  return webp_presets[0].preset;
}
