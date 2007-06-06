/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * PNM reading and writing code Copyright (C) 1996 Erik Nygren
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#define PLUG_IN_BINARY "dicom"


/* A lot of Dicom images are wrongly encoded. By guessing the endian
 * we can get around this problem.
 */
#define GUESS_ENDIAN 1

/* Declare local data types */
typedef struct _DicomInfo
{
  gint       width, height;	 /* The size of the image                  */
  gint       maxval;		 /* For 16 and 24 bit image files, the max
				    value which we need to normalize to    */
  gint       samples_per_pixel;  /* Number of image planes (0 for pbm)     */
  gint       bpp;
} DicomInfo;

/* Local function prototypes */
static void      query                 (void);
static void      run                   (const gchar      *name,
                                        gint              nparams,
                                        const GimpParam  *param,
                                        gint             *nreturn_vals,
                                        GimpParam       **return_vals);
static gint32    load_image            (const gchar      *filename);
static gboolean  save_image            (const gchar      *filename,
                                        gint32            image_ID,
                                        gint32            drawable_ID);
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
static void      add_tag_string        (GByteArray       *group_stream,
                                        gint              group,
                                        gint              element,
                                        const gchar      *value_rep,
                                        const gchar      *s);
static void      add_tag_int           (GByteArray       *group_stream,
                                        gint              group,
                                        gint              element,
                                        const gchar      *value_rep,
                                        gint              value);
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
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,    "image",        "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
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
                          N_("Digital Imaging and Communications in Medicine image"),
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

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;

	  values[1].type         = GIMP_PDB_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
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
	  export = gimp_export_image (&image_ID, &drawable_ID, "DICOM",
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
	  if (! save_image (param[3].data.d_string, image_ID, drawable_ID))
	    status = GIMP_PDB_EXECUTION_ERROR;
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

static gint32
load_image (const gchar *filename)
{
  GimpPixelRgn    pixel_rgn;
  gint32 volatile image_ID          = -1;
  gint32          layer_ID;
  GimpDrawable   *drawable;
  FILE           *DICOM;
  gchar           buf[500];    /* buffer for random things like scanning */
  DicomInfo      *dicominfo;
  gint            width             = 0;
  gint            height            = 0;
  gint            samples_per_pixel = 0;
  gint            bpp               = 0;
  guint8         *pix_buf           = NULL;
  gboolean        toggle_endian     = FALSE;

  /* open the file */
  DICOM = g_fopen (filename, "rb");

  if (!DICOM)
    {
      g_message (_("Could not open '%s' for reading: %s"),
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
      return -1;
    }

  fread (buf, 1, 4, DICOM); /* This should be dicom */
  if (g_ascii_strncasecmp (buf,"DICM",4) != 0)
    {
      g_message (_("'%s' is not a DICOM file."),
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  while (!feof (DICOM))
    {
      guint16  group_word;
      guint16  element_word;
      gchar    value_rep[3];
      guint32  element_length;
      guint32  ctx_ul;
      guint16  ctx_us;
      guint8  *value;
      guint32  tag;
      gboolean do_toggle_endian= FALSE;
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
	continue;

      /* Sequence of items item tag... Ignore as well */
      if (tag == 0xFFFEE000)
	continue;

      /* Read contents. Allocate a bit more to make room for casts to int
       below. */
      value = g_new0 (guint8, element_length + 4);
      fread (value, 1, element_length, DICOM);

      /* Some special casts that are used below */
      ctx_ul = *(guint32 *) value;
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
	    case 0x0100:  /* bits_allocated */
	      bpp = ctx_us;
	      break;
	    case 0x0103:  /* pixel representation */
	      toggle_endian = ctx_us;
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
          g_free (value);
        }
    }

  dicominfo->width  = width;
  dicominfo->height = height;
  dicominfo->bpp    = bpp;
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
  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable,
		       0, 0, drawable->width, drawable->height, TRUE, FALSE);

#if GUESS_ENDIAN
  if (bpp == 16)
    guess_and_set_endian2 ((guint16 *) pix_buf, width * height);
#endif

  dicom_loader (pix_buf, dicominfo, &pixel_rgn);

  /* free the structures */
  g_free (pix_buf);
  g_free (dicominfo);

  /* close the file */
  fclose (DICOM);

  /* Tell GIMP to display the image. */
  gimp_drawable_flush (drawable);

  return image_ID;
}

static void
dicom_loader (guint8        *pix_buffer,
	      DicomInfo     *info,
	      GimpPixelRgn  *pixel_rgn)
{
  guchar  *data, *d;
  gint     row_idx, i;
  gint     start, end, scanlines;
  gint     width             = info->width;
  gint     height            = info->height;
  gint     samples_per_pixel = info->samples_per_pixel;
  guint16 *buf16             = (guint16 *) pix_buffer;
  gint     pix_idx;
  guint16  max               = 0;

  if (info->bpp == 16)
    {
      /* Reorder the buffer and look for max */
      max = 0;
      for (pix_idx=0; pix_idx < width * height * samples_per_pixel; pix_idx++)
	{
	  guint pix_gl = g_ntohs (GUINT16_SWAP_LE_BE (buf16[pix_idx]));

	  if (pix_gl > max)
	    max = pix_gl;

	  buf16[pix_idx] = pix_gl;
	}
    }

  data = g_malloc (gimp_tile_height () * width * samples_per_pixel);

  for (row_idx = 0; row_idx < height; )
    {
      start = row_idx;
      end = row_idx + gimp_tile_height ();
      end = MIN (end, height);
      scanlines = end - start;
      d = data;

      for (i = 0; i < scanlines; i++)
	{
	  if (info->bpp == 16)
	    {
	      gint     col_idx;
	      guint16 *row_start;

              row_start = buf16 + (row_idx + i) * width * samples_per_pixel;

	      for (col_idx = 0; col_idx < width * samples_per_pixel; col_idx++)
		d[col_idx] =
                  (guint8) (255.0 * (gdouble)(row_start[col_idx]) / max);
	    }
	  else if (info->bpp == 8)
	    {
	      gint    col_idx;
	      guint8 *row_start;

              row_start = pix_buffer + (row_idx + i) * width * samples_per_pixel;
	      for (col_idx = 0; col_idx < width * samples_per_pixel; col_idx++)
		d[col_idx] = row_start[col_idx];
	    }

	  d += width * samples_per_pixel;
	}

      gimp_progress_update ((gdouble) row_idx / (gdouble) height);
      gimp_pixel_rgn_set_rect (pixel_rgn, data, 0, row_idx, width, scanlines);
      row_idx += scanlines;
    }

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

/* save_image() saves an image in the dicom format. The DICOM format
 * requires a lot of tags to be set. Some of them have real uses, others
 * must just be filled with dummy values.
 */
static gboolean
save_image (const gchar  *filename,
	    gint32        image_ID,
	    gint32        drawable_ID)
{
  FILE          *DICOM;
  GimpImageType  drawable_type;
  GimpDrawable  *drawable;
  GimpPixelRgn   pixel_rgn;
  GByteArray    *group_stream;
  gint           group;
  GDate         *date;
  gchar          today_string[16];
  gchar         *photometric_interp;
  gint           samples_per_pixel;
  gboolean       retval = TRUE;

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
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      gimp_drawable_detach (drawable);
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

  /* Meta element group */
  group = 0x0002;
  /* 0002,0001 - File Meta Information Version */
  add_tag_pointer (group_stream, group, 0x0001, "OB",
                   (const guint8 *) "\0\1", 2);
  /* 0002,0010 - Transfer syntax uid */
  add_tag_string (group_stream, group, 0x0010, "UI", "1.2.840.10008.1.2.1");
  /* 0002,0013 - Implementation version name */
  add_tag_string (group_stream, group, 0x0013, "SH", "Gimp Dicom Plugin 1.0");
  write_group_to_file (DICOM, group, group_stream);

  /* Identifying group */
  group = 0x0008;
  /* Study date */
  add_tag_string (group_stream, group, 0x0020, "DA", today_string);
  /* Series date */
  add_tag_string (group_stream, group, 0x0021, "DA", today_string);
  /* Acquisition date */
  add_tag_string (group_stream, group, 0x0022, "DA", today_string);
  /* Content Date */
  add_tag_string (group_stream, group, 0x0023, "DA", today_string);
  /* Modality - I have to add something.. */
  add_tag_string (group_stream, group, 0x0060, "CS", "MR");
  write_group_to_file (DICOM, group, group_stream);

  /* Patient group */
  group = 0x0010;
  /* Patient name */
  add_tag_string (group_stream, group, 0x0010, "PN", "Wilber Doe");
  /* Patient ID */
  add_tag_string (group_stream, group, 0x0020, "CS", "314159265");
  /* Patient Birth date */
  add_tag_string (group_stream, group, 0x0030, "DA", today_string);
  /* Patient sex */
  add_tag_string (group_stream, group, 0x0040, "CS", "" /* unknown */);
  write_group_to_file (DICOM, group, group_stream);

  /* Relationship group */
  group = 0x0020;
  /* Instance number */
  add_tag_string (group_stream, group, 0x0013, "IS", "1");
  write_group_to_file (DICOM, group, group_stream);

  /* Image presentation group */
  group = 0x0028;
  /* Samples per pixel */
  add_tag_int   (group_stream, group, 0x0002, "US", samples_per_pixel);
  /* Photometric interpretation */
  add_tag_string (group_stream, group, 0x0004, "CS", photometric_interp);
  /* Planar configuration for color images */
  if (samples_per_pixel == 3)
    add_tag_int (group_stream, group, 0x0006, "US", 0);
  /* rows */
  add_tag_int   (group_stream, group, 0x0010, "US", drawable->height);
  /* columns */
  add_tag_int   (group_stream, group, 0x0011, "US", drawable->width);
  /* Bits allocated */
  add_tag_int   (group_stream, group, 0x0100, "US", 8);
  /* Bits Stored */
  add_tag_int   (group_stream, group, 0x0101, "US", 8);
  /* High bit */
  add_tag_int   (group_stream, group, 0x0102, "US", 7);
  /* Pixel representation */
  add_tag_int   (group_stream, group, 0x0103, "US", 0);
  write_group_to_file (DICOM, group, group_stream);

  /* Pixel data */
  group = 0x7fe0;
  {
    guchar *src;

    src = g_new (guchar,
                 drawable->height * drawable->width * samples_per_pixel);

    gimp_pixel_rgn_init (&pixel_rgn, drawable,
                         0, 0, drawable->width, drawable->height,
                         FALSE, FALSE);
    gimp_pixel_rgn_get_rect (&pixel_rgn,
                             src, 0, 0, drawable->width, drawable->height);
    add_tag_pointer (group_stream, group, 0x0010, "OW", (guint8 *) src,
                     drawable->width * drawable->height * samples_per_pixel);

    g_free (src);
  }

  retval = write_group_to_file (DICOM, group, group_stream);

  fclose (DICOM);

  g_byte_array_free (group_stream, TRUE);
  gimp_drawable_detach (drawable);

  return retval;
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

  is_long = (strstr ("OB|OW|SQ|UN", value_rep) != NULL) || length > 65535;

  swapped16 = g_ntohs (GUINT16_SWAP_LE_BE (group));
  g_byte_array_append (group_stream, (guint8 *) &swapped16, 2);

  swapped16 = g_ntohs (GUINT16_SWAP_LE_BE (element));
  g_byte_array_append (group_stream, (guint8 *) &swapped16, 2);

  g_byte_array_append (group_stream, (const guchar *) value_rep, 2);
  if (is_long)
    {

      g_byte_array_append (group_stream, (const guchar *) "\0\0", 2);

      swapped32 = g_ntohl (GUINT32_SWAP_LE_BE (length));
      g_byte_array_append (group_stream, (guint8 *) &swapped32, 4);
    }
  else
    {
      swapped16 = g_ntohs (GUINT16_SWAP_LE_BE (length));
      g_byte_array_append (group_stream, (guint8 *) &swapped16, 2);
    }

  g_byte_array_append (group_stream, data, length);
}

/* Convenience function for adding a string to the dicom stream */
static void
add_tag_string (GByteArray  *group_stream,
		gint         group,
		gint         element,
		const gchar *value_rep,
		const gchar *s)
{
  add_tag_pointer (group_stream,
                   group, element, value_rep, (const guint8 *) s, strlen (s));
}

/* Convenience function for adding an integer to the dicom stream */
static void
add_tag_int (GByteArray  *group_stream,
             gint         group,
             gint         element,
             const gchar *value_rep,
             gint         value)
{
  gint len;

  if (strcmp (value_rep, "US") == 0)
    len = 2;
  else
    len = 4;

  add_tag_pointer (group_stream,
                   group, element, value_rep, (const guint8 *) &value, len);
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
