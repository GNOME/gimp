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


typedef struct _GimpBrushPixmap GimpBrushPixmap;
typedef struct _GimpBrushPipe GimpBrushPipe;


#define GIMP_TYPE_BRUSH_PIXMAP    (gimp_brush_pixmap_get_type ())
#define GIMP_BRUSH_PIXMAP(obj)    (GTK_CHECK_CAST ((obj), GIMP_TYPE_BRUSH_PIXMAP, GimpBrushPixmap))
#define GIMP_IS_BRUSH_PIXMAP(obj) (GTK_CHECK_TYPE ((obj), GIMP_TYPE_BRUSH_PIXMAP))

#define GIMP_TYPE_BRUSH_PIPE    (gimp_brush_pipe_get_type ())
#define GIMP_BRUSH_PIPE(obj)    (GTK_CHECK_CAST ((obj), GIMP_TYPE_BRUSH_PIPE, GimpBrushPipe))
#define GIMP_IS_BRUSH_PIPE(obj) (GTK_CHECK_TYPE ((obj), GIMP_TYPE_BRUSH_PIPE))


GtkType         gimp_brush_pixmap_get_type (void);
GtkType         gimp_brush_pipe_get_type   (void);

GimpBrushPipe * gimp_brush_pipe_load       (gchar *filename);
GimpBrushPipe * gimp_brush_pixmap_load     (gchar *filename);

TempBuf       * gimp_brush_pixmap_pixmap   (GimpBrushPixmap *brush);


/* appearantly GIMP_IS_BRUSH_PIPE () returning TRUE is no indication
 * that you really have a brush_pipe in front of you, so here we introduce
 * a macro that works:
 */
#define GIMP_IS_REALLY_A_BRUSH_PIPE(obj) (GIMP_IS_BRUSH_PIPE (obj) && GIMP_BRUSH_PIPE (obj)->nbrushes > 1)


#endif  /* __GIMP_BRUSH_PIPE_H__ */
