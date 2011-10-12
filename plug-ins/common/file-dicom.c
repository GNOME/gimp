/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * PNM reading and writing code Copyright (C) 1996 Erik Nygren
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

/*
 * The dicom reading and writing code was written from scratch
 * by Dov Grobgeld.  (dov@imagic.weizman.ac.il).
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <time.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-dicom-load"
#define SAVE_PROC      "file-dicom-save"
#define PLUG_IN_BINARY "file-dicom"
#define PLUG_IN_ROLE   "gimp-file-dicom"


/* A lot of Dicom images are wrongly encoded. By guessing the endian
 * we can get around this problem.
 */
#define GUESS_ENDIAN 1

/* Declare local data types */
typedef struct _DicomInfo
{
  guint      width, height;      /* The size of the image                  */
  gint       maxval;             /* For 16 and 24 bit image files, the max
                                    value which we need to normalize to    */
  gint       samples_per_pixel;  /* Number of image planes (0 for pbm)     */
  gint       bpp;
  gint       bits_stored;
  gint       high_bit;
  gboolean   is_signed;
} DicomInfo;

/* Local function prototypes */
static void      query                 (void);
static void      run                   (const gchar      *name,
                                        gint              nparams,
                                        const GimpParam  *param,
                                        gint             *nreturn_vals,
                                        GimpParam       **return_vals);
static gint32    load_image            (const gchar      *filename,
                                        GError          **error);
static gboolean  save_image            (const gchar      *filename,
                                        gint32            image_ID,
                                        gint32            drawable_ID,
                                        GError          **error);
static void      dicom_loader          (guint8           *pix_buf,
                                        DicomInfo        *info,
                                        GimpPixelRgn     *pixel_rgn);
static void      guess_and_set_endian2 (guint16          *buf16,
                                        gint              length);
static void      toggle_endian2        (guint16          *buf16,
                                        gint              length);
static void      add_tag_pointer       (GByteArray       *group_stream,
                                        gint              group,
                                        gint              element,
                                        const gchar      *value_rep,
                                        const guint8     *data,
                                        gint              length);
static GSList *  dicom_add_tags        (FILE             *DICOM,
                                        GByteArray       *group_stream,
                                        GSList           *elements);
static gboolean  write_group_to_file   (FILE             *DICOM,
                                        gint              group,
                                        GByteArray       *group_stream);


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
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,    "image",        "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save" },
  };

  gimp_install_procedure (LOAD_PROC,
                          "loads files of the dicom file format",
                          "Load a file in the DICOM standard format."
                          "The standard is defined at "
                          "http://medical.nema.org/. The plug-in currently "
                          "only supports reading images with uncompressed "
                          "pixel sections.",
                          "Dov Grobgeld",
                          "Dov Grobgeld <dov@imagic.weizmann.ac.il>",
                          "2003",
                          N_("DICOM image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-dcm");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "dcm,dicom",
                                    "",
                                    "128,string,DICM"
                                    );

  gimp_install_procedure (SAVE_PROC,
                          "Save file in the DICOM file format",
                          "Save an image in the medical standard DICOM image "
                          "formats. The standard is defined at "
                          "http://medical.nema.org/. The file format is "
                          "defined in section 10 of the standard. The files "
                          "are saved uncompressed and the compulsory DICOM "
                          "tags are filled with default dummy values.",
                          "Dov Grobgeld",
                          "Dov Grobgeld <dov@imagic.weizmann.ac.il>",
                          "2003",
                          N_("Digital Imaging and Communications in "
                             "Medicine image"),
                          "RGB, GRAY",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-dcm");
  gimp_register_save_handler (SAVE_PROC, "dcm,dicom", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_ID;
  gint32             drawable_ID;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error  = NULL;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string, &error);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;

          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;

          if (error)
            {
              *nreturn_vals = 2;
              values[1].type          = GIMP_PDB_STRING;
              values[1].data.d_string = error->message;
            }
        }
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID, NULL,
                                      GIMP_EXPORT_CAN_HANDLE_RGB |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY);
          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
          break;

        default:
          break;
        }

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 5)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (! save_image (param[3].data.d_string, image_ID, drawable_ID,
                            &error))
            {
              status = GIMP_PDB_EXECUTION_ERROR;

              if (error)
                {
                  *nreturn_vals = 2;
                  values[1].type          = GIMP_PDB_STRING;
                  values[1].data.d_string = error->message;
                }
            }
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

/**
 * add_parasites_to_image:
 * @data:      pointer to a GimpParasite to be attached to the image
 *             specified by @user_data.
 * @user_data: pointer to the image_ID to which parasite @data should
 *             be added.
 *
 * Attaches parasite to image and also frees that parasite
**/
static void
add_parasites_to_image (gpointer data,
                        gpointer user_data)
{
  GimpParasite *parasite = (GimpParasite *) data;
  gint32       *image_ID = (gint32 *) user_data;

  gimp_image_attach_parasite (*image_ID, parasite);
  gimp_parasite_free (parasite);
}

static gint32
load_image (const gchar  *filename,
            GError      **error)
{
  GimpPixelRgn    pixel_rgn;
  gint32 volatile image_ID          = -1;
  gint32          layer_ID;
  GimpDrawable   *drawable;
  GSList         *elements          = NULL;
  FILE           *DICOM;
  gchar           buf[500];    /* buffer for random things like scanning */
  DicomInfo      *dicominfo;
  guint           width             = 0;
  guint           height            = 0;
  gint            samples_per_pixel = 0;
  gint            bpp               = 0;
  gint            bits_stored       = 0;
  gint            high_bit          = 0;
  guint8         *pix_buf           = NULL;
  gboolean        is_signed         = FALSE;
  guint8          in_sequence       = 0;

  /* open the file */
  DICOM = g_fopen (filename, "rb");

  if (! DICOM)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  /* allocate the necessary structures */
  dicominfo = g_new0 (DicomInfo, 1);

  /* Parse the file */
  fread (buf, 1, 128, DICOM); /* skip past buffer */

  /* Check for unsupported formats */
  if (g_ascii_strncasecmp (buf, "PAPYRUS", 7) == 0)
    {
      g_message ("'%s' is a PAPYRUS DICOM file.\n"
                 "This plug-in does not support this type yet.",
                 gimp_filename_to_utf8 (filename));
      g_free (dicominfo);
      fclose (DICOM);
      return -1;
    }

  fread (buf, 1, 4, DICOM); /* This should be dicom */
  if (g_ascii_strncasecmp (buf,"DICM",4) != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a DICOM file."),
                   gimp_filename_to_utf8 (filename));
      g_free (dicominfo);
      fclose (DICOM);
      return -1;
    }

  while (!feof (DICOM))
    {
      guint16  group_word;
      guint16  element_word;
      gchar    value_rep[3];
      guint32  element_length;
      guint16  ctx_us;
      guint8  *value;
      guint32  tag;
      gboolean do_toggle_endian = FALSE;
      gboolean implicit_encoding = FALSE;

      if (fread (&group_word, 1, 2, DICOM) == 0)
        break;
      group_word = g_ntohs (GUINT16_SWAP_LE_BE (group_word));

      fread (&element_word, 1, 2, DICOM);
      element_word = g_ntohs (GUINT16_SWAP_LE_BE (element_word));

      tag = (group_word << 16) | element_word;
      fread(value_rep, 2, 1, DICOM);
      value_rep[2] = 0;

      /* Check if the value rep looks valid. There probably is a
         better way of checking this...
       */
      if ((/* Always need lookup for implicit encoding */
           tag > 0x0002ffff && implicit_encoding)
          /* This heuristics isn't used if we are doing implicit
             encoding according to the value representation... */
          || ((value_rep[0] < 'A' || value_rep[0] > 'Z'
          || value_rep[1] < 'A' || value_rep[1] > 'Z')

          /* I found this in one of Ednas images. It seems like a
             bug...
          */
              && !(value_rep[0] == ' ' && value_rep[1]))
          )
        {
          /* Look up type from the dictionary. At the time we dont
             support this option... */
          gchar element_length_chars[4];

          /* Store the bytes that were read */
          element_length_chars[0] = value_rep[0];
          element_length_chars[1] = value_rep[1];

          /* Unknown value rep. It is not used right now anyhow */
          strcpy (value_rep, "??");

          /* For implicit value_values the length is always four bytes,
             so we need to read another two. */
          fread (&element_length_chars[2], 1, 2, DICOM);

          /* Now cast to integer and insert into element_length */
          element_length =
            g_ntohl (GUINT32_SWAP_LE_BE (*((gint *) element_length_chars)));
      }
      /* Binary value reps are OB, OW, SQ or UN */
      else if (strncmp (value_rep, "OB", 2) == 0
          || strncmp (value_rep, "OW", 2) == 0
          || strncmp (value_rep, "SQ", 2) == 0
          || strncmp (value_rep, "UN", 2) == 0)
        {
          fread (&element_length, 1, 2, DICOM); /* skip two bytes */
          fread (&element_length, 1, 4, DICOM);
          element_length = g_ntohl (GUINT32_SWAP_LE_BE (element_length));
        }
      /* Short length */
      else
        {
          guint16 el16;

          fread (&el16, 1, 2, DICOM);
          element_length = g_ntohs (GUINT16_SWAP_LE_BE (el16));
        }

      /* Sequence of items - just ignore the delimiters... */
      if (element_length == 0xffffffff)
        {
          in_sequence = 1;
          continue;
        }
      /* End of Sequence tag */
      if (tag == 0xFFFEE0DD)
        {
          in_sequence = 0;
          continue;
        }

      /* Sequence of items item tag... Ignore as well */
      if (tag == 0xFFFEE000)
        continue;

      /* Even for pixel data, we don't handle very large element
         lengths */

      if (element_length >= (G_MAXUINT - 6))
        {
          g_message ("'%s' seems to have an incorrect value field length.",
                     gimp_filename_to_utf8 (filename));
          gimp_quit ();
        }

      /* Read contents. Allocate a bit more to make room for casts to int
       below. */
      value = g_new0 (guint8, element_length + 4);
      fread (value, 1, element_length, DICOM);

      /* ignore everything inside of a sequence */
      if (in_sequence)
        {
          g_free (value);
          continue;
        }
      /* Some special casts that are used below */
      ctx_us = *(guint16 *) value;

      /* Recognize some critical tags */
      if (group_word == 0x0002)
        {
          switch (element_word)
            {
            case 0x0010:   /* transfer syntax id */
              if (strcmp("1.2.840.10008.1.2", (char*)value) == 0)
                {
                  do_toggle_endian = FALSE;
                  implicit_encoding = TRUE;
                }
              else if (strcmp("1.2.840.10008.1.2.1", (char*)value) == 0)
                do_toggle_endian = FALSE;
              else if (strcmp("1.2.840.10008.1.2.2", (char*)value) == 0)
                do_toggle_endian = TRUE;
              break;
            }
        }
      else if (group_word == 0x0028)
        {
          switch (element_word)
            {
            case 0x0002:  /* samples per pixel */
              samples_per_pixel = ctx_us;
              break;
            case 0x0010:  /* rows */
              height = ctx_us;
              break;
            case 0x0011:  /* columns */
              width = ctx_us;
              break;
            case 0x0100:  /* bits allocated */
              bpp = ctx_us;
              break;
            case 0x0101:  /* bits stored */
              bits_stored = ctx_us;
              break;
            case 0x0102:  /* high bit */
              high_bit = ctx_us;
              break;
            case 0x0103:  /* is pixel representation signed? */
              is_signed = (ctx_us == 0) ? FALSE : TRUE;
              break;
            }
        }

      /* Pixel data */
      if (group_word == 0x7fe0 && element_word == 0x0010)
        {
          pix_buf = value;
        }
      else
        {
          /* save this element to a parasite for later writing */
          GimpParasite *parasite;
          gchar         pname[255];

          /* all elements are retrievable using gimp_get_parasite_list() */
          g_snprintf (pname, sizeof (pname),
                      "dcm/%04x-%04x-%s", group_word, element_word, value_rep);
          if ((parasite = gimp_parasite_new (pname,
                                             GIMP_PARASITE_PERSISTENT,
                                             element_length, value)))
            {
              /*
               * at this point, the image has not yet been created, so
               * image_ID is not valid.  keep the parasite around
               * until we're able to attach it.
               */

              /* add to our list of parasites to be added (prepending
               * for speed. we'll reverse it later)
               */
              elements = g_slist_prepend (elements, parasite);
            }

          g_free (value);
        }
    }

  if ((bpp != 8) && (bpp != 16))
    {
      g_message ("'%s' has a bpp of %d which GIMP cannot handle.",
                 gimp_filename_to_utf8 (filename), bpp);
      gimp_quit ();
    }

  if ((width > GIMP_MAX_IMAGE_SIZE) || (height > GIMP_MAX_IMAGE_SIZE))
    {
      g_message ("'%s' has a larger image size (%d x %d) than GIMP can handle.",
                 gimp_filename_to_utf8 (filename), width, height);
      gimp_quit ();
    }

  if (samples_per_pixel > 3)
    {
      g_message ("'%s' has samples per pixel of %d which GIMP cannot handle.",
                 gimp_filename_to_utf8 (filename), samples_per_pixel);
      gimp_quit ();
    }

  dicominfo->width  = width;
  dicominfo->height = height;
  dicominfo->bpp    = bpp;

  dicominfo->bits_stored = bits_stored;
  dicominfo->high_bit = high_bit;
  dicominfo->is_signed = is_signed;
  dicominfo->samples_per_pixel = samples_per_pixel;
  dicominfo->maxval = -1;   /* External normalization factor - not used yet */

  /* Create a new image of the proper size and associate the filename with it.
   */
  image_ID = gimp_image_new (dicominfo->width, dicominfo->height,
                             (dicominfo->samples_per_pixel >= 3 ?
                              GIMP_RGB : GIMP_GRAY));
  gimp_image_set_filename (image_ID, filename);

  layer_ID = gimp_layer_new (image_ID, _("Background"),
                             dicominfo->width, dicominfo->height,
                             (dicominfo->samples_per_pixel >= 3 ?
                              GIMP_RGB_IMAGE : GIMP_GRAY_IMAGE),
                             100, GIMP_NORMAL_MODE);
  gimp_image_insert_layer (image_ID, layer_ID, -1, 0);

  drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0, drawable->width, drawable->height, TRUE, FALSE);

#if GUESS_ENDIAN
  if (bpp == 16)
    guess_and_set_endian2 ((guint16 *) pix_buf, width * height);
#endif

  dicom_loader (pix_buf, dicominfo, &pixel_rgn);

  if (elements)
    {
      /* flip the parasites back around into the order they were
       * created (read from the file)
       */
      elements = g_slist_reverse (elements);
      /* and add each one to the image */
      g_slist_foreach (elements, add_parasites_to_image, (gpointer) &image_ID);
      g_slist_free (elements);
    }

  g_free (pix_buf);
  g_free (dicominfo);

  fclose (DICOM);

  gimp_drawable_detach (drawable);

  return image_ID;
}

static void
dicom_loader (guint8       *pix_buffer,
              DicomInfo    *info,
              GimpPixelRgn *pixel_rgn)
{
  guchar  *data;
  gint     row_idx;
  gint     width             = info->width;
  gint     height            = info->height;
  gint     samples_per_pixel = info->samples_per_pixel;
  guint16 *buf16             = (guint16 *) pix_buffer;

  if (info->bpp == 16)
    {
      gulong pix_idx;
      guint  shift = info->high_bit + 1 - info->bits_stored;

      /* Reorder the buffer; also shift the data so that the LSB
       * of the pixel data is at the LSB of the 16-bit array entries
       * (i.e., compensate for high_bit and bits_stored).
       */
      for (pix_idx = 0; pix_idx < width * height * samples_per_pixel; pix_idx++)
        buf16[pix_idx] = g_htons (buf16[pix_idx]) >> shift;
    }

  data = g_malloc (gimp_tile_height () * width * samples_per_pixel);

  for (row_idx = 0; row_idx < height; )
    {
      guchar *d = data;
      gint    start;
      gint    end;
      gint    scanlines;
      gint    i;

      start = row_idx;
      end   = row_idx + gimp_tile_height ();
      end   = MIN (end, height);

      scanlines = end - start;

      for (i = 0; i < scanlines; i++)
        {
          if (info->bpp == 16)
            {
              guint16 *row_start;
              gint     col_idx;

              row_start = buf16 + (row_idx + i) * width * samples_per_pixel;

              for (col_idx = 0; col_idx < width * samples_per_pixel; col_idx++)
                {
                  /* Shift it by 8 bits, or less in case bits_stored
                   * is less than bpp.
                   */
                  d[col_idx] = (guint8) (row_start[col_idx] >>
                                         (info->bits_stored - 8));
                  if (info->is_signed)
                    {
                      /* If the data is negative, make it 0. Otherwise,
                       * multiply the positive value by 2, so that the
                       * positive values span between 0 and 254.
                       */
                      if (d[col_idx] > 127)
                        d[col_idx] = 0;
                      else
                        d[col_idx] <<= 1;
                    }
                }
            }
          else if (info->bpp == 8)
            {
              guint8 *row_start;
              gint    col_idx;

              row_start = (pix_buffer +
                           (row_idx + i) * width * samples_per_pixel);

              for (col_idx = 0; col_idx < width * samples_per_pixel; col_idx++)
                {
                  /* Shift it by 0 bits, or more in case bits_stored is
                   * less than bpp.
                   */
                  d[col_idx] = row_start[col_idx] << (8 - info->bits_stored);

                  if (info->is_signed)
                    {
                      /* If the data is negative, make it 0. Otherwise,
                       * multiply the positive value by 2, so that the
                       * positive values span between 0 and 254.
                       */
                      if (d[col_idx] > 127)
                        d[col_idx] = 0;
                      else
                        d[col_idx] <<= 1;
                    }
                }
            }

          d += width * samples_per_pixel;
        }

      gimp_progress_update ((gdouble) row_idx / (gdouble) height);
      gimp_pixel_rgn_set_rect (pixel_rgn, data, 0, row_idx, width, scanlines);
      row_idx += scanlines;
    }
  gimp_progress_update (1.0);

  g_free (data);
}


/* Guess and set endian. Guesses the endian of a buffer by
 * checking the maximum value of the first and the last byte
 * in the words of the buffer. It assumes that the least
 * significant byte has a larger maximum than the most
 * significant byte.
 */
static void
guess_and_set_endian2 (guint16 *buf16,
                       int length)
{
  guint16 *p          = buf16;
  gint     max_first  = -1;
  gint     max_second = -1;

  while (p<buf16+length)
    {
      if (*(guint8*)p > max_first)
        max_first = *(guint8*)p;
      if (((guint8*)p)[1] > max_second)
        max_second = ((guint8*)p)[1];
      p++;
    }

  if (   ((max_second > max_first) && (G_BYTE_ORDER == G_LITTLE_ENDIAN))
         || ((max_second < max_first) && (G_BYTE_ORDER == G_BIG_ENDIAN)))
    toggle_endian2 (buf16, length);
}

/* toggle_endian2 toggles the endian for a 16 bit entity.  */
static void
toggle_endian2 (guint16 *buf16,
                gint     length)
{
  guint16 *p = buf16;

  while (p < buf16 + length)
    {
      *p = ((*p & 0xff) << 8) | (*p >> 8);
      p++;
    }
}

typedef struct
{
  guint16   group_word;
  guint16   element_word;
  gchar     value_rep[3];
  guint32   element_length;
  guint8   *value;
  gboolean  free;
} DICOMELEMENT;

/**
 * dicom_add_element:
 * @elements:     head of a GSList containing DICOMELEMENT structures.
 * @group_word:   Dicom Element group number for the tag to be added to
 *                @elements.
 * @element_word: Dicom Element element number for the tag to be added
 *                to @elements.
 * @value_rep:    a string representing the Dicom VR for the new element.
 * @value:        a pointer to an integer containing the value for the
 *                element to be created.
 *
 * Creates a DICOMELEMENT object and inserts it into @elements.
 *
 * Return value: the new head of @elements
**/
static GSList *
dicom_add_element (GSList      *elements,
                   guint16      group_word,
                   guint16      element_word,
                   const gchar *value_rep,
                   guint32      element_length,
                   guint8      *value)
{
  DICOMELEMENT *element = g_slice_new0 (DICOMELEMENT);

  element->group_word     = group_word;
  element->element_word   = element_word;
  strncpy (element->value_rep, value_rep, sizeof (element->value_rep));
  element->element_length = element_length;
  element->value          = value;

  return g_slist_prepend (elements, element);
}

static GSList *
dicom_add_element_copy (GSList       *elements,
                        guint16       group_word,
                        guint16       element_word,
                        gchar        *value_rep,
                        guint32       element_length,
                        const guint8 *value)
{
  elements = dicom_add_element (elements,
                                group_word, element_word, value_rep,
                                element_length,
                                g_memdup (value, element_length));

  ((DICOMELEMENT *) elements->data)->free = TRUE;

  return elements;
}

/**
 * dicom_add_element_int:
 * @elements:     head of a GSList containing DICOMELEMENT structures.

 * @group_word:   Dicom Element group number for the tag to be added to
 *                @elements.
 * @element_word: Dicom Element element number for the tag to be added to
 *                @elements.
 * @value_rep:    a string representing the Dicom VR for the new element.
 * @value:        a pointer to an integer containing the value for the
 *                element to be created.
 *
 * Creates a DICOMELEMENT object from the passed integer pointer and
 * adds it to @elements.  Note: value should be the address of a
 * guint16 for @value_rep == %US or guint32 for other values of
 * @value_rep
 *
 * Return value: the new head of @elements
 */
static GSList *
dicom_add_element_int (GSList  *elements,
                       guint16  group_word,
                       guint16  element_word,
                       gchar   *value_rep,
                       guint8  *value)
{
  guint32 len;

  if (strcmp (value_rep, "US") == 0)
    len = 2;
  else
    len = 4;

  return dicom_add_element (elements,
                            group_word, element_word, value_rep,
                            len, value);
}

/**
 * dicom_element_done:
 * @data: pointer to a DICOMELEMENT structure which is to be destroyed.
 *
 * Destroys the DICOMELEMENT passed as @data
**/
static void
dicom_element_done (gpointer data)
{
  if (data)
    {
      DICOMELEMENT *e = data;

      if (e->free)
        g_free (e->value);

      g_slice_free (DICOMELEMENT, data);
    }
}

/**
 * dicom_elements_destroy:
 * @elements: head of a GSList containing DICOMELEMENT structures.
 *
 * Destroys the list of DICOMELEMENTs
**/
static void
dicom_elements_destroy (GSList *elements)
{
  if (elements)
    g_slist_free_full (elements, dicom_element_done);
}

/**
 * dicom_destroy_element:
 * @elements: head of a GSList containing DICOMELEMENT structures.
 * @ele: a DICOMELEMENT structure to be removed from @elements
 *
 * Removes the specified DICOMELEMENT from @elements and Destroys it
 *
 * Return value: the new head of @elements
**/
static GSList *
dicom_destroy_element (GSList       *elements,
                       DICOMELEMENT *ele)
{
  if (ele)
    {
      elements = g_slist_remove_all (elements, ele);

      if (ele->free)
        g_free (ele->value);

      g_slice_free (DICOMELEMENT, ele);
    }

  return elements;
}

/**
 * dicom_elements_compare:
 * @a: pointer to a DICOMELEMENT structure.
 * @b: pointer to a DICOMELEMENT structure.
 *
 * Determines the equality of @a and @b as strcmp
 *
 * Return value: an integer indicating the equality of @a and @b.
**/
static gint
dicom_elements_compare (gconstpointer a,
                        gconstpointer b)
{
  DICOMELEMENT *e1 = (DICOMELEMENT *)a;
  DICOMELEMENT *e2 = (DICOMELEMENT *)b;

  if (e1->group_word == e2->group_word)
    {
      if (e1->element_word == e2->element_word)
        {
          return 0;
        }
      else if (e1->element_word > e2->element_word)
        {
          return 1;
        }
      else
        {
          return -1;
        }
    }
  else if (e1->group_word < e2->group_word)
    {
      return -1;
    }

  return 1;
}

/**
 * dicom_element_find_by_num:
 * @head: head of a GSList containing DICOMELEMENT structures.
 * @group_word: Dicom Element group number for the tag to be found.
 * @element_word: Dicom Element element number for the tag to be found.
 *
 * Retrieves the specified DICOMELEMENT from @head, if available.
 *
 * Return value: a DICOMELEMENT matching the specified group,element,
 *               or NULL if the specified element was not found.
**/
static DICOMELEMENT *
dicom_element_find_by_num (GSList  *head,
                           guint16  group_word,
                           guint16  element_word)
{
  DICOMELEMENT data = { group_word,element_word, "", 0, NULL};
  GSList *ele = g_slist_find_custom (head,&data,dicom_elements_compare);
  return (ele ? ele->data : NULL);
}

/**
 * dicom_get_elements_list:
 * @image_ID: the image_ID from which to read parasites in order to
 *            retrieve the dicom elements
 *
 * Reads all DICOMELEMENTs from the specified image's parasites.
 *
 * Return value: a GSList of all known dicom elements
**/
static GSList *
dicom_get_elements_list (gint32 image_ID)
{
  GSList        *elements = NULL;
  GimpParasite  *parasite;
  gchar        **parasites = NULL;
  gint           count = 0;

  parasites = gimp_image_get_parasite_list (image_ID, &count);

  if (parasites && count > 0)
    {
      gint i;

      for (i = 0; i < count; i++)
        {
          if (strncmp (parasites[i], "dcm", 3) == 0)
            {
              parasite = gimp_image_get_parasite (image_ID, parasites[i]);

              if (parasite)
                {
                  gchar buf[1024];
                  gchar *ptr1;
                  gchar *ptr2;
                  gchar value_rep[3]   = "";
                  guint16 group_word   = 0;
                  guint16 element_word = 0;

                  /* sacrificial buffer */
                  strncpy (buf, parasites[i], sizeof (buf));

                  /* buf should now hold a string of the form
                   * dcm/XXXX-XXXX-AA where XXXX are Hex values for
                   * group and element respectively AA is the Value
                   * Representation of the element
                   *
                   * start off by jumping over the dcm/ to the first Hex blob
                   */
                  ptr1 = strchr (buf, '/');

                  if (ptr1)
                    {
                      gchar t[15];

                      ptr1++;
                      ptr2 = strchr (ptr1,'-');

                      if (ptr2)
                        *ptr2 = '\0';

                      g_snprintf (t, sizeof (t), "0x%s", ptr1);
                      group_word = (guint16) g_ascii_strtoull (t, NULL, 16);
                      ptr1 = ptr2 + 1;
                    }

                  /* now get the second Hex blob */
                  if (ptr1)
                    {
                      gchar t[15];

                      ptr2 = strchr (ptr1, '-');

                      if (ptr2)
                        *ptr2 = '\0';

                      g_snprintf (t, sizeof (t), "0x%s", ptr1);
                      element_word = (guint16) g_ascii_strtoull (t, NULL, 16);
                      ptr1 = ptr2 + 1;
                    }

                  /* and lastly, the VR */
                  if (ptr1)
                    strncpy (value_rep, ptr1, sizeof (value_rep));

                  /*
                   * If all went according to plan, we should be able
                   * to add this element
                   */
                  if (group_word > 0 && element_word > 0)
                    {
                      const guint8 *val = gimp_parasite_data (parasite);
                      const guint   len = gimp_parasite_data_size (parasite);

                      /* and add the dicom element, asking to have
                         it's value copied for later garbage collection */
                      elements = dicom_add_element_copy (elements,
                                                         group_word,
                                                         element_word,
                                                         value_rep, len, val);
                    }

                  gimp_parasite_free (parasite);
                }
            }

          /* make sure we free each individual parasite name, in
           * addition to the array of names
           */
          g_free (parasites[i]);
        }
    }
  /* cleanup the array of names */
  if (parasites)
    {
      g_free (parasites);
    }
  return elements;
}

/**
 * dicom_remove_gimp_specified_elements:
 * @elements:  GSList to remove elements from
 * @samples_per_pixel: samples per pixel of the image to be written.
 *                     if set to %3 the planar configuration for color images
 *                     will also be removed from @elements
 *
 * Removes certain DICOMELEMENTs from the elements list which are specific to the output of this plugin.
 *
 * Return value: the new head of @elements
**/
static GSList *
dicom_remove_gimp_specified_elements (GSList *elements,
                                      gint samples_per_pixel)
{
  DICOMELEMENT remove[] = {
    /* Image presentation group */
    /* Samples per pixel */
    {0x0028, 0x0002, "", 0, NULL},
    /* Photometric interpretation */
    {0x0028, 0x0004, "", 0, NULL},
    /* rows */
    {0x0028, 0x0010, "", 0, NULL},
    /* columns */
    {0x0028, 0x0011, "", 0, NULL},
    /* Bits allocated */
    {0x0028, 0x0100, "", 0, NULL},
    /* Bits Stored */
    {0x0028, 0x0101, "", 0, NULL},
    /* High bit */
    {0x0028, 0x0102, "", 0, NULL},
    /* Pixel representation */
    {0x0028, 0x0103, "", 0, NULL},

    {0,0,"",0,NULL}
  };
  DICOMELEMENT *ele;
  gint i;

  /*
   * Remove all Dicom elements which will be set as part of the writing of the new file
   */
  for (i=0; remove[i].group_word > 0;i++)
    {
      if ((ele = dicom_element_find_by_num (elements,remove[i].group_word,remove[i].element_word)))
        {
          elements = dicom_destroy_element (elements,ele);
        }
    }
  /* special case - allow this to be overwritten if necessary */
  if (samples_per_pixel == 3)
    {
      /* Planar configuration for color images */
      if ((ele = dicom_element_find_by_num (elements,0x0028,0x0006)))
        {
          elements = dicom_destroy_element (elements,ele);
        }
    }
  return elements;
}

/**
 * dicom_ensure_required_elements_present:
 * @elements:     GSList to remove elements from
 * @today_string: string containing today's date in DICOM format. This
 *                is used to default any required Dicom elements of date
 *                type to today's date.
 *
 * Defaults DICOMELEMENTs to the values set by previous version of
 * this plugin, but only if they do not already exist.
 *
 * Return value: the new head of @elements
**/
static GSList *
dicom_ensure_required_elements_present (GSList *elements,
                                        gchar *today_string)
{
  const DICOMELEMENT defaults[] = {
    /* Meta element group */
    /* 0002, 0001 - File Meta Information Version */
    { 0x0002, 0x0001, "OB", 2, (guint8 *) "\0\1" },
    /* 0002, 0010 - Transfer syntax uid */
    { 0x0002, 0x0001, "UI",
      strlen ("1.2.840.10008.1.2.1"), (guint8 *) "1.2.840.10008.1.2.1"},
    /* 0002, 0013 - Implementation version name */
    { 0x0002, 0x0013, "SH",
      strlen ("GIMP Dicom Plugin 1.0"), (guint8 *) "GIMP Dicom Plugin 1.0" },
    /* Identifying group */
    /* Study date */
    { 0x0008, 0x0020, "DA",
      strlen (today_string), (guint8 *) today_string },
    /* Series date */
    { 0x0008, 0x0021, "DA",
      strlen (today_string), (guint8 *) today_string },
    /* Acquisition date */
    { 0x0008, 0x0022, "DA",
      strlen (today_string), (guint8 *) today_string },
    /* Content Date */
    { 0x0008, 0x0023, "DA",
      strlen (today_string), (guint8 *) today_string},
    /* Modality - I have to add something.. */
    { 0x0008, 0x0060, "CS", strlen ("MR"), (guint8 *) "MR" },
    /* Patient group */
    /* Patient name */
    { 0x0010,  0x0010, "PN",
      strlen ("DOE^WILBER"), (guint8 *) "DOE^WILBER" },
    /* Patient ID */
    { 0x0010,  0x0020, "CS",
      strlen ("314159265"), (guint8 *) "314159265" },
    /* Patient Birth date */
    { 0x0010,  0x0030, "DA",
      strlen (today_string), (guint8 *) today_string },
    /* Patient sex */
    { 0x0010,  0x0040, "CS", strlen (""), (guint8 *) "" /* unknown */ },
    /* Relationship group */
    /* Instance number */
    { 0x0020, 0x0013, "IS", strlen ("1"), (guint8 *) "1" },

    { 0, 0, "", 0, NULL }
  };
  gint i;

  /*
   * Make sure that all of the default elements have a value
   */
  for (i=0; defaults[i].group_word > 0; i++)
    {
      if (dicom_element_find_by_num (elements,
                                     defaults[i].group_word,
                                     defaults[i].element_word) == NULL)
        {
          elements = dicom_add_element (elements,
                                        defaults[i].group_word,
                                        defaults[i].element_word,
                                        defaults[i].value_rep,
                                        defaults[i].element_length,
                                        defaults[i].value);
        }
    }

  return elements;
}

/* save_image() saves an image in the dicom format. The DICOM format
 * requires a lot of tags to be set. Some of them have real uses, others
 * must just be filled with dummy values.
 */
static gboolean
save_image (const gchar  *filename,
            gint32        image_ID,
            gint32        drawable_ID,
            GError      **error)
{
  FILE          *DICOM;
  GimpImageType  drawable_type;
  GimpDrawable  *drawable;
  GimpPixelRgn   pixel_rgn;
  GByteArray    *group_stream;
  GSList        *elements = NULL;
  gint           group;
  GDate         *date;
  gchar          today_string[16];
  gchar         *photometric_interp;
  gint           samples_per_pixel;
  gboolean       retval = TRUE;
  guint16        zero = 0;
  guint16        seven = 7;
  guint16        eight = 8;
  guchar        *src = NULL;

  drawable_type = gimp_drawable_type (drawable_ID);
  drawable = gimp_drawable_get (drawable_ID);

  /*  Make sure we're not saving an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable_ID))
    {
      g_message (_("Cannot save images with alpha channel."));
      return FALSE;
    }

  switch (drawable_type)
    {
    case GIMP_GRAY_IMAGE:
      samples_per_pixel = 1;
      photometric_interp = "MONOCHROME2";
      break;
    case GIMP_RGB_IMAGE:
      samples_per_pixel = 3;
      photometric_interp = "RGB";
      break;
    default:
      g_message (_("Cannot operate on unknown image types."));
      return FALSE;
    }

  date = g_date_new ();
  g_date_set_time_t (date, time (NULL));
  g_snprintf (today_string, sizeof (today_string),
              "%04d%02d%02d", date->year, date->month, date->day);
  g_date_free (date);

  /* Open the output file. */
  DICOM = g_fopen (filename, "wb");

  if (!DICOM)
    {
      gimp_drawable_detach (drawable);
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  /* Print dicom header */
  {
    guint8 val = 0;
    gint   i;

    for (i = 0; i < 0x80; i++)
      fwrite (&val, 1, 1, DICOM);
  }
  fprintf (DICOM, "DICM");

  group_stream = g_byte_array_new ();

  elements = dicom_get_elements_list (image_ID);
  if (0/*replaceElementsList*/)
    {
      /* to do */
    }
  else if (1/*insist_on_basic_elements*/)
    {
      elements = dicom_ensure_required_elements_present (elements,today_string);
    }

  /*
   * Set value of custom elements
   */
  elements = dicom_remove_gimp_specified_elements (elements,samples_per_pixel);

  /* Image presentation group */
  group = 0x0028;
  /* Samples per pixel */
  elements = dicom_add_element_int (elements, group, 0x0002, "US",
                                    (guint8 *) &samples_per_pixel);
  /* Photometric interpretation */
  elements = dicom_add_element (elements, group, 0x0004, "CS",
                                strlen (photometric_interp),
                                (guint8 *) photometric_interp);
  /* Planar configuration for color images */
  if (samples_per_pixel == 3)
    elements = dicom_add_element_int (elements, group, 0x0006, "US",
                                      (guint8 *) &zero);
  /* rows */
  elements = dicom_add_element_int (elements, group, 0x0010, "US",
                                    (guint8 *) &(drawable->height));
  /* columns */
  elements = dicom_add_element_int (elements, group, 0x0011, "US",
                                    (guint8 *) &(drawable->width));
  /* Bits allocated */
  elements = dicom_add_element_int (elements, group, 0x0100, "US",
                                    (guint8 *) &eight);
  /* Bits Stored */
  elements = dicom_add_element_int (elements, group, 0x0101, "US",
                                    (guint8 *) &eight);
  /* High bit */
  elements = dicom_add_element_int (elements, group, 0x0102, "US",
                                    (guint8 *) &seven);
  /* Pixel representation */
  elements = dicom_add_element_int (elements, group, 0x0103, "US",
                                    (guint8 *) &zero);

  /* Pixel data */
  group = 0x7fe0;
  src = g_new (guchar,
               drawable->height * drawable->width * samples_per_pixel);
  if (src)
    {
      gimp_pixel_rgn_init (&pixel_rgn, drawable,
                           0, 0, drawable->width, drawable->height,
                           FALSE, FALSE);
      gimp_pixel_rgn_get_rect (&pixel_rgn,
                               src, 0, 0, drawable->width, drawable->height);
      elements = dicom_add_element (elements, group, 0x0010, "OW",
                                    drawable->width * drawable->height *
                                    samples_per_pixel,
                                    (guint8 *) src);

      elements = dicom_add_tags (DICOM, group_stream, elements);

      g_free (src);
    }
  else
    {
      retval = FALSE;
    }

  fclose (DICOM);

  dicom_elements_destroy (elements);
  g_byte_array_free (group_stream, TRUE);
  gimp_drawable_detach (drawable);

  return retval;
}

/**
 * dicom_print_tags:
 * @data: pointer to a DICOMELEMENT structure which is to be written to file
 * @user_data: structure containing state information and output parameters
 *
 * Writes the specified DICOMELEMENT to @user_data's group_stream member.
 * Between groups, flushes the group_stream to @user_data's DICOM member.
 */
static void
dicom_print_tags(gpointer data,
                 gpointer user_data)
{
  struct {
    FILE       *DICOM;
    GByteArray *group_stream;
    gint        last_group;
  } *d = user_data;
  DICOMELEMENT *e = (DICOMELEMENT *) data;

  if (d->last_group >= 0 && e->group_word != d->last_group)
    {
      write_group_to_file (d->DICOM, d->last_group, d->group_stream);
    }

  add_tag_pointer (d->group_stream,
                   e->group_word, e->element_word,
                   e->value_rep,e->value, e->element_length);
  d->last_group = e->group_word;
}

/**
 * dicom_add_tags:
 * @DICOM:        File pointer to which @elements should be written.
 * @group_stream: byte array used for staging Dicom Element groups
 *                before flushing them to disk.
 * @elements:     GSList container the Dicom Element elements from
 *
 * Writes all Dicom tags in @elements to the file @DICOM
 *
 * Return value: the new head of @elements
**/
static GSList *
dicom_add_tags (FILE       *DICOM,
                GByteArray *group_stream,
                GSList     *elements)
{
  struct {
    FILE       *DICOM;
    GByteArray *group_stream;
    gint        last_group;
  } data = { DICOM, group_stream, -1 };

  elements = g_slist_sort (elements, dicom_elements_compare);
  g_slist_foreach (elements, dicom_print_tags, &data);
  /* make sure that the final group is written to the file */
  write_group_to_file (data.DICOM, data.last_group, data.group_stream);

  return elements;
}

/* add_tag_pointer () adds to the group_stream one single value with its
 * corresponding value_rep. Note that we use "explicit VR".
 */
static void
add_tag_pointer (GByteArray   *group_stream,
                 gint          group,
                 gint          element,
                 const gchar  *value_rep,
                 const guint8 *data,
                 gint          length)
{
  gboolean is_long;
  guint16  swapped16;
  guint32  swapped32;
  guint    pad = 0;

  is_long = (strstr ("OB|OW|SQ|UN", value_rep) != NULL) || length > 65535;

  swapped16 = g_ntohs (GUINT16_SWAP_LE_BE (group));
  g_byte_array_append (group_stream, (guint8 *) &swapped16, 2);

  swapped16 = g_ntohs (GUINT16_SWAP_LE_BE (element));
  g_byte_array_append (group_stream, (guint8 *) &swapped16, 2);

  g_byte_array_append (group_stream, (const guchar *) value_rep, 2);

  if (length % 2 != 0)
    {
      /* the dicom standard requires all elements to be of even byte
       * length. this element would be odd, so we must pad it before
       * adding it
       */
      pad = 1;
    }

  if (is_long)
    {

      g_byte_array_append (group_stream, (const guchar *) "\0\0", 2);

      swapped32 = g_ntohl (GUINT32_SWAP_LE_BE (length + pad));
      g_byte_array_append (group_stream, (guint8 *) &swapped32, 4);
    }
  else
    {
      swapped16 = g_ntohs (GUINT16_SWAP_LE_BE (length + pad));
      g_byte_array_append (group_stream, (guint8 *) &swapped16, 2);
    }

  g_byte_array_append (group_stream, data, length);

  if (pad)
    {
       /* add a padding byte to the stream
        *
        * From ftp://medical.nema.org/medical/dicom/2009/09_05pu3.pdf:
        *
        * Values with VRs constructed of character strings, except in
        * the case of the VR UI, shall be padded with SPACE characters
        * (20H, in the Default Character Repertoire) when necessary to
        * achieve even length.  Values with a VR of UI shall be padded
        * with a single trailing NULL (00H) character when necessary
        * to achieve even length. Values with a VR of OB shall be
        * padded with a single trailing NULL byte value (00H) when
        * necessary to achieve even length.
        */
      if (strstr ("UI|OB", value_rep) != NULL)
        {
          g_byte_array_append (group_stream, (guint8 *) 0x0000, 1);
        }
      else
        {
          g_byte_array_append (group_stream, (guint8 *) " ", 1);
        }
    }
}

/* Once a group has been built it has to be wrapped with a meta-group
 * tag before it is written to the DICOM file. This is done by
 * write_group_to_file.
 */
static gboolean
write_group_to_file (FILE       *DICOM,
                     gint        group,
                     GByteArray *group_stream)
{
  gboolean retval = TRUE;
  guint16  swapped16;
  guint32  swapped32;

  /* Add header to the group and output it */
  swapped16 = g_ntohs (GUINT16_SWAP_LE_BE (group));

  fwrite ((gchar *) &swapped16, 1, 2, DICOM);
  fputc (0, DICOM);
  fputc (0, DICOM);
  fputc ('U', DICOM);
  fputc ('L', DICOM);

  swapped16 = g_ntohs (GUINT16_SWAP_LE_BE (4));
  fwrite ((gchar *) &swapped16, 1, 2, DICOM);

  swapped32 = g_ntohl (GUINT32_SWAP_LE_BE (group_stream->len));
  fwrite ((gchar *) &swapped32, 1, 4, DICOM);

  if (fwrite (group_stream->data,
              1, group_stream->len, DICOM) != group_stream->len)
    retval = FALSE;

  g_byte_array_set_size (group_stream, 0);

  return retval;
}
