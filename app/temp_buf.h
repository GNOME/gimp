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
#ifndef __TEMP_BUF_H__
#define __TEMP_BUF_H__

typedef struct _TempBuf TempBuf;
typedef struct _TempBuf MaskBuf;

struct _TempBuf
{
  gint      bytes;      /*  The necessary info  */
  gint      width;
  gint      height;
  gint      x, y;       /*  origin of data source  */

  gboolean  swapped;    /*  flag indicating whether buf is cached to disk  */
  gchar    *filename;   /*  filename of cached information  */

  guchar   *data;       /*  The data buffer     */
};


/*  The temp buffer functions  */

TempBuf * temp_buf_new        (gint, gint, gint, gint, gint, guchar *);
TempBuf * temp_buf_copy       (TempBuf *, TempBuf *);
TempBuf * temp_buf_resize     (TempBuf *, gint, gint, gint, gint, gint);
TempBuf * temp_buf_copy_area  (TempBuf *, TempBuf *,
			       gint, gint, gint, gint, gint);
void      temp_buf_free       (TempBuf *);
guchar  * temp_buf_data       (TempBuf *);

/* The mask buffer functions  */

MaskBuf * mask_buf_new        (gint, gint);
void      mask_buf_free       (MaskBuf *);
guchar  * mask_buf_data       (MaskBuf *);

/*  The disk caching functions  */

void      temp_buf_swap       (TempBuf *);
void      temp_buf_unswap     (TempBuf *);
void      temp_buf_swap_free  (TempBuf *);


/*  Called by app_procs:exit() to free up the cached undo buffer  */

void      swapping_free       (void);

#endif  /*  __TEMP_BUF_H__  */
