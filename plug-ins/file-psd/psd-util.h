/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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

#ifndef __PSD_UTIL_H__
#define __PSD_UTIL_H__

/*
 *  Set file read error
 */
void                    psd_set_error          (const gboolean  file_eof,
                                                const gint      err_no,
                                                GError        **error);

/*
 * Reads a pascal string from the file padded to a multiple of mod_len
 * and returns a utf-8 string.
 */
gchar                 * fread_pascal_string    (gint32         *bytes_read,
                                                gint32         *bytes_written,
                                                const guint16   mod_len,
                                                FILE           *f,
                                                GError        **error);

/*
 *  Converts utf-8 string to current locale and writes as pascal
 *  string with padding to a multiple of mod_len.
 */
gint32                  fwrite_pascal_string   (const gchar    *src,
                                                const guint16   mod_len,
                                                FILE           *f,
                                                GError        **error);

/*
 * Reads a utf-16 string from the file padded to a multiple of mod_len
 * and returns a utf-8 string.
 */
gchar                 * fread_unicode_string   (gint32         *bytes_read,
                                                gint32         *bytes_written,
                                                const guint16   mod_len,
                                                FILE           *f,
                                                GError        **error);

/*
 *  Converts utf-8 string to utf-16 and writes 4 byte length
 *  then string padding to multiple of mod_len.
 */
gint32                  fwrite_unicode_string  (const gchar    *src,
                                                const guint16   mod_len,
                                                FILE           *f,
                                                GError        **error);

gint                    decode_packbits        (const gchar    *src,
                                                gchar          *dst,
                                                guint16         packed_len,
                                                guint32         unpacked_len);

gchar                 * encode_packbits        (const gchar    *src,
                                                const guint32   unpacked_len,
                                                guint16        *packed_len);

GimpLayerModeEffects    psd_to_gimp_blend_mode (const gchar    *psd_mode);

gchar *                 gimp_to_psd_blend_mode (const GimpLayerModeEffects gimp_layer_mode);

#endif /* __PSD_UTIL_H__ */
