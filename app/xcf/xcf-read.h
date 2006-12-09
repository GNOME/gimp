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

#ifndef __XCF_READ_H__
#define __XCF_READ_H__


guint   xcf_read_int32  (FILE     *fp,
                         guint32  *data,
                         gint      count);
guint   xcf_read_float  (FILE     *fp,
                         gfloat   *data,
                         gint      count);
guint   xcf_read_int8   (FILE     *fp,
                         guint8   *data,
                         gint      count);
guint   xcf_read_string (FILE     *fp,
                         gchar   **data,
                         gint      count);


#endif  /* __XCF_READ_H__ */
