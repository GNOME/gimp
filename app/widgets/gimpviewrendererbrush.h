/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushpreview.h
 * Copyright (C) 2001 Michael Natterer
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

#ifndef __GIMP_BRUSH_PREVIEW_H__
#define __GIMP_BRUSH_PREVIEW_H__


#include "gimppreview.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_BRUSH_PREVIEW            (gimp_brush_preview_get_type ())
#define GIMP_BRUSH_PREVIEW(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_BRUSH_PREVIEW, GimpBrushPreview))
#define GIMP_BRUSH_PREVIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH_PREVIEW, GimpBrushPreviewClass))
#define GIMP_IS_BRUSH_PREVIEW(obj)         (GTK_CHECK_TYPE (obj, GIMP_TYPE_BRUSH_PREVIEW))
#define GIMP_IS_BRUSH_PREVIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH_PREVIEW))


typedef struct _GimpBrushPreviewClass  GimpBrushPreviewClass;

struct _GimpBrushPreview
{
  GimpPreview  parent_instance;

  guint        pipe_timeout_id;
  gint         pipe_animation_index;
};

struct _GimpBrushPreviewClass
{
  GimpPreviewClass  parent_class;
};


GtkType   gimp_brush_preview_get_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_BRUSH_PREVIEW_H__ */
