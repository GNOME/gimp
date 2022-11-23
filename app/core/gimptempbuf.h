/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_TEMP_BUF_H__
#define __LIGMA_TEMP_BUF_H__


LigmaTempBuf * ligma_temp_buf_new               (gint               width,
                                               gint               height,
                                               const Babl        *format) G_GNUC_WARN_UNUSED_RESULT;
LigmaTempBuf * ligma_temp_buf_new_from_pixbuf   (GdkPixbuf         *pixbuf,
                                               const Babl        *f_or_null) G_GNUC_WARN_UNUSED_RESULT;
LigmaTempBuf * ligma_temp_buf_copy              (const LigmaTempBuf *src) G_GNUC_WARN_UNUSED_RESULT;

LigmaTempBuf * ligma_temp_buf_ref               (const LigmaTempBuf *buf);
void          ligma_temp_buf_unref             (const LigmaTempBuf *buf);

LigmaTempBuf * ligma_temp_buf_scale             (const LigmaTempBuf *buf,
                                               gint               width,
                                               gint               height) G_GNUC_WARN_UNUSED_RESULT;

gint          ligma_temp_buf_get_width         (const LigmaTempBuf *buf);
gint          ligma_temp_buf_get_height        (const LigmaTempBuf *buf);

const Babl  * ligma_temp_buf_get_format        (const LigmaTempBuf *buf);
void          ligma_temp_buf_set_format        (LigmaTempBuf       *buf,
                                               const Babl        *format);

guchar      * ligma_temp_buf_get_data          (const LigmaTempBuf *buf);
gsize         ligma_temp_buf_get_data_size     (const LigmaTempBuf *buf);

guchar      * ligma_temp_buf_data_clear        (LigmaTempBuf       *buf);

gpointer      ligma_temp_buf_lock              (const LigmaTempBuf *buf,
                                               const Babl        *format,
                                               GeglAccessMode     access_mode) G_GNUC_WARN_UNUSED_RESULT;
void          ligma_temp_buf_unlock            (const LigmaTempBuf *buf,
                                               gconstpointer      data);

gsize         ligma_temp_buf_get_memsize       (const LigmaTempBuf *buf);

GeglBuffer  * ligma_temp_buf_create_buffer     (const LigmaTempBuf *temp_buf) G_GNUC_WARN_UNUSED_RESULT;
GdkPixbuf   * ligma_temp_buf_create_pixbuf     (const LigmaTempBuf *temp_buf) G_GNUC_WARN_UNUSED_RESULT;

LigmaTempBuf * ligma_gegl_buffer_get_temp_buf   (GeglBuffer        *buffer);


/*  stats  */

guint64       ligma_temp_buf_get_total_memsize (void);


#endif  /*  __LIGMA_TEMP_BUF_H__  */
