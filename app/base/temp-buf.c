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

#include <gtk/gtk.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef G_OS_WIN32
#include <process.h>		/* For _getpid() */
#endif
 
#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpimage.h"

#include "gimprc.h"
#include "pixel_region.h"
#include "temp_buf.h"
#include "image_render.h"


static guchar * temp_buf_allocate (guint);
static void     temp_buf_to_color (TempBuf *src_buf,
				   TempBuf *dest_buf);
static void     temp_buf_to_gray  (TempBuf *src_buf,
				   TempBuf *dest_buf);


/*  Memory management  */

static guchar *
temp_buf_allocate (guint size)
{
  guchar *data;

  data = g_new (guchar, size);

  return data;
}


/*  The conversion routines  */

static void
temp_buf_to_color (TempBuf *src_buf,
		   TempBuf *dest_buf)
{
  guchar *src;
  guchar *dest;
  glong   num_bytes;

  src = temp_buf_data (src_buf);
  dest = temp_buf_data (dest_buf);

  num_bytes = src_buf->width * src_buf->height;

  while (num_bytes--)
    {
      guchar tmpch;
      *dest++ = *src++;  /* alpha channel */
      *dest++ = tmpch = *src++;
      *dest++ = tmpch;
      *dest++ = tmpch;
    }
}


static void
temp_buf_to_gray (TempBuf *src_buf,
		  TempBuf *dest_buf)
{
  guchar *src;
  guchar *dest;
  glong   num_bytes;
  gfloat  pix;

  src = temp_buf_data (src_buf);
  dest = temp_buf_data (dest_buf);

  num_bytes = src_buf->width * src_buf->height;

  while (num_bytes--)
    {
      *dest++ = *src++;  /* alpha channel */

      pix = INTENSITY (*src++, *src++, *src++);

      *dest++ = (guchar) pix;
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
  gint     j;
  guchar  *data;
  TempBuf *temp;

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
      if (bytes == 1)
        {
          memset (data, *col, width * height);
        }
      else if ((bytes == 3) && (col[1] == *col) && (*col == col[2]))
        {
          memset (data, *col, width * height * 3);
        }
      else if ((bytes == 4) &&
	       (col[1] == *col) && (*col == col[2]) && (col[2] == col[3]))
        {
          memset (data, *col, (width * height) << 2);
        }
      else
        {
          /* No, we cannot */
          guchar *dptr;

          /* Fill the first row */
          dptr = data;

          for (i = width - 1; i >= 0; --i)
            {
              guchar *init;

              j    = bytes;
              init = col;

              while (j--)
                *dptr++ = *init++;
            }

          /* Now copy from it (we set bytes to bytesperrow now) */
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
   parameters into a newly allocated tempbuf */

TempBuf *
temp_buf_new_check (gint width,
		    gint height,
		    GimpCheckType check_type,
	            GimpCheckSize check_size)
{
  TempBuf *newbuf = NULL;
  guchar *data = 0;
  guchar check_shift = 0;
  guchar fg_color = 0;
  guchar bg_color = 0;
  gint j, i = 0;

  if (check_type < LIGHT_CHECKS || check_type > BLACK_ONLY)
    g_error ("invalid check_type argument to temp_buf_check: %d", check_type);
  if (check_size < SMALL_CHECKS || check_size > LARGE_CHECKS)
    g_error ("invalid check_size argument to temp_buf_check: %d", check_size);

  switch (check_size)
    {
    case SMALL_CHECKS:
      check_shift = 2;
      break;
    case MEDIUM_CHECKS:
      check_shift = 4;
      break;
    case LARGE_CHECKS:
      check_shift = 6;
    }
 
  switch (check_type)
    {
      case LIGHT_CHECKS:
        fg_color = 204;
        bg_color = 255;
	break;
      case GRAY_CHECKS:
        fg_color = 102;
        bg_color = 153;
	break;
      case DARK_CHECKS:
        fg_color = 0;
        bg_color = 51;
	break;
      case WHITE_ONLY:
        fg_color = 255;
        bg_color = 255;
	break;
      case GRAY_ONLY:
        fg_color = 127;
        bg_color = 127;
	break;
      case BLACK_ONLY:
        fg_color = 0;
        bg_color = 0;
    }
 
  newbuf = temp_buf_new (width, height, 3, 0, 0, NULL);
  data = temp_buf_data (newbuf);
  
  for (j = 1; i <= height; j++)
    {
      for (i = 1; i <= width; i++)
	{
	  *(data + i - 1) = ((j & check_shift) && (i & check_shift)) 
	                    ? fg_color : bg_color;
	}	  
    } 
 
  return newbuf;
}

TempBuf *
temp_buf_copy (TempBuf *src,
	       TempBuf *dest)
{
  TempBuf *new;
  glong    length;

  if (!src)
    {
      g_message ("trying to copy a temp buf which is NULL.");
      return dest;
    }

  if (!dest)
    {
      new = temp_buf_new (src->width, src->height, src->bytes, 0, 0, NULL);
    }
  else
    {
      new = dest;
      if (dest->width != src->width || dest->height != src->height)
	g_message ("In temp_buf_copy, the widths or heights don't match.");
      /*  The temp buf is smart, and can translate between color and gray  */
      /*  (only necessary if not we allocated it */
      if (src->bytes != new->bytes)
        {
          if (src->bytes == 4)  /* RGB color */
	    temp_buf_to_gray (src, new);
          else if (src->bytes == 2) /* grayscale */
	    temp_buf_to_color (src, new);
          else
	    g_message ("Cannot convert from indexed color.");

	  return new;
        }
    }

  /* make the copy */
  length = src->width * src->height * src->bytes;
  memcpy (temp_buf_data (new), temp_buf_data (src), length);

  return new;
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
	buf->data = g_realloc (buf->data, size);
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
  PixelRegion  srcR, destR;
  guchar       empty[MAX_CHANNELS] = { 0, 0, 0, 0 };
  gint         x1, y1, x2, y2;

  g_return_val_if_fail (src  != NULL, dest);
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
  srcR.bytes     = src->bytes;
  srcR.w         = width;
  srcR.h         = height;
  srcR.rowstride = src->bytes * src->width;
  srcR.data      = (temp_buf_data (src) +
		    y1 * srcR.rowstride +
		    x1 * srcR.bytes);

  destR.bytes     = dest->bytes;
  destR.rowstride = new->bytes * new->width;
  destR.data      = (temp_buf_data (new) +
		     dest_y * destR.rowstride +
		     dest_x * destR.bytes);

  copy_region (&srcR, &destR);

  return new;
}

void
temp_buf_free (TempBuf *temp_buf)
{  if (temp_buf->data)
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
  if (temp_buf->swapped)
    temp_buf_unswap (temp_buf);
  
  memset (temp_buf->data, 0,
	  temp_buf->height * temp_buf->width * temp_buf->bytes);

  return temp_buf->data;
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


/*  a static counter for generating unique filenames  */
static gint swap_index = 0;

/*  a static pointer which keeps track of the last request for a swapped buffer  */
static TempBuf * cached_in_memory = NULL;


static gchar *
generate_unique_filename (void)
{
  pid_t pid;

  pid = getpid ();
  return g_strdup_printf ("%s" G_DIR_SEPARATOR_S "gimp%d.%d",
			  temp_path, (int) pid, swap_index++);
}

void
temp_buf_swap (TempBuf *buf)
{
  TempBuf     *swap;
  gchar       *filename;
  struct stat  stat_buf;
  gint         err;
  FILE        *fp;

  if (!buf || buf->swapped)
    return;

  /*  Set the swapped flag  */
  buf->swapped = TRUE;

  if (stingy_memory_use)
    swap = buf;
  else
    {
      swap = cached_in_memory;
      cached_in_memory = buf;
    }

  /*  For the case where there is no temp buf ready to be moved to disk, return  */
  if (!swap)
    return;

  /*  Get a unique filename for caching the data to a UNIX file  */
  filename = generate_unique_filename ();

  /*  Check if generated filename is valid  */
  err = stat (filename, &stat_buf);
  if (!err)
    {
      if (stat_buf.st_mode & S_IFDIR)
	{
	  g_message ("Error in temp buf caching: \"%s\" is a directory (cannot overwrite)", filename);
	  g_free (filename);
	  return;
	}
    }

  /*  Open file for overwrite  */
  if ((fp = fopen (filename, "wb")))
    {
      size_t blocks_written;
      blocks_written = fwrite (swap->data, swap->width * swap->height * swap->bytes, 1, fp);
      /* Check whether all bytes were written and fclose() was able to flush its buffers */
      if ((0 != fclose (fp)) || (1 != blocks_written))
        {
          (void) unlink (filename);
          perror ("Write error on temp buf");
          g_message ("Cannot write \"%s\"", filename);
          g_free (filename);
          return;
        }
    }
  else
    {
      (void) unlink (filename);
      perror ("Error in temp buf caching");
      g_message ("Cannot write \"%s\"", filename);
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
  struct stat  stat_buf;
  FILE        *fp;
  gboolean     succ = FALSE;

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

  /*  Find out if the filename of the swapped data is an existing file... */
  /*  (buf->filname HAS to be != 0 */
  if (!stat (buf->filename, &stat_buf))
    {
      if ((fp = fopen (buf->filename, "rb")))
	{
	  size_t blocks_read;
	  blocks_read = fread (buf->data, buf->width * buf->height * buf->bytes, 1, fp);
	  (void) fclose (fp);
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
    g_message ("Error in temp buf caching: information swapped to disk was lost!");

  g_free (buf->filename);   /*  free filename  */
  buf->filename = NULL;
}

void
temp_buf_swap_free (TempBuf *buf)
{
  struct stat stat_buf;

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
  if (!stat (buf->filename, &stat_buf))
    {
      /*  Delete the swap file  */
      unlink (buf->filename);
    }
  else
    g_message ("Error in temp buf disk swapping: information swapped to disk was lost!");

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
