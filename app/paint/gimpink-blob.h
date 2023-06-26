/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpink-blob.h: routines for manipulating scan converted convex polygons.
 * Copyright 1998, Owen Taylor <otaylor@gtk.org>
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
 *
*/

#ifndef __GIMP_INK_BLOB_H__
#define __GIMP_INK_BLOB_H__


typedef struct _GimpBlobPoint GimpBlobPoint;
typedef struct _GimpBlobSpan  GimpBlobSpan;
typedef struct _GimpBlob      GimpBlob;

typedef GimpBlob * (* GimpBlobFunc) (gdouble xc,
                                     gdouble yc,
                                     gdouble xp,
                                     gdouble yp,
                                     gdouble xq,
                                     gdouble yq);

struct _GimpBlobPoint
{
  gint x;
  gint y;
};

struct _GimpBlobSpan
{
  gint left;
  gint right;
};

struct _GimpBlob
{
  gint         y;
  gint         height;
  GimpBlobSpan data[1];
};


GimpBlob * gimp_blob_polygon      (GimpBlobPoint *points,
                                   gint           n_points);
GimpBlob * gimp_blob_square       (gdouble        xc,
                                   gdouble        yc,
                                   gdouble        xp,
                                   gdouble        yp,
                                   gdouble        xq,
                                   gdouble        yq);
GimpBlob * gimp_blob_diamond      (gdouble        xc,
                                   gdouble        yc,
                                   gdouble        xp,
                                   gdouble        yp,
                                   gdouble        xq,
                                   gdouble        yq);
GimpBlob * gimp_blob_ellipse      (gdouble        xc,
                                   gdouble        yc,
                                   gdouble        xp,
                                   gdouble        yp,
                                   gdouble        xq,
                                   gdouble        yq);
void       gimp_blob_bounds       (GimpBlob      *b,
                                   gint          *x,
                                   gint          *y,
                                   gint          *width,
                                   gint          *height);
GimpBlob * gimp_blob_convex_union (GimpBlob      *b1,
                                   GimpBlob      *b2);
GimpBlob * gimp_blob_duplicate    (GimpBlob      *b);
void       gimp_blob_move         (GimpBlob      *b,
                                   gint           x,
                                   gint           y);


#endif /* __GIMP_INK_BLOB_H__ */
