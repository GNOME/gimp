/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
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

#ifndef __DXT_H__
#define __DXT_H__

typedef enum dxt_flags_e
{
  DXT_BC1           = 1 << 0,
  DXT_BC2           = 1 << 1,
  DXT_BC3           = 1 << 2,
  DXT_PERCEPTUAL    = 1 << 3,
} dxt_flags_t;

int dxt_compress   (unsigned char *dst,
                    unsigned char *src,
                    int            format,
                    unsigned int   width,
                    unsigned int   height,
                    int            bpp,
                    int            mipmaps,
                    int            flags);
int dxt_decompress (unsigned char *dst,
                    unsigned char *src,
                    int            format,
                    unsigned int   size,
                    unsigned int   width,
                    unsigned int   height,
                    int            bpp,
                    int            normals);

#endif /* __DXT_H__ */
