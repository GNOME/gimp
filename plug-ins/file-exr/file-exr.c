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

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "openexr-wrapper.h"

#define LOAD_PROC       "file-exr-load"
#define PLUG_IN_BINARY  "file-exr"
#define PLUG_IN_VERSION "0.0.0"


/*
 * Declare some local functions.
 */
static void     query            (void);
static void     run              (const gchar      *name,
                                  gint              nparams,
                                  const GimpParam  *param,
                                  gint             *nreturn_vals,
                                  GimpParam       **return_vals);

static gint32   load_image       (const gchar      *filename,
                                  gboolean          interactive,
                                  GError          **error);

static void     sanitize_comment (gchar            *comment);


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
                                    "exr",
                                    "",
                                    "0,long,0x762f3101");
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
  EXRLoader        *loader;
  gint              width;
  gint              height;
  gboolean          has_alpha;
  GimpImageBaseType image_type;
  GimpPrecision     image_precision;
  gint32            image = -1;
  GimpImageType     layer_type;
  gint32            layer;
  const Babl       *format;
  GeglBuffer       *buffer = NULL;
  gint              bpp;
  gint              tile_height;
  gchar            *pixels = NULL;
  gint              begin;
  gint32            success = FALSE;
  gchar            *comment = NULL;
  GimpColorProfile *profile = NULL;
  guchar           *exif_data;
  guint             exif_size;
  guchar           *xmp_data;
  guint             xmp_size;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  loader = exr_loader_new (filename);

  if (! loader)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error opening file '%s' for reading"),
                   gimp_filename_to_utf8 (filename));
      goto out;
    }

  width  = exr_loader_get_width (loader);
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

  /* try to load an icc profile, it will be generated on the fly if
   * chromaticities are given
   */
  if (image_type == GIMP_RGB)
    {
      profile = exr_loader_get_profile (loader);

      if (profile)
        gimp_image_set_color_profile (image, profile);
    }

  layer = gimp_layer_new (image, _("Background"), width, height,
                          layer_type, 100,
                          gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, -1, 0);

  buffer = gimp_drawable_get_buffer (layer);
  format = gimp_drawable_get_format (layer);
  bpp = babl_format_get_bytes_per_pixel (format);

  tile_height = gimp_tile_height ();
  pixels = g_new0 (gchar, tile_height * width * bpp);

  for (begin = 0; begin < height; begin += tile_height)
    {
      gint end;
      gint num;
      gint i;

      end = MIN (begin + tile_height, height);
      num = end - begin;

      for (i = 0; i < num; i++)
        {
          gint retval;

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

  /* try to read the file comment */
  comment = exr_loader_get_comment (loader);
  if (comment)
    {
      GimpParasite *parasite;

      sanitize_comment (comment);
      parasite = gimp_parasite_new ("gimp-comment",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (comment) + 1,
                                    comment);
      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }

  /* check if the image contains Exif or Xmp data and read it */
  exif_data = exr_loader_get_exif (loader, &exif_size);
  xmp_data  = exr_loader_get_xmp  (loader, &xmp_size);

  if (exif_data || xmp_data)
    {
      GimpMetadata          *metadata = gimp_metadata_new ();
      GimpMetadataLoadFlags  flags    = GIMP_METADATA_LOAD_ALL;

      if (exif_data)
        {
          gimp_metadata_set_from_exif (metadata, exif_data, exif_size, NULL);
          g_free (exif_data);
        }

      if (xmp_data)
        {
          gimp_metadata_set_from_xmp (metadata, xmp_data, xmp_size, NULL);
          g_free (xmp_data);
        }

      if (comment)
        flags &= ~GIMP_METADATA_LOAD_COMMENT;

      if (profile)
        flags &= ~GIMP_METADATA_LOAD_COLORSPACE;

      gimp_image_metadata_load_finish (image, "image/exr",
                                       metadata, flags, interactive);
      g_object_unref (metadata);
    }

  gimp_progress_update (1.0);

  success = TRUE;

 out:
  g_clear_object (&profile);
  g_clear_object (&buffer);
  g_clear_pointer (&pixels, g_free);
  g_clear_pointer (&comment, g_free);
  g_clear_pointer (&loader, exr_loader_unref);

  if (success)
    return image;

  if (image != -1)
    gimp_image_delete (image);

  return -1;
}

/* copy & pasted from file-jpeg/jpeg-load.c */
static void
sanitize_comment (gchar *comment)
{
  const gchar *start_invalid;

  if (! g_utf8_validate (comment, -1, &start_invalid))
    {
      guchar *c;

      for (c = (guchar *) start_invalid; *c; c++)
        {
          if (*c > 126 || (*c < 32 && *c != '\t' && *c != '\n' && *c != '\r'))
            *c = '?';
        }
    }
}
