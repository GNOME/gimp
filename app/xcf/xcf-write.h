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

#ifndef __XCF_WRITE_H__
#define __XCF_WRITE_H__


guint   xcf_write_int8        (XcfInfo        *info,
                               const guint8   *data,
                               gint            count,
                               GError        **error);
guint   xcf_write_int16       (XcfInfo        *info,
                               const guint16  *data,
                               gint            count,
                               GError        **error);
guint   xcf_write_int32       (XcfInfo        *info,
                               const guint32  *data,
                               gint            count,
                               GError        **error);
guint   xcf_write_int64       (XcfInfo        *info,
                               const guint64  *data,
                               gint            count,
                               GError        **error);
guint   xcf_write_offset      (XcfInfo        *info,
                               const goffset  *data,
                               gint            count,
                               GError        **error);
guint   xcf_write_zero_offset (XcfInfo        *info,
                               gint            count,
                               GError        **error);
guint   xcf_write_float       (XcfInfo        *info,
                               const gfloat   *data,
                               gint            count,
                               GError        **error);
guint   xcf_write_string      (XcfInfo        *info,
                               gchar         **data,
                               gint            count,
                               GError        **error);
guint   xcf_write_component   (XcfInfo        *info,
                               gint            bpc,
                               const guint8   *data,
                               gint            count,
                               GError        **error);

void    xcf_write_to_be       (gint            bpc,
                               guint8         *data,
                               gint            count);


#endif  /* __XCF_WRITE_H__ */
