/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_BRUSH_H__
#define __GIMP_BRUSH_H__


#include "gimpdata.h"

/* FIXME: remove the GimpPaintTool dependency  */
#include "tools/tools-types.h"


#define GIMP_BRUSH_FILE_EXTENSION        ".gbr"
#define GIMP_BRUSH_PIXMAP_FILE_EXTENSION ".gpb"


#define GIMP_TYPE_BRUSH            (gimp_brush_get_type ())
#define GIMP_BRUSH(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_BRUSH, GimpBrush))
#define GIMP_BRUSH_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH, GimpBrushClass))
#define GIMP_IS_BRUSH(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_BRUSH))
#define GIMP_IS_BRUSH_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH))


typedef struct _GimpBrushClass GimpBrushClass;

struct _GimpBrush
{
  GimpData      parent_instance;

  TempBuf      *mask;       /*  the actual mask                */
  TempBuf      *pixmap;     /*  optional pixmap data           */

  gint          spacing;    /*  brush's spacing                */
  GimpVector2   x_axis;     /*  for calculating brush spacing  */
  GimpVector2   y_axis;     /*  for calculating brush spacing  */
};

struct _GimpBrushClass
{
  GimpDataClass parent_class;

  /* FIXME: these are no virtual function pointers but bad hacks: */
  GimpBrush * (* select_brush)     (GimpPaintTool *paint_tool);
  gboolean    (* want_null_motion) (GimpPaintTool *paint_tool);
};


GtkType     gimp_brush_get_type     (void);

GimpData  * gimp_brush_new          (const gchar     *name);
GimpData  * gimp_brush_get_standard (void);
GimpData  * gimp_brush_load         (const gchar     *filename);

GimpBrush * gimp_brush_load_brush   (gint             fd,
				     const gchar     *filename);

TempBuf   * gimp_brush_get_mask     (const GimpBrush *brush);
TempBuf   * gimp_brush_get_pixmap   (const GimpBrush *brush);

gint        gimp_brush_get_spacing  (const GimpBrush *brush);
void        gimp_brush_set_spacing  (GimpBrush       *brush,
				     gint             spacing);


#endif /* __GIMP_BRUSH_H__ */
