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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "openexr-wrapper.h"

#define LOAD_PROC          "file-exr-load"
#define PLUG_IN_BINARY     "file-exr"
#define PLUG_IN_ROLE       "gimp-file-exr"
#define PLUG_IN_VERSION    "0.0.0"


/*
 * Declare some local functions.
 */
static void     query       (void);
static void     run         (const gchar      *name,
                             gint              nparams,
                             const GimpParam  *param,
                             gint             *nreturn_vals,
                             GimpParam       **return_vals);

static gint32    load_image (const gchar      *filename,
                             gboolean          interactive,
                             GError          **error);
/*
 * Some global variables.
 */

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
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files in the OpenEXR file format",
                          "This plug-in loads OpenEXR files. ",
                          "Dominik Ernst <dernst@gmx.de>, "
                          "Mukund Sivaraman <muks@banu.com>",
                          "Dominik Ernst <dernst@gmx.de>, "
                          "Mukund Sivaraman <muks@banu.com>",
                          PLUG_IN_VERSION,
                          N_("OpenEXR image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-exr");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "exr", "", "0,lelong,20000630");
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
  GError           *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      run_mode = param[0].data.d_int32;

      image_ID = load_image (param[1].data.d_string,
                             run_mode == GIMP_RUN_INTERACTIVE, &error);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[1].type = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
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

static gint32
load_image (const gchar  *filename,
            gboolean      interactive,
            GError      **error)
{
  gint32 status = -1;
  EXRLoader *loader;
  int width;
  int height;
  gboolean has_alpha;
  GimpImageBaseType image_type;
  GimpPrecision image_precision;
  gint32 image = -1;
  GimpImageType layer_type;
  int layer;
  const Babl *format;
  GeglBuffer *buffer = NULL;
  int bpp;
  int tile_height;
  gchar *pixels = NULL;
  int begin;
  int end;
  int num;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  loader = exr_loader_new (filename);
  if (!loader)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error opening file '%s' for reading"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  width = exr_loader_get_width (loader);
  height = exr_loader_get_height (loader);
  if ((width < 1) || (height < 1))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error querying image dimensions from '%s'"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  has_alpha = exr_loader_has_alpha (loader) ? TRUE : FALSE;

  switch (exr_loader_get_precision (loader))
    {
    case PREC_UINT:
      image_precision = GIMP_PRECISION_U32_LINEAR;
      break;
    case PREC_HALF:
      image_precision = GIMP_PRECISION_HALF_LINEAR;
      break;
    case PREC_FLOAT:
      image_precision = GIMP_PRECISION_FLOAT_LINEAR;
      break;
    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error querying image precision from '%s'"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  switch (exr_loader_get_image_type (loader))
    {
    case IMAGE_TYPE_RGB:
      image_type = GIMP_RGB;
      layer_type = has_alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE;
      break;
    case IMAGE_TYPE_GRAY:
      image_type = GIMP_GRAY;
      layer_type = has_alpha ? GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE;
      break;
    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error querying image type from '%s'"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  image = gimp_image_new_with_precision (width, height,
                                         image_type, image_precision);
  if (image == -1)
    {
      g_set_error (error, 0, 0,
                   _("Could not create new image for '%s': %s"),
                   gimp_filename_to_utf8 (filename), gimp_get_pdb_error ());
      goto out;
    }

  gimp_image_set_filename (image, filename);

  layer = gimp_layer_new (image, _("Background"), width, height,
                          layer_type, 100, GIMP_NORMAL_MODE);
  gimp_image_insert_layer (image, layer, -1, 0);

  buffer = gimp_drawable_get_buffer (layer);
  format = gimp_drawable_get_format (layer);
  bpp = babl_format_get_bytes_per_pixel (format);

  tile_height = gimp_tile_height ();
  pixels = g_new0 (gchar, tile_height * width * bpp);

  for (begin = 0; begin < height; begin += tile_height)
    {
      int retval;
      int i;
      end = MIN (begin + tile_height, height);
      num = end - begin;

      for (i = 0; i < num; i++)
        {
          retval = exr_loader_read_pixel_row (loader,
                                              pixels + (i * width * bpp),
                                              bpp, begin + i);
          if (retval < 0)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Error reading pixel data from '%s'"),
                           gimp_filename_to_utf8 (filename));
              goto out;
            }
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, begin, width, num),
                       0, NULL, pixels, GEGL_AUTO_ROWSTRIDE);

      gimp_progress_update ((gdouble) begin / (gdouble) height);
    }

  gimp_progress_update (1.0);

  status = image;

 out:
  if (buffer)
    g_object_unref (buffer);

  if ((status != image) && (image != -1))
    {
      /* This should clean up any associated layers too. */
      gimp_image_delete (image);
    }

  if (pixels)
    g_free (pixels);

  if (loader)
    exr_loader_unref (loader);

  return status;
}
