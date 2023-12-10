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

#ifndef __FORMATS_H__
#define __FORMATS_H__


typedef struct
{
  D3DFORMAT         d3d9_format;
  DXGI_FORMAT       dxgi_format;
  guchar            channel_order[4];
  guchar            channel_bits[4];
  guchar            output_bit_depth;
  gboolean          use_alpha;
  gboolean          is_float;
  gboolean          is_signed;
  GimpImageBaseType gimp_type;
} fmt_read_info_t;


guint             get_d3d9format       (guint   bpp,
                                        guint   rmask,
                                        guint   gmask,
                                        guint   bmask,
                                        guint   amask,
                                        guint   flags);

gboolean          dxgiformat_supported (guint   hdr_dxgifmt);

fmt_read_info_t   get_format_read_info (guint   d3d9_fmt,
                                        guint   dxgi_fmt);

guint32           requantize_component (guint32 component,
                                        guchar  bits_in,
                                        guchar  bits_out);

void              float_from_9e5       (guint32 channels[4]);
void              float_from_7e3a2     (guint32 channels[4]);
void              float_from_6e4a2     (guint32 channels[4]);
void              reconstruct_z        (guint32 channels[4]);

size_t            get_bpp_d3d9         (guint   fmt);
size_t            get_bpp_dxgi         (guint   fmt);


#endif /* __FORMATS_H__ */