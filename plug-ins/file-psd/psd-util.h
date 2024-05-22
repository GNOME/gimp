/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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

#ifndef __PSD_UTIL_H__
#define __PSD_UTIL_H__

#define PSD_TELL(f)     g_seekable_tell (G_SEEKABLE (input))

/*
 *  Set file read error
 */
void                    psd_set_error          (GError             **error);

gint                    psd_read               (GInputStream        *input,
                                                gconstpointer        data,
                                                gint                 count,
                                                GError             **error);

gboolean                psd_read_len           (GInputStream        *input,
                                                guint64             *data,
                                                gint                 psd_version,
                                                GError            **error);

gboolean                psd_seek               (GInputStream        *input,
                                                goffset              offset,
                                                GSeekType            type,
                                                GError             **error);

/*
 * Reads a pascal string from the file padded to a multiple of mod_len
 * and returns a utf-8 string.
 */
gchar                 * fread_pascal_string    (gint32              *bytes_read,
                                                gint32              *bytes_written,
                                                guint16              mod_len,
                                                GInputStream        *input,
                                                GError             **error);

/*
 *  Converts utf-8 string to current locale and writes as pascal
 *  string with padding to a multiple of mod_len.
 */
gint32                  fwrite_pascal_string   (const gchar         *src,
                                                guint16              mod_len,
                                                GOutputStream       *output,
                                                GError             **error);

/*
 * Reads a utf-16 string from the file padded to a multiple of mod_len
 * and returns a utf-8 string.
 */
gchar                 * fread_unicode_string   (gint32              *bytes_read,
                                                gint32              *bytes_written,
                                                guint16              mod_len,
                                                gboolean             ibm_pc_format,
                                                GInputStream        *input,
                                                GError             **error);

/*
 *  Converts utf-8 string to utf-16 and writes 4 byte length
 *  then string padding to multiple of mod_len.
 */
gint32                  fwrite_unicode_string  (const gchar         *src,
                                                guint16              mod_len,
                                                GOutputStream       *output,
                                                GError             **error);

gint                    decode_packbits        (const gchar         *src,
                                                gchar               *dst,
                                                guint16              packed_len,
                                                guint32              unpacked_len);

gchar                 * encode_packbits        (const gchar         *src,
                                                guint32              unpacked_len,
                                                guint16             *packed_len);

void                    psd_to_gimp_blend_mode (PSDlayer             *psd_layer,
                                                LayerModeInfo        *mode_info);

const gchar *           gimp_to_psd_blend_mode (const LayerModeInfo  *mode_info);

GimpColorTag            psd_to_gimp_layer_color_tag (guint16          layer_color_tag);

guint16                 gimp_to_psd_layer_color_tag (GimpColorTag     layer_color_tag);

#endif /* __PSD_UTIL_H__ */
