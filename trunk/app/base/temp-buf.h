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

#ifndef __TEMP_BUF_H__
#define __TEMP_BUF_H__


struct _TempBuf
{
  gint    bytes;      /*  the necessary info                             */
  gint    width;
  gint    height;
  gint    x, y;       /*  origin of data source                          */
  guchar *data;       /*  The data buffer. Do never access this field
                          directly, use temp_buf_data() instead !!       */
};


/*  The temp buffer functions  */

TempBuf * temp_buf_new         (gint           width,
                                gint           height,
                                gint           bytes,
                                gint           x,
                                gint           y,
                                const guchar  *color);
TempBuf * temp_buf_new_check   (gint           width,
                                gint           height,
                                GimpCheckType  check_type,
                                GimpCheckSize  check_size);
TempBuf * temp_buf_copy        (TempBuf       *src,
                                TempBuf       *dest);
TempBuf * temp_buf_resize      (TempBuf       *buf,
                                gint           bytes,
                                gint           x,
                                gint           y,
                                gint           width,
                                gint           height);
TempBuf * temp_buf_scale       (TempBuf       *buf,
                                gint           width,
                                gint           height) G_GNUC_WARN_UNUSED_RESULT;
TempBuf * temp_buf_copy_area   (TempBuf       *src,
                                TempBuf       *dest,
                                gint           x,
                                gint           y,
                                gint           width,
                                gint           height,
                                gint           dest_x,
                                gint           dest_y);
void      temp_buf_free        (TempBuf       *buf);
guchar  * temp_buf_data        (TempBuf       *buf);
guchar  * temp_buf_data_clear  (TempBuf       *buf);

gsize     temp_buf_get_memsize (TempBuf       *buf);


#endif  /*  __TEMP_BUF_H__  */
