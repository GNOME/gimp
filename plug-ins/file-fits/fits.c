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

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "fits-io.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-fits-load"
#define SAVE_PROC      "file-fits-save"
#define PLUG_IN_BINARY "file-fits"
#define PLUG_IN_ROLE   "gimp-file-fits"


/* Load info */
typedef struct
{
  gint replace;     /* replacement for blank/NaN-values    */
  gint use_datamin; /* Use DATAMIN/MAX-scaling if possible */
  gint compose;     /* compose images with naxis==3        */
} FITSLoadVals;


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
#define FITS (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FITS_TYPE, Fits))

GType                   fits_get_type         (void) G_GNUC_CONST;

static GList          * fits_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * fits_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * fits_load             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);
static GimpValueArray * fits_save             (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               GFile                *file,
                                               const GimpValueArray *args,
                                               gpointer              run_data);

static GimpImage      * load_image            (const gchar        *filename,
                                               GError            **error);
static gint             save_image            (const gchar        *filename,
                                               GimpImage          *image,
                                               GimpDrawable       *drawable,
                                               GError            **error);

static FitsHduList    * create_fits_header    (FitsFile           *ofp,
                                               guint               width,
                                               guint               height,
                                               guint               channels,
                                               guint               bitpix);

static gint             save_fits             (FitsFile           *ofp,
                                               GimpImage          *image,
                                               GimpDrawable       *drawable);

static GimpImage      * create_new_image      (const gchar        *filename,
                                               guint               pagenum,
                                               guint               width,
                                               guint               height,
                                               GimpImageBaseType   itype,
                                               GimpImageType       dtype,
                                               GimpPrecision       iprecision,
                                               GimpLayer         **layer,
                                               GeglBuffer        **buffer);

static void             check_load_vals       (void);

static GimpImage      * load_fits             (const gchar        *filename,
                                               FitsFile           *ifp,
                                               guint               picnum,
                                               guint               ncompose);

static gboolean         load_dialog           (void);
static void             show_fits_errors      (void);


G_DEFINE_TYPE (Fits, fits, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FITS_TYPE)


static FITSLoadVals plvals =
{
  0,        /* Replace with black */
  0,        /* Do autoscale on pixel-values */
  0         /* Don't compose images */
};

static GimpRunMode l_run_mode;


static void
fits_class_init (FitsClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = fits_query_procedures;
  plug_in_class->create_procedure = fits_create_procedure;
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
  list = g_list_append (list, g_strdup (SAVE_PROC));

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
                                     N_("Flexible Image Transport System"));

      gimp_procedure_set_documentation (procedure,
                                        "Load file of the FITS file format",
                                        "Load file of the FITS file format "
                                        "(Flexible Image Transport System)",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner (peter@kirchgessner.net)",
                                      "1997");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-fits");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "fit,fits");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,SIMPLE");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           fits_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB, GRAY, INDEXED");

      gimp_procedure_set_menu_label (procedure,
                                     N_("Flexible Image Transport System"));

      gimp_procedure_set_documentation (procedure,
                                        "Export file in the FITS file format",
                                        "FITS exporting handles all image "
                                        "types except those with alpha channels.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner (peter@kirchgessner.net)",
                                      "1997");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-fits");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "fit,fits");
    }

  return procedure;
}

static GimpValueArray *
fits_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  l_run_mode = run_mode;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (LOAD_PROC, &plvals);

      if (! load_dialog ())
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (LOAD_PROC, &plvals);
      break;

    default:
      break;
    }

  check_load_vals ();

  image = load_image (g_file_get_path (file), &error);

  /* Write out error messages of FITS-Library */
  show_fits_errors ();

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  gimp_set_data (LOAD_PROC, &plvals, sizeof (FITSLoadVals));

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
fits_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable         *drawable,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  l_run_mode = run_mode;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY, FALSE);

      export = gimp_export_image (&image, &drawable, "FITS",
                                  GIMP_EXPORT_CAN_HANDLE_RGB  |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (! save_image (g_file_get_path (file),
                    image, drawable,
                    &error))
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpImage *
load_image (const gchar  *filename,
            GError      **error)
{
  GimpImage   *image;
  GimpImage  **image_list;
  GimpImage  **nl;
  guint        picnum;
  gint         k, n_images, max_images, hdu_picnum;
  gint         compose;
  FILE        *fp;
  FitsFile    *ifp;
  FitsHduList *hdu;

  fp = g_fopen (filename, "rb");
  if (!fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }
  fclose (fp);

  ifp = fits_open (filename, "r");
  if (ifp == NULL)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", _("Error during open of FITS file"));
      return NULL;
    }
  if (ifp->n_pic <= 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", _("FITS file keeps no displayable images"));
      fits_close (ifp);
      return NULL;
    }

  image_list = g_new (GimpImage *, 10);
  n_images = 0;
  max_images = 10;

  for (picnum = 1; picnum <= ifp->n_pic; )
    {
      /* Get image info to see if we can compose them */
      hdu = fits_image_info (ifp, picnum, &hdu_picnum);
      if (hdu == NULL)
        break;

      /* Get number of FITS-images to compose */
      compose = (plvals.compose && (hdu_picnum == 1) && (hdu->naxis == 3) &&
                 (hdu->naxisn[2] > 1) && (hdu->naxisn[2] <= 4));

      if (compose)
        compose = hdu->naxisn[2];
      else
        compose = 1;  /* Load as GRAY */

      image = load_fits (filename, ifp, picnum, compose);

      /* Write out error messages of FITS-Library */
      show_fits_errors ();

      if (! image)
        break;

      if (n_images == max_images)
        {
          nl = (GimpImage **) g_realloc (image_list,
                                         (max_images + 10) * sizeof (GimpImage *));
          if (nl == NULL)
            break;

          image_list = nl;
          max_images += 10;
        }

      image_list[n_images++] = image;

      picnum += compose;
    }

  /* Write out error messages of FITS-Library */
  show_fits_errors ();

  fits_close (ifp);

  /* Display images in reverse order. The last will be displayed by GIMP itself*/
  if (l_run_mode != GIMP_RUN_NONINTERACTIVE)
    {
      for (k = n_images-1; k >= 1; k--)
        {
          gimp_image_undo_enable (image_list[k]);
          gimp_image_clean_all (image_list[k]);
          gimp_display_new (image_list[k]);
        }
    }

  image = (n_images > 0) ? image_list[0] : NULL;
  g_free (image_list);

  return image;
}

static gint
save_image (const gchar   *filename,
            GimpImage     *image,
            GimpDrawable  *drawable,
            GError       **error)
{
  FitsFile      *ofp;
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
                             gimp_filename_to_utf8 (filename));

  /* Open the output file. */
  ofp = fits_open (filename, "w");
  if (!ofp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return (FALSE);
    }

  retval = save_fits (ofp, image, drawable);

  fits_close (ofp);

  return retval;
}


/* Check (and correct) the load values plvals */
static void
check_load_vals (void)
{
  if (plvals.replace > 255) plvals.replace = 255;
}


/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static GimpImage *
create_new_image (const gchar        *filename,
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
  gchar     *tmp;

  image = gimp_image_new_with_precision (width, height, itype, iprecision);

  if ((tmp = g_malloc (strlen (filename) + 64)) != NULL)
    {
      sprintf (tmp, "%s-img%ld", filename, (long)pagenum);
      gimp_image_set_filename (image, tmp);
      g_free (tmp);
    }
  else
    gimp_image_set_filename (image, filename);

  gimp_image_undo_disable (image);
  *layer = gimp_layer_new (image, _("Background"), width, height,
                           dtype, 100,
                           gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, *layer, NULL, 0);

  *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (*layer));

  return image;
}


/* Load FITS image. ncompose gives the number of FITS-images which have
 * to be composed together. This will result in different GIMP image types:
 * 1: GRAY, 2: GRAYA, 3: RGB, 4: RGBA
 */
static GimpImage *
load_fits (const gchar *filename,
           FitsFile    *ifp,
           guint        picnum,
           guint        ncompose)
{
  register guchar   *dest, *src;
  guchar            *data, *data_end, *linebuf;
  int                width, height, tile_height, scan_lines;
  int                i, j, max_scan;
  double             a, b;
  GimpImage         *image;
  GimpLayer         *layer;
  GeglBuffer        *buffer;
  GimpImageBaseType  itype;
  GimpImageType      dtype;
  GimpPrecision      iprecision;
  gint               err = 0;
  FitsHduList       *hdulist;
  FitsPixTransform   trans;
  double             datamax, replacetransform;
  const Babl        *type, *format;

  hdulist = fits_seek_image (ifp, (int)picnum);
  if (hdulist == NULL)
    return NULL;

  width  = hdulist->naxisn[0];  /* Set the size of the FITS image */
  height = hdulist->naxisn[1];

  switch (hdulist->bitpix)
    {
    case 8:
      iprecision = GIMP_PRECISION_U8_NON_LINEAR;
      type = babl_type ("u8");
      datamax = 255.0;
      replacetransform = 1.0;
      break;
    case 16:
      iprecision = GIMP_PRECISION_U16_NON_LINEAR; /* FIXME precision */
      type = babl_type ("u16");
      datamax = 65535.0;
      replacetransform = 257;
      break;
    case 32:
      iprecision = GIMP_PRECISION_U32_LINEAR;
      type = babl_type ("u32");
      datamax = 4294967295.0;
      replacetransform = 16843009;
      break;
    case -32:
      iprecision = GIMP_PRECISION_FLOAT_LINEAR;
      type = babl_type ("float");
      datamax = 1.0;
      replacetransform = 1.0 / 255.0;
      break;
    case -64:
      iprecision = GIMP_PRECISION_DOUBLE_LINEAR;
      type = babl_type ("double");
      datamax = 1.0;
      replacetransform = 1.0 / 255.0;
      break;
    default:
      return NULL;
    }

  if (ncompose == 2)
    {
      itype = GIMP_GRAY;
      dtype = GIMP_GRAYA_IMAGE;
      format = babl_format_new (babl_model ("Y'A"),
                                type,
                                babl_component ("Y'"),
                                babl_component ("A"),
                                NULL);
    }
  else if (ncompose == 3)
    {
      itype = GIMP_RGB;
      dtype = GIMP_RGB_IMAGE;
      format = babl_format_new (babl_model ("R'G'B'"),
                                type,
                                babl_component ("R'"),
                                babl_component ("G'"),
                                babl_component ("B'"),
                                NULL);
    }
  else if (ncompose == 4)
    {
      itype = GIMP_RGB;
      dtype = GIMP_RGBA_IMAGE;
      format = babl_format_new (babl_model ("R'G'B'A"),
                                type,
                                babl_component ("R'"),
                                babl_component ("G'"),
                                babl_component ("B'"),
                                babl_component ("A"),
                                NULL);
    }
  else
    {
      ncompose = 1;
      itype = GIMP_GRAY;
      dtype = GIMP_GRAY_IMAGE;
      format = babl_format_new (babl_model ("Y'"),
                                type,
                                babl_component ("Y'"),
                                NULL);
    }

  image = create_new_image (filename, picnum, width, height,
                            itype, dtype, iprecision,
                            &layer, &buffer);

  tile_height = gimp_tile_height ();

  data = g_malloc (tile_height * width * ncompose * hdulist->bpp);
  if (data == NULL)
    return NULL;

  data_end = data + tile_height * width * ncompose * hdulist->bpp;

  /* If the transformation from pixel value to data value has been
   * specified, use it
   */
  if (plvals.use_datamin    &&
      hdulist->used.datamin && hdulist->used.datamax &&
      hdulist->used.bzero   && hdulist->used.bscale)
    {
      a = (hdulist->datamin - hdulist->bzero) / hdulist->bscale;
      b = (hdulist->datamax - hdulist->bzero) / hdulist->bscale;

      if (a < b)
        trans.pixmin = a, trans.pixmax = b;
      else
        trans.pixmin = b, trans.pixmax = a;
    }
  else
    {
      trans.pixmin = hdulist->pixmin;
      trans.pixmax = hdulist->pixmax;
    }

  trans.datamin     = 0.0;
  trans.datamax     = datamax;
  trans.replacement = plvals.replace * replacetransform;
  trans.dsttyp      = 'k';

  /* FITS stores images with bottom row first. Therefore we have to
   * fill the image from bottom to top.
   */

  if (ncompose == 1)
    {
      dest = data + tile_height * width * hdulist->bpp;
      scan_lines = 0;

      for (i = 0; i < height; i++)
        {
          /* Read FITS line */
          dest -= width * hdulist->bpp;
          if (fits_read_pixel (ifp, hdulist, width, &trans, dest) != width)
            {
              err = 1;
              break;
            }

          scan_lines++;

          if ((i % 20) == 0)
            gimp_progress_update ((gdouble) (i + 1) / (gdouble) height);

          if ((scan_lines == tile_height) || ((i + 1) == height))
            {
              gegl_buffer_set (buffer,
                               GEGL_RECTANGLE (0, height - i - 1,
                                               width, scan_lines), 0,
                               format, dest, GEGL_AUTO_ROWSTRIDE);

              scan_lines = 0;
              dest = data + tile_height * width * hdulist->bpp;
            }

          if (err)
            break;
        }
    }
  else   /* multiple images to compose */
    {
      gint channel;

      linebuf = g_malloc (width * hdulist->bpp);
      if (linebuf == NULL)
        return NULL;

      for (channel = 0; channel < ncompose; channel++)
        {
          dest = data + tile_height * width * hdulist->bpp * ncompose + channel * hdulist->bpp;
          scan_lines = 0;

          for (i = 0; i < height; i++)
            {
              if ((channel > 0) && ((i % tile_height) == 0))
                {
                  /* Reload a region for follow up channels */
                  max_scan = tile_height;

                  if (i + tile_height > height)
                    max_scan = height - i;

                  gegl_buffer_get (buffer,
                                   GEGL_RECTANGLE (0, height - i - max_scan,
                                                   width, max_scan), 1.0,
                                   format, data_end - max_scan * width * hdulist->bpp * ncompose,
                                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
                }

              /* Read FITS scanline */
              dest -= width * ncompose * hdulist->bpp;
              if (fits_read_pixel (ifp, hdulist, width, &trans, linebuf) != width)
                {
                  err = 1;
                  break;
                }
              j = width;
              src = linebuf;
              while (j--)
                {
                  memcpy (dest, src, hdulist->bpp);
                  src += hdulist->bpp;
                  dest += ncompose * hdulist->bpp;
                }
              dest -= width * ncompose * hdulist->bpp;
              scan_lines++;

              if ((i % 20) == 0)
                gimp_progress_update ((gdouble) (channel * height + i + 1) /
                                      (gdouble) (height * ncompose));

              if ((scan_lines == tile_height) || ((i + 1) == height))
                {
                  gegl_buffer_set (buffer,
                                   GEGL_RECTANGLE (0, height - i - 1,
                                                   width, scan_lines), 0,
                                   format, dest - channel * hdulist->bpp, GEGL_AUTO_ROWSTRIDE);

                  scan_lines = 0;
                  dest = data + tile_height * width * ncompose * hdulist->bpp + channel * hdulist->bpp;
                }

              if (err)
                break;
            }
        }

      g_free (linebuf);
    }

  g_free (data);

  if (err)
    g_message (_("EOF encountered on reading"));

  g_object_unref (buffer);

  gimp_progress_update (1.0);

  return err ? NULL : image;
}


static FitsHduList *
create_fits_header (FitsFile *ofp,
                    guint     width,
                    guint     height,
                    guint     channels,
                    guint     bitpix)
{
  FitsHduList *hdulist;
  gint         print_ctype3 = 0; /* The CTYPE3-card may not be FITS-conforming */

  static const char *ctype3_card[] =
  {
    NULL, NULL, NULL,  /* bpp = 0: no additional card */
    "COMMENT Image type within GIMP: GIMP_GRAY_IMAGE",
    NULL,
    NULL,
    "COMMENT Image type within GIMP: GIMP_GRAYA_IMAGE (gray with alpha channel)",
    "COMMENT Sequence for NAXIS3   : GRAY, ALPHA",
    "CTYPE3  = 'GRAYA   '           / GRAY IMAGE WITH ALPHA CHANNEL",
    "COMMENT Image type within GIMP: GIMP_RGB_IMAGE",
    "COMMENT Sequence for NAXIS3   : RED, GREEN, BLUE",
    "CTYPE3  = 'RGB     '           / RGB IMAGE",
    "COMMENT Image type within GIMP: GIMP_RGBA_IMAGE (rgb with alpha channel)",
    "COMMENT Sequence for NAXIS3   : RED, GREEN, BLUE, ALPHA",
    "CTYPE3  = 'RGBA    '           / RGB IMAGE WITH ALPHA CHANNEL"
  };

  hdulist = fits_add_hdu (ofp);
  if (hdulist == NULL)
    return NULL;

  hdulist->used.simple  = 1;
  hdulist->bitpix       = bitpix;
  hdulist->naxis        = (channels == 1) ? 2 : 3;
  hdulist->naxisn[0]    = width;
  hdulist->naxisn[1]    = height;
  hdulist->naxisn[2]    = channels;
  hdulist->used.datamin = 1;
  hdulist->datamin      = 0.0;
  hdulist->used.datamax = 1;
  hdulist->used.bzero   = 1;
  hdulist->bzero        = 0.0;
  hdulist->used.bscale  = 1;
  hdulist->bscale       = 1.0;

  switch (bitpix)
    {
    case 8:
      hdulist->datamax = 255;
      break;
    case 16:
      hdulist->datamax = 65535;
      break;
    case 32:
      hdulist->datamax = 4294967295.0; /* .0 to silence gcc */
      break;
    case -32:
      hdulist->datamax = 1.0;
      break;
    case -64:
      hdulist->datamax = 1.0;
      break;
    default:
      return NULL;
    }

  fits_add_card (hdulist, "");
  fits_add_card (hdulist,
                 "HISTORY THIS FITS FILE WAS GENERATED BY GIMP USING FITSRW");
  fits_add_card (hdulist, "");
  fits_add_card (hdulist,
                 "COMMENT FitsRW is (C) Peter Kirchgessner (peter@kirchgessner.net), but available");
  fits_add_card (hdulist,
                 "COMMENT under the GNU general public licence.");
  fits_add_card (hdulist,
                 "COMMENT For sources see http://www.kirchgessner.net");
  fits_add_card (hdulist, "");
  fits_add_card (hdulist, ctype3_card[channels * 3]);

  if (ctype3_card[channels * 3 + 1] != NULL)
    fits_add_card (hdulist, ctype3_card[channels * 3 + 1]);

  if (print_ctype3 && (ctype3_card[channels * 3 + 2] != NULL))
    fits_add_card (hdulist, ctype3_card[channels * 3 + 2]);

  fits_add_card (hdulist, "");

  return hdulist;
}


/* Save direct colors (GRAY, GRAYA, RGB, RGBA) */
static gint
save_fits (FitsFile     *ofp,
           GimpImage    *image,
           GimpDrawable *drawable)
{
  gint           height, width, i, j, channel, channelnum;
  gint           tile_height, bpp, bpsl, bitpix, bpc;
  long           nbytes;
  guchar        *data, *src;
  GeglBuffer    *buffer;
  const Babl    *format, *type;
  FitsHduList   *hdu;

  buffer = gimp_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  format = gegl_buffer_get_format (buffer);
  type   = babl_format_get_type (format, 0);

  if (type == babl_type ("u8"))
    {
      bitpix = 8;
    }
  else if (type == babl_type ("u16"))
    {
      bitpix = 16;
    }
  else if (type == babl_type ("u32"))
    {
      bitpix = 32;
    }
  else if (type == babl_type ("half"))
    {
      bitpix = -32;
      type = babl_type ("float");
    }
  else if (type == babl_type ("float"))
    {
      bitpix = -32;
    }
  else if (type == babl_type ("double"))
    {
      bitpix = -64;
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
      break;
    }

  channelnum = babl_format_get_n_components (format);
  bpp        = babl_format_get_bytes_per_pixel (format);

  bpc  = bpp / channelnum; /* Bytes per channel */
  bpsl = width * bpp;      /* Bytes per scanline */

  tile_height = gimp_tile_height ();

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = (guchar *) g_malloc (width * height * bpp);

  hdu = create_fits_header (ofp, width, height, channelnum, bitpix);
  if (hdu == NULL)
    return FALSE;

  if (fits_write_header (ofp, hdu) < 0)
    return FALSE;

  nbytes = 0;
  for (channel = 0; channel < channelnum; channel++)
    {
      for (i = 0; i < height; i++)
        {
          if ((i % tile_height) == 0)
            {
              gint scan_lines;

              scan_lines = (i + tile_height-1 < height) ?
                            tile_height : (height - i);

              gegl_buffer_get (buffer,
                               GEGL_RECTANGLE (0, height - i - scan_lines,
                                               width, scan_lines), 1.0,
                               format, data,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

              src = data + bpsl * (scan_lines - 1) + channel * bpc;
            }

          if (channelnum == 1 && bitpix == 8)  /* One channel and 8 bit? Write the scanline */
            {
              fwrite (src, bpc, width, ofp->fp);
              src += bpsl;
            }
          else           /* Multiple channels or high bit depth */
            {
            /* Write out bytes for current channel */
            /* FIXME: Don't assume a little endian arch */
            switch (bitpix)
              {
              case 8:
                for (j = 0; j < width; j++)
                  {
                    putc (*src, ofp->fp);
                    src += bpp;
                  }
                break;
              case 16:
                for (j = 0; j < width; j++)
                  {
                    *((guint16*)src) += 32768;
                    putc (*(src + 1), ofp->fp);
                    putc (*(src + 0), ofp->fp);
                    src += bpp;
                  }
                break;
              case 32:
                for (j = 0; j < width; j++)
                  {
                    *((guint32*)src) += 2147483648.0; /* .0 to silence gcc */
                    putc (*(src + 3), ofp->fp);
                    putc (*(src + 2), ofp->fp);
                    putc (*(src + 1), ofp->fp);
                    putc (*(src + 0), ofp->fp);
                    src += bpp;
                  }
                break;
              case -32:
                for (j = 0; j < width; j++)
                  {
                    putc (*(src + 3), ofp->fp);
                    putc (*(src + 2), ofp->fp);
                    putc (*(src + 1), ofp->fp);
                    putc (*(src + 0), ofp->fp);
                    src += bpp;
                  }
                break;
              case -64:
                for (j = 0; j < width; j++)
                  {
                    putc (*(src + 7), ofp->fp);
                    putc (*(src + 6), ofp->fp);
                    putc (*(src + 5), ofp->fp);
                    putc (*(src + 4), ofp->fp);
                    putc (*(src + 3), ofp->fp);
                    putc (*(src + 2), ofp->fp);
                    putc (*(src + 1), ofp->fp);
                    putc (*(src + 0), ofp->fp);
                    src += bpp;
                  }
                break;
              default:
                return FALSE;
              }
            }

          nbytes += width * bpc;
          src -= 2 * bpsl;

          if ((i % 20) == 0)
            gimp_progress_update ((gdouble) (i + channel * height) /
                                  (gdouble) (height * channelnum));
        }
    }

  nbytes = nbytes % FITS_RECORD_SIZE;
  if (nbytes)
    {
      while (nbytes++ < FITS_RECORD_SIZE)
        putc (0, ofp->fp);
    }

  g_free (data);

  g_object_unref (buffer);

  gimp_progress_update (1.0);

  if (ferror (ofp->fp))
    {
      g_message (_("Write error occurred"));
      return FALSE;
    }

  return TRUE;
}


/*  Load interface functions  */

static gboolean
load_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *frame;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Load FITS File"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Open"),   GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gimp_int_radio_group_new (TRUE, _("Replacement for undefined pixels"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &plvals.replace, plvals.replace,

                                    _("Black"), 0,   NULL,
                                    _("White"), 255, NULL,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame =
    gimp_int_radio_group_new (TRUE, _("Pixel value scaling"),
                              G_CALLBACK (gimp_radio_button_update),
                              &plvals.use_datamin, plvals.use_datamin,

                              _("Automatic"),          FALSE, NULL,
                              _("By DATAMIN/DATAMAX"), TRUE,  NULL,

                              NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame =
    gimp_int_radio_group_new (TRUE, _("Image Composing"),
                              G_CALLBACK (gimp_radio_button_update),
                              &plvals.compose, plvals.compose,

                              C_("composing", "None"),   FALSE, NULL,
                              "NAXIS=3, NAXIS3=2,...,4", TRUE,  NULL,

                              NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
show_fits_errors (void)
{
  const gchar *msg;

  /* Write out error messages of FITS-Library */
  while ((msg = fits_get_error ()) != NULL)
    g_message ("%s", msg);
}
