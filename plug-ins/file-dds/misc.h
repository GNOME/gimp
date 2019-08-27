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

void decode_ycocg_image        (GimpDrawable *drawable,
                                gboolean      shadow);
void decode_ycocg_scaled_image (GimpDrawable *drawable,
                                gboolean      shadow);
void decode_alpha_exp_image    (GimpDrawable *drawable,
                                gboolean      shadow);

#endif /* __MISC_H__ */
