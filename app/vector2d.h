/* vector2d
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
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

#ifndef __VECTOR2D_H__
#define __VECTOR2D_H__

typedef struct _vector2d
{
  double x, y;
} vector2d;

void   vector2d_set         (vector2d *v, double x, double y);
double vector2d_dot_product (vector2d *v1, vector2d *v2);
double vector2d_magnitude   (vector2d *v);

#endif /* __VECTOR2D_H__ */
