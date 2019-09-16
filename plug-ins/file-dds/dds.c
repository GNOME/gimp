/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <libgimp/stdplugins-intl.h>

#include "ddsplugin.h"
#include "dds.h"
#include "misc.h"


static void query (void);
static void run   (const gchar     *name,
                   gint             nparams,
                   const GimpParam *param,
                   gint            *nreturn_vals,
                   GimpParam      **return_vals);


GimpPlugInInfo PLUG_IN_INFO =
{
  0,
  0,
  query,
  run
};

DDSWriteVals dds_write_vals =
{
  DDS_COMPRESS_NONE,
  DDS_MIPMAP_NONE,
  DDS_SAVE_SELECTED_LAYER,
  DDS_FORMAT_DEFAULT,
  -1,
  DDS_MIPMAP_FILTER_DEFAULT,
  DDS_MIPMAP_WRAP_DEFAULT,
  0,
  0,
  0.0,
  0,
  0,
  0,
  0.5
};

DDSReadVals dds_read_vals =
{
  1,
  1
};

static GimpParamDef load_args[] =
{
  { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
  { GIMP_PDB_STRING, "filename", "The name of the file to load"},
  { GIMP_PDB_STRING, "raw_filename", "The name entered"},
  { GIMP_PDB_INT32, "load_mipmaps", "Load mipmaps if present"},
  { GIMP_PDB_INT32, "decode_images", "Decode YCoCg/AExp images when detected"}
};

static GimpParamDef load_return_vals[] =
{
  { GIMP_PDB_IMAGE, "image", "Output image"}
};

static GimpParamDef save_args[] =
{
  { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
  { GIMP_PDB_IMAGE, "image", "Input image"},
  { GIMP_PDB_DRAWABLE, "drawable", "Drawable to save"},
  { GIMP_PDB_STRING, "filename", "The name of the file to save the image as"},
  { GIMP_PDB_STRING, "raw_filename", "The name entered"},
  { GIMP_PDB_INT32, "compression_format", "Compression format (0 = None, 1 = BC1/DXT1, 2 = BC2/DXT3, 3 = BC3/DXT5, 4 = BC3n/DXT5nm, 5 = BC4/ATI1N, 6 = BC5/ATI2N, 7 = RXGB (DXT5), 8 = Alpha Exponent (DXT5), 9 = YCoCg (DXT5), 10 = YCoCg scaled (DXT5))"},
  { GIMP_PDB_INT32, "mipmaps", "How to handle mipmaps (0 = No mipmaps, 1 = Generate mipmaps, 2 = Use existing mipmaps (layers)"},
  { GIMP_PDB_INT32, "savetype", "How to save the image (0 = selected layer, 1 = cube map, 2 = volume map, 3 = texture array"},
  { GIMP_PDB_INT32, "format", "Custom pixel format (0 = default, 1 = R5G6B5, 2 = RGBA4, 3 = RGB5A1, 4 = RGB10A2)"},
  { GIMP_PDB_INT32, "transparent_index", "Index of transparent color or -1 to disable (for indexed images only)."},
  { GIMP_PDB_INT32, "mipmap_filter", "Filtering to use when generating mipmaps (0 = default, 1 = nearest, 2 = box, 3 = triangle, 4 = quadratic, 5 = bspline, 6 = mitchell, 7 = lanczos, 8 = kaiser)"},
  { GIMP_PDB_INT32, "mipmap_wrap", "Wrap mode to use when generating mipmaps (0 = default, 1 = mirror, 2 = repeat, 3 = clamp)"},
  { GIMP_PDB_INT32, "gamma_correct", "Use gamma correct mipmap filtering"},
  { GIMP_PDB_INT32, "srgb", "Use sRGB colorspace for gamma correction"},
  { GIMP_PDB_FLOAT, "gamma", "Gamma value to use for gamma correction (i.e. 2.2)"},
  { GIMP_PDB_INT32, "perceptual_metric", "Use a perceptual error metric during compression"},
  { GIMP_PDB_INT32, "preserve_alpha_coverage", "Preserve alpha test converage for alpha channel maps"},
  { GIMP_PDB_FLOAT, "alpha_test_threshold", "Alpha test threshold value for which alpha test converage should be preserved"}
};

#if 0
static GimpParamDef decode_args[] =
{
  { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
  { GIMP_PDB_IMAGE, "image", "Input image"},
  { GIMP_PDB_DRAWABLE, "drawable", "Drawable to save"}
};
#endif


MAIN ()


static void
query (void)
{
  gimp_install_procedure (LOAD_PROC,
                          "Loads files in DDS image format",
                          "Loads files in DDS image format",
                          "Shawn Kirst",
                          "Shawn Kirst",
                          "2008",
                          N_("DDS image"),
                          0,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/dds");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "dds",
                                    "",
                                    "0,string,DDS");

  gimp_install_procedure (SAVE_PROC,
                          "Saves files in DDS image format",
                          "Saves files in DDS image format",
                          "Shawn Kirst",
                          "Shawn Kirst",
                          "2008",
                          N_("DDS image"),
                          "INDEXED, GRAY, RGB",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, 0);

  gimp_register_file_handler_mime (SAVE_PROC, "image/dds");
  gimp_register_save_handler (SAVE_PROC,
                              "dds",
                              "");

#if 0
  gimp_install_procedure (DECODE_YCOCG_PROC,
                          "Converts YCoCg encoded pixels to RGB",
                          "Converts YCoCg encoded pixels to RGB",
                          "Shawn Kirst",
                          "Shawn Kirst",
                          "2008",
                          N_("Decode YCoCg"),
                          "RGBA",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (decode_args), 0,
                          decode_args, 0);
  /*gimp_plugin_menu_register (DECODE_YCOCG_PROC, "<Image>/Filters/Colors");*/

  gimp_install_procedure (DECODE_YCOCG_SCALED_PROC,
                          "Converts YCoCg (scaled) encoded pixels to RGB",
                          "Converts YCoCg (scaled) encoded pixels to RGB",
                          "Shawn Kirst",
                          "Shawn Kirst",
                          "2008",
                          N_("Decode YCoCg (scaled)"),
                          "RGBA",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (decode_args), 0,
                          decode_args, 0);
  /*gimp_plugin_menu_register (DECODE_YCOCG_SCALED_PROC, "<Image>/Filters/Colors");*/

  gimp_install_procedure (DECODE_ALPHA_EXP_PROC,
                          "Converts alpha exponent encoded pixels to RGB",
                          "Converts alpha exponent encoded pixels to RGB",
                          "Shawn Kirst",
                          "Shawn Kirst",
                          "2008",
                          N_("Decode Alpha exponent"),
                          "RGBA",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (decode_args), 0,
                          decode_args, 0);
  /*gimp_plugin_menu_register (DECODE_ALPHA_EXP_PROC, "<Image>/Filters/Colors");*/
#endif
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
  gint32            imageID;
  gint32            drawableID;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;

  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (! strcmp (name, LOAD_PROC))
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          gimp_ui_init ("dds", 0);
          gimp_get_data (LOAD_PROC, &dds_read_vals);
          break;

        case GIMP_RUN_NONINTERACTIVE:
          dds_read_vals.mipmaps = param[3].data.d_int32;
          dds_read_vals.decode_images = param[4].data.d_int32;
          if (nparams != G_N_ELEMENTS (load_args))
            status = GIMP_PDB_CALLING_ERROR;
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          status = read_dds (param[1].data.d_string, &imageID,
                             run_mode == GIMP_RUN_INTERACTIVE);
          if (status == GIMP_PDB_SUCCESS && imageID != -1)
            {
              *nreturn_vals = 2;
              values[1].type = GIMP_PDB_IMAGE;
              values[1].data.d_image = imageID;
              if (run_mode == GIMP_RUN_INTERACTIVE)
                gimp_set_data (LOAD_PROC, &dds_read_vals, sizeof (dds_read_vals));
            }
          else if (status != GIMP_PDB_CANCEL)
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      imageID    = param[1].data.d_int32;
      drawableID = param[2].data.d_int32;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init ("dds", 0);
          export = gimp_export_image (&imageID, &drawableID, "DDS",
                                     (GIMP_EXPORT_CAN_HANDLE_RGB |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA |
                                      GIMP_EXPORT_CAN_HANDLE_LAYERS));
          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }

        default:
          break;
        }

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          gimp_get_data (SAVE_PROC, &dds_write_vals);
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != G_N_ELEMENTS (save_args))
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              dds_write_vals.compression             = param[5].data.d_int32;
              dds_write_vals.mipmaps                 = param[6].data.d_int32;
              dds_write_vals.savetype                = param[7].data.d_int32;
              dds_write_vals.format                  = param[8].data.d_int32;
              dds_write_vals.transindex              = param[9].data.d_int32;
              dds_write_vals.mipmap_filter           = param[10].data.d_int32;
              dds_write_vals.mipmap_wrap             = param[11].data.d_int32;
              dds_write_vals.gamma_correct           = param[12].data.d_int32;
              dds_write_vals.srgb                    = param[13].data.d_int32;
              dds_write_vals.gamma                   = param[14].data.d_float;
              dds_write_vals.perceptual_metric       = param[15].data.d_int32;
              dds_write_vals.preserve_alpha_coverage = param[16].data.d_int32;
              dds_write_vals.alpha_test_threshold    = param[17].data.d_float;

              if ((dds_write_vals.compression <  DDS_COMPRESS_NONE) ||
                 (dds_write_vals.compression >= DDS_COMPRESS_MAX))
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }

              if ((dds_write_vals.mipmaps <  DDS_MIPMAP_NONE) ||
                 (dds_write_vals.mipmaps >= DDS_MIPMAP_MAX))
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }

              if ((dds_write_vals.savetype <  DDS_SAVE_SELECTED_LAYER) ||
                 (dds_write_vals.savetype >= DDS_SAVE_MAX))
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }

              if ((dds_write_vals.format <  DDS_FORMAT_DEFAULT) ||
                 (dds_write_vals.format >= DDS_FORMAT_MAX))
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }

              if ((dds_write_vals.mipmap_filter <  DDS_MIPMAP_FILTER_DEFAULT) ||
                 (dds_write_vals.mipmap_filter >= DDS_MIPMAP_FILTER_MAX))
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }

              if ((dds_write_vals.mipmap_wrap <  DDS_MIPMAP_WRAP_DEFAULT) ||
                 (dds_write_vals.mipmap_wrap >= DDS_MIPMAP_WRAP_MAX))
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          gimp_get_data (SAVE_PROC, &dds_write_vals);
          break;

        default:
          break;
        }

      if (dds_write_vals.gamma < 1e-04f)
        /* gimp_gamma () got removed and was always returning 2.2 anyway.
         * XXX Review this piece of code if we expect gamma value could
         * be parameterized.
         */
        dds_write_vals.gamma = 2.2;

      if (status == GIMP_PDB_SUCCESS)
        {
          status = write_dds (param[3].data.d_string, imageID, drawableID,
                              run_mode == GIMP_RUN_INTERACTIVE);
          if (status == GIMP_PDB_SUCCESS)
            gimp_set_data (SAVE_PROC, &dds_write_vals, sizeof (dds_write_vals));
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (imageID);
    }
#if 0
  else if (! strcmp (name, DECODE_YCOCG_PROC))
    {
      imageID = param[1].data.d_int32;
      drawableID = param[2].data.d_int32;

      decode_ycocg_image (drawableID, TRUE);

      status = GIMP_PDB_SUCCESS;

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else if (! strcmp (name, DECODE_YCOCG_SCALED_PROC))
    {
      imageID = param[1].data.d_int32;
      drawableID = param[2].data.d_int32;

      decode_ycocg_scaled_image (drawableID, TRUE);

      status = GIMP_PDB_SUCCESS;

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else if (! strcmp (name, DECODE_ALPHA_EXP_PROC))
    {
      imageID = param[1].data.d_int32;
      drawableID = param[2].data.d_int32;

      decode_alpha_exp_image (drawableID, TRUE);

      status = GIMP_PDB_SUCCESS;

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
#endif
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}
