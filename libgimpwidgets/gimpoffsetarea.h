/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaoffsetarea.h
 * Copyright (C) 2001  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_OFFSET_AREA_H__
#define __LIGMA_OFFSET_AREA_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */

#define LIGMA_TYPE_OFFSET_AREA            (ligma_offset_area_get_type ())
#define LIGMA_OFFSET_AREA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OFFSET_AREA, LigmaOffsetArea))
#define LIGMA_OFFSET_AREA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_OFFSET_AREA, LigmaOffsetAreaClass))
#define LIGMA_IS_OFFSET_AREA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OFFSET_AREA))
#define LIGMA_IS_OFFSET_AREA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_OFFSET_AREA))
#define LIGMA_OFFSET_AREA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_OFFSET_AREA, LigmaOffsetAreaClass))


typedef struct _LigmaOffsetAreaPrivate LigmaOffsetAreaPrivate;
typedef struct _LigmaOffsetAreaClass   LigmaOffsetAreaClass;

struct _LigmaOffsetArea
{
  GtkDrawingArea         parent_instance;

  LigmaOffsetAreaPrivate *priv;
};

struct _LigmaOffsetAreaClass
{
  GtkDrawingAreaClass  parent_class;

  void (* offsets_changed) (LigmaOffsetArea *offset_area,
                            gint            offset_x,
                            gint            offset_y);

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


GType       ligma_offset_area_get_type    (void) G_GNUC_CONST;

GtkWidget * ligma_offset_area_new         (gint            orig_width,
                                          gint            orig_height);
void        ligma_offset_area_set_pixbuf  (LigmaOffsetArea *offset_area,
                                          GdkPixbuf      *pixbuf);

void        ligma_offset_area_set_size    (LigmaOffsetArea *offset_area,
                                          gint            width,
                                          gint            height);
void        ligma_offset_area_set_offsets (LigmaOffsetArea *offset_area,
                                          gint            offset_x,
                                          gint            offset_y);


G_END_DECLS

#endif /* __LIGMA_OFFSET_AREA_H__ */
