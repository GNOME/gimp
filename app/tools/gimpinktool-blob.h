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

#ifndef __BLOB_H__
#define __BLOB_H__

typedef struct _BlobSpan BlobSpan;
typedef struct _Blob Blob;

struct _BlobSpan {
  int left;
  int right;
};

struct _Blob {
  int y;
  int height;
  BlobSpan data[0];		/* slightly in violation of ANSI-C? */
};


Blob *blob_convex_union (Blob *b1, Blob *b2);
Blob *blob_ellipse (double xc, double yc, double xp, double yp, double xq, double yq);
void blob_bounds(Blob *b, int *x, int *y, int *width, int *height);

#endif __BLOB_H__
