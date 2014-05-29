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

#include <string.h>
#include <errno.h>
#include <setjmp.h>

#include <gio/gio.h>
#include <glib/gstdio.h>
#include <gexiv2/gexiv2.h>

#include <jpeglib.h>
#include <jerror.h>

#include <lcms2.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "jpeg.h"
#include "jpeg-icc.h"
#include "jpeg-settings.h"
#include "jpeg-load.h"

static gboolean  jpeg_load_resolution       (gint32    image_ID,
                                             struct jpeg_decompress_struct
                                                       *cinfo);

static void      jpeg_load_sanitize_comment (gchar    *comment);

static gpointer  jpeg_load_cmyk_transform   (guint8   *profile_data,
                                             gsize     profile_len);
static void      jpeg_load_cmyk_to_rgb      (guchar   *buf,
                                             glong     pixels,
                                             gpointer  transform);

gint32 volatile  preview_image_ID;
gint32           preview_layer_ID;

gint32
load_image (const gchar  *filename,
            GimpRunMode   runmode,
            gboolean      preview,
            gboolean     *resolution_loaded,
            GError      **error)
{
  gint32 volatile  image_ID;
  gint32           layer_ID;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr           jerr;
  jpeg_saved_marker_ptr         marker;
  FILE            *infile;
  guchar          *buf;
  guchar         **rowbuf;
  GimpImageBaseType image_type;
  GimpImageType    layer_type;
  GeglBuffer      *buffer = NULL;
  const Babl      *format;
  gint             tile_height;
  gint             scanlines;
  gint             i, start, end;
  cmsHTRANSFORM    cmyk_transform = NULL;

  /* We set up the normal JPEG error routines. */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  if (!preview)
    {
      jerr.pub.output_message = my_output_message;
    }

  if ((infile = g_fopen (filename, "rb")) == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  if (!preview)
    gimp_progress_init_printf (_("Opening '%s'"),
                               gimp_filename_to_utf8 (filename));

  image_ID = -1;

  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_decompress (&cinfo);
      if (infile)
        fclose (infile);

      if (image_ID != -1 && !preview)
        gimp_image_delete (image_ID);

      if (preview)
        destroy_preview ();

      if (buffer)
        g_object_unref (buffer);

      return -1;
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
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  jpeg_start_decompress (&cinfo);

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   */

  /* temporary buffer */
  tile_height = gimp_tile_height ();
  buf = g_new (guchar,
               tile_height * cinfo.output_width * cinfo.output_components);

  rowbuf = g_new (guchar *, tile_height);

  for (i = 0; i < tile_height; i++)
    rowbuf[i] = buf + cinfo.output_width * cinfo.output_components * i;

  switch (cinfo.output_components)
    {
    case 1:
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAY_IMAGE;
      break;

    case 3:
      image_type = GIMP_RGB;
      layer_type = GIMP_RGB_IMAGE;
      break;

    case 4:
      if (cinfo.out_color_space == JCS_CMYK)
        {
          image_type = GIMP_RGB;
          layer_type = GIMP_RGB_IMAGE;
          break;
        }
      /*fallthrough*/

    default:
      g_message ("Don't know how to load JPEG images "
                 "with %d color channels, using colorspace %d (%d).",
                 cinfo.output_components, cinfo.out_color_space,
                 cinfo.jpeg_color_space);
      return -1;
      break;
    }

  if (preview)
    {
      image_ID = preview_image_ID;
    }
  else
    {
      image_ID = gimp_image_new_with_precision (cinfo.output_width,
                                                cinfo.output_height,
                                                image_type,
                                                GIMP_PRECISION_U8_GAMMA);

      gimp_image_undo_disable (image_ID);
      gimp_image_set_filename (image_ID, filename);
    }

  if (preview)
    {
      preview_layer_ID = gimp_layer_new (preview_image_ID, _("JPEG preview"),
                                         cinfo.output_width,
                                         cinfo.output_height,
                                         layer_type, 100, GIMP_NORMAL_MODE);
      layer_ID = preview_layer_ID;
    }
  else
    {
      layer_ID = gimp_layer_new (image_ID, _("Background"),
                                 cinfo.output_width,
                                 cinfo.output_height,
                                 layer_type, 100, GIMP_NORMAL_MODE);
    }

  if (! preview)
    {
      GString  *comment_buffer = NULL;
      guint8   *profile        = NULL;
      guint     profile_size   = 0;

      /* Step 5.0: save the original JPEG settings in a parasite */
      jpeg_detect_original_settings (&cinfo, image_ID);

      /* Step 5.1: check for comments, or Exif metadata in APP1 markers */
      for (marker = cinfo.marker_list; marker; marker = marker->next)
        {
          const gchar *data = (const gchar *) marker->data;
          gsize        len  = marker->data_length;

          if (marker->marker == JPEG_COM)
            {
#ifdef GIMP_UNSTABLE
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
#ifdef GIMP_UNSTABLE
              g_print ("jpeg-load: found Exif block (%d bytes)\n",
                       (gint) (len - sizeof (JPEG_APP_HEADER_EXIF)));
#endif
            }
        }

      if (jpeg_load_resolution (image_ID, &cinfo))
        {
          if (resolution_loaded)
            *resolution_loaded = TRUE;
        }

      /* if we found any comments, then make a parasite for them */
      if (comment_buffer && comment_buffer->len)
        {
          GimpParasite *parasite;

          jpeg_load_sanitize_comment (comment_buffer->str);
          parasite = gimp_parasite_new ("gimp-comment",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (comment_buffer->str) + 1,
                                        comment_buffer->str);
          gimp_image_attach_parasite (image_ID, parasite);
          gimp_parasite_free (parasite);

          g_string_free (comment_buffer, TRUE);
        }

      /* Step 5.3: check for an embedded ICC profile in APP2 markers */
      jpeg_icc_read_profile (&cinfo, &profile, &profile_size);

      if (cinfo.out_color_space == JCS_CMYK)
        {
          cmyk_transform = jpeg_load_cmyk_transform (profile, profile_size);
        }
      else if (profile) /* don't attach the profile if we are transforming */
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("icc-profile",
                                        GIMP_PARASITE_PERSISTENT |
                                        GIMP_PARASITE_UNDOABLE,
                                        profile_size, profile);
          gimp_image_attach_parasite (image_ID, parasite);
          gimp_parasite_free (parasite);
        }

      g_free (profile);

      /* Do not attach the "jpeg-save-options" parasite to the image
       * because this conflicts with the global defaults (bug #75398).
       */
    }

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */

  buffer = gimp_drawable_get_buffer (layer_ID);
  format = babl_format (image_type == GIMP_RGB ? "R'G'B' u8" : "Y' u8");

  while (cinfo.output_scanline < cinfo.output_height)
    {
      start = cinfo.output_scanline;
      end   = cinfo.output_scanline + tile_height;
      end   = MIN (end, cinfo.output_height);

      scanlines = end - start;

      for (i = 0; i < scanlines; i++)
        jpeg_read_scanlines (&cinfo, (JSAMPARRAY) &rowbuf[i], 1);

      if (cinfo.out_color_space == JCS_CMYK)
        jpeg_load_cmyk_to_rgb (buf, cinfo.output_width * scanlines,
                               cmyk_transform);

      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, start, cinfo.output_width, scanlines),
                       0,
                       format,
                       buf,
                       GEGL_AUTO_ROWSTRIDE);

      if (! preview && (cinfo.output_scanline % 32) == 0)
        gimp_progress_update ((gdouble) cinfo.output_scanline /
                              (gdouble) cinfo.output_height);
    }

  /* Step 7: Finish decompression */

  jpeg_finish_decompress (&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  if (cmyk_transform)
    cmsDeleteTransform (cmyk_transform);

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
      gimp_progress_update (1.0);
    }

  gimp_image_insert_layer (image_ID, layer_ID, -1, 0);

  return image_ID;
}

static gboolean
jpeg_load_resolution (gint32                         image_ID,
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

          gimp_image_get_resolution (image_ID, &xresolution, &yresolution);

          xresolution *= asymmetry;
          break;

        case 1: /* dots per inch */
          break;

        case 2: /* dots per cm */
          xresolution *= 2.54;
          yresolution *= 2.54;
          gimp_image_set_unit (image_ID, GIMP_UNIT_MM);
          break;

        default:
          g_message ("Unknown density unit %d, assuming dots per inch.",
                     cinfo->density_unit);
          break;
        }

      gimp_image_set_resolution (image_ID, xresolution, yresolution);

      return TRUE;
    }

  return FALSE;
}

/*
 * A number of JPEG files have comments written in a local character set
 * instead of UTF-8.  Some of these files may have been saved by older
 * versions of GIMP.  It is not possible to reliably detect the character
 * set used, but it is better to keep all characters in the ASCII range
 * and replace the non-ASCII characters instead of discarding the whole
 * comment.  This is especially useful if the comment contains only a few
 * non-ASCII characters such as a copyright sign, a soft hyphen, etc.
 */
static void
jpeg_load_sanitize_comment (gchar *comment)
{
  if (! g_utf8_validate (comment, -1, NULL))
    {
      gchar *c;

      for (c = comment; *c; c++)
        {
          if (*c > 126 || (*c < 32 && *c != '\t' && *c != '\n' && *c != '\r'))
            *c = '?';
        }
    }
}

gint32
load_thumbnail_image (GFile         *file,
                      gint          *width,
                      gint          *height,
                      GimpImageType *type,
                      GError       **error)
{
  gint32 volatile               image_ID = -1;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr           jerr;
  FILE                         *infile   = NULL;

  gimp_progress_init_printf (_("Opening thumbnail for '%s'"),
                             g_file_get_parse_name (file));

  image_ID = gimp_image_metadata_load_thumbnail (file, error);
  if (image_ID < 1)
    return -1;

  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit     = my_error_exit;
  jerr.pub.output_message = my_output_message;

  if ((infile = g_fopen (g_file_get_path (file), "rb")) == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   g_file_get_parse_name (file), g_strerror (errno));

      if (image_ID != -1)
        gimp_image_delete (image_ID);

      return -1;
    }

  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.  We
       * need to clean up the JPEG object, close the input file,
       * and return.
       */
      jpeg_destroy_decompress (&cinfo);

      if (image_ID != -1)
        gimp_image_delete (image_ID);

      return -1;
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
      *type = GIMP_GRAY_IMAGE;
      break;

    case 3:
      *type = GIMP_RGB_IMAGE;
      break;

    case 4:
      if (cinfo.out_color_space == JCS_CMYK)
        {
          *type = GIMP_RGB_IMAGE;
          break;
        }
      /*fallthrough*/

    default:
      g_message ("Don't know how to load JPEG images "
                 "with %d color channels, using colorspace %d (%d).",
                 cinfo.output_components, cinfo.out_color_space,
                 cinfo.jpeg_color_space);

      gimp_image_delete (image_ID);
      image_ID = -1;
      break;
    }

  /* Step 4: Release JPEG decompression object */

  /* This is an important step since it will release a good deal
   * of memory.
   */
  jpeg_destroy_decompress (&cinfo);

  fclose (infile);

  return image_ID;
}

static gpointer
jpeg_load_cmyk_transform (guint8 *profile_data,
                          gsize   profile_len)
{
  GimpColorConfig *config       = gimp_get_color_configuration ();
  cmsHPROFILE      cmyk_profile = NULL;
  cmsHPROFILE      rgb_profile  = NULL;
  cmsUInt32Number  flags        = 0;
  cmsHTRANSFORM    transform;

  /*  try to load the embedded CMYK profile  */
  if (profile_data)
    {
      cmyk_profile = cmsOpenProfileFromMem (profile_data, profile_len);

      if (cmyk_profile)
        {
          if (! gimp_lcms_profile_is_cmyk (cmyk_profile))
            {
              cmsCloseProfile (cmyk_profile);
              cmyk_profile = NULL;
            }
        }
    }

  /*  if that fails, try to load the CMYK profile configured in the prefs  */
  if (! cmyk_profile && config->cmyk_profile)
    {
      cmyk_profile = cmsOpenProfileFromFile (config->cmyk_profile, "r");

      if (cmyk_profile && ! gimp_lcms_profile_is_cmyk (cmyk_profile))
        {
          cmsCloseProfile (cmyk_profile);
          cmyk_profile = NULL;
        }
    }

  /*  bail out if we can't load any CMYK profile  */
  if (! cmyk_profile)
    {
      g_object_unref (config);
      return NULL;
    }

  /*  try to load the RGB profile configured in the prefs  */
  if (config->rgb_profile)
    {
      rgb_profile = cmsOpenProfileFromFile (config->rgb_profile, "r");

      if (rgb_profile && ! gimp_lcms_profile_is_rgb (rgb_profile))
        {
          cmsCloseProfile (rgb_profile);
          rgb_profile = NULL;
        }
    }

  /*  make the real sRGB profile as a fallback  */
  if (! rgb_profile)
    {
      rgb_profile = gimp_lcms_create_srgb_profile ();
    }

  if (config->display_intent ==
      GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC)
    {
      flags |= cmsFLAGS_BLACKPOINTCOMPENSATION;
    }

  transform = cmsCreateTransform (cmyk_profile, TYPE_CMYK_8_REV,
                                  rgb_profile,  TYPE_RGB_8,
                                  config->display_intent,
                                  flags);

  cmsCloseProfile (cmyk_profile);
  cmsCloseProfile (rgb_profile);

  g_object_unref (config);

  return transform;
}


static void
jpeg_load_cmyk_to_rgb (guchar   *buf,
                       glong     pixels,
                       gpointer  transform)
{
  const guchar *src  = buf;
  guchar       *dest = buf;

  if (transform)
    {
      cmsDoTransform (transform, buf, buf, pixels);
      return;
    }

  while (pixels--)
    {
      guint c = src[0];
      guint m = src[1];
      guint y = src[2];
      guint k = src[3];

      dest[0] = (c * k) / 255;
      dest[1] = (m * k) / 255;
      dest[2] = (y * k) / 255;

      src  += 4;
      dest += 3;
    }
}
