/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppreview.h
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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_PREVIEW_H__
#define __GIMP_PREVIEW_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PREVIEW            (gimp_preview_get_type ())
#define GIMP_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PREVIEW, GimpPreview))
#define GIMP_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PREVIEW, GimpPreviewClass))
#define GIMP_IS_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PREVIEW))
#define GIMP_IS_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PREVIEW))
#define GIMP_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PREVIEW, GimpPreviewClass))


typedef struct _GimpPreviewPrivate GimpPreviewPrivate;
typedef struct _GimpPreviewClass   GimpPreviewClass;

struct _GimpPreview
{
  GtkBox              parent_instance;

  GimpPreviewPrivate *priv;
};

struct _GimpPreviewClass
{
  GtkBoxClass  parent_class;

  /* virtual methods */
  void   (* draw)        (GimpPreview     *preview);
  void   (* draw_thumb)  (GimpPreview     *preview,
                          GimpPreviewArea *area,
                          gint             width,
                          gint             height);
  void   (* draw_buffer) (GimpPreview     *preview,
                          const guchar    *buffer,
                          gint             rowstride);
  void   (* set_cursor)  (GimpPreview     *preview);

  void   (* transform)   (GimpPreview     *preview,
                          gint             src_x,
                          gint             src_y,
                          gint            *dest_x,
                          gint            *dest_y);
  void   (* untransform) (GimpPreview     *preview,
                          gint             src_x,
                          gint             src_y,
                          gint            *dest_x,
                          gint            *dest_y);

  /* signal */
  void   (* invalidated) (GimpPreview     *preview);

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


GType       gimp_preview_get_type           (void) G_GNUC_CONST;

void        gimp_preview_set_update         (GimpPreview  *preview,
                                             gboolean      update);
gboolean    gimp_preview_get_update         (GimpPreview  *preview);

void        gimp_preview_set_bounds         (GimpPreview  *preview,
                                             gint          xmin,
                                             gint          ymin,
                                             gint          xmax,
                                             gint          ymax);
void        gimp_preview_get_bounds         (GimpPreview  *preview,
                                             gint         *xmin,
                                             gint         *ymin,
                                             gint         *xmax,
                                             gint         *ymax);

void        gimp_preview_set_size           (GimpPreview  *preview,
                                             gint          width,
                                             gint          height);
void        gimp_preview_get_size           (GimpPreview  *preview,
                                             gint         *width,
                                             gint         *height);

void        gimp_preview_set_offsets        (GimpPreview  *preview,
                                             gint          xoff,
                                             gint          yoff);
void        gimp_preview_get_offsets        (GimpPreview  *preview,
                                             gint         *xoff,
                                             gint         *yoff);

void        gimp_preview_get_position       (GimpPreview  *preview,
                                             gint         *x,
                                             gint         *y);

void        gimp_preview_transform          (GimpPreview *preview,
                                             gint         src_x,
                                             gint         src_y,
                                             gint        *dest_x,
                                             gint        *dest_y);
void        gimp_preview_untransform        (GimpPreview *preview,
                                             gint         src_x,
                                             gint         src_y,
                                             gint        *dest_x,
                                             gint        *dest_y);

GtkWidget * gimp_preview_get_frame          (GimpPreview  *preview);
GtkWidget * gimp_preview_get_table          (GimpPreview  *preview);
GtkWidget * gimp_preview_get_area           (GimpPreview  *preview);

void        gimp_preview_draw               (GimpPreview  *preview);
void        gimp_preview_draw_buffer        (GimpPreview  *preview,
                                             const guchar *buffer,
                                             gint          rowstride);

void        gimp_preview_invalidate         (GimpPreview  *preview);

void        gimp_preview_set_default_cursor (GimpPreview  *preview,
                                             GdkCursor    *cursor);
GdkCursor * gimp_preview_get_default_cursor (GimpPreview  *preview);

GtkWidget * gimp_preview_get_controls       (GimpPreview  *preview);


G_END_DECLS

#endif /* __GIMP_PREVIEW_H__ */
