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

#pragma once


guint   xcf_read_int8      (XcfInfo  *info,
                            guint8   *data,
                            gint      count);
guint   xcf_read_int16     (XcfInfo  *info,
                            guint16  *data,
                            gint      count);
guint   xcf_read_int32     (XcfInfo  *info,
                            guint32  *data,
                            gint      count);
guint   xcf_read_int64     (XcfInfo  *info,
                            guint64  *data,
                            gint      count);
guint   xcf_read_offset    (XcfInfo  *info,
                            goffset  *data,
                            gint      count);
guint   xcf_read_float     (XcfInfo  *info,
                            gfloat   *data,
                            gint      count);
guint   xcf_read_string    (XcfInfo  *info,
                            gchar   **data,
                            gint      count);
guint   xcf_read_component (XcfInfo  *info,
                            gint      bpc,
                            guint8   *data,
                            gint      count);

void    xcf_read_from_be   (gint      bpc,
                            guint8   *data,
                            gint      count);
