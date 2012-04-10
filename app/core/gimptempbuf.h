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

#ifndef __GIMP_TEMP_BUF_H__
#define __GIMP_TEMP_BUF_H__


struct _GimpTempBuf
{
  gint        ref_count;
  const Babl *format;
  gint        width;
  gint        height;
  guchar     *data;
};


GimpTempBuf * gimp_temp_buf_new             (gint               width,
                                             gint               height,
                                             const Babl        *fomat) G_GNUC_WARN_UNUSED_RESULT;
GimpTempBuf * gimp_temp_buf_copy            (const GimpTempBuf *src) G_GNUC_WARN_UNUSED_RESULT;

GimpTempBuf * gimp_temp_buf_ref             (GimpTempBuf       *buf);
void          gimp_temp_buf_unref           (GimpTempBuf       *buf);

GimpTempBuf * gimp_temp_buf_scale           (const GimpTempBuf *buf,
                                             gint               width,
                                             gint               height) G_GNUC_WARN_UNUSED_RESULT;

guchar      * gimp_temp_buf_get_data        (const GimpTempBuf *buf);
gsize         gimp_temp_buf_get_data_size   (const GimpTempBuf *buf);
guchar      * gimp_temp_buf_data_clear      (GimpTempBuf       *buf);

gsize         gimp_temp_buf_get_memsize     (const GimpTempBuf *buf);

GeglBuffer  * gimp_temp_buf_create_buffer   (GimpTempBuf       *temp_buf) G_GNUC_WARN_UNUSED_RESULT;
GimpTempBuf * gimp_gegl_buffer_get_temp_buf (GeglBuffer        *buffer);



#endif  /*  __GIMP_TEMP_BUF_H__  */
