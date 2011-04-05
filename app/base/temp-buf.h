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

#ifndef __TEMP_BUF_H__
#define __TEMP_BUF_H__


struct _TempBuf
{
  gint      bytes;      /*  number of bytes per pixel (1,2,3 or 4)         */
  gint      width;
  gint      height;
  gint      x, y;       /*  origin of data source                          */
  guchar   *data;       /*  The data buffer. Do never access this field
                            directly, use temp_buf_get_data() instead !!   */
};


/*  The temp buffer functions  */

TempBuf * temp_buf_new           (gint           width,
                                  gint           height,
                                  gint           bytes,
                                  gint           x,
                                  gint           y,
                                  const guchar  *color);
TempBuf * temp_buf_copy          (TempBuf       *src,
                                  TempBuf       *dest);
TempBuf * temp_buf_resize        (TempBuf       *buf,
                                  gint           bytes,
                                  gint           x,
                                  gint           y,
                                  gint           width,
                                  gint           height);
TempBuf * temp_buf_scale         (TempBuf       *buf,
                                  gint           width,
                                  gint           height) G_GNUC_WARN_UNUSED_RESULT;
TempBuf * temp_buf_copy_area     (TempBuf       *src,
                                  TempBuf       *dest,
                                  gint           x,
                                  gint           y,
                                  gint           width,
                                  gint           height,
                                  gint           dest_x,
                                  gint           dest_y);

void      temp_buf_demultiply    (TempBuf       *buf);

void      temp_buf_free          (TempBuf       *buf);
guchar  * temp_buf_get_data      (const TempBuf *buf);
gsize     temp_buf_get_data_size (TempBuf       *buf);
guchar  * temp_buf_data_clear    (TempBuf       *buf);

gsize     temp_buf_get_memsize   (TempBuf       *buf);
void      temp_buf_dump          (TempBuf       *buf,
                                  const gchar   *filename);


#endif  /*  __TEMP_BUF_H__  */
