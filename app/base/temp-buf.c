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

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <process.h>		/* For _getpid() */
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "base-types.h"

#include "config/gimpbaseconfig.h"
#include "config/gimpconfig-path.h"

#include "pixel-region.h"
#include "temp-buf.h"

#include "paint-funcs/paint-funcs.h"


static guchar * temp_buf_allocate (guint    size);
static void     temp_buf_to_color (TempBuf *src_buf,
				   TempBuf *dest_buf);
static void     temp_buf_to_gray  (TempBuf *src_buf,
				   TempBuf *dest_buf);


#ifdef __GNUC__
#warning FIXME: extern GimpBaseConfig *base_config;
#endif
extern GimpBaseConfig *base_config;


/*  Memory management  */

static guchar *
temp_buf_allocate (guint size)
{
  return g_new (guchar, size);
}


/*  The conversion routines  */

static void
temp_buf_to_color (TempBuf *src_buf,
		   TempBuf *dest_buf)
{
  guchar *src;
  guchar *dest;
  glong   num_pixels;

  src  = temp_buf_data (src_buf);
  dest = temp_buf_data (dest_buf);

  num_pixels = src_buf->width * src_buf->height;

  switch (dest_buf->bytes)
    {
    case 3:
      g_return_if_fail (src_buf->bytes == 1);
      while (num_pixels--)
        {
          guchar tmpch;
          *dest++ = tmpch = *src++;
          *dest++ = tmpch;
          *dest++ = tmpch;
        }
      break;

    case 4:
      g_return_if_fail (src_buf->bytes == 2);
      while (num_pixels--)
        {
          guchar tmpch;
          *dest++ = tmpch = *src++;
          *dest++ = tmpch;
          *dest++ = tmpch;

          *dest++ = *src++;  /* alpha channel */
        }
      break;

    default:
      g_return_if_reached ();
      break;
    }
}

static void
temp_buf_to_gray (TempBuf *src_buf,
		  TempBuf *dest_buf)
{
  guchar *src;
  guchar *dest;
  glong   num_pixels;
  gfloat  pix;

  src  = temp_buf_data (src_buf);
  dest = temp_buf_data (dest_buf);

  num_pixels = src_buf->width * src_buf->height;

  switch (dest_buf->bytes)
    {
    case 1:
      g_return_if_fail (src_buf->bytes == 3);
      while (num_pixels--)
        {
          pix = GIMP_RGB_INTENSITY (src[0], src[1], src[2]) + 0.5;
          *dest++ = (guchar) pix;

          src += 3;
        }
      break;

    case 2:
      g_return_if_fail (src_buf->bytes == 4);
      while (num_pixels--)
        {
          pix = GIMP_RGB_INTENSITY (src[0], src[1], src[2]) + 0.5;
          *dest++ = (guchar) pix;

          *dest++ = src[3];  /* alpha channel */

          src += 4;
        }
      break;

    default:
      g_return_if_reached ();
      break;
    }
}

TempBuf *
temp_buf_new (gint    width,
	      gint    height,
	      gint    bytes,
	      gint    x,
	      gint    y,
	      guchar *col)
{
  glong    i;
  guchar  *data;
  TempBuf *temp;

  g_return_val_if_fail (width > 0 && height > 0, NULL);
  g_return_val_if_fail (bytes > 0, NULL);

  temp = g_new (TempBuf, 1);

  temp->width    = width;
  temp->height   = height;
  temp->bytes    = bytes;
  temp->x        = x;
  temp->y        = y;
  temp->swapped  = FALSE;
  temp->filename = NULL;

  temp->data = data = temp_buf_allocate (width * height * bytes);

  /*  initialize the data  */
  if (col)
    {
      /* First check if we can save a lot of work */
      for (i = 1; i < bytes; i++)
        {
          if (col[0] != col[i])
            break;
        }

      if (i == bytes)
        {
          memset (data, *col, width * height * bytes);
        }
      else /* No, we cannot */
        {
          guchar *dptr = data;

          /* Fill the first row */
          for (i = width - 1; i >= 0; --i)
            {
              guchar *init = col;
              gint    j    = bytes;

              while (j--)
                *dptr++ = *init++;
            }

          /* Now copy from it (we set bytes to bytes-per-row now) */
          bytes *= width;

          while (--height)
            {
              memcpy (dptr, data, bytes);
              dptr += bytes;
            }
        }
    }

  return temp;
}

/* This function simply renders a checkerboard with the given
   parameters into a newly allocated RGB tempbuf */

TempBuf *
temp_buf_new_check (gint           width,
		    gint           height,
		    GimpCheckType  check_type,
	            GimpCheckSize  check_size)
{
  TempBuf *newbuf;
  guchar  *data;
  guchar   check_shift = 0;
  guchar   check_mod   = 0;
  guchar   check_light = 0;
  guchar   check_dark  = 0;
  gint     x, y;

  g_return_val_if_fail (width > 0 && height > 0, NULL);

  switch (check_size)
    {
    case GIMP_CHECK_SIZE_SMALL_CHECKS:
      check_mod   = 0x3;
      check_shift = 2;
      break;
    case GIMP_CHECK_SIZE_MEDIUM_CHECKS:
      check_mod   = 0x7;
      check_shift = 3;
      break;
    case GIMP_CHECK_SIZE_LARGE_CHECKS:
      check_mod   = 0xf;
      check_shift = 4;
      break;
    }

  gimp_checks_get_shades (check_type, &check_light, &check_dark);

  newbuf = temp_buf_new (width, height, 3, 0, 0, NULL);
  data = temp_buf_data (newbuf);

  for (y = 0; y < height; y++)
    {
      guchar check_dark  = y >> check_shift;
      guchar color = (check_dark & 0x1) ? check_light : check_dark;

      for (x = 0; x < width; x++)
	{
          *data++ = color;
          *data++ = color;
          *data++ = color;

          if (((x + 1) & check_mod) == 0)
            {
              check_dark += 1;
              color = (check_dark & 0x1) ? check_light : check_dark;
            }
        }
    }

  return newbuf;
}

TempBuf *
temp_buf_copy (TempBuf *src,
	       TempBuf *dest)
{
  glong length;

  g_return_val_if_fail (src != NULL, NULL);
  g_return_val_if_fail (! dest || (dest->width  == src->width &&
                                   dest->height == src->height), NULL);

  if (! dest)
    {
      dest = temp_buf_new (src->width, src->height, src->bytes, 0, 0, NULL);
    }

  if (src->bytes != dest->bytes)
    {
      if (src->bytes == 4 && dest->bytes == 2)       /* RGBA  -> GRAYA */
        temp_buf_to_gray (src, dest);
      else if (src->bytes == 3 && dest->bytes == 1)  /* RGB   -> GRAY  */
        temp_buf_to_gray (src, dest);
      else if (src->bytes == 2 && dest->bytes == 4)  /* GRAYA -> RGBA  */
        temp_buf_to_color (src, dest);
      else if (src->bytes == 1 && dest->bytes == 3)  /* GRAY  -> RGB   */
        temp_buf_to_color (src, dest);
      else
        g_warning ("temp_buf_copy(): unimplemented color conversion");
    }
  else
    {
      /* make the copy */
      length = src->width * src->height * src->bytes;
      memcpy (temp_buf_data (dest), temp_buf_data (src), length);
    }

  return dest;
}

TempBuf *
temp_buf_resize (TempBuf *buf,
		 gint     bytes,
		 gint     x,
		 gint     y,
		 gint     width,
		 gint     height)
{
  gint size;

  g_return_val_if_fail (width > 0 && height > 0, NULL);

  /*  calculate the requested size  */
  size = width * height * bytes;

  /*  First, configure the canvas buffer  */
  if (!buf)
    {
      buf = temp_buf_new (width, height, bytes, x, y, NULL);
    }
  else
    {
      if (size != (buf->width * buf->height * buf->bytes))
        {
          /*  Make sure the temp buf is unswapped  */
          temp_buf_unswap (buf);

          /*  Reallocate the data for it  */
          buf->data = g_renew (guchar, buf->data, size);
        }

      /*  Make sure the temp buf fields are valid  */
      buf->x      = x;
      buf->y      = y;
      buf->width  = width;
      buf->height = height;
      buf->bytes  = bytes;
    }

  return buf;
}

TempBuf *
temp_buf_scale (TempBuf *src,
		gint     new_width,
		gint     new_height)
{
  gint     loop1;
  gint     loop2;
  gdouble  x_ratio;
  gdouble  y_ratio;
  guchar  *src_data;
  guchar  *dest_data;
  TempBuf *dest;

  g_return_val_if_fail (src != NULL, NULL);
  g_return_val_if_fail (new_width > 0 && new_height > 0, NULL);

  dest = temp_buf_new (new_width,
		       new_height,
		       src->bytes,
		       0, 0, NULL);

  src_data  = temp_buf_data (src);
  dest_data = temp_buf_data (dest);

  x_ratio = (gdouble) src->width  / (gdouble) new_width;
  y_ratio = (gdouble) src->height / (gdouble) new_height;

  for (loop1 = 0 ; loop1 < new_height ; loop1++)
    {
      for (loop2 = 0 ; loop2 < new_width ; loop2++)
	{
	  gint    i;
	  guchar *src_pixel;
	  guchar *dest_pixel;

	  src_pixel = src_data +
	    (gint) (loop2 * x_ratio) * src->bytes +
	    (gint) (loop1 * y_ratio) * src->bytes * src->width;

	  dest_pixel = dest_data +
	    (loop2 + loop1 * new_width) * src->bytes;

	  for (i = 0 ; i < src->bytes; i++)
	    *dest_pixel++ = *src_pixel++;
	}
    }

  return dest;
}

TempBuf *
temp_buf_copy_area (TempBuf *src,
		    TempBuf *dest,
		    gint     x,
		    gint     y,
		    gint     width,
		    gint     height,
		    gint     dest_x,
		    gint     dest_y)
{
  TempBuf     *new;
  PixelRegion  srcPR  = { 0, };
  PixelRegion  destPR = { 0, };
  guchar       empty[MAX_CHANNELS] = { 0, 0, 0, 0 };
  gint         x1, y1, x2, y2;

  g_return_val_if_fail (src != NULL, dest);
  g_return_val_if_fail (!dest || dest->bytes == src->bytes, dest);

  g_return_val_if_fail (width  + dest_x > 0, dest);
  g_return_val_if_fail (height + dest_y > 0, dest);

  g_return_val_if_fail (!dest || dest->width  >= width  + dest_x, dest);
  g_return_val_if_fail (!dest || dest->height >= height + dest_y, dest);

  /*  some bounds checking  */
  x1 = CLAMP (x, 0, src->width  - 1);
  y1 = CLAMP (y, 0, src->height - 1);
  x2 = CLAMP (x + width  - 1, 0, src->width  - 1);
  y2 = CLAMP (y + height - 1, 0, src->height - 1);

  if (!(x2 - x1) || !(y2 - y1))
    return dest;

  width  = x2 - x1 + 1;
  height = y2 - y1 + 1;

  if (! dest)
    {
      new = temp_buf_new (width  + dest_x,
			  height + dest_y,
			  src->bytes,
			  0, 0,
			  empty);
    }
  else
    {
      new = dest;
    }

  /*  Copy the region  */
  srcPR.bytes     = src->bytes;
  srcPR.w         = width;
  srcPR.h         = height;
  srcPR.rowstride = src->bytes * src->width;
  srcPR.data      = temp_buf_data (src) + (y1 * srcPR.rowstride +
					   x1 * srcPR.bytes);

  destPR.bytes     = dest->bytes;
  destPR.rowstride = new->bytes * new->width;
  destPR.data      = temp_buf_data (new) + (dest_y * destPR.rowstride +
					    dest_x * destPR.bytes);

  copy_region (&srcPR, &destPR);

  return new;
}

void
temp_buf_free (TempBuf *temp_buf)
{
  g_return_if_fail (temp_buf != NULL);

  if (temp_buf->data)
    g_free (temp_buf->data);

  if (temp_buf->swapped)
    temp_buf_swap_free (temp_buf);

  g_free (temp_buf);
}

guchar *
temp_buf_data (TempBuf *temp_buf)
{
  if (temp_buf->swapped)
    temp_buf_unswap (temp_buf);

  return temp_buf->data;
}

guchar *
temp_buf_data_clear (TempBuf *temp_buf)
{
  g_return_val_if_fail (temp_buf != NULL, NULL);

  if (temp_buf->swapped)
    temp_buf_unswap (temp_buf);

  memset (temp_buf->data, 0,
	  temp_buf->height * temp_buf->width * temp_buf->bytes);

  return temp_buf->data;
}

gsize
temp_buf_get_memsize (TempBuf *temp_buf)
{
  gsize memsize = 0;

  g_return_val_if_fail (temp_buf != NULL, 0);

  memsize += sizeof (TempBuf);

  if (temp_buf->swapped)
    {
      memsize += strlen (temp_buf->filename) + 1;
    }
  else
    {
      memsize += ((gsize) temp_buf->bytes *
                          temp_buf->width *
                          temp_buf->height);
    }

  return memsize;
}


/******************************************************************
 *  Mask buffer functions                                         *
 ******************************************************************/


MaskBuf *
mask_buf_new (gint width,
	      gint height)
{
  static guchar empty = 0;

  return temp_buf_new (width, height, 1, 0, 0, &empty);
}

void
mask_buf_free (MaskBuf *mask)
{
  temp_buf_free ((TempBuf *) mask);
}

guchar *
mask_buf_data (MaskBuf *mask_buf)
{
  return temp_buf_data ((TempBuf *) mask_buf);
}

guchar *
mask_buf_data_clear (MaskBuf *mask_buf)
{
  return temp_buf_data_clear ((TempBuf *) mask_buf);
}


/******************************************************************
 *  temp buffer disk caching functions                            *
 ******************************************************************/

/*  NOTES:
 *  Disk caching is setup as follows:
 *    On a call to temp_buf_swap, the TempBuf parameter is stored
 *    in a temporary variable called cached_in_memory.
 *    On the next call to temp_buf_swap, if cached_in_memory is non-null,
 *    cached_in_memory is moved to disk, and the latest TempBuf parameter
 *    is stored in cached_in_memory.  This method keeps the latest TempBuf
 *    structure in memory instead of moving it directly to disk as requested.
 *    On a call to temp_buf_unswap, if cached_in_memory is non-null, it is
 *    compared against the requested TempBuf.  If they are the same, nothing
 *    must be moved in from disk since it still resides in memory.  However,
 *    if the two pointers are different, the requested TempBuf is retrieved
 *    from disk.  In the former case, cached_in_memory is set to NULL;
 *    in the latter case, cached_in_memory is left unchanged.
 *    If temp_buf_swap_free is called, cached_in_memory must be checked
 *    against the temp buf being freed.  If they are the same, then
 *    cached_in_memory must be set to NULL;
 *
 *  In the case where memory usage is set to "stingy":
 *    temp bufs are not cached in memory at all, they go right to disk.
 */


/*  a static counter for generating unique filenames
 */
static gint tmp_file_index = 0;


/*  a static pointer which keeps track of the last request for
 *  a swapped buffer
 */
static TempBuf *cached_in_memory = NULL;


static gchar *
generate_unique_tmp_filename (GimpBaseConfig *config)
{
  pid_t  pid;
  gchar *tmpdir;
  gchar *tmpfile;
  gchar *path;

  tmpdir = gimp_config_path_expand (config->temp_path, TRUE, NULL);

  pid = getpid ();

  tmpfile = g_strdup_printf ("gimp%d.%d",
                             (gint) pid,
                             tmp_file_index++);

  path = g_build_filename (tmpdir, tmpfile, NULL);

  g_free (tmpfile);
  g_free (tmpdir);

  return path;
}

void
temp_buf_swap (TempBuf *buf)
{
  TempBuf *swap;
  gchar   *filename;
  FILE    *fp;

  if (!buf || buf->swapped)
    return;

  /*  Set the swapped flag  */
  buf->swapped = TRUE;

  if (base_config->stingy_memory_use)
    {
      swap = buf;
    }
  else
    {
      swap = cached_in_memory;
      cached_in_memory = buf;
    }

  /*  For the case where there is no temp buf ready
   *  to be moved to disk, return
   */
  if (! swap)
    return;

  /*  Get a unique filename for caching the data to a UNIX file  */
  filename = generate_unique_tmp_filename (base_config);

  /*  Check if generated filename is valid  */
  if (g_file_test (filename, G_FILE_TEST_IS_DIR))
    {
      g_message ("Error in temp buf caching: \"%s\" is a directory (cannot overwrite)",
		 gimp_filename_to_utf8 (filename));
      g_free (filename);
      return;
    }

  /*  Open file for overwrite  */
  if ((fp = fopen (filename, "wb")))
    {
      gsize blocks_written;

      blocks_written = fwrite (swap->data,
                               swap->width * swap->height * swap->bytes, 1,
                               fp);

      /* Check whether all bytes were written and fclose() was able
         to flush its buffers */
      if ((0 != fclose (fp)) || (1 != blocks_written))
        {
          unlink (filename);
          perror ("Write error on temp buf");
          g_message ("Cannot write \"%s\"",
		     gimp_filename_to_utf8 (filename));
          g_free (filename);
          return;
        }
    }
  else
    {
      unlink (filename);
      perror ("Error in temp buf caching");
      g_message ("Cannot write \"%s\"",
		 gimp_filename_to_utf8 (filename));
      g_free (filename);
      return;
    }
  /*  Finally, free the buffer's data  */
  g_free (swap->data);
  swap->data = NULL;

  swap->filename = filename;
}

void
temp_buf_unswap (TempBuf *buf)
{
  FILE     *fp;
  gboolean  succ = FALSE;

  if (!buf || !buf->swapped)
    return;

  /*  Set the swapped flag  */
  buf->swapped = FALSE;

  /*  If the requested temp buf is still in memory, simply return  */
  if (cached_in_memory == buf)
    {
      cached_in_memory = NULL;
      return;
    }

  /*  Allocate memory for the buffer's data  */
  buf->data   = temp_buf_allocate (buf->width * buf->height * buf->bytes);

  if (g_file_test (buf->filename, G_FILE_TEST_IS_REGULAR))
    {
      if ((fp = fopen (buf->filename, "rb")))
	{
	  gsize blocks_read;

	  blocks_read = fread (buf->data,
                               buf->width * buf->height * buf->bytes, 1,
                               fp);

	  fclose (fp);
	  if (blocks_read != 1)
            perror ("Read error on temp buf");
	  else
	    succ = TRUE;
	}
      else
	perror ("Error in temp buf caching");

      /*  Delete the swap file  */
      unlink (buf->filename);
    }

  if (!succ)
    g_message ("Error in temp buf caching: "
               "information swapped to disk was lost!");

  g_free (buf->filename);   /*  free filename  */
  buf->filename = NULL;
}

void
temp_buf_swap_free (TempBuf *buf)
{
  if (!buf->swapped)
    return;

  /*  Set the swapped flag  */
  buf->swapped = FALSE;

  /*  If the requested temp buf is cached in memory...  */
  if (cached_in_memory == buf)
    {
      cached_in_memory = NULL;
      return;
    }

  /*  Find out if the filename of the swapped data is an existing file... */
  if (g_file_test (buf->filename, G_FILE_TEST_IS_REGULAR))
    {
      /*  Delete the swap file  */
      unlink (buf->filename);
    }
  else
    g_message ("Error in temp buf disk swapping: "
               "information swapped to disk was lost!");

  if (buf->filename)
    g_free (buf->filename);   /*  free filename  */
  buf->filename = NULL;
}

void
swapping_free (void)
{
  if (cached_in_memory)
    temp_buf_free (cached_in_memory);
}
