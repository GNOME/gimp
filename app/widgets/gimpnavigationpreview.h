/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpNavigationPreview Widget
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

#ifndef __GIMP_NAVIGATION_PREVIEW_H__
#define __GIMP_NAVIGATION_PREVIEW_H__


#include "gimpimagepreview.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_NAVIGATION_PREVIEW            (gimp_navigation_preview_get_type ())
#define GIMP_NAVIGATION_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_NAVIGATION_PREVIEW, GimpNavigationPreview))
#define GIMP_NAVIGATION_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_NAVIGATION_PREVIEW, GimpNavigationPreviewClass))
#define GIMP_IS_NAVIGATION_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_NAVIGATION_PREVIEW))
#define GIMP_IS_NAVIGATION_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_NAVIGATION_PREVIEW))


typedef struct _GimpNavigationPreviewClass  GimpNavigationPreviewClass;

struct _GimpNavigationPreview
{
  GimpImagePreview  parent_instance;

  /*  values in image coordinates  */
  gint      x;
  gint      y;
  gint      width;
  gint      height;

  /*  values in preview coordinates  */
  gint      p_x;
  gint      p_y;
  gint      p_width;
  gint      p_height;

  gint      motion_offset_x;
  gint      motion_offset_y;
  gboolean  has_grab;

  GdkGC    *gc;
};

struct _GimpNavigationPreviewClass
{
  GimpPreviewClass  parent_class;

  void (* marker_changed) (GimpNavigationPreview *preview,
			   gint                   x,
			   gint                   y);
};


GtkType     gimp_navigation_preview_get_type   (void);

GtkWidget * gimp_navigation_preview_new        (GimpImage             *gimage,
						gint                   size);
void        gimp_navigation_preview_set_marker (GimpNavigationPreview *preview,
						gint                   x,
						gint                   y,
						gint                   width,
						gint                   height);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_NAVIGATION_PREVIEW_H__ */
