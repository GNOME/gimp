/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapreview.h
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_PREVIEW_H__
#define __LIGMA_PREVIEW_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_PREVIEW            (ligma_preview_get_type ())
#define LIGMA_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PREVIEW, LigmaPreview))
#define LIGMA_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PREVIEW, LigmaPreviewClass))
#define LIGMA_IS_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PREVIEW))
#define LIGMA_IS_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PREVIEW))
#define LIGMA_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PREVIEW, LigmaPreviewClass))


typedef struct _LigmaPreviewPrivate LigmaPreviewPrivate;
typedef struct _LigmaPreviewClass   LigmaPreviewClass;

struct _LigmaPreview
{
  GtkBox              parent_instance;

  LigmaPreviewPrivate *priv;
};

struct _LigmaPreviewClass
{
  GtkBoxClass  parent_class;

  /* virtual methods */
  void   (* draw)        (LigmaPreview     *preview);
  void   (* draw_thumb)  (LigmaPreview     *preview,
                          LigmaPreviewArea *area,
                          gint             width,
                          gint             height);
  void   (* draw_buffer) (LigmaPreview     *preview,
                          const guchar    *buffer,
                          gint             rowstride);
  void   (* set_cursor)  (LigmaPreview     *preview);

  void   (* transform)   (LigmaPreview     *preview,
                          gint             src_x,
                          gint             src_y,
                          gint            *dest_x,
                          gint            *dest_y);
  void   (* untransform) (LigmaPreview     *preview,
                          gint             src_x,
                          gint             src_y,
                          gint            *dest_x,
                          gint            *dest_y);

  /* signal */
  void   (* invalidated) (LigmaPreview     *preview);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType       ligma_preview_get_type           (void) G_GNUC_CONST;

void        ligma_preview_set_update         (LigmaPreview  *preview,
                                             gboolean      update);
gboolean    ligma_preview_get_update         (LigmaPreview  *preview);

void        ligma_preview_set_bounds         (LigmaPreview  *preview,
                                             gint          xmin,
                                             gint          ymin,
                                             gint          xmax,
                                             gint          ymax);
void        ligma_preview_get_bounds         (LigmaPreview  *preview,
                                             gint         *xmin,
                                             gint         *ymin,
                                             gint         *xmax,
                                             gint         *ymax);

void        ligma_preview_set_size           (LigmaPreview  *preview,
                                             gint          width,
                                             gint          height);
void        ligma_preview_get_size           (LigmaPreview  *preview,
                                             gint         *width,
                                             gint         *height);

void        ligma_preview_set_offsets        (LigmaPreview  *preview,
                                             gint          xoff,
                                             gint          yoff);
void        ligma_preview_get_offsets        (LigmaPreview  *preview,
                                             gint         *xoff,
                                             gint         *yoff);

void        ligma_preview_get_position       (LigmaPreview  *preview,
                                             gint         *x,
                                             gint         *y);

void        ligma_preview_transform          (LigmaPreview *preview,
                                             gint         src_x,
                                             gint         src_y,
                                             gint        *dest_x,
                                             gint        *dest_y);
void        ligma_preview_untransform        (LigmaPreview *preview,
                                             gint         src_x,
                                             gint         src_y,
                                             gint        *dest_x,
                                             gint        *dest_y);

GtkWidget * ligma_preview_get_frame          (LigmaPreview  *preview);
GtkWidget * ligma_preview_get_grid           (LigmaPreview  *preview);
GtkWidget * ligma_preview_get_area           (LigmaPreview  *preview);

void        ligma_preview_draw               (LigmaPreview  *preview);
void        ligma_preview_draw_buffer        (LigmaPreview  *preview,
                                             const guchar *buffer,
                                             gint          rowstride);

void        ligma_preview_invalidate         (LigmaPreview  *preview);

void        ligma_preview_set_default_cursor (LigmaPreview  *preview,
                                             GdkCursor    *cursor);
GdkCursor * ligma_preview_get_default_cursor (LigmaPreview  *preview);

GtkWidget * ligma_preview_get_controls       (LigmaPreview  *preview);


G_END_DECLS

#endif /* __LIGMA_PREVIEW_H__ */
