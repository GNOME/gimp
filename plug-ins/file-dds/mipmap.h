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


gint    get_num_mipmaps            (gint     width,
                                    gint     height);

guint   get_mipmapped_size         (gint     width,
                                    gint     height,
                                    gint     bpp,
                                    gint     level,
                                    gint     num,
                                    gint     format);

guint   get_volume_mipmapped_size  (gint     width,
                                    gint     height,
                                    gint     depth,
                                    gint     bpp,
                                    gint     level,
                                    gint     num,
                                    gint     format);

gint    get_next_mipmap_dimensions (gint    *next_w,
                                    gint    *next_h,
                                    gint     curr_w,
                                    gint     curr_h);

gint    generate_mipmaps           (guchar  *dst,
                                    guchar  *src,
                                    guint    width,
                                    guint    height,
                                    gint     bpp,
                                    gint     indexed,
                                    gint     mipmaps,
                                    gint     filter,
                                    gint     wrap,
                                    gint     gamma_correct,
                                    gfloat   gamma,
                                    gint     preserve_alpha_test_coverage,
                                    gfloat   alpha_test_threshold);

gint    generate_volume_mipmaps    (guchar  *dst,
                                    guchar  *src,
                                    guint    width,
                                    guint    height,
                                    guint    depth,
                                    gint     bpp,
                                    gint     indexed,
                                    gint     mipmaps,
                                    gint     filter,
                                    gint     wrap,
                                    gint     gamma_correct,
                                    gfloat   gamma);


#endif /* __MIPMAP_H__ */
