/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Adrian Likins and Tor Lillqvist
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

#ifndef __GIMPBRUSHPIPEP_H__
#define __GIMPBRUSHPIPEP_H__

/* A GimpBrushPixmap always exists as part in one and only one GimpBrushPipe
 * It contains a back-pointer to the GimpBrushPipe so that we can select
 * the next brush in the pipe with just a reference to the GimpBrushPipe.
 */

struct _GimpBrushPixmap
{
  GimpBrush gbrush;
  TempBuf *pixmap_mask;
  GimpBrushPipe *pipe;
};

struct _GimpBrushPipe
{
  GimpBrushPixmap pixmap;	/* Also itself a pixmap brush */
  GimpBrushPixmap *current;	/* Currently selected brush */
  int dimension;
  int *rank;			/* Size in each dimension */
  int nbrushes;			/* Might be less than the product of the
				 * ranks in some special case */
  GimpBrushPixmap **brushes;
  PipeSelectModes *select;	/* One mode per dimension */
  int *index;			/* Current index for incremental dimensions */
};

typedef struct _GimpBrushPixmapClass
{
  GimpBrushClass parent_class;
} GimpBrushPixmapClass;

typedef struct _GimpBrushPipeClass
{
  GimpBrushPixmapClass parent_class;
} GimpBrushPipeClass;

#endif  /* __GIMPBRUSHPIPEP_H__ */
