/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawablepreview.h
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

#ifndef __GIMP_DRAWABLE_PREVIEW_H__
#define __GIMP_DRAWABLE_PREVIEW_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_DRAWABLE_PREVIEW            (gimp_drawable_preview_get_type ())
#define GIMP_DRAWABLE_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAWABLE_PREVIEW, GimpDrawablePreview))
#define GIMP_DRAWABLE_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAWABLE_PREVIEW, GimpDrawablePreviewClass))
#define GIMP_IS_DRAWABLE_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAWABLE_PREVIEW))
#define GIMP_IS_DRAWABLE_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAWABLE_PREVIEW))
#define GIMP_DRAWABLE_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DRAWABLE_PREVIEW, GimpDrawablePreviewClass))


typedef struct _GimpDrawablePreviewClass  GimpDrawablePreviewClass;

struct _GimpDrawablePreview
{
  GimpScrolledPreview  parent_instance;

  GimpDrawable        *drawable;
};

struct _GimpDrawablePreviewClass
{
  GimpScrolledPreviewClass parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType          gimp_drawable_preview_get_type             (void) G_GNUC_CONST;

GtkWidget    * gimp_drawable_preview_new_from_drawable_id (gint32               drawable_ID);
gint32         gimp_drawable_preview_get_drawable_id      (GimpDrawablePreview *preview);

GIMP_DEPRECATED_FOR(gimp_drawable_preview_new_from_drawable_id)
GtkWidget    * gimp_drawable_preview_new                  (GimpDrawable        *drawable,
                                                           gboolean            *toggle);
GIMP_DEPRECATED_FOR(gimp_drawable_preview_get_drawable_id)
GimpDrawable * gimp_drawable_preview_get_drawable         (GimpDrawablePreview *preview);

GIMP_DEPRECATED_FOR(gimp_preview_draw_buffer)
void           gimp_drawable_preview_draw_region          (GimpDrawablePreview *preview,
                                                           const GimpPixelRgn  *region);

/*  for internal use only  */
G_GNUC_INTERNAL void      _gimp_drawable_preview_area_draw_thumb (GimpPreviewArea *area,
                                                                  gint32           drawable_ID,
                                                                  gint             width,
                                                                  gint             height);
G_GNUC_INTERNAL gboolean  _gimp_drawable_preview_get_bounds      (gint32           drawable_ID,
                                                                  gint            *xmin,
                                                                  gint            *ymin,
                                                                  gint            *xmax,
                                                                  gint            *ymax);


G_END_DECLS

#endif /* __GIMP_DRAWABLE_PREVIEW_H__ */

