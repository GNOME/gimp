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


struct _GimpTempBuf
{
  const Babl *format;  /*  pixel format  */
  gint        width;
  gint        height;
  gint        x, y;      /*  origin of data source                          */
  guchar     *data;      /*  The data buffer. Do never access this field
                           directly, use temp_buf_get_data() instead !!   */
};


/*  The temp buffer functions  */

GimpTempBuf * temp_buf_new           (gint               width,
                                      gint               height,
                                      const Babl        *fomat);
GimpTempBuf * temp_buf_copy          (GimpTempBuf       *src);
GimpTempBuf * temp_buf_scale         (GimpTempBuf       *buf,
                                      gint               width,
                                      gint               height) G_GNUC_WARN_UNUSED_RESULT;

void          temp_buf_demultiply    (GimpTempBuf       *buf);

void          temp_buf_free          (GimpTempBuf       *buf);
guchar      * temp_buf_get_data      (const GimpTempBuf *buf);
gsize         temp_buf_get_data_size (GimpTempBuf       *buf);
guchar      * temp_buf_data_clear    (GimpTempBuf       *buf);

gsize         temp_buf_get_memsize   (GimpTempBuf       *buf);
void          temp_buf_dump          (GimpTempBuf       *buf,
                                      const gchar       *filename);

GeglBuffer  * gimp_temp_buf_create_buffer (GimpTempBuf  *temp_buf,
                                          gboolean       take_ownership);


#endif  /*  __TEMP_BUF_H__  */
