/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmaink-blob.h: routines for manipulating scan converted convex polygons.
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

#ifndef __LIGMA_INK_BLOB_H__
#define __LIGMA_INK_BLOB_H__


typedef struct _LigmaBlobPoint LigmaBlobPoint;
typedef struct _LigmaBlobSpan  LigmaBlobSpan;
typedef struct _LigmaBlob      LigmaBlob;

typedef LigmaBlob * (* LigmaBlobFunc) (gdouble xc,
                                     gdouble yc,
                                     gdouble xp,
                                     gdouble yp,
                                     gdouble xq,
                                     gdouble yq);

struct _LigmaBlobPoint
{
  gint x;
  gint y;
};

struct _LigmaBlobSpan
{
  gint left;
  gint right;
};

struct _LigmaBlob
{
  gint         y;
  gint         height;
  LigmaBlobSpan data[1];
};


LigmaBlob * ligma_blob_polygon      (LigmaBlobPoint *points,
                                   gint           n_points);
LigmaBlob * ligma_blob_square       (gdouble        xc,
                                   gdouble        yc,
                                   gdouble        xp,
                                   gdouble        yp,
                                   gdouble        xq,
                                   gdouble        yq);
LigmaBlob * ligma_blob_diamond      (gdouble        xc,
                                   gdouble        yc,
                                   gdouble        xp,
                                   gdouble        yp,
                                   gdouble        xq,
                                   gdouble        yq);
LigmaBlob * ligma_blob_ellipse      (gdouble        xc,
                                   gdouble        yc,
                                   gdouble        xp,
                                   gdouble        yp,
                                   gdouble        xq,
                                   gdouble        yq);
void       ligma_blob_bounds       (LigmaBlob      *b,
                                   gint          *x,
                                   gint          *y,
                                   gint          *width,
                                   gint          *height);
LigmaBlob * ligma_blob_convex_union (LigmaBlob      *b1,
                                   LigmaBlob      *b2);
LigmaBlob * ligma_blob_duplicate    (LigmaBlob      *b);


#endif /* __LIGMA_INK_BLOB_H__ */
