/* blob.h: routines for manipulating scan converted convex
 *         polygons.
 *
 * Copyright 1998, Owen Taylor <otaylor@gtk.org>
 *
 * > Please contact the above author before modifying the copy <
 * > of this file in the GIMP distribution. Thanks.            <
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
 *
*/

#ifndef __GIMP_INK_BLOB_H__
#define __GIMP_INK_BLOB_H__


typedef struct _BlobPoint BlobPoint;
typedef struct _BlobSpan  BlobSpan;
typedef struct _Blob      Blob;

typedef Blob * (* BlobFunc) (gdouble xc,
                             gdouble yc,
                             gdouble xp,
                             gdouble yp,
                             gdouble xq,
                             gdouble yq);

struct _BlobPoint
{
  gint x;
  gint y;
};

struct _BlobSpan
{
  gint left;
  gint right;
};

struct _Blob
{
  gint     y;
  gint     height;
  BlobSpan data[1];
};


Blob * blob_polygon      (BlobPoint *points,
                          gint       npoints);
Blob * blob_square       (gdouble    xc,
                          gdouble    yc,
                          gdouble    xp,
                          gdouble    yp,
                          gdouble    xq,
                          gdouble    yq);
Blob * blob_diamond      (gdouble    xc,
                          gdouble    yc,
                          gdouble    xp,
                          gdouble    yp,
                          gdouble    xq,
                          gdouble    yq);
Blob * blob_ellipse      (gdouble    xc,
                          gdouble    yc,
                          gdouble    xp,
                          gdouble    yp,
                          gdouble    xq,
                          gdouble    yq);
void   blob_bounds       (Blob      *b,
                          gint      *x,
                          gint      *y,
                          gint      *width,
                          gint      *height);
Blob * blob_convex_union (Blob      *b1,
                          Blob      *b2);
Blob * blob_duplicate    (Blob      *b);


#endif /* __GIMP_INK_BLOB_H__ */
