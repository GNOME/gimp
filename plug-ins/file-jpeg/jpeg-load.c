/* LIGMA - The GNU Image Manipulation Program
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

#include <string.h>
#include <errno.h>
#include <setjmp.h>

#include <gio/gio.h>
#include <glib/gstdio.h>
#include <gexiv2/gexiv2.h>

#include <jpeglib.h>
#include <jerror.h>

#include <lcms2.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"

#include "jpeg.h"
#include "jpeg-icc.h"
#include "jpeg-settings.h"
#include "jpeg-load.h"

static gboolean  jpeg_load_resolution       (LigmaImage *image,
                                             struct jpeg_decompress_struct
                                                       *cinfo);

static void      jpeg_load_sanitize_comment (gchar    *comment);


LigmaImage * volatile  preview_image;
LigmaLayer *           preview_layer;

LigmaImage *
load_image (GFile        *file,
            LigmaRunMode   runmode,
            gboolean      preview,
            gboolean     *resolution_loaded,
            GError      **error)
{
  LigmaImage * volatile image;
  LigmaLayer           *layer;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr           jerr;
  jpeg_saved_marker_ptr         marker;
  FILE              *infile;
  guchar            *buf;
  guchar           **rowbuf;
  LigmaImageBaseType  image_type;
  LigmaImageType      layer_type;
  GeglBuffer        *buffer       = NULL;
  const Babl        *format;
  const Babl        *space;
  const gchar       *encoding;
  const gchar       *layer_name   = NULL;
  LigmaColorProfile  *cmyk_profile = NULL;
  gint               tile_height;
  gint               i;

  /* We set up the normal JPEG error routines. */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  if (! preview)
    {
      jerr.pub.output_message = my_output_message;

      ligma_progress_init_printf (_("Opening '%s'"),
                                 ligma_file_get_utf8_name (file));
    }

  infile = g_fopen (g_file_peek_path (file), "rb");

  if (! infile)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   ligma_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  image = NULL;

  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_decompress (&cinfo);
      if (infile)
        fclose (infile);

      if (image && !preview)
        ligma_image_delete (image);

      if (preview)
        destroy_preview ();

      if (buffer)
        g_object_unref (buffer);

      return NULL;
    }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src (&cinfo, infile);

  if (! preview)
    {
      /* - step 2.1: tell the lib to save the comments */
      jpeg_save_markers (&cinfo, JPEG_COM, 0xffff);

      /* - step 2.2: tell the lib to save APP1 data (Exif or XMP) */
      jpeg_save_markers (&cinfo, JPEG_APP0 + 1, 0xffff);

      /* - step 2.3: tell the lib to save APP2 data (ICC profiles) */
      jpeg_save_markers (&cinfo, JPEG_APP0 + 2, 0xffff);
    }

  /* Step 3: read file parameters with jpeg_read_header() */

  jpeg_read_header (&cinfo, TRUE);

  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here, except set the DCT
   * method.
   */

  cinfo.dct_method = JDCT_FLOAT;

  /* Step 5: Start decompressor */

  jpeg_start_decompress (&cinfo);

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   */

  /* temporary buffer */
  tile_height = ligma_tile_height ();
  buf = g_new (guchar,
               tile_height * cinfo.output_width * cinfo.output_components);

  rowbuf = g_new (guchar *, tile_height);

  for (i = 0; i < tile_height; i++)
    rowbuf[i] = buf + cinfo.output_width * cinfo.output_components * i;

  switch (cinfo.output_components)
    {
    case 1:
      image_type = LIGMA_GRAY;
      layer_type = LIGMA_GRAY_IMAGE;
      break;

    case 3:
      image_type = LIGMA_RGB;
      layer_type = LIGMA_RGB_IMAGE;
      break;

    case 4:
      if (cinfo.out_color_space == JCS_CMYK)
        {
          image_type = LIGMA_RGB;
          layer_type = LIGMA_RGB_IMAGE;
          break;
        }
      /*fallthrough*/

    default:
      g_message ("Don't know how to load JPEG images "
                 "with %d color channels, using colorspace %d (%d).",
                 cinfo.output_components, cinfo.out_color_space,
                 cinfo.jpeg_color_space);
      return NULL;
      break;
    }

  if (preview)
    {
      layer_name = _("JPEG preview");

      image = preview_image;
    }
  else
    {
      GString *comment_buffer = NULL;
      guint8  *icc_data       = NULL;
      guint    icc_length     = 0;

      layer_name = _("Background");

      image = ligma_image_new_with_precision (cinfo.output_width,
                                             cinfo.output_height,
                                             image_type,
                                             LIGMA_PRECISION_U8_NON_LINEAR);

      ligma_image_undo_disable (image);
      ligma_image_set_file (image, file);

      /* Step 5.0: save the original JPEG settings in a parasite */
      jpeg_detect_original_settings (&cinfo, image);

      /* Step 5.1: check for comments, or Exif metadata in APP1 markers */
      for (marker = cinfo.marker_list; marker; marker = marker->next)
        {
          const gchar *data = (const gchar *) marker->data;
          gsize        len  = marker->data_length;

          if (marker->marker == JPEG_COM)
            {
#ifdef LIGMA_UNSTABLE
              g_print ("jpeg-load: found image comment (%d bytes)\n",
                       marker->data_length);
#endif

              if (! comment_buffer)
                {
                  comment_buffer = g_string_new_len (data, len);
                }
              else
                {
                  /* concatenate multiple comments, separate them with LF */
                  g_string_append_c (comment_buffer, '\n');
                  g_string_append_len (comment_buffer, data, len);
                }
            }
          else if ((marker->marker == JPEG_APP0 + 1)
                   && (len > sizeof (JPEG_APP_HEADER_EXIF) + 8)
                   && ! strcmp (JPEG_APP_HEADER_EXIF, data))
            {
#ifdef LIGMA_UNSTABLE
              g_print ("jpeg-load: found Exif block (%d bytes)\n",
                       (gint) (len - sizeof (JPEG_APP_HEADER_EXIF)));
#endif
            }
        }

      if (jpeg_load_resolution (image, &cinfo))
        {
          if (resolution_loaded)
            *resolution_loaded = TRUE;
        }

      /* if we found any comments, then make a parasite for them */
      if (comment_buffer && comment_buffer->len)
        {
          LigmaParasite *parasite;

          jpeg_load_sanitize_comment (comment_buffer->str);
          parasite = ligma_parasite_new ("ligma-comment",
                                        LIGMA_PARASITE_PERSISTENT,
                                        strlen (comment_buffer->str) + 1,
                                        comment_buffer->str);
          ligma_image_attach_parasite (image, parasite);
          ligma_parasite_free (parasite);

          g_string_free (comment_buffer, TRUE);
        }

      /* Step 5.3: check for an embedded ICC profile in APP2 markers */
      jpeg_icc_read_profile (&cinfo, &icc_data, &icc_length);

      if (icc_data)
        {
          LigmaColorProfile *profile;

          profile = ligma_color_profile_new_from_icc_profile (icc_data,
                                                             icc_length,
                                                             NULL);
          if (cinfo.out_color_space == JCS_CMYK)
            {
              /* don't attach the profile if we are transforming */
              cmyk_profile = profile;
              profile = NULL;
            }

          if (profile)
            {
              ligma_image_set_color_profile (image, profile);
              g_object_unref (profile);
            }
        }

      g_free (icc_data);

      /* Do not attach the "jpeg-save-options" parasite to the image
       * because this conflicts with the global defaults (bug #75398).
       */
    }

  layer = ligma_layer_new (image, layer_name,
                          cinfo.output_width,
                          cinfo.output_height,
                          layer_type,
                          100,
                          ligma_image_get_default_new_layer_mode (image));

  if (preview)
    preview_layer = layer;

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

  if (cinfo.out_color_space == JCS_CMYK)
    {
      encoding = "cmyk u8";
      if (cmyk_profile)
        {
          space = ligma_color_profile_get_space (cmyk_profile,
                                                LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                error);
          ligma_image_set_simulation_profile (image, cmyk_profile);
        }
      else
        {
          space = NULL;
        }
    }
  else
    {
      if (image_type == LIGMA_RGB)
        encoding = "R'G'B' u8";
      else
        encoding = "Y' u8";
      space = ligma_drawable_get_format (LIGMA_DRAWABLE (layer));
    }
  format = babl_format_with_space (encoding, space);

  while (cinfo.output_scanline < cinfo.output_height)
    {
      gint     start, end;
      gint     scanlines;
      gboolean image_truncated = FALSE;

      start = cinfo.output_scanline;
      end   = cinfo.output_scanline + tile_height;
      end   = MIN (end, cinfo.output_height);

      scanlines = end - start;

      /*  in case of error we now jump here, so pertially loaded imaged
       *  don't get discarded
       */
      if (setjmp (jerr.setjmp_buffer))
        {
          image_truncated = TRUE;

          goto set_buffer;
        }

      for (i = 0; i < scanlines; i++)
        jpeg_read_scanlines (&cinfo, (JSAMPARRAY) &rowbuf[i], 1);

    set_buffer:
      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, start, cinfo.output_width, scanlines),
                       0,
                       format,
                       buf,
                       GEGL_AUTO_ROWSTRIDE);

      if (image_truncated)
        /*  jumping to finish skips jpeg_finish_decompress(), its state
         *  might be broken by whatever caused the loading failure
         */
        goto finish;

      if (! preview && (cinfo.output_scanline % 32) == 0)
        ligma_progress_update ((gdouble) cinfo.output_scanline /
                              (gdouble) cinfo.output_height);
    }

  /* Step 7: Finish decompression */

  jpeg_finish_decompress (&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

 finish:

  g_clear_object (&cmyk_profile);
  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress (&cinfo);

  g_object_unref (buffer);

  /* free up the temporary buffers */
  g_free (rowbuf);
  g_free (buf);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose (infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.num_warnings is nonzero).
   */

  /* Detach from the drawable and add it to the image.
   */
  if (! preview)
    {
      ligma_progress_update (1.0);
    }

  ligma_image_insert_layer (image, layer, NULL, 0);

  return image;
}

static gboolean
jpeg_load_resolution (LigmaImage                     *image,
                      struct jpeg_decompress_struct *cinfo)
{
  if (cinfo->saw_JFIF_marker && cinfo->X_density != 0 && cinfo->Y_density != 0)
    {
      gdouble xresolution = cinfo->X_density;
      gdouble yresolution = cinfo->Y_density;
      gdouble asymmetry   = 1.0;

      switch (cinfo->density_unit)
        {
        case 0: /* unknown -> set the aspect ratio but use the default
                 *  image resolution
                 */
          asymmetry = xresolution / yresolution;

          ligma_image_get_resolution (image, &xresolution, &yresolution);

          xresolution *= asymmetry;
          break;

        case 1: /* dots per inch */
          break;

        case 2: /* dots per cm */
          xresolution *= 2.54;
          yresolution *= 2.54;
          ligma_image_set_unit (image, LIGMA_UNIT_MM);
          break;

        default:
          g_message ("Unknown density unit %d, assuming dots per inch.",
                     cinfo->density_unit);
          break;
        }

      ligma_image_set_resolution (image, xresolution, yresolution);

      return TRUE;
    }

  return FALSE;
}

/*
 * A number of JPEG files have comments written in a local character set
 * instead of UTF-8.  Some of these files may have been saved by older
 * versions of LIGMA.  It is not possible to reliably detect the character
 * set used, but it is better to keep all characters in the ASCII range
 * and replace the non-ASCII characters instead of discarding the whole
 * comment.  This is especially useful if the comment contains only a few
 * non-ASCII characters such as a copyright sign, a soft hyphen, etc.
 */
static void
jpeg_load_sanitize_comment (gchar *comment)
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

LigmaImage *
load_thumbnail_image (GFile         *file,
                      gint          *width,
                      gint          *height,
                      LigmaImageType *type,
                      GError       **error)
{
  LigmaImage * volatile          image = NULL;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr           jerr;
  FILE                         *infile   = NULL;

  ligma_progress_init_printf (_("Opening thumbnail for '%s'"),
                             g_file_get_parse_name (file));

  image = ligma_image_metadata_load_thumbnail (file, error);
  if (! image)
    return NULL;

  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit     = my_error_exit;
  jerr.pub.output_message = my_output_message;

  if ((infile = g_fopen (g_file_peek_path (file), "rb")) == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   g_file_get_parse_name (file), g_strerror (errno));

      if (image)
        ligma_image_delete (image);

      return NULL;
    }

  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.  We
       * need to clean up the JPEG object, close the input file,
       * and return.
       */
      jpeg_destroy_decompress (&cinfo);

      if (image)
        ligma_image_delete (image);

      return NULL;
    }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src (&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  jpeg_read_header (&cinfo, TRUE);

  jpeg_start_decompress (&cinfo);

  *width  = cinfo.output_width;
  *height = cinfo.output_height;

  switch (cinfo.output_components)
    {
    case 1:
      *type = LIGMA_GRAY_IMAGE;
      break;

    case 3:
      *type = LIGMA_RGB_IMAGE;
      break;

    case 4:
      if (cinfo.out_color_space == JCS_CMYK)
        {
          *type = LIGMA_RGB_IMAGE;
          break;
        }
      /*fallthrough*/

    default:
      g_message ("Don't know how to load JPEG images "
                 "with %d color channels, using colorspace %d (%d).",
                 cinfo.output_components, cinfo.out_color_space,
                 cinfo.jpeg_color_space);

      ligma_image_delete (image);
      image = NULL;
      break;
    }

  /* Step 4: Release JPEG decompression object */

  /* This is an important step since it will release a good deal
   * of memory.
   */
  jpeg_destroy_decompress (&cinfo);

  fclose (infile);

  return image;
}
