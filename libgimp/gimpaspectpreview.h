/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpaspectpreview.h
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

#ifndef __GIMP_ASPECT_PREVIEW_H__
#define __GIMP_ASPECT_PREVIEW_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_ASPECT_PREVIEW            (gimp_aspect_preview_get_type ())
#define GIMP_ASPECT_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ASPECT_PREVIEW, GimpAspectPreview))
#define GIMP_ASPECT_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ASPECT_PREVIEW, GimpAspectPreviewClass))
#define GIMP_IS_ASPECT_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ASPECT_PREVIEW))
#define GIMP_IS_ASPECT_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ASPECT_PREVIEW))
#define GIMP_ASPECT_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ASPECT_PREVIEW, GimpAspectPreviewClass))


typedef struct _GimpAspectPreviewClass  GimpAspectPreviewClass;

struct _GimpAspectPreview
{
  GimpPreview   parent_instance;

  /*< private >*/
  GimpDrawable *drawable;
};

struct _GimpAspectPreviewClass
{
  GimpPreviewClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_aspect_preview_get_type             (void) G_GNUC_CONST;

GtkWidget * gimp_aspect_preview_new_from_drawable_id (gint32        drawable_ID);

GIMP_DEPRECATED_FOR(gimp_aspect_preview_new_from_drawable_id)
GtkWidget * gimp_aspect_preview_new                  (GimpDrawable *drawable,
                                                      gboolean     *toggle);


G_END_DECLS

#endif /* __GIMP_ASPECT_PREVIEW_H__ */
