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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_TEMP_BUF_H__
#define __GIMP_TEMP_BUF_H__


GimpTempBuf * gimp_temp_buf_new               (gint               width,
                                               gint               height,
                                               const Babl        *format) G_GNUC_WARN_UNUSED_RESULT;
GimpTempBuf * gimp_temp_buf_new_from_pixbuf   (GdkPixbuf         *pixbuf,
                                               const Babl        *f_or_null) G_GNUC_WARN_UNUSED_RESULT;
GimpTempBuf * gimp_temp_buf_copy              (const GimpTempBuf *src) G_GNUC_WARN_UNUSED_RESULT;

GimpTempBuf * gimp_temp_buf_ref               (const GimpTempBuf *buf);
void          gimp_temp_buf_unref             (const GimpTempBuf *buf);

GimpTempBuf * gimp_temp_buf_scale             (const GimpTempBuf *buf,
                                               gint               width,
                                               gint               height) G_GNUC_WARN_UNUSED_RESULT;

gint          gimp_temp_buf_get_width         (const GimpTempBuf *buf);
gint          gimp_temp_buf_get_height        (const GimpTempBuf *buf);

const Babl  * gimp_temp_buf_get_format        (const GimpTempBuf *buf);
void          gimp_temp_buf_set_format        (GimpTempBuf       *buf,
                                               const Babl        *format);

guchar      * gimp_temp_buf_get_data          (const GimpTempBuf *buf);
gsize         gimp_temp_buf_get_data_size     (const GimpTempBuf *buf);

guchar      * gimp_temp_buf_data_clear        (GimpTempBuf       *buf);

gpointer      gimp_temp_buf_lock              (const GimpTempBuf *buf,
                                               const Babl        *format,
                                               GeglAccessMode     access_mode) G_GNUC_WARN_UNUSED_RESULT;
void          gimp_temp_buf_unlock            (const GimpTempBuf *buf,
                                               gconstpointer      data);

gsize         gimp_temp_buf_get_memsize       (const GimpTempBuf *buf);

GeglBuffer  * gimp_temp_buf_create_buffer     (const GimpTempBuf *temp_buf) G_GNUC_WARN_UNUSED_RESULT;
GdkPixbuf   * gimp_temp_buf_create_pixbuf     (const GimpTempBuf *temp_buf) G_GNUC_WARN_UNUSED_RESULT;

GimpTempBuf * gimp_gegl_buffer_get_temp_buf   (GeglBuffer        *buffer);


/*  stats  */

guint64       gimp_temp_buf_get_total_memsize (void);


#endif  /*  __GIMP_TEMP_BUF_H__  */
