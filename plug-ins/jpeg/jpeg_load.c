/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "jpeg.h"
#include "jpeg_load.h"

/* Read next byte */
static guint
jpeg_getc (j_decompress_ptr cinfo)
{
  struct jpeg_source_mgr *datasrc = cinfo->src;

  if (datasrc->bytes_in_buffer == 0)
    {
      if (! (*datasrc->fill_input_buffer) (cinfo))
        ERREXIT (cinfo, JERR_CANT_SUSPEND);
    }
  datasrc->bytes_in_buffer--;

  return *datasrc->next_input_byte++;
}

/* We need our own marker parser, since early versions of libjpeg
 * don't keep a conventient list of the marker's that have been
 * seen. */

/*
 * Marker processor for COM markers.
 * This replaces the library's built-in processor, which just skips the marker.
 * Note this code relies on a non-suspending data source.
 */
static GString *local_image_comments = NULL;

static boolean
COM_handler (j_decompress_ptr cinfo)
{
  gint  length;
  guint ch;

  length = jpeg_getc (cinfo) << 8;
  length += jpeg_getc (cinfo);
  if (length < 2)
    return FALSE;
  length -= 2;                  /* discount the length word itself */

  if (!local_image_comments)
    local_image_comments = g_string_new (NULL);
  else
    g_string_append_c (local_image_comments, '\n');

  while (length-- > 0)
    {
      ch = jpeg_getc (cinfo);
      g_string_append_c (local_image_comments, ch);
    }

  return TRUE;
}

gint32
load_image (const gchar *filename,
            GimpRunMode  runmode,
            gboolean     preview)
{
  GimpPixelRgn     pixel_rgn;
  GimpDrawable    *drawable;
  gint32 volatile  image_ID;
  gint32           layer_ID;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr           jerr;
  FILE    *infile;
  guchar  *buf;
  guchar  * volatile padded_buf = NULL;
  guchar **rowbuf;
  gint     image_type;
  gint     layer_type;
  gint     tile_height;
  gint     scanlines;
  gint     i, start, end;

  GimpParasite * volatile comment_parasite = NULL;

#ifdef HAVE_EXIF
  GimpParasite *exif_parasite = NULL;
  ExifData     *exif_data     = NULL;
#endif

  /* We set up the normal JPEG error routines. */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  /* flag warnings, so we try to ignore corrupt EXIF data */
  if (!preview)
    {
      cinfo.client_data = GINT_TO_POINTER (FALSE);

      jerr.pub.emit_message   = my_emit_message;
      jerr.pub.output_message = my_output_message;
    }

  if ((infile = fopen (filename, "rb")) == NULL)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      gimp_quit ();
    }

  if (!preview)
    {
      gchar *name = g_strdup_printf (_("Opening '%s'..."),
                                     gimp_filename_to_utf8 (filename));

      gimp_progress_init (name);
      g_free (name);
    }

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
        destroy_preview();
      gimp_quit ();
    }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src (&cinfo, infile);

  /* pw - step 2.1 let the lib know we want the comments. */

  jpeg_set_marker_processor (&cinfo, JPEG_COM, COM_handler);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header (&cinfo, TRUE);

  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  if (!preview)
    {
      /* if we had any comments then make a parasite for them */
      if (local_image_comments && local_image_comments->len)
        {
          gchar *comment = local_image_comments->str;

          g_string_free (local_image_comments, FALSE);

          local_image_comments = NULL;

          if (g_utf8_validate (comment, -1, NULL))
            comment_parasite = gimp_parasite_new ("gimp-comment",
                                                  GIMP_PARASITE_PERSISTENT,
                                                  strlen (comment) + 1,
                                                  comment);
          g_free (comment);
        }

      /* Do not attach the "jpeg-save-options" parasite to the image
       * because this conflics with the global defaults.  See bug #75398:
       * http://bugzilla.gnome.org/show_bug.cgi?id=75398 */
    }

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
   * In this example, we need to make an output work buffer of the right size.
   */
  /* temporary buffer */
  tile_height = gimp_tile_height ();
  buf = g_new (guchar,
               tile_height * cinfo.output_width * cinfo.output_components);

  rowbuf = g_new (guchar*, tile_height);

  for (i = 0; i < tile_height; i++)
    rowbuf[i] = buf + cinfo.output_width * cinfo.output_components * i;

  /* Create a new image of the proper size and associate the filename with it.

     Preview layers, not being on the bottom of a layer stack, MUST HAVE
     AN ALPHA CHANNEL!
   */
  switch (cinfo.output_components)
    {
    case 1:
      image_type = GIMP_GRAY;
      layer_type = preview ? GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE;
      break;

    case 3:
      image_type = GIMP_RGB;
      layer_type = preview ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE;
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
      g_message ("Don't know how to load JPEGs\n"
                 "with %d color channels\n"
                 "using colorspace %d (%d)",
                 cinfo.output_components, cinfo.out_color_space,
                 cinfo.jpeg_color_space);
      gimp_quit ();
      break;
    }

  if (preview)
    padded_buf = g_new (guchar, tile_height * cinfo.output_width *
                        (cinfo.output_components + 1));
  else if (cinfo.out_color_space == JCS_CMYK)
    padded_buf = g_new (guchar, tile_height * cinfo.output_width * 3);
  else
    padded_buf = NULL;

  if (preview)
    {
      image_ID = image_ID_global;
    }
  else
    {
      image_ID = gimp_image_new (cinfo.output_width, cinfo.output_height,
                                 image_type);
      gimp_image_set_filename (image_ID, filename);
    }

  if (preview)
    {
      layer_ID_global = layer_ID =
        gimp_layer_new (image_ID, _("JPEG preview"),
                        cinfo.output_width,
                        cinfo.output_height,
                        layer_type, 100, GIMP_NORMAL_MODE);
    }
  else
    {
      layer_ID = gimp_layer_new (image_ID, _("Background"),
                                 cinfo.output_width,
                                 cinfo.output_height,
                                 layer_type, 100, GIMP_NORMAL_MODE);
    }

  drawable_global = drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                       drawable->width, drawable->height, TRUE, FALSE);

  /* Step 5.1: if the file had resolution information, set it on the image */
  if (!preview && cinfo.saw_JFIF_marker)
    {
      gdouble xresolution;
      gdouble yresolution;
      gdouble asymmetry;

      xresolution = cinfo.X_density;
      yresolution = cinfo.Y_density;

      switch (cinfo.density_unit)
        {
        case 0: /* unknown -> set the aspect ratio but use the default
                *  image resolution
                */
          if (cinfo.Y_density != 0)
            asymmetry = xresolution / yresolution;
          else
            asymmetry = 1.0;

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
          g_message ("Unknown density unit %d\nassuming dots per inch",
                     cinfo.density_unit);
          break;
        }

      gimp_image_set_resolution (image_ID, xresolution, yresolution);
    }

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height)
    {
      start = cinfo.output_scanline;
      end   = cinfo.output_scanline + tile_height;
      end   = MIN (end, cinfo.output_height);
      scanlines = end - start;

      for (i = 0; i < scanlines; i++)
        jpeg_read_scanlines (&cinfo, (JSAMPARRAY) &rowbuf[i], 1);

      if (preview) /* Add a dummy alpha channel -- convert buf to padded_buf */
        {
          guchar *dest = padded_buf;
          guchar *src  = buf;
          gint    num  = drawable->width * scanlines;

          switch (cinfo.output_components)
            {
            case 1:
              for (i = 0; i < num; i++)
                {
                  *(dest++) = *(src++);
                  *(dest++) = 255;
                }
              break;

            case 3:
              for (i = 0; i < num; i++)
                {
                  *(dest++) = *(src++);
                  *(dest++) = *(src++);
                  *(dest++) = *(src++);
                  *(dest++) = 255;
                }
              break;

            default:
              g_warning ("JPEG - shouldn't have gotten here.\nReport to http://bugzilla.gnome.org/");
              break;
            }
        }
      else if (cinfo.out_color_space == JCS_CMYK) /* buf-> RGB in padded_buf */
        {
          guchar *dest = padded_buf;
          guchar *src  = buf;
          gint    num  = drawable->width * scanlines;

          for (i = 0; i < num; i++)
            {
              guint r_c, g_m, b_y, a_k;

              r_c = *(src++);
              g_m = *(src++);
              b_y = *(src++);
              a_k = *(src++);
              *(dest++) = (r_c * a_k) / 255;
              *(dest++) = (g_m * a_k) / 255;
              *(dest++) = (b_y * a_k) / 255;
            }
        }

      gimp_pixel_rgn_set_rect (&pixel_rgn, padded_buf ? padded_buf : buf,
                               0, start, drawable->width, scanlines);

      if (! preview)
        gimp_progress_update ((gdouble) cinfo.output_scanline /
                              (gdouble) cinfo.output_height);
    }

  /* Step 7: Finish decompression */

  jpeg_finish_decompress (&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress (&cinfo);

  /* free up the temporary buffers */
  g_free (rowbuf);
  g_free (buf);
  g_free (padded_buf);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose (infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.num_warnings is nonzero).
   */

  /* Tell the GIMP to display the image.
   */
  gimp_image_add_layer (image_ID, layer_ID, 0);
  gimp_drawable_flush (drawable);

  /* pw - Last of all, attach the parasites (couldn't do it earlier -
     there was no image. */

  if (!preview)
    {
      if (comment_parasite)
        {
          gimp_image_parasite_attach (image_ID, comment_parasite);
          gimp_parasite_free (comment_parasite);

          comment_parasite = NULL;
        }

#ifdef HAVE_EXIF
#define EXIF_HEADER_SIZE 8

      if (! GPOINTER_TO_INT (cinfo.client_data))
        {
          exif_data = exif_data_new_from_file (filename);
          if (exif_data)
            {
              guchar *exif_buf;
              guint   exif_buf_len;

              exif_data_save_data (exif_data, &exif_buf, &exif_buf_len);
              exif_data_unref (exif_data);

              if (exif_buf_len > EXIF_HEADER_SIZE)
                {
                  exif_parasite = gimp_parasite_new ("exif-data",
                                                     GIMP_PARASITE_PERSISTENT,
                                                     exif_buf_len, exif_buf);
                  gimp_image_parasite_attach (image_ID, exif_parasite);
                  gimp_parasite_free (exif_parasite);
                }

              free (exif_buf);
            }
        }
#endif
    }

  return image_ID;
}

#ifdef HAVE_EXIF

typedef struct
{
  struct jpeg_source_mgr pub;   /* public fields */

  gchar   *buffer;
  gint     size;
  JOCTET   terminal[2];
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

static void
init_source (j_decompress_ptr cinfo)
{
}


static boolean
fill_input_buffer (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  /* Since we have given all we have got already
   * we simply fake an end of file
   */

  src->pub.next_input_byte = src->terminal;
  src->pub.bytes_in_buffer = 2;
  src->terminal[0]         = (JOCTET) 0xFF;
  src->terminal[1]         = (JOCTET) JPEG_EOI;

  return TRUE;
}

static void
skip_input_data (j_decompress_ptr cinfo,
                 long             num_bytes)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  src->pub.next_input_byte = src->pub.next_input_byte + num_bytes;
}

static void
term_source (j_decompress_ptr cinfo)
{
}

gint32
load_thumbnail_image (const gchar *filename,
                      gint        *width,
                      gint        *height)
{
  gint32 volatile  image_ID;
  ExifData        *exif_data;
  GimpPixelRgn     pixel_rgn;
  GimpDrawable    *drawable;
  gint32           layer_ID;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr           jerr;
  guchar     *buf;
  guchar  * volatile padded_buf = NULL;
  guchar    **rowbuf;
  gint        image_type;
  gint        layer_type;
  gint        tile_height;
  gint        scanlines;
  gint        i, start, end;
  my_src_ptr  src;
  FILE       *infile;

  image_ID = -1;
  exif_data = exif_data_new_from_file (filename);

  if (! ((exif_data) && (exif_data->data) && (exif_data->size > 0)))
    return -1;

  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  cinfo.client_data = GINT_TO_POINTER (FALSE);

  jerr.pub.emit_message   = my_emit_message;
  jerr.pub.output_message = my_output_message;

  {
    gchar *name = g_strdup_printf (_("Opening thumbnail for '%s'..."),
                                   gimp_filename_to_utf8 (filename));
    gimp_progress_init (name);
    g_free (name);
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

      if (exif_data)
        {
          exif_data_unref (exif_data);
          exif_data = NULL;
        }

      return -1;
    }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */

  if (cinfo.src == NULL)
    cinfo.src = (struct jpeg_source_mgr *)(*cinfo.mem->alloc_small)
      ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
       sizeof (my_source_mgr));

  src = (my_src_ptr) cinfo.src;

  src->pub.init_source       = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data   = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart;
  src->pub.term_source       = term_source;

  src->pub.bytes_in_buffer   = exif_data->size;
  src->pub.next_input_byte   = exif_data->data;

  src->buffer = exif_data->data;
  src->size = exif_data->size;

  /* Step 3: read file parameters with jpeg_read_header() */

  jpeg_read_header (&cinfo, TRUE);

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  jpeg_start_decompress (&cinfo);

  /* We may need to do some setup of our own at this point before
   * reading the data.  After jpeg_start_decompress() we have the
   * correct scaled output image dimensions available, as well as
   * the output colormap if we asked for color quantization.  In
   * this example, we need to make an output work buffer of the
   * right size.
   */

  /* temporary buffer */
  tile_height = gimp_tile_height ();
  buf = g_new (guchar,
               tile_height * cinfo.output_width * cinfo.output_components);

  rowbuf = g_new (guchar *, tile_height);

  for (i = 0; i < tile_height; i++)
    rowbuf[i] = buf + cinfo.output_width * cinfo.output_components * i;

  /* Create a new image of the proper size and associate the
   * filename with it.
   *
   * Preview layers, not being on the bottom of a layer stack,
   * MUST HAVE AN ALPHA CHANNEL!
   */
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
      g_message ("Don't know how to load JPEGs\n"
                 "with %d color channels\n"
                 "using colorspace %d (%d)",
                 cinfo.output_components, cinfo.out_color_space,
                 cinfo.jpeg_color_space);

      if (exif_data)
        {
          exif_data_unref (exif_data);
          exif_data = NULL;
        }

      return -1;
      break;
    }

  if (cinfo.out_color_space == JCS_CMYK)
    padded_buf = g_new (guchar, tile_height * cinfo.output_width * 3);
  else
    padded_buf = NULL;

  image_ID = gimp_image_new (cinfo.output_width, cinfo.output_height,
                             image_type);
  gimp_image_set_filename (image_ID, filename);

  layer_ID = gimp_layer_new (image_ID, _("Background"),
                             cinfo.output_width,
                             cinfo.output_height,
                             layer_type, 100, GIMP_NORMAL_MODE);

  drawable_global = drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                       drawable->width, drawable->height, TRUE, FALSE);

  /* Step 5.1: if the file had resolution information, set it on the image */
  if (cinfo.saw_JFIF_marker)
    {
      gdouble xresolution;
      gdouble yresolution;
      gdouble asymmetry;

      xresolution = cinfo.X_density;
      yresolution = cinfo.Y_density;

      switch (cinfo.density_unit)
        {
        case 0: /* unknown -> set the aspect ratio but use the default
                 *  image resolution
                 */
          if (cinfo.Y_density != 0)
            asymmetry = xresolution / yresolution;
          else
            asymmetry = 1.0;

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
          g_message ("Unknown density unit %d\nassuming dots per inch",
                     cinfo.density_unit);
          break;
        }

      gimp_image_set_resolution (image_ID, xresolution, yresolution);
    }

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height)
    {
      start = cinfo.output_scanline;
      end   = cinfo.output_scanline + tile_height;
      end   = MIN (end, cinfo.output_height);
      scanlines = end - start;

      for (i = 0; i < scanlines; i++)
        jpeg_read_scanlines (&cinfo, (JSAMPARRAY) &rowbuf[i], 1);

      if (cinfo.out_color_space == JCS_CMYK) /* buf-> RGB in padded_buf */
        {
          guchar *dest = padded_buf;
          guchar *src  = buf;
          gint    num  = drawable->width * scanlines;

          for (i = 0; i < num; i++)
            {
              guint r_c, g_m, b_y, a_k;

              r_c = *(src++);
              g_m = *(src++);
              b_y = *(src++);
              a_k = *(src++);
              *(dest++) = (r_c * a_k) / 255;
              *(dest++) = (g_m * a_k) / 255;
              *(dest++) = (b_y * a_k) / 255;
            }
        }

      gimp_pixel_rgn_set_rect (&pixel_rgn, padded_buf ? padded_buf : buf,
                               0, start, drawable->width, scanlines);

      gimp_progress_update ((gdouble) cinfo.output_scanline /
                            (gdouble) cinfo.output_height);
    }

  /* Step 7: Finish decompression */

  jpeg_finish_decompress (&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal
   * of memory.
   */
  jpeg_destroy_decompress (&cinfo);

  /* free up the temporary buffers */
  g_free (rowbuf);
  g_free (buf);
  g_free (padded_buf);

  /* At this point you may want to check to see whether any
   * corrupt-data warnings occurred (test whether
   * jerr.num_warnings is nonzero).
   */
  gimp_image_add_layer (image_ID, layer_ID, 0);


  /* NOW to get the dimensions of the actual image to return the
   * calling app
   */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  cinfo.client_data = GINT_TO_POINTER (FALSE);

  jerr.pub.emit_message   = my_emit_message;
  jerr.pub.output_message = my_output_message;

  if ((infile = fopen (filename, "rb")) == NULL)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));

      if (exif_data)
        {
          exif_data_unref (exif_data);
          exif_data = NULL;
        }

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

      if (exif_data)
        {
          exif_data_unref (exif_data);
          exif_data = NULL;
        }

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

  /* Step 4: Release JPEG decompression object */

  /* This is an important step since it will release a good deal
   * of memory.
   */
  jpeg_destroy_decompress (&cinfo);

  if (exif_data)
    {
      exif_data_unref (exif_data);
      exif_data = NULL;
    }

  return image_ID;
}

#endif /* HAVE_EXIF */
