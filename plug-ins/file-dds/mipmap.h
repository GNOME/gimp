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

#ifndef __MIPMAP_H__
#define __MIPMAP_H__

int          get_num_mipmaps            (int            width,
                                         int            height);
unsigned int get_mipmapped_size         (int            width,
                                         int            height,
                                         int            bpp,
                                         int            level,
                                         int            num,
                                         int            format);
unsigned int get_volume_mipmapped_size  (int            width,
                                         int            height,
                                         int            depth,
                                         int            bpp,
                                         int            level,
                                         int            num,
                                         int            format);
int          get_next_mipmap_dimensions (int           *next_w,
                                         int           *next_h,
                                         int            curr_w,
                                         int            curr_h);

float        cubic_interpolate          (float          a,
                                         float          b,
                                         float          c,
                                         float          d,
                                         float          x);
int          generate_mipmaps           (unsigned char *dst,
                                         unsigned char *src,
                                         unsigned int   width,
                                         unsigned int   height,
                                         int            bpp,
                                         int            indexed,
                                         int            mipmaps,
                                         int            filter,
                                         int            wrap,
                                         int            gamma_correct,
                                         float          gamma,
                                         int            preserve_alpha_test_coverage,
                                         float          alpha_test_threshold);
int          generate_volume_mipmaps    (unsigned char *dst,
                                         unsigned char *src,
                                         unsigned int   width,
                                         unsigned int   height,
                                         unsigned int   depth,
                                         int            bpp,
                                         int            indexed,
                                         int            mipmaps,
                                         int            filter,
                                         int            wrap,
                                         int            gamma_correct,
                                         float          gamma);

#endif /* __MIPMAP_H__ */
