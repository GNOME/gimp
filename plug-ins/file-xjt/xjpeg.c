/* GIMP - The GNU Image Manipulation Program
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

/* JPEG loading and saving routines adapted for the GIMP XJT fileformat
 *  -Wolfgang Hofer
 *
 * This filter is heavily based upon the "example.c" file in libjpeg.
 * In fact most of the loading and saving code was simply cut-and-pasted
 *  from that file. The filter, therefore, also uses libjpeg.
 */

/* revision history:
 * version 1.1.15a; 2000/01/25  hof: use g_malloc, g_free
 * version 1.00.00; 1998/10/26  hof: 1.st (pre) release
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <glib/gstdio.h>

/* Include for External Libraries */
#include <jpeglib.h>

/* GIMP includes */
#include "libgimp/gimp.h"

#include "xjpeg.h"

extern int xjt_debug;

/* Declare local functions.
 */


typedef struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
} *my_error_ptr;


/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp (myerr->setjmp_buffer, 1);
}


/* ============================================================================
 * xjpg_load_layer
 *   load layer from jpeg file
 * ============================================================================
 */

gint32
xjpg_load_layer (const char    *filename,
                 gint32         image_id,
                 int            image_type,
                 char          *layer_name,
                 gdouble        layer_opacity,
                 GimpLayerModeEffects     layer_mode
	    )
{
  GimpPixelRgn l_pixel_rgn;
  GimpDrawable *l_drawable;
  gint32     l_layer_id;
  GimpImageType  l_layer_type;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  FILE *infile;
  guchar *l_buf;
  guchar **l_rowbuf;
  int l_tile_height;
  int l_scanlines;
  int l_idx, l_start, l_end;

  /* We set up the normal JPEG error routines. */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  l_layer_type = GIMP_GRAY_IMAGE;

  if ((infile = g_fopen (filename, "rb")) == NULL)
  {
      g_warning ("can't open \"%s\"\n", filename);
      return -1;
  }


  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
  {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_decompress (&cinfo);
      if (infile)
	fclose (infile);

      g_printerr ("XJT: JPEG load error\n");
      return -1;
  }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src (&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header (&cinfo, TRUE);
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
   * In this example, we need to make an output work buffer of the right size.
   */
  /* temporary buffer */
  l_tile_height = gimp_tile_height ();
  l_buf = g_new (guchar, l_tile_height * cinfo.output_width * cinfo.output_components);
  l_rowbuf = g_new (guchar*, l_tile_height);

  for (l_idx = 0; l_idx < l_tile_height; l_idx++)
  {
    l_rowbuf[l_idx] = l_buf + cinfo.output_width * cinfo.output_components * l_idx;
  }

  /* Check jpeg file for layer type */
  switch (cinfo.output_components)
    {
    case 1:
      l_layer_type = GIMP_GRAY_IMAGE;
      break;
    case 3:
      l_layer_type = GIMP_RGB_IMAGE;
      break;
    default:
      g_printerr ("XJT: cant load layer %s (type is not GRAY and not RGB)\n", filename);
      fclose (infile);
      return -1;
    }


  l_layer_id = gimp_layer_new (image_id, layer_name,
			     cinfo.output_width,
			     cinfo.output_height,
			     l_layer_type,
			     layer_opacity,
			     layer_mode);
  if(l_layer_id < 0)
  {
      g_printerr ("XJT: cant create new layer\n");
      fclose (infile);
      return -1;
  }

  l_drawable = gimp_drawable_get (l_layer_id);
  gimp_pixel_rgn_init (&l_pixel_rgn, l_drawable, 0, 0, l_drawable->width, l_drawable->height, TRUE, FALSE);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height)
  {
      l_start = cinfo.output_scanline;
      l_end = cinfo.output_scanline + l_tile_height;
      l_end = MIN (l_end, cinfo.output_height);
      l_scanlines = l_end - l_start;

      for (l_idx = 0; l_idx < l_scanlines; l_idx++)
      {
	jpeg_read_scanlines (&cinfo, (JSAMPARRAY) &l_rowbuf[l_idx], 1);
      }

      gimp_pixel_rgn_set_rect (&l_pixel_rgn, l_buf, 0, l_start, l_drawable->width, l_scanlines);

      gimp_progress_update ((double) cinfo.output_scanline / (double) cinfo.output_height);
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
  g_free (l_rowbuf);
  g_free (l_buf);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose (infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.num_warnings is nonzero).
   */

  return (l_layer_id);

}	/* end xjpg_load_layer */


/* ============================================================================
 * xjpg_load_layer_alpha
 *   load the layers alpha channel from jpeg file.
 * ============================================================================
 */

gint
xjpg_load_layer_alpha (const char *filename,
                       gint32      image_id,
                       gint32      layer_id)
{
  GimpPixelRgn l_pixel_rgn;
  GimpDrawable *l_drawable;
  GimpImageType  l_layer_type;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  FILE *infile;
  guchar *l_buf;
  guchar *l_dstbuf;
  guchar **l_rowbuf;
  int l_tile_height;
  int l_scanlines;
  int l_idx, l_start, l_end;
  int l_alpha_offset;
  guchar *l_buf_ptr;
  guchar *l_dstbuf_ptr;

  /* We set up the normal JPEG error routines. */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  l_layer_type = GIMP_GRAY_IMAGE;

  /* add alpha channel */
  gimp_layer_add_alpha (layer_id);

  if ((infile = g_fopen (filename, "rb")) == NULL)
  {
      /* No alpha found, thats OK, use full opaque alpha channel
       * (there is no need not store alpha channels on full opaque channels)
       * (fixme: if filename exists but is not readable
       *         we should return -1 to indicate an error
       */
      return 0;  /* OK */
  }

  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
  {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_decompress (&cinfo);
      if (infile)
	fclose (infile);

      g_printerr ("XJT: JPEG alpha load error\n");
      return -1;
  }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src (&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header (&cinfo, TRUE);
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
   * In this example, we need to make an output work buffer of the right size.
   */
  /* temporary buffer (for read in jpeg lines) */
  l_tile_height = gimp_tile_height ();
  l_buf = g_new (guchar, l_tile_height * cinfo.output_width * cinfo.output_components);
  l_rowbuf = g_new (guchar*, l_tile_height);

  for (l_idx = 0; l_idx < l_tile_height; l_idx++)
  {
    l_rowbuf[l_idx] = l_buf + cinfo.output_width * cinfo.output_components * l_idx;
  }

  l_drawable = gimp_drawable_get (layer_id);
  if(l_drawable == NULL)
  {
    g_printerr ("XJT: gimp_drawable_get failed on layer id %d\n", (int)layer_id);
    fclose(infile);
    return -1;
  }

  /* Check if jpeg file can be used as alpha channel
   */
  if((cinfo.output_components != 1) ||
     (cinfo.output_width  != l_drawable->width) ||
     (cinfo.output_height != l_drawable->height))
  {
     g_printerr ("XJT: cant load %s as alpha channel\n", filename);
     fclose (infile);
     return -1;
  }

  /* buffer to read in the layer and merge with the alpha from jpeg file */
  l_dstbuf = g_new (guchar, l_tile_height * l_drawable->width * l_drawable->bpp);

  gimp_pixel_rgn_init (&l_pixel_rgn, l_drawable, 0, 0, l_drawable->width, l_drawable->height, TRUE, FALSE);
  l_alpha_offset = l_drawable->bpp -1;

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height)
  {
      l_start = cinfo.output_scanline;
      l_end = cinfo.output_scanline + l_tile_height;
      l_end = MIN (l_end, cinfo.output_height);
      l_scanlines = l_end - l_start;

      for (l_idx = 0; l_idx < l_scanlines; l_idx++)
      {
	jpeg_read_scanlines (&cinfo, (JSAMPARRAY) &l_rowbuf[l_idx], 1);
      }

      gimp_pixel_rgn_get_rect (&l_pixel_rgn, l_dstbuf, 0, l_start, l_drawable->width, l_scanlines);

      /* copy the loaded jpeg data (from buf) to the layers alpha channel data */
      l_idx = l_tile_height * l_drawable->width;
      l_buf_ptr = l_buf;
      l_dstbuf_ptr = l_dstbuf;
      while(l_idx--)
      {
         l_dstbuf_ptr  += l_alpha_offset;
         *l_dstbuf_ptr++ = *l_buf_ptr++;
      }

      gimp_pixel_rgn_set_rect (&l_pixel_rgn, l_dstbuf, 0, l_start, l_drawable->width, l_scanlines);

      gimp_progress_update ((double) cinfo.output_scanline / (double) cinfo.output_height);
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
  g_free (l_rowbuf);
  g_free (l_buf);
  g_free (l_dstbuf);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose (infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.num_warnings is nonzero).
   */


  return (0);  /* OK */

}	/* xjpg_load_layer_alpha */


/* ============================================================================
 * xjpg_load_channel
 *   load channel from jpeg file
 *   (call this procedure with drawable_id == -1  to create a new channel,
 *    if a positive drawable_id is supplied, its content will be overwritten)
 * ============================================================================
 */

gint32
xjpg_load_channel (const char   *filename,
                   gint32        image_id,
                   gint32        drawable_id,
                   char         *channel_name,
                   gdouble       channel_opacity,
                   guchar red, guchar  green, guchar blue)
{
  GimpPixelRgn l_pixel_rgn;
  GimpDrawable *l_drawable;
  gint32     l_drawable_id;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  FILE *infile;
  guchar *l_buf;
  guchar **l_rowbuf;
  int l_tile_height;
  int l_scanlines;
  int l_idx, l_start, l_end;
  GimpRGB l_color;

  gimp_rgba_set_uchar (&l_color, red, green, blue, 255);

  /* We set up the normal JPEG error routines. */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  if ((infile = g_fopen (filename, "rb")) == NULL)
  {
      g_warning ("can't open \"%s\"\n", filename);
      return -1;
  }


  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
  {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_decompress (&cinfo);
      if (infile)
	fclose (infile);

      g_printerr ("XJT: JPEG load error\n");
      return -1;
  }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src (&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header (&cinfo, TRUE);
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
   * In this example, we need to make an output work buffer of the right size.
   */
  /* temporary buffer */
  l_tile_height = gimp_tile_height ();
  l_buf = g_new (guchar, l_tile_height * cinfo.output_width * cinfo.output_components);
  l_rowbuf = g_new (guchar*, l_tile_height);

  for (l_idx = 0; l_idx < l_tile_height; l_idx++)
  {
    l_rowbuf[l_idx] = l_buf + cinfo.output_width * cinfo.output_components * l_idx;
  }

  /* Check if jpeg file has one component (a channel cant have more than one)
   */
  if(cinfo.output_components != 1)
  {
      g_printerr ("XJT: cant load RGB layer %s into GRAY Image\n", filename);
      fclose (infile);
      return -1;
  }

  if(drawable_id < 0)
  {
     l_drawable_id = gimp_channel_new (image_id,
				       channel_name,
				       cinfo.output_width,
				       cinfo.output_height,
				       channel_opacity,
				       &l_color);
     if(l_drawable_id < 0)
     {
         g_printerr ("XJT: cant create new channel\n");
         fclose (infile);
         return -1;
     }
  }
  else
  {
    l_drawable_id = drawable_id;  /* overwrite the given drawable */
  }

  l_drawable = gimp_drawable_get (l_drawable_id);

  if((l_drawable->width != cinfo.output_width)
  || (l_drawable->height != cinfo.output_height)
  || (l_drawable->bpp != cinfo.output_components) )
  {
         g_printerr ("XJT: cant load-overwrite drawable (size missmatch)\n");
         fclose (infile);
         return -1;
  }

  gimp_pixel_rgn_init (&l_pixel_rgn, l_drawable, 0, 0, l_drawable->width, l_drawable->height, TRUE, FALSE);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height)
  {
      l_start = cinfo.output_scanline;
      l_end = cinfo.output_scanline + l_tile_height;
      l_end = MIN (l_end, cinfo.output_height);
      l_scanlines = l_end - l_start;

      for (l_idx = 0; l_idx < l_scanlines; l_idx++)
      {
	jpeg_read_scanlines (&cinfo, (JSAMPARRAY) &l_rowbuf[l_idx], 1);
      }

      gimp_pixel_rgn_set_rect (&l_pixel_rgn, l_buf, 0, l_start, l_drawable->width, l_scanlines);

      gimp_progress_update ((double) cinfo.output_scanline / (double) cinfo.output_height);
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
  g_free (l_rowbuf);
  g_free (l_buf);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose (infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.num_warnings is nonzero).
   */

  /* Tell GIMP to display the image.
   */
  /* gimp_drawable_flush (l_drawable); */

  return (l_drawable_id);

}	/* end xjpg_load_channel */



/* ============================================================================
 * xjpg_save_drawable
 *   save as drawable as jpeg file depending on save_mode:
 *   - save the drawable without alpha channel.
 *     (optional clear full transparent pixels to 0,
 *      resulting in better compression)
 *   - save the alpha channel
 * ============================================================================
 */


gint
xjpg_save_drawable (const char     *filename,
                    gint32          image_ID,
                    gint32          drawable_ID,
                    gint            save_mode,
                    t_JpegSaveVals *jsvals)
{
  GimpPixelRgn pixel_rgn;
  GimpDrawable *drawable;
  GimpImageType drawable_type;
  struct jpeg_compress_struct cinfo;
  struct my_error_mgr jerr;
  FILE * volatile outfile;
  guchar *temp, *t;
  guchar *data;
  guchar *src, *s;
  int has_alpha;
  int rowstride, yend;
  int i, j;
  int alpha_offset;
  guchar alpha_byte;
  guchar volatile l_alpha_sum;

  alpha_offset = 0;
  has_alpha = 0;
  src = NULL;
  temp = NULL;
  data = NULL;
  l_alpha_sum = 0xff;

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  switch (drawable_type)
  {
    case GIMP_RGB_IMAGE:
    case GIMP_GRAY_IMAGE:
      if(save_mode == JSVM_ALPHA)
        return FALSE;              /* there is no alpha to save */
      break;
    case GIMP_RGBA_IMAGE:
    case GIMP_GRAYA_IMAGE:
      break;

    case GIMP_INDEXED_IMAGE:
      /*g_message ("jpeg: cannot operate on indexed color images");*/
      return FALSE;
      break;
    default:
      /*g_message ("jpeg: cannot operate on unknown image types");*/
      return FALSE;
      break;
  }



  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);


  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  outfile = NULL;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_compress (&cinfo);
      if (outfile)
	fclose (outfile);
      if (drawable)
	gimp_drawable_detach (drawable);

      return FALSE;
    }

  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress (&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  if ((outfile = g_fopen (filename, "wb")) == NULL)
    {
      g_message ("can't open %s\n", filename);
      return FALSE;
    }
  jpeg_stdio_dest (&cinfo, outfile);

  /* Get the input image and a pointer to its data.
   */
  switch (drawable_type)
  {
    case GIMP_RGB_IMAGE:
    case GIMP_GRAY_IMAGE:
      /* # of color components per pixel */
      cinfo.input_components = drawable->bpp;
      has_alpha = 0;
      alpha_offset = 0;
      break;
    case GIMP_RGBA_IMAGE:
    case GIMP_GRAYA_IMAGE:
      if(save_mode == JSVM_ALPHA)
      {
 	cinfo.input_components = 1;
      }
      else
      {
        /* # of color components per pixel (minus the GIMP alpha channel) */
	cinfo.input_components = drawable->bpp - 1;
      }
      alpha_offset = drawable->bpp -1;
      has_alpha = 1;
      break;
    default:
      return FALSE;
      break;
  }

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  /* image width and height, in pixels */
  cinfo.image_width = drawable->width;
  cinfo.image_height = drawable->height;
  /* colorspace of input image */
  cinfo.in_color_space = ( (save_mode != JSVM_ALPHA) &&
                          (drawable_type == GIMP_RGB_IMAGE ||
			   drawable_type == GIMP_RGBA_IMAGE))
    ? JCS_RGB : JCS_GRAYSCALE;
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults (&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality (&cinfo, (int) (jsvals->quality * 100), TRUE /* limit to baseline-JPEG values */);
  cinfo.smoothing_factor = (int) (jsvals->smoothing * 100);
  cinfo.optimize_coding = jsvals->optimize;

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress (&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  /* JSAMPLEs per row in image_buffer */
  rowstride = drawable->bpp * drawable->width;
  temp = (guchar *) g_malloc (cinfo.image_width * cinfo.input_components);
  data = (guchar *) g_malloc (rowstride * gimp_tile_height ());
  src = data;

  while (cinfo.next_scanline < cinfo.image_height)
  {
      if ((cinfo.next_scanline % gimp_tile_height ()) == 0)
      {
	  yend = cinfo.next_scanline + gimp_tile_height ();
	  yend = MIN (yend, cinfo.image_height);
	  gimp_pixel_rgn_get_rect (&pixel_rgn, data, 0, cinfo.next_scanline, cinfo.image_width,
				   (yend - cinfo.next_scanline));
	  src = data;
      }

      t = temp;
      s = src;
      i = cinfo.image_width;

      switch(save_mode)
      {
	case JSVM_DRAWABLE:
	    if(jsvals->clr_transparent)
	    {
	      /* save drawable (clear pixels where alpha is full transparent) */
	      while (i--)
	      {
	          alpha_byte = s[cinfo.input_components];
		  for (j = 0; j < cinfo.input_components; j++)
		  {
		     if(alpha_byte != 0) { *t++ = *s++;   }
		     else                { *t++ = 0; s++; }
		  }
		  if (has_alpha)  /* ignore alpha channel */
		  {
		    s++;
		  }
	      }
	    }
	    else
	    {
	      /* save the drawable as it is (ignore alpha channel) */
	      while (i--)
	      {
		  for (j = 0; j < cinfo.input_components; j++)
		  {
                    *t++ = *s++;
		  }
		  if (has_alpha)  /* ignore alpha channel */
		  {
		    s++;
		  }
	      }
	    }
	    break;
        case JSVM_ALPHA:
	    /* save the drawable's alpha cahnnel */
	    while (i--)
	    {
		s += alpha_offset;
		l_alpha_sum &= (*s);  /* check all alpha bytes for full opacity */
                *t++ = *s++;
	    }
	    break;
      }

      src += rowstride;
      jpeg_write_scanlines (&cinfo, (JSAMPARRAY) &temp, 1);

      if ((cinfo.next_scanline % 5) == 0)
	gimp_progress_update ((double) cinfo.next_scanline / (double) cinfo.image_height);
  }

  /* Step 6: Finish compression */
  jpeg_finish_compress (&cinfo);
  /* After finish_compress, we can close the output file. */
  fclose (outfile);

  if((save_mode == JSVM_ALPHA) && (l_alpha_sum == 0xff))
  {
    /* all bytes in the alpha channel are set to 0xff
     * == full opaque image. We can remove the file
     * to save diskspace
     */
    g_remove(filename);
  }

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress (&cinfo);
  /* free the temporary buffer */
  g_free (temp);
  g_free (data);

  gimp_drawable_detach (drawable);

  return TRUE;
}	/* end xjpg_save_drawable */
