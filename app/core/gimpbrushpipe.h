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

#ifndef __GIMP_BRUSH_PIPE_H__
#define __GIMP_BRUSH_PIPE_H__


#include "tools.h"
#include "paint_core.h"
#include "gimpbrush.h"
#include "temp_buf.h"


typedef struct _GimpBrushPipe GimpBrushPipe;


#define GIMP_TYPE_BRUSH_PIPE    (gimp_brush_pipe_get_type ())
#define GIMP_BRUSH_PIPE(obj)    (GTK_CHECK_CAST ((obj), GIMP_TYPE_BRUSH_PIPE, GimpBrushPipe))
#define GIMP_IS_BRUSH_PIPE(obj) (GTK_CHECK_TYPE ((obj), GIMP_TYPE_BRUSH_PIPE))


typedef enum
{
  PIPE_SELECT_CONSTANT,
  PIPE_SELECT_INCREMENTAL,
  PIPE_SELECT_ANGULAR,
  PIPE_SELECT_VELOCITY,
  PIPE_SELECT_RANDOM,
  PIPE_SELECT_PRESSURE,
  PIPE_SELECT_TILT_X,
  PIPE_SELECT_TILT_Y
} PipeSelectModes;


struct _GimpBrushPipe
{
  GimpBrush         gbrush;     /* Also itself a brush */

  gint              dimension;
  gint             *rank;	/* Size in each dimension */
  gint             *stride;	/* Aux for indexing */
  PipeSelectModes  *select;	/* One mode per dimension */

  gint             *index;	/* Current index for incremental dimensions */

  gint              nbrushes;	/* Might be less than the product of the
				 * ranks in some odd special case */
  GimpBrush       **brushes;
  GimpBrush        *current;    /* Currently selected brush */
};

typedef struct _GimpBrushPipeClass GimpBrushPipeClass;

struct _GimpBrushPipeClass
{
  GimpBrushClass  parent_class;
};


GtkType     gimp_brush_pixmap_get_type (void);
GtkType     gimp_brush_pipe_get_type   (void);

GimpBrush * gimp_brush_pipe_load       (gchar *filename);


#endif  /* __GIMP_BRUSH_PIPE_H__ */



