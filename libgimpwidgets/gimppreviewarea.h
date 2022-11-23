/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#ifndef __LIGMA_PREVIEW_AREA_H__
#define __LIGMA_PREVIEW_AREA_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_PREVIEW_AREA            (ligma_preview_area_get_type ())
#define LIGMA_PREVIEW_AREA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PREVIEW_AREA, LigmaPreviewArea))
#define LIGMA_PREVIEW_AREA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PREVIEW_AREA, LigmaPreviewAreaClass))
#define LIGMA_IS_PREVIEW_AREA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PREVIEW_AREA))
#define LIGMA_IS_PREVIEW_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PREVIEW_AREA))
#define LIGMA_PREVIEW_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PREVIEW_AREA, LigmaPreviewArea))


typedef struct _LigmaPreviewAreaPrivate LigmaPreviewAreaPrivate;
typedef struct _LigmaPreviewAreaClass   LigmaPreviewAreaClass;

struct _LigmaPreviewArea
{
  GtkDrawingArea          parent_instance;

  LigmaPreviewAreaPrivate *priv;
};

struct _LigmaPreviewAreaClass
{
  GtkDrawingAreaClass  parent_class;

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


GType       ligma_preview_area_get_type         (void) G_GNUC_CONST;

GtkWidget * ligma_preview_area_new              (void);

void        ligma_preview_area_draw             (LigmaPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                LigmaImageType    type,
                                                const guchar    *buf,
                                                gint             rowstride);
void        ligma_preview_area_blend            (LigmaPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                LigmaImageType    type,
                                                const guchar    *buf1,
                                                gint             rowstride1,
                                                const guchar    *buf2,
                                                gint             rowstride2,
                                                guchar           opacity);
void        ligma_preview_area_mask             (LigmaPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                LigmaImageType    type,
                                                const guchar    *buf1,
                                                gint             rowstride1,
                                                const guchar    *buf2,
                                                gint             rowstride2,
                                                const guchar    *mask,
                                                gint             rowstride_mask);
void        ligma_preview_area_fill             (LigmaPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                guchar           red,
                                                guchar           green,
                                                guchar           blue);

void        ligma_preview_area_set_offsets      (LigmaPreviewArea *area,
                                                gint             x,
                                                gint             y);

void        ligma_preview_area_set_colormap     (LigmaPreviewArea *area,
                                                const guchar    *colormap,
                                                gint             num_colors);

void        ligma_preview_area_set_color_config (LigmaPreviewArea *area,
                                                LigmaColorConfig *config);

void        ligma_preview_area_get_size         (LigmaPreviewArea *area,
                                                gint            *width,
                                                gint            *height);
void        ligma_preview_area_set_max_size     (LigmaPreviewArea *area,
                                                gint             width,
                                                gint             height);

void        ligma_preview_area_menu_popup       (LigmaPreviewArea *area,
                                                GdkEventButton  *event);


G_END_DECLS

#endif /* __LIGMA_PREVIEW_AREA_H__ */
