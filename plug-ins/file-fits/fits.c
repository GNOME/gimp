/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * FITS file plugin
 * reading and writing code Copyright (C) 1997 Peter Kirchgessner
 * e-mail: peter@kirchgessner.net, WWW: http://www.kirchgessner.net
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
 *
 */

/* Event history:
 * V 1.00, PK, 05-May-97: Creation
 * V 1.01, PK, 19-May-97: Problem with compilation on Irix fixed
 * V 1.02, PK, 08-Jun-97: Bug with saving gray images fixed
 * V 1.03, PK, 05-Oct-97: Parse rc-file
 * V 1.04, PK, 12-Oct-97: No progress bars for non-interactive mode
 * V 1.05, nn, 20-Dec-97: Initialize image_ID in run()
 * V 1.06, PK, 21-Nov-99: Internationalization
 *                        Fix bug with gimp_export_image()
 *                        (moved it from load to save)
 * V 1.07, PK, 16-Aug-06: Fix problems with internationalization
 *                        (writing 255,0 instead of 255.0)
 *                        Fix problem with not filling up properly last record
 */

#include "config.h"

#include <math.h>
#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <fitsio.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-fits-load"
#define EXPORT_PROC    "file-fits-export"
#define PLUG_IN_BINARY "file-fits"
#define PLUG_IN_ROLE   "gimp-file-fits"


typedef struct _Fits      Fits;
typedef struct _FitsClass FitsClass;

struct _Fits
{
  GimpPlugIn      parent_instance;
};

struct _FitsClass
{
  GimpPlugInClass parent_class;
};


#define FITS_TYPE  (fits_get_type ())
#define FITS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FITS_TYPE, Fits))

GType                   fits_get_type         (void) G_GNUC_CONST;

typedef struct
{
  gint   naxis;
  glong  naxisn[3];
  gint   bitpix;
  gint   bpp;
  gint   datatype;
} FitsHduData;

static GList          * fits_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * fits_create_procedure (GimpPlugIn            *plug_in,
                                               const gchar           *name);

static GimpValueArray * fits_load             (GimpProcedure         *procedure,
                                               GimpRunMode            run_mode,
                                               GFile                 *file,
                                               GimpMetadata          *metadata,
                                               GimpMetadataLoadFlags *flags,
                                               GimpProcedureConfig   *config,
                                               gpointer               run_data);
static GimpValueArray * fits_export           (GimpProcedure         *procedure,
                                               GimpRunMode            run_mode,
                                               GimpImage             *image,
                                               GFile                 *file,
                                               GimpExportOptions     *options,
                                               GimpMetadata          *metadata,
                                               GimpProcedureConfig   *config,
                                               gpointer               run_data);

static GimpImage      * load_image            (GFile                 *file,
                                               GObject               *config,
                                               GimpRunMode            run_mode,
                                               GError               **error);
static gint             export_image          (GFile                 *file,
                                               GimpImage             *image,
                                               GimpDrawable          *drawable,
                                               GError               **error);

static gint             export_fits           (GFile                 *file,
                                               GimpImage             *image,
                                               GimpDrawable          *drawable);

static GimpImage      * create_new_image      (GFile                 *file,
                                               guint                  pagenum,
                                               guint                  width,
                                               guint                  height,
                                               GimpImageBaseType      itype,
                                               GimpImageType          dtype,
                                               GimpPrecision          iprecision,
                                               GimpLayer            **layer,
                                               GeglBuffer           **buffer);

static gboolean         load_dialog           (GimpProcedure         *procedure,
                                               GObject               *config);
static void             show_fits_errors      (gint                   status);


G_DEFINE_TYPE (Fits, fits, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FITS_TYPE)
DEFINE_STD_SET_I18N


static void
fits_class_init (FitsClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = fits_query_procedures;
  plug_in_class->create_procedure = fits_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
fits_init (Fits *fits)
{
}

static GList *
fits_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
fits_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           fits_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     _("Flexible Image Transport System"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load file of the FITS file format"),
                                        _("Load file of the FITS file format "
                                          "(Flexible Image Transport System)"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner (peter@kirchgessner.net), "
                                      "Alex Sa.",
                                      "1997 - 2023");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-fits");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "fit,fits");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,SIMPLE");

      gimp_procedure_add_choice_aux_argument (procedure, "replace",
                                              _("Re_placement for undefined pixels"),
                                              _("Replacement for undefined pixels"),
                                              gimp_choice_new_with_values ("black",  0,   _("Black"), NULL,
                                                                           "white",  255, _("White"), NULL,
                                                                           NULL),
                                              "black",
                                              G_PARAM_READWRITE);
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, fits_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB, GRAY, INDEXED");

      gimp_procedure_set_menu_label (procedure,
                                     _("Flexible Image Transport System"));

      gimp_procedure_set_documentation (procedure,
                                        _("Export file in the FITS file format"),
                                        _("FITS exporting handles all image "
                                          "types except those with alpha channels."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner (peter@kirchgessner.net), "
                                      "Alex Sa.",
                                      "1997 - 2023");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-fits");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "fit,fits");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB  |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED,
                                              NULL, NULL, NULL);
    }

  return procedure;
}

static GimpValueArray *
fits_load (GimpProcedure         *procedure,
           GimpRunMode            run_mode,
           GFile                 *file,
           GimpMetadata          *metadata,
           GimpMetadataLoadFlags *flags,
           GimpProcedureConfig   *config,
           gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      if (! load_dialog (procedure, G_OBJECT (config)))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
    }

  image = load_image (file, G_OBJECT (config), run_mode, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
fits_export (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             GFile                *file,
             GimpExportOptions    *options,
             GimpMetadata         *metadata,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  GimpImage          *duplicate_image;
  GimpDrawable      **flipped_drawables;
  GimpPDBStatusType   status = GIMP_PDB_SUCCESS;
  GimpExportReturn    export = GIMP_EXPORT_IGNORE;
  GList             *drawables;
  GError             *error  = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_ui_init (PLUG_IN_BINARY);

  export = gimp_export_options_get_image (options, &image);

  drawables = gimp_image_list_layers (image);

  /* Flip image vertical since FITS writes from bottom to top */
  duplicate_image = gimp_image_duplicate (image);
  gimp_image_flip (duplicate_image, GIMP_ORIENTATION_VERTICAL);
  flipped_drawables = gimp_image_get_selected_drawables (duplicate_image);

  if (! export_image (file, image, flipped_drawables[0], &error))
    status = GIMP_PDB_EXECUTION_ERROR;

  gimp_image_delete (duplicate_image);
  g_free (flipped_drawables);

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpImage *
load_image (GFile        *file,
            GObject      *config,
            GimpRunMode   run_mode,
            GError      **error)
{
  GimpImage         *image       = NULL;
  GimpLayer         *layer;
  GeglBuffer        *buffer;
  FILE              *fp;
  fitsfile          *ifp;
  FitsHduData        hdu;
  gint               n_pics;
  gint               count       = 1;
  gint               width;
  gint               height;
  gint               row_length;
  int                status      = 0;
  glong              fpixel[3]   = {1, 1, 1};
  GimpImageBaseType  itype       = GIMP_GRAY;
  GimpImageType      dtype       = GIMP_GRAYA_IMAGE;
  GimpPrecision      iprecision  = GIMP_PRECISION_U16_NON_LINEAR;
  const Babl        *type        = NULL;
  const Babl        *format      = NULL;
  gdouble           *pixels;
  gdouble            datamin     = 1.0E30f;
  gdouble            datamax     = -1.0E30f;
  gint               channels    = 1;
  gint               replace;
  gdouble            replace_val = 0;

  replace = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                 "replace");

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  fclose (fp);

  if (fits_open_diskfile (&ifp, g_file_peek_path (file), READONLY, &status))
    show_fits_errors (status);

  if (! ifp)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", _("Error during opening of FITS file"));
      return NULL;
    }

  /* Get first item */
  fits_get_num_hdus (ifp, &n_pics, &status);

  if (status)
    show_fits_errors (status);

  if (n_pics <= 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", _("FITS file keeps no displayable images"));
      fits_close_file (ifp, &status);
      return NULL;
    }

  while (count <= n_pics)
    {
      hdu.naxis = 0;

      fits_movabs_hdu (ifp, count, NULL, &status);

      fits_get_img_param (ifp, 3, &hdu.bitpix, &hdu.naxis, hdu.naxisn,
                          &status);

      /* Skip if invalid dimensions; possibly header data */
      if (hdu.naxis < 2)
        {
          count++;
          continue;
        }

      width  = hdu.naxisn[0];
      height = hdu.naxisn[1];

      type = babl_type ("double");
      switch (hdu.bitpix)
      {
        case 8:
          iprecision = GIMP_PRECISION_U8_LINEAR;
          if (replace)
            replace_val = 255;
          break;
        case 16:
          iprecision = GIMP_PRECISION_U16_NON_LINEAR;
          if (replace)
            replace_val = G_MAXSHORT;
          break;
        case 32:
          iprecision = GIMP_PRECISION_U32_LINEAR;
          if (replace)
            replace_val = G_MAXINT;
          break;
        case -32:
          iprecision = GIMP_PRECISION_FLOAT_LINEAR;
          if (replace)
            replace_val = G_MAXFLOAT;
          break;
        case -64:
          iprecision = GIMP_PRECISION_DOUBLE_LINEAR;
          if (replace)
            replace_val = G_MAXDOUBLE;
          break;
      }

      if (hdu.naxis == 2)
        {
          itype = GIMP_GRAY;
          dtype = GIMP_GRAYA_IMAGE;
          format = babl_format_new (babl_model ("Y'"),
                                    type,
                                    babl_component ("Y'"),
                                    NULL);
        }
      else if (hdu.naxisn[2])
        {
          /* Original RGB format */
          if (hdu.naxisn[0] == 3)
            {
              width  = hdu.naxisn[1];
              height = hdu.naxisn[2];
            }
          channels = 3;

          itype  = GIMP_RGB;
          dtype  = GIMP_RGB_IMAGE;
          format = babl_format_new (babl_model ("R'G'B'"),
                                    type,
                                    babl_component ("R'"),
                                    babl_component ("G'"),
                                    babl_component ("B'"),
                                    NULL);
        }

      /* If RGB FITS image, we need to read in the whole image so we can convert
       * the planes format to RGB */
      if (hdu.naxis == 2)
        pixels = (gdouble *) malloc (width * sizeof (gdouble) * channels);
      else
        pixels = (gdouble *) malloc (width * height * sizeof (gdouble) * channels);

      if (! image)
        {
          image = create_new_image (file, count, width, height,
                                    itype, dtype, iprecision,
                                    &layer, &buffer);
        }
      else
        {
          layer = gimp_layer_new (image, _("FITS HDU"), width, height,
                                  dtype, 100,
                                  gimp_image_get_default_new_layer_mode (image));
          gimp_image_insert_layer (image, layer, NULL, 0);
          buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
        }

      row_length = width * channels;

      /* Calculate min/max pixel value for normalizing */
      for (fpixel[1] = height; fpixel[1] >= 1; fpixel[1]--)
        {
          if (fits_read_pix (ifp, TDOUBLE, fpixel, row_length, NULL,
                             pixels, NULL, &status))
            break;

          for (gint ii = 0; ii < row_length; ii++)
            {
              if (pixels[ii] < datamin)
                datamin = pixels[ii];

              if (pixels[ii] > datamax)
                datamax = pixels[ii];
            }
        }

      if (status)
        show_fits_errors (status);

      /* Read pixel values in */
      if (hdu.naxis == 2)
        {
          for (fpixel[1] = height; fpixel[1] >= 1; fpixel[1]--)
            {
              if (fits_read_pix (ifp, TDOUBLE, fpixel, row_length, &replace_val,
                                 pixels, NULL, &status))
                break;

              if (datamin < datamax)
                {
                  for (gint ii = 0; ii < row_length; ii++)
                    pixels[ii] = (pixels[ii] - datamin) / (datamax - datamin);
                }

              gegl_buffer_set (buffer,
                               GEGL_RECTANGLE (0, height - fpixel[1],
                                               width, 1), 0,
                               format, pixels, GEGL_AUTO_ROWSTRIDE);
            }
        }
      else if (hdu.naxisn[2] && hdu.naxisn[2] == 3)
        {
          gint total_size = width * height * channels;

          fits_read_img (ifp, TDOUBLE, 1, total_size, &replace_val,
                         pixels, NULL, &status);

          if (! status)
            {
              gdouble *temp;

              temp = (gdouble *) malloc (width * height * sizeof (gdouble) * channels);

              if (datamin < datamax)
                {
                  for (gint ii = 0; ii < total_size; ii++)
                    pixels[ii] = (pixels[ii] - datamin) / (datamax - datamin);
                }

              for (gint ii = 0; ii < (total_size / 3); ii++)
                {
                  temp[(ii * 3)]     = pixels[ii];
                  temp[(ii * 3) + 1] = pixels[ii + (total_size / 3)];
                  temp[(ii * 3) + 2] = pixels[ii + ((total_size / 3) * 2)];
                }

              gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                               format, temp, GEGL_AUTO_ROWSTRIDE);

              if (temp)
                g_free (temp);
            }
        }

      if (status)
        show_fits_errors (status);

      g_object_unref (buffer);

      if (hdu.naxisn[2] && hdu.naxisn[2] == 3)
        {
          /* Per SiriL developers, FITS images should be loaded from the
           * bottom up. fits_read_img () loads them from top down, so we
           * should flip the layer. */
          gimp_item_transform_flip_simple (GIMP_ITEM (layer),
                                           GIMP_ORIENTATION_VERTICAL,
                                           TRUE, -1.0);
        }

      if (pixels)
        g_free (pixels);

      count++;
    }

  if (! image)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", _("FITS file keeps no displayable images"));
      fits_close_file (ifp, &status);
      return NULL;
    }

  /* As there might be different sized layers,
   * we need to resize the canvas afterwards */
  gimp_image_resize_to_layers (image);

  fits_close_file (ifp, &status);

  return image;
}

static gint
export_image (GFile         *file,
              GimpImage     *image,
              GimpDrawable  *drawable,
              GError       **error)
{
  GimpImageType  drawable_type;
  gint           retval;

  drawable_type = gimp_drawable_type (drawable);

  /*  Make sure we're not exporting an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s",
                   _("FITS export cannot handle images with alpha channels"));
      return FALSE;
    }

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE: case GIMP_INDEXEDA_IMAGE:
    case GIMP_GRAY_IMAGE:    case GIMP_GRAYA_IMAGE:
    case GIMP_RGB_IMAGE:     case GIMP_RGBA_IMAGE:
      break;
    default:
      g_message (_("Cannot operate on unknown image types."));
      return (FALSE);
      break;
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));



  retval = export_fits (file, image, drawable);

  return retval;
}

/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static GimpImage *
create_new_image (GFile              *file,
                  guint               pagenum,
                  guint               width,
                  guint               height,
                  GimpImageBaseType   itype,
                  GimpImageType       dtype,
                  GimpPrecision       iprecision,
                  GimpLayer         **layer,
                  GeglBuffer        **buffer)
{
  GimpImage *image;

  image = gimp_image_new_with_precision (width, height, itype, iprecision);

  gimp_image_undo_disable (image);
  *layer = gimp_layer_new (image, _("Background"), width, height,
                           dtype, 100,
                           gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, *layer, NULL, 0);

  *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (*layer));

  return image;
}

/* Export direct colors (GRAY, GRAYA, RGB, RGBA) */
static gint
export_fits (GFile        *file,
             GimpImage    *image,
             GimpDrawable *drawable)
{
  fitsfile      *fptr;
  gint           status = 0;
  gint           height, width, channelnum;
  gint           bitpix;
  gint           naxis  = 2;
  glong          naxes[3];
  gint           export_type;
  gint           nelements;
  void          *data   = NULL;
  GeglBuffer    *buffer;
  const Babl    *format, *type;

  buffer = gimp_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  format = gegl_buffer_get_format (buffer);
  type   = babl_format_get_type (format, 0);

  naxes[0]  = width;
  naxes[1]  = height;
  nelements = width * height;

  if (type == babl_type ("u8"))
    {
      bitpix      = 8;
      export_type = TBYTE;
    }
  else if (type == babl_type ("u16"))
    {
      bitpix      = 16;
      export_type = TSHORT;
    }
  else if (type == babl_type ("u32"))
    {
      bitpix      = 32;
      export_type = TLONG;
    }
  else if (type == babl_type ("half") ||
           type == babl_type ("float"))
    {
      bitpix      = -32;
      type        = babl_type ("float");
      export_type = TFLOAT;
    }
  else if (type == babl_type ("double"))
    {
      bitpix      = -64;
      export_type = TDOUBLE;
    }
  else
    {
      return FALSE;
    }

  switch (gimp_drawable_type (drawable))
    {
    case GIMP_GRAY_IMAGE:
      format = babl_format_new (babl_model ("Y'"),
                                type,
                                babl_component ("Y'"),
                                NULL);
      break;

    case GIMP_GRAYA_IMAGE:
      format = babl_format_new (babl_model ("Y'A"),
                                type,
                                babl_component ("Y'"),
                                babl_component ("A"),
                                NULL);
      break;

    case GIMP_RGB_IMAGE:
    case GIMP_INDEXED_IMAGE:
      format = babl_format_new (babl_model ("R'G'B'"),
                                type,
                                babl_component ("R'"),
                                babl_component ("G'"),
                                babl_component ("B'"),
                                NULL);
      naxis    = 3;
      naxes[2] = 3;
      break;

    case GIMP_RGBA_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      format = babl_format_new (babl_model ("R'G'B'A"),
                                type,
                                babl_component ("R'"),
                                babl_component ("G'"),
                                babl_component ("B'"),
                                babl_component ("A"),
                                NULL);
      naxis    = 4;
      naxes[2] = 4;
      break;
    }

  channelnum = babl_format_get_n_components (format);

  /* allocate a buffer for retrieving information from the pixel region  */
  if (export_type == TFLOAT)
    data = (gfloat *) g_malloc (width * height * sizeof (gfloat) * channelnum);
  else if (export_type == TDOUBLE)
    data = (gdouble *) g_malloc (width * height * sizeof (gdouble) *
                                 channelnum);
  else
    data = (guchar *) g_malloc (width * height * sizeof (guchar *) *
                                (bitpix / 8) * channelnum);

  /* CFITSIO can't overwrite files unless you start the filename
   * with a "!". Instead, we'll try to open the existing file
   * in READWRITE mode, clear it, and then recreate it.
   */
  if (fits_create_file (&fptr, g_file_peek_path (file), &status))
    {
      /* You have to set status back to 0 - subsequent successful
         functions do not remove the error value */
      status = 0;

      if (fits_open_file (&fptr, g_file_peek_path (file), READWRITE, &status))
        {
          show_fits_errors (status);
          return FALSE;
        }
      fits_delete_file (fptr, &status);

      if (fits_create_file (&fptr, g_file_peek_path (file), &status))
        {
          show_fits_errors (status);
          return FALSE;
        }
    }

  if (fits_create_img (fptr, bitpix, naxis, naxes, &status))
    {
      show_fits_errors (status);
      return FALSE;
    }

  /* FITS uses signed 16/32 integers, so we need to convert the unsigned
   * values to that range via an offset */
  if (bitpix == 16 || bitpix == 32)
    {
      GeglBufferIterator *iter;

      iter = gegl_buffer_iterator_new (buffer,
                                       GEGL_RECTANGLE (0, 0, width, height), 0,
                                       format,
                                       GEGL_BUFFER_READWRITE,
                                       GEGL_ABYSS_NONE, 1);

      while (gegl_buffer_iterator_next (iter))
        {
          gint length = iter->length;

          if (bitpix == 16)
            {
              gushort *pixel  = iter->items[0].data;
              gushort  offset = pow (2, 15);

              while (length--)
                {
                  for (gint i = 0; i < channelnum; i++)
                    pixel[i] -= offset;

                  pixel += channelnum;
                }
            }
          else
            {
              guint32 *pixel = iter->items[0].data;
              guint32  offset = pow (2, 31);

              while (length--)
                {
                  for (gint i = 0; i < channelnum; i++)
                    pixel[i] -= offset;

                  pixel += channelnum;
                }
            }
        }
    }

  /* Grayscale images can be exported as-is. RGB images must be
   * converted into planes of RRR...GGG...BBB... */
  if (naxis == 2)
    {
      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, 0, width, height), 1.0,
                       format, data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (fits_write_img (fptr, export_type, 1, nelements, data, &status))
        {
          show_fits_errors (status);
          return FALSE;
        }
    }
  else
    {
      glong       fpixel[3]     = {1, 1, 1};
      gdouble    *rgb_data;
      gdouble    *rgb_output;
      const Babl *rgb_format;
      const Babl *output_format = babl_format ("Y' double");
      const Babl *converted_format;


      rgb_format = (channelnum == 3) ? babl_format ("R'G'B' double") :
                                       babl_format ("R'G'B'A double");

      /* We export a single channel at a time, so we need a
       * an output format with a single channel */
      converted_format = babl_format_new (babl_model ("Y'"), type,
                                          babl_component ("Y'"),
                                          NULL);

      rgb_data = (gdouble *) g_malloc (width * height * sizeof (gdouble) *
                                       channelnum);
      rgb_output = (gdouble *) g_malloc (width * height * sizeof (gdouble));
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
                       rgb_format, rgb_data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (gint rgb = 0; rgb < channelnum; rgb++)
        {
          gint  offset           = 0;
          gint  src_offset       = 0;
          void *converted_output = NULL;

          for (gint i = 0; i < height - 1; i++)
            {
              for (gint j = 0; j < (width * channelnum); j += channelnum)
                {
                  rgb_output[(j / channelnum) + offset] =
                    rgb_data[j + src_offset + rgb];
                }

              src_offset += width * channelnum;
              offset   += width;
            }

          if (export_type == TFLOAT)
            converted_output = (gfloat *) g_malloc (width * height *
                                                    sizeof (gfloat));
          else if (export_type == TDOUBLE)
            converted_output = (gdouble *) g_malloc (width * height *
                                                     sizeof (gdouble));
          else
            converted_output = (guchar *) g_malloc (width * height    *
                                                    sizeof (guchar *) *
                                                    (bitpix / 8));

          babl_process (babl_fish (output_format, converted_format),
                        rgb_output, converted_output, nelements);

          if (fits_write_pix (fptr, export_type, fpixel, nelements,
                              converted_output, &status))
            {
              show_fits_errors (status);
              return FALSE;
            }
          fpixel[2]++;

          g_free (converted_output);
        }

      g_free (rgb_data);
      g_free (rgb_output);
    }

  g_free (data);

  g_object_unref (buffer);

  /* Add history of file update */
  fits_write_history (fptr,
                      "THIS FITS FILE WAS GENERATED BY GIMP USING CFITSIO",
                      &status);

  gimp_progress_update (1.0);

  if (fits_close_file (fptr, &status))
    {
      show_fits_errors (status);
      return FALSE;
    }

  return TRUE;
}


/*  Load interface functions  */

static gboolean
load_dialog (GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Open FITS File"));

  frame = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                            "replace", GIMP_TYPE_INT_RADIO_FRAME);
  gtk_widget_set_margin_bottom (frame, 12);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
show_fits_errors (gint status)
{
  gchar status_str[FLEN_STATUS];

  /* Write out error messages of FITS-Library */
  fits_get_errstatus (status, status_str);
  g_message ("FITS: %s\n", status_str);
}
