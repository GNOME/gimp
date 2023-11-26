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

#ifndef __MISC_H__
#define __MISC_H__


void  decode_ycocg          (GimpDrawable *drawable);

void  decode_ycocg_scaled   (GimpDrawable *drawable);

void  decode_alpha_exponent (GimpDrawable *drawable);

void  encode_ycocg          (guchar *dst,
                             gint    r,
                             gint    g,
                             gint    b);

void  encode_alpha_exponent (guchar *dst,
                             gint    r,
                             gint    g,
                             gint    b,
                             gint    a);

void  encode_YCoCg_block    (guchar *dst,
                             guchar *block);


#endif /* __MISC_H__ */
