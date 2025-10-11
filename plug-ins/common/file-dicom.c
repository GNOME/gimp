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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * The dicom reading and writing code was written from scratch
 * by Dov Grobgeld.  (dov.grobgeld@gmail.com).
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
#define EXPORT_PROC    "file-dicom-export"
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
  gboolean   planar;
  gboolean   bw_inverted;
} DicomInfo;


typedef struct _Dicom      Dicom;
typedef struct _DicomClass DicomClass;

struct _Dicom
{
  GimpPlugIn      parent_instance;
};

struct _DicomClass
{
  GimpPlugInClass parent_class;
};


#define DICOM_TYPE  (dicom_get_type ())
#define DICOM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DICOM_TYPE, Dicom))

GType                   dicom_get_type         (void) G_GNUC_CONST;

static GList          * dicom_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * dicom_create_procedure (GimpPlugIn            *plug_in,
                                                const gchar           *name);

static GimpValueArray * dicom_load             (GimpProcedure         *procedure,
                                                GimpRunMode            run_mode,
                                                GFile                 *file,
                                                GimpMetadata          *metadata,
                                                GimpMetadataLoadFlags *flags,
                                                GimpProcedureConfig   *config,
                                                gpointer               run_data);
static GimpValueArray * dicom_export           (GimpProcedure         *procedure,
                                                GimpRunMode            run_mode,
                                                GimpImage             *image,
                                                GFile                 *file,
                                                GimpExportOptions     *options,
                                                GimpMetadata          *metadata,
                                                GimpProcedureConfig   *config,
                                                gpointer               run_data);

static GimpImage      * load_image             (GFile                 *file,
                                                GError               **error);
static gboolean         export_image           (GFile                 *file,
                                                GimpImage             *image,
                                                GimpDrawable          *drawable,
                                                GError               **error);
static void             dicom_loader           (guint8                *pix_buf,
                                                DicomInfo             *info,
                                                GeglBuffer            *buffer);
static void             guess_and_set_endian2  (guint16               *buf16,
                                                gint                   length);
static void             toggle_endian2         (guint16               *buf16,
                                                gint                   length);
static void             add_tag_pointer        (GByteArray            *group_stream,
                                                gint                   group,
                                                gint                   element,
                                                const gchar           *value_rep,
                                                const guint8          *data,
                                                gint                   length);
static GSList         * dicom_add_tags         (FILE                  *dicom,
                                                GByteArray            *group_stream,
                                                GSList                *elements);
static gboolean         write_group_to_file    (FILE                  *dicom,
                                                gint                   group,
                                                GByteArray            *group_stream);


G_DEFINE_TYPE (Dicom, dicom, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (DICOM_TYPE)
DEFINE_STD_SET_I18N


static void
dicom_class_init (DicomClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = dicom_query_procedures;
  plug_in_class->create_procedure = dicom_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
dicom_init (Dicom *dicom)
{
}

static GList *
dicom_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
dicom_create_procedure (GimpPlugIn  *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           dicom_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("DICOM image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads files of the dicom file format"),
                                        _("Load a file in the DICOM standard "
                                          "format. The standard is defined at "
                                          "http://medical.nema.org/. The plug-in "
                                          "currently only supports reading "
                                          "images with uncompressed pixel "
                                          "sections."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dov Grobgeld",
                                      "Dov Grobgeld <dov@imagic.weizmann.ac.il>",
                                      "2003");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-dcm");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "dcm,dicom");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "128,string,DICM");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, dicom_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB, GRAY");

      gimp_procedure_set_menu_label (procedure,
                                     _("Digital Imaging and Communications in "
                                       "Medicine image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Save file in the DICOM file format"),
                                        _("Save an image in the medical "
                                          "standard DICOM image formats. "
                                          "The standard is defined at "
                                          "http://medical.nema.org/. The file "
                                          "format is defined in section 10 of "
                                          "the standard. The files are saved "
                                          "uncompressed and the compulsory DICOM "
                                          "tags are filled with default dummy "
                                          "values."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dov Grobgeld",
                                      "Dov Grobgeld <dov@imagic.weizmann.ac.il>",
                                      "2003");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-dcm");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "dcm,dicom");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB   |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY,
                                              NULL, NULL, NULL);
    }

  return procedure;
}

static GimpValueArray *
dicom_load (GimpProcedure         *procedure,
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

  image = load_image (file, &error);

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
dicom_export (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GFile                *file,
              GimpExportOptions    *options,
              GimpMetadata         *metadata,
              GimpProcedureConfig  *config,
              gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GList             *drawables;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  export = gimp_export_options_get_image (options, &image);

  drawables = gimp_image_list_layers (image);

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! export_image (file, image, drawables->data,
                          &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
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
  GimpParasite *parasite = data;
  GimpImage    *image    = user_data;

  gimp_image_attach_parasite (image, parasite);
  gimp_parasite_free (parasite);
}

static GimpImage *
load_image (GFile   *file,
            GError **error)
{
  GimpImage  *image = NULL;
  GimpLayer  *layer;
  GeglBuffer *buffer;
  GSList     *elements          = NULL;
  FILE       *dicom;
  gchar       buf[500];    /* buffer for random things like scanning */
  DicomInfo  *dicominfo;
  guint       width             = 0;
  guint       height            = 0;
  gint        samples_per_pixel = 0;
  gint        bpp               = 0;
  gint        bits_stored       = 0;
  gint        high_bit          = 0;
  guint8     *pix_buf           = NULL;
  guint64     pixbuf_size       = 0;
  gboolean    is_signed         = FALSE;
  guint8      in_sequence       = 0;
  gboolean    implicit_encoding = FALSE;
  gboolean    big_endian        = FALSE;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  dicom = g_fopen (g_file_peek_path (file), "rb");

  if (! dicom)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  /* allocate the necessary structures */
  dicominfo = g_new0 (DicomInfo, 1);

  /* Parse the file */
  fread (buf, 1, 128, dicom); /* skip past buffer */

  /* Check for unsupported formats */
  if (g_ascii_strncasecmp (buf, "PAPYRUS", 7) == 0)
    {
      g_message ("'%s' is a PAPYRUS DICOM file.\n"
                 "This plug-in does not support this type yet.",
                 gimp_file_get_utf8_name (file));
      g_free (dicominfo);
      fclose (dicom);
      return NULL;
    }

  fread (buf, 1, 4, dicom); /* This should be dicom */
  if (g_ascii_strncasecmp (buf,"DICM",4) != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a DICOM file."),
                   gimp_file_get_utf8_name (file));
      g_free (dicominfo);
      fclose (dicom);
      return NULL;
    }

  while (!feof (dicom))
    {
      guint16  group_word;
      guint16  element_word;
      gchar    value_rep[3];
      guint32  element_length;
      guint16  ctx_us;
      guint8  *value;
      guint32  tag;
      size_t   actual_read;

      if (fread (&group_word, 1, 2, dicom) == 0)
        break;
      group_word = g_ntohs (GUINT16_SWAP_LE_BE (group_word));

      fread (&element_word, 1, 2, dicom);
      element_word = g_ntohs (GUINT16_SWAP_LE_BE (element_word));

      if (group_word != 0x0002 && big_endian)
        {
          group_word   = GUINT16_SWAP_LE_BE (group_word);
          element_word = GUINT16_SWAP_LE_BE (element_word);
        }

      tag = (group_word << 16) | element_word;
      fread(value_rep, 2, 1, dicom);
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
          /* Look up type from the dictionary. At the time we don't
             support this option... */
          gchar element_length_chars[4];

          /* Store the bytes that were read */
          element_length_chars[0] = value_rep[0];
          element_length_chars[1] = value_rep[1];

          /* Unknown value rep. It is not used right now anyhow */
          strcpy (value_rep, "??");

          /* For implicit value_values the length is always four bytes,
             so we need to read another two. */
          fread (&element_length_chars[2], 1, 2, dicom);

          /* Now cast to integer and insert into element_length */
          if (big_endian && group_word != 0x0002)
            element_length =
              g_ntohl (*((gint *) element_length_chars));
          else
            element_length =
              g_ntohl (GUINT32_SWAP_LE_BE (*((gint *) element_length_chars)));
      }
      /* Binary value reps are OB, OW, SQ or UN */
      else if (strncmp (value_rep, "OB", 2) == 0
          || strncmp (value_rep, "OW", 2) == 0
          || strncmp (value_rep, "SQ", 2) == 0
          || strncmp (value_rep, "UN", 2) == 0)
        {
          fread (&element_length, 1, 2, dicom); /* skip two bytes */
          fread (&element_length, 1, 4, dicom);
          if (big_endian && group_word != 0x0002)
            element_length = g_ntohl (element_length);
          else
            element_length = g_ntohl (GUINT32_SWAP_LE_BE (element_length));
        }
      /* Short length */
      else
        {
          guint16 el16;

          fread (&el16, 1, 2, dicom);
          if (big_endian && group_word != 0x0002)
            element_length = g_ntohs (el16);
          else
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
          g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                       _("'%s' has an incorrect value for field size. Possibly corrupt image."),
                       gimp_file_get_utf8_name (file));
          g_free (dicominfo);
          fclose (dicom);
          return NULL;
        }

      /* Read contents. Allocate a bit more to make room for casts to int
       below. */
      value = g_new0 (guint8, element_length + 4);
      actual_read = fread (value, 1, element_length, dicom);
      if (actual_read < element_length)
        {
          g_warning ("Missing data: needed %u bytes, got %u. Possibly corrupt image.",
                     element_length, (guint32) actual_read);
          element_length = actual_read;
        }

      /* ignore everything inside of a sequence */
      if (in_sequence)
        {
          g_free (value);
          continue;
        }
      /* Some special casts that are used below */
      ctx_us = *(guint16 *) value;
      if (big_endian && group_word != 0x0002)
        ctx_us = GUINT16_SWAP_LE_BE (ctx_us);

      g_debug ("group: %04x, element: %04x, length: %u",
               group_word, element_word, element_length);
      g_debug ("Value: %s", (char*)value);
      /* Recognize some critical tags */
      if (group_word == 0x0002)
        {
          switch (element_word)
            {
            case 0x0010:   /* transfer syntax id */
              if (strcmp("1.2.840.10008.1.2", (char*)value) == 0)
                {
                  implicit_encoding = TRUE;
                  g_debug ("Transfer syntax: Implicit VR Endian: Default Transfer Syntax for DICOM.");
                }
              else if (strcmp("1.2.840.10008.1.2.1", (char*)value) == 0)
                {
                  g_debug ("Transfer syntax: Explicit VR Little Endian.");
                }
              else if (strcmp("1.2.840.10008.1.2.1.99", (char*)value) == 0)
                {
                  g_debug ("Transfer syntax: Deflated Explicit VR Little Endian.");
                }
              else if (strcmp("1.2.840.10008.1.2.2", (char*)value) == 0)
                {
                  /* This Transfer Syntax was retired in 2006. For the most recent description of it, see PS3.5 2016b */
                  big_endian = TRUE;
                  g_debug ("Transfer syntax: Deprecated Explicit VR Big Endian.");
                }
              else
                {
                  g_debug ("Transfer syntax %s is not supported by GIMP.", (gchar *) value);
                  g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                              _("Transfer syntax %s is not supported by GIMP."),
                              (gchar *) value);
                  g_free (dicominfo);
                  fclose (dicom);
                  return NULL;
                }
              break;
            }
        }
      else if (group_word == 0x0028)
        {
          gboolean supported = TRUE;

          switch (element_word)
            {
            case 0x0002:  /* samples per pixel */
              samples_per_pixel = ctx_us;
              g_debug ("spp: %d", samples_per_pixel);
              break;
            case 0x0004:  /* photometric interpretation */
              g_debug ("photometric interpretation: %s", (gchar *) value);

              if (samples_per_pixel == 1)
                {
                  if (strncmp ((gchar *) value, "MONOCHROME1", 11) == 0)
                    {
                      /* The minimum sample value is intended to be displayed
                       * as white after any VOI gray scale transformations
                       * have been performed. */
                      dicominfo->bw_inverted = TRUE;
                    }
                  else if (strncmp ((gchar *) value, "MONOCHROME2", 11) == 0)
                    {
                      /* The minimum sample value is intended to be displayed
                       * as black after any VOI gray scale transformations
                       * have been performed. */
                      dicominfo->bw_inverted = FALSE;
                    }
                  else
                    supported = FALSE;
                }
              else if (samples_per_pixel == 3)
                {
                  if (strncmp ((gchar *) value, "RGB", 2) != 0)
                    {
                      supported = FALSE;
                    }
                }
              else
                {
                  supported = FALSE;
                }
              if (! supported)
                {
                  g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                               _("%s is not supported by GIMP in combination "
                                 "with samples per pixel: %d"),
                               (gchar *) value, samples_per_pixel);
                  g_free (dicominfo);
                  fclose (dicom);
                  return NULL;
                }

              break;
            case 0x0006:  /* planar configuration */
              g_debug ("planar configuration: %u", ctx_us);
              dicominfo->planar = (ctx_us == 1);
              break;
            case 0x0008:  /* number of frames */
              g_debug ("number of frames: %d", ctx_us);
              break;
            case 0x0010:  /* rows */
              height = ctx_us;
              g_debug ("height: %d", height);
              break;
            case 0x0011:  /* columns */
              width = ctx_us;
              g_debug ("width: %d", width);
              break;
            case 0x0100:  /* bits allocated */
              bpp = ctx_us;
              g_debug ("bpp: %d", bpp);
              break;
            case 0x0101:  /* bits stored */
              bits_stored = ctx_us;
              g_debug ("bits stored: %d", bits_stored);
              break;
            case 0x0102:  /* high bit */
              high_bit = ctx_us;
              g_debug ("high bit: %d", high_bit);
              break;
            case 0x0103:  /* is pixel representation signed? */
              is_signed = (ctx_us == 0) ? FALSE : TRUE;
              g_debug ("is signed: %d", ctx_us);
              break;
            }
        }

      /* Pixel data */
      if (group_word == 0x7fe0 && element_word == 0x0010)
        {
          pix_buf = value;
          pixbuf_size = element_length;
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
               * image is not valid.  keep the parasite around until
               * we're able to attach it.
               */

              /* add to our list of parasites to be added (prepending
               * for speed. we'll reverse it later)
               */
              elements = g_slist_prepend (elements, parasite);
            }

          g_free (value);
        }
    }

  g_debug ("Bpp: %d, wxh: %u x %u, spp: %d\n", bpp, width, height, samples_per_pixel);

  if ((bpp != 8) && (bpp != 16))
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("'%s' has a bpp of %d which GIMP cannot handle."),
                   gimp_file_get_utf8_name (file), bpp);
      g_free (pix_buf);
      g_free (dicominfo);
      fclose (dicom);
      return NULL;
    }

  if ((width > GIMP_MAX_IMAGE_SIZE) || (height > GIMP_MAX_IMAGE_SIZE))
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("'%s' has a larger image size (%d x %d) than GIMP can handle."),
                   gimp_file_get_utf8_name (file), width, height);
      g_free (pix_buf);
      g_free (dicominfo);
      fclose (dicom);
      return NULL;
    }

  if (samples_per_pixel > 3)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("'%s' has samples per pixel of %d which GIMP cannot handle."),
                   gimp_file_get_utf8_name (file), samples_per_pixel);
      g_free (pix_buf);
      g_free (dicominfo);
      fclose (dicom);
      return NULL;
    }

  if ((guint64) width * height * (bpp >> 3) * samples_per_pixel > pixbuf_size)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("'%s' has not enough pixel data. Possibly corrupt image."),
                   gimp_file_get_utf8_name (file));
      g_free (pix_buf);
      g_free (dicominfo);
      fclose (dicom);
      return NULL;
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
  image = gimp_image_new (dicominfo->width, dicominfo->height,
                          (dicominfo->samples_per_pixel >= 3 ?
                           GIMP_RGB : GIMP_GRAY));

  layer = gimp_layer_new (image, _("Background"),
                          dicominfo->width, dicominfo->height,
                          (dicominfo->samples_per_pixel >= 3 ?
                           GIMP_RGB_IMAGE : GIMP_GRAY_IMAGE),
                          100,
                          gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, NULL, 0);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

#if GUESS_ENDIAN
  if (bpp == 16)
    guess_and_set_endian2 ((guint16 *) pix_buf, width * height);
#endif

  dicom_loader (pix_buf, dicominfo, buffer);

  if (elements)
    {
      /* flip the parasites back around into the order they were
       * created (read from the file)
       */
      elements = g_slist_reverse (elements);
      /* and add each one to the image */
      g_slist_foreach (elements, add_parasites_to_image, image);
      g_slist_free (elements);
    }

  g_free (pix_buf);
  g_free (dicominfo);

  fclose (dicom);

  g_object_unref (buffer);

  return image;
}

static void
dicom_loader (guint8     *pix_buffer,
              DicomInfo  *info,
              GeglBuffer *buffer)
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
        buf16[pix_idx] = g_ntohs (GUINT16_SWAP_LE_BE  (buf16[pix_idx])) >> shift;
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
                  if (info->bw_inverted)
                    {
                      d[col_idx] = ~d[col_idx];
                    }

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
              if (! info->planar)
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
                      if (info->bw_inverted)
                        {
                          d[col_idx] = ~d[col_idx];
                        }

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
              else
                {
                  /* planar organization of color data */
                  guint8 *row_start;
                  gint    col_idx;
                  gint    plane_size = width * height;

                  row_start = (pix_buffer + (row_idx + i) * width);

                  for (col_idx = 0; col_idx < width; col_idx++)
                    {
                      /* Shift it by 0 bits, or more in case bits_stored is
                       * less than bpp.
                       */
                      gint  pix_idx;
                      gint  src_offset = col_idx;

                      for (pix_idx = 0; pix_idx < samples_per_pixel; pix_idx++)
                        {
                          gint  dest_idx = col_idx * samples_per_pixel + pix_idx;

                          d[dest_idx] = row_start[src_offset] << (8 - info->bits_stored);
                          if (info->is_signed)
                            {
                              /* If the data is negative, make it 0. Otherwise,
                               * multiply the positive value by 2, so that the
                               * positive values span between 0 and 254.
                               */
                              if (d[dest_idx] > 127)
                                d[dest_idx] = 0;
                              else
                                d[dest_idx] <<= 1;
                            }
                          src_offset += plane_size;
                        }
                    }
                }
            }

          d += width * samples_per_pixel;
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, row_idx, width, scanlines), 0,
                       NULL, data, GEGL_AUTO_ROWSTRIDE);

      row_idx += scanlines;

      gimp_progress_update ((gdouble) row_idx / (gdouble) height);
    }

  g_free (data);

  gimp_progress_update (1.0);
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
 * Returns: the new head of @elements
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
  g_strlcpy (element->value_rep, value_rep, sizeof (element->value_rep));
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
                                g_memdup2 (value, element_length));

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
 * Returns: the new head of @elements
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
 * Returns: the new head of @elements
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
 * Returns: an integer indicating the equality of @a and @b.
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
 * Returns: a DICOMELEMENT matching the specified group,element,
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
 * @image: the image from which to read parasites in order to
 *         retrieve the dicom elements
 *
 * Reads all DICOMELEMENTs from the specified image's parasites.
 *
 * Returns: a GSList of all known dicom elements
**/
static GSList *
dicom_get_elements_list (GimpImage *image)
{
  GSList        *elements = NULL;
  GimpParasite  *parasite;
  gchar        **parasites = NULL;

  parasites = gimp_image_get_parasite_list (image);

  if (parasites)
    {
      guint count;
      guint i;

      count = g_strv_length (parasites);

      for (i = 0; i < count; i++)
        {
          if (strncmp (parasites[i], "dcm", 3) == 0)
            {
              parasite = gimp_image_get_parasite (image, parasites[i]);

              if (parasite)
                {
                  gchar buf[1024];
                  gchar *ptr1;
                  gchar *ptr2;
                  gchar value_rep[3]   = "";
                  guint16 group_word   = 0;
                  guint16 element_word = 0;

                  /* sacrificial buffer */
                  g_strlcpy (buf, parasites[i], sizeof (buf));

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
                    g_strlcpy (value_rep, ptr1, sizeof (value_rep));

                  /*
                   * If all went according to plan, we should be able
                   * to add this element
                   */
                  if (group_word > 0 && element_word > 0)
                    {
                      const guint8 *val;
                      guint32       len;

                      val = (const guint8 *) gimp_parasite_get_data (parasite, &len);

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
        }
    }

  /* cleanup the array of names */
  g_strfreev (parasites);

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
 * Returns: the new head of @elements
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
 * Returns: the new head of @elements
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
    { 0x0002, 0x0010, "UI",
      strlen ("1.2.840.10008.1.2.1"), (guint8 *) "1.2.840.10008.1.2.1"},
    /* 0002, 0013 - Implementation version name */
    { 0x0002, 0x0013, "SH",
      strlen ("GIMP Dicom Plugin 1.0"), (guint8 *) "GIMP Dicom Plugin 1.0" },
    /* Identifying group */
    /* ImageType */
    { 0x0008, 0x0008, "CS",
      strlen ("ORIGINAL\\PRIMARY"), (guint8 *) "ORIGINAL\\PRIMARY" },
    { 0x0008, 0x0016, "UI",
      strlen ("1.2.840.10008.5.1.4.1.1.7"), (guint8 *) "1.2.840.10008.5.1.4.1.1.7" },
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
    /* Content Time */
    { 0x0008, 0x0030, "TM",
      strlen ("000000.000000"), (guint8 *) "000000.000000"},
    /* AccessionNumber */
    { 0x0008, 0x0050, "SH", strlen (""), (guint8 *) "" },
    /* Modality */
    { 0x0008, 0x0060, "CS", strlen ("MR"), (guint8 *) "MR" },
    /* ConversionType */
    { 0x0008, 0x0064, "CS", strlen ("WSD"), (guint8 *) "WSD" },
    /* ReferringPhysiciansName */
    { 0x0008, 0x0090, "PN", strlen (""), (guint8 *) "" },
    /* Patient group */
    /* Patient name */
    { 0x0010,  0x0010, "PN",
      strlen ("DOE^WILBER"), (guint8 *) "DOE^WILBER" },
    /* Patient ID */
    { 0x0010,  0x0020, "LO",
      strlen ("314159265"), (guint8 *) "314159265" },
    /* Patient Birth date */
    { 0x0010,  0x0030, "DA",
      strlen (today_string), (guint8 *) today_string },
    /* Patient sex */
    { 0x0010,  0x0040, "CS", strlen (""), (guint8 *) "" /* unknown */ },
    /* Relationship group */
    /* StudyId */
    { 0x0020, 0x0010, "IS", strlen ("1"), (guint8 *) "1" },
    /* SeriesNumber */
    { 0x0020, 0x0011, "IS", strlen ("1"), (guint8 *) "1" },
    /* AcquisitionNumber */
    { 0x0020, 0x0012, "IS", strlen ("1"), (guint8 *) "1" },
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

/* export_image() saves an image in the dicom format. The DICOM format
 * requires a lot of tags to be set. Some of them have real uses, others
 * must just be filled with dummy values.
 */
static gboolean
export_image (GFile        *file,
              GimpImage    *image,
              GimpDrawable *drawable,
              GError      **error)
{
  FILE          *dicom;
  GimpImageType  drawable_type;
  GeglBuffer    *buffer;
  const Babl    *format;
  gint           width;
  gint           height;
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

  drawable_type = gimp_drawable_type (drawable);

  /*  Make sure we're not saving an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable))
    {
      g_message (_("Cannot save images with alpha channel."));
      return FALSE;
    }

  switch (drawable_type)
    {
    case GIMP_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      samples_per_pixel = 1;
      photometric_interp = "MONOCHROME2";
      break;

    case GIMP_RGB_IMAGE:
      format = babl_format ("R'G'B' u8");
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
  dicom = g_fopen (g_file_peek_path (file), "wb");

  if (! dicom)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  buffer = gimp_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  /* Print dicom header */
  {
    guint8 val = 0;
    gint   i;

    for (i = 0; i < 0x80; i++)
      fwrite (&val, 1, 1, dicom);
  }
  fprintf (dicom, "DICM");

  group_stream = g_byte_array_new ();

  elements = dicom_get_elements_list (image);
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
                                    (guint8 *) &height);
  /* columns */
  elements = dicom_add_element_int (elements, group, 0x0011, "US",
                                    (guint8 *) &width);
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
  src = g_new (guchar, height * width * samples_per_pixel);
  if (src)
    {
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
                       format, src,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      elements = dicom_add_element (elements, group, 0x0010, "OW",
                                    width * height * samples_per_pixel,
                                    (guint8 *) src);

      elements = dicom_add_tags (dicom, group_stream, elements);

      g_free (src);
    }
  else
    {
      retval = FALSE;
    }

  fclose (dicom);

  dicom_elements_destroy (elements);
  g_byte_array_free (group_stream, TRUE);
  g_object_unref (buffer);

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
    FILE       *dicom;
    GByteArray *group_stream;
    gint        last_group;
  } *d = user_data;
  DICOMELEMENT *e = (DICOMELEMENT *) data;

  if (d->last_group >= 0 && e->group_word != d->last_group)
    {
      write_group_to_file (d->dicom, d->last_group, d->group_stream);
    }

  add_tag_pointer (d->group_stream,
                   e->group_word, e->element_word,
                   e->value_rep,e->value, e->element_length);
  d->last_group = e->group_word;
}

/**
 * dicom_add_tags:
 * @dicom:        File pointer to which @elements should be written.
 * @group_stream: byte array used for staging Dicom Element groups
 *                before flushing them to disk.
 * @elements:     GSList container the Dicom Element elements from
 *
 * Writes all Dicom tags in @elements to the file @dicom
 *
 * Returns: the new head of @elements
**/
static GSList *
dicom_add_tags (FILE       *dicom,
                GByteArray *group_stream,
                GSList     *elements)
{
  struct {
    FILE       *dicom;
    GByteArray *group_stream;
    gint        last_group;
  } data = { dicom, group_stream, -1 };

  elements = g_slist_sort (elements, dicom_elements_compare);
  g_slist_foreach (elements, dicom_print_tags, &data);
  /* make sure that the final group is written to the file */
  write_group_to_file (data.dicom, data.last_group, data.group_stream);

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
          g_byte_array_append (group_stream, (guint8 *) "\0", 1);
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
write_group_to_file (FILE       *dicom,
                     gint        group,
                     GByteArray *group_stream)
{
  gboolean retval = TRUE;
  guint16  swapped16;
  guint32  swapped32;

  /* Add header to the group and output it */
  swapped16 = g_ntohs (GUINT16_SWAP_LE_BE (group));

  fwrite ((gchar *) &swapped16, 1, 2, dicom);
  fputc (0, dicom);
  fputc (0, dicom);
  fputc ('U', dicom);
  fputc ('L', dicom);

  swapped16 = g_ntohs (GUINT16_SWAP_LE_BE (4));
  fwrite ((gchar *) &swapped16, 1, 2, dicom);

  swapped32 = g_ntohl (GUINT32_SWAP_LE_BE (group_stream->len));
  fwrite ((gchar *) &swapped32, 1, 4, dicom);

  if (fwrite (group_stream->data,
              1, group_stream->len, dicom) != group_stream->len)
    retval = FALSE;

  g_byte_array_set_size (group_stream, 0);

  return retval;
}
