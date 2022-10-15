/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * file-icns.h
 * Copyright (C) 2004 Brion Vibber <brion@pobox.com>
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

#ifndef __ICNS_H__
#define __ICNS_H__


#ifdef ICNS_DBG
#define D(x) \
{ \
  printf("ICNS plugin: "); \
  printf x; \
}
#else
#define D(x)
#endif

#define PLUG_IN_BINARY      "file-icns"
#define PLUG_IN_ROLE        "gimp-file-icns"


typedef struct _IcnsResourceHeader
{
  /* Big-endian! */
  gchar    type[4];
  guint32  size;
} IcnsResourceHeader;

typedef struct _IcnsResource
{
  gchar    type[5];
  guint32  size;
  guint32  cursor;
  guchar  *data;
} IcnsResource;

void
fourcc_get_string (gchar *fourcc,
                   gchar *buf);

#endif /* __ICNS_H__ */
