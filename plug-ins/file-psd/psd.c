/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "psd.h"
#include "psd-load.h"
#include "psd-save.h"
#include "psd-thumb-load.h"

#include "libgimp/stdplugins-intl.h"


/*  Local function prototypes  */

static void  query (void);
static void  run   (const gchar     *name,
                    gint             nparams,
                    const GimpParam *param,
                    gint            *nreturn_vals,
                    GimpParam      **return_vals);


/*  Local variables  */

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
  /* File Load */
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  /* Thumbnail Load */
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

  /* File save */
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32,    "compression",  "Compression type: { NONE (0), LZW (1), PACKBITS (2)" },
    { GIMP_PDB_INT32,    "fill-order",   "Fill Order: { MSB to LSB (0), LSB to MSB (1)" }
  };

  /* File load */
  gimp_install_procedure (LOAD_PROC,
                          "Loads images from the Photoshop PSD file format",
                          "This plug-in loads images in Adobe "
                          "Photoshop (TM) native PSD format.",
                          "John Marshall",
                          "John Marshall",
                          "2007",
                          N_("Photoshop image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-psd");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "psd",
                                    "",
                                    "0,string,8BPS");

  /* File load (merged) */
  gimp_install_procedure (LOAD_MERGED_PROC,
                          "Loads merged images from the Photoshop PSD file format",
                          "This plug-in loads the merged image data in Adobe "
                          "Photoshop (TM) native PSD format.",
                          "Ell",
                          "Ell",
                          "2018",
                          N_("Photoshop image (merged)"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_priority (LOAD_MERGED_PROC, +1);
  gimp_register_file_handler_mime (LOAD_MERGED_PROC, "image/x-psd");
  gimp_register_magic_load_handler (LOAD_MERGED_PROC,
                                    "psd",
                                    "",
                                    "0,string,8BPS");

  /* Thumbnail load */
  gimp_install_procedure (LOAD_THUMB_PROC,
                          "Loads thumbnails from the Photoshop PSD file format",
                          "This plug-in loads thumbnail images from Adobe "
                          "Photoshop (TM) native PSD format files.",
                          "John Marshall",
                          "John Marshall",
                          "2007",
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (thumb_args),
                          G_N_ELEMENTS (thumb_return_vals),
                          thumb_args, thumb_return_vals);

  gimp_register_thumbnail_loader (LOAD_PROC, LOAD_THUMB_PROC);

  gimp_install_procedure (SAVE_PROC,
                          "saves files in the Photoshop(tm) PSD file format",
                          "This filter saves files of Adobe Photoshop(tm) native PSD format.  These files may be of any image type supported by GIMP, with or without layers, layer masks, aux channels and guides.",
                          "Monigotes",
                          "Monigotes",
                          "2000",
                          N_("Photoshop image"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-psd");
  gimp_register_save_handler (SAVE_PROC, "psd", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[4];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  GError           *error  = NULL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0 ||
      strcmp (name, LOAD_MERGED_PROC) == 0)
    {
      gboolean resolution_loaded = FALSE;
      gboolean profile_loaded    = FALSE;
      gboolean interactive;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          interactive = TRUE;
          break;
        default:
          interactive = FALSE;
          break;
        }

      image_ID = load_image (param[1].data.d_string,
                             strcmp (name, LOAD_MERGED_PROC) == 0,
                             &resolution_loaded,
                             &profile_loaded,
                             &error);

      if (image_ID != -1)
        {
          GFile        *file = g_file_new_for_path (param[1].data.d_string);
          GimpMetadata *metadata;

          metadata = gimp_image_metadata_load_prepare (image_ID, "image/x-psd",
                                                       file, NULL);

          if (metadata)
            {
              GimpMetadataLoadFlags flags = GIMP_METADATA_LOAD_ALL;

              if (resolution_loaded)
                flags &= ~GIMP_METADATA_LOAD_RESOLUTION;

              if (profile_loaded)
                flags &= ~GIMP_METADATA_LOAD_COLORSPACE;

              gimp_image_metadata_load_finish (image_ID, "image/x-psd",
                                               metadata, flags,
                                               interactive);

              g_object_unref (metadata);
            }

          g_object_unref (file);

          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, LOAD_THUMB_PROC) == 0)
    {
      if (nparams < 2)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          const gchar *filename = param[0].data.d_string;
          gint         width    = 0;
          gint         height   = 0;

          image_ID = load_thumbnail_image (filename, &width, &height, &error);

          if (image_ID != -1)
            {
              *nreturn_vals = 4;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
              values[2].type         = GIMP_PDB_INT32;
              values[2].data.d_int32 = width;
              values[3].type         = GIMP_PDB_INT32;
              values[3].data.d_int32 = height;
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      gint32                 drawable_id;
      GimpMetadata          *metadata;
      GimpMetadataSaveFlags  metadata_flags;
      GimpExportReturn       export = GIMP_EXPORT_IGNORE;

      IFDBG(2) g_debug ("\n---------------- %s ----------------\n",
                        param[3].data.d_string);

      image_ID    = param[1].data.d_int32;
      drawable_id = param[2].data.d_int32;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_id, "PSD",
                                      GIMP_EXPORT_CAN_HANDLE_RGB     |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                      GIMP_EXPORT_CAN_HANDLE_LAYERS  |
                                      GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
          break;

        default:
          break;
        }

      metadata = gimp_image_metadata_save_prepare (image_ID,
                                                   "image/x-psd",
                                                   &metadata_flags);

      if (save_image (param[3].data.d_string, image_ID, &error))
        {
          if (metadata)
            {
              GFile *file;

              gimp_metadata_set_bits_per_sample (metadata, 8);

              file = g_file_new_for_path (param[3].data.d_string);
              gimp_image_metadata_save_finish (image_ID,
                                               "image/x-psd",
                                               metadata, metadata_flags,
                                               file, NULL);
              g_object_unref (file);
            }

          values[0].data.d_status = GIMP_PDB_SUCCESS;
        }
      else
        {
          values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

          if (error)
            {
              *nreturn_vals = 2;
              values[1].type          = GIMP_PDB_STRING;
              values[1].data.d_string = error->message;
            }
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);

      if (metadata)
        g_object_unref (metadata);
    }

  /* Unknown procedure */
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}
