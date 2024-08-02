/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpzoompreview.h
 * Copyright (C) 2005  David Odin <dindinx@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_ZOOM_PREVIEW_H__
#define __GIMP_ZOOM_PREVIEW_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_ZOOM_PREVIEW (gimp_zoom_preview_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpZoomPreview, gimp_zoom_preview, GIMP, ZOOM_PREVIEW, GimpScrolledPreview)

struct _GimpZoomPreviewClass
{
  GimpScrolledPreviewClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GtkWidget     * gimp_zoom_preview_new_from_drawable (GimpDrawable *drawable);
GtkWidget     * gimp_zoom_preview_new_with_model_from_drawable
                                                 (GimpDrawable    *drawable,
                                                  GimpZoomModel   *model);

guchar        * gimp_zoom_preview_get_source     (GimpZoomPreview *preview,
                                                  gint            *width,
                                                  gint            *height,
                                                  gint            *bpp);

GimpDrawable  * gimp_zoom_preview_get_drawable   (GimpZoomPreview *preview);
GimpZoomModel * gimp_zoom_preview_get_model      (GimpZoomPreview *preview);
gdouble         gimp_zoom_preview_get_factor     (GimpZoomPreview *preview);


G_END_DECLS

#endif /* __GIMP_ZOOM_PREVIEW_H__ */
