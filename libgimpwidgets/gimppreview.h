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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_PREVIEW_H__
#define __GIMP_PREVIEW_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PREVIEW (gimp_preview_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpPreview, gimp_preview, GIMP, PREVIEW, GtkBox)

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
GtkWidget * gimp_preview_get_grid           (GimpPreview  *preview);
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
