/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmadrawable.h
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

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_DRAWABLE_H__
#define __LIGMA_DRAWABLE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#include <libligma/ligmaitem.h>


#define LIGMA_TYPE_DRAWABLE (ligma_drawable_get_type ())
G_DECLARE_DERIVABLE_TYPE (LigmaDrawable, ligma_drawable, LIGMA, DRAWABLE, LigmaItem)


struct _LigmaDrawableClass
{
  LigmaItemClass parent_class;

  /* Padding for future expansion */
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
  void (*_ligma_reserved5) (void);
  void (*_ligma_reserved6) (void);
  void (*_ligma_reserved7) (void);
  void (*_ligma_reserved8) (void);
  void (*_ligma_reserved9) (void);
};


LigmaDrawable * ligma_drawable_get_by_id              (gint32        drawable_id);

GeglBuffer   * ligma_drawable_get_buffer             (LigmaDrawable  *drawable);
GeglBuffer   * ligma_drawable_get_shadow_buffer      (LigmaDrawable  *drawable);

const Babl   * ligma_drawable_get_format             (LigmaDrawable  *drawable);
const Babl   * ligma_drawable_get_thumbnail_format   (LigmaDrawable  *drawable);

guchar       * ligma_drawable_get_thumbnail_data     (LigmaDrawable  *drawable,
                                                     gint          *width,
                                                     gint          *height,
                                                     gint          *bpp);
GdkPixbuf    * ligma_drawable_get_thumbnail          (LigmaDrawable  *drawable,
                                                     gint           width,
                                                     gint           height,
                                                     LigmaPixbufTransparency alpha);

guchar       * ligma_drawable_get_sub_thumbnail_data (LigmaDrawable  *drawable,
                                                     gint           src_x,
                                                     gint           src_y,
                                                     gint           src_width,
                                                     gint           src_height,
                                                     gint          *dest_width,
                                                     gint          *dest_height,
                                                     gint          *bpp);
GdkPixbuf    * ligma_drawable_get_sub_thumbnail      (LigmaDrawable  *drawable,
                                                     gint           src_x,
                                                     gint           src_y,
                                                     gint           src_width,
                                                     gint           src_height,
                                                     gint           dest_width,
                                                     gint           dest_height,
                                                     LigmaPixbufTransparency alpha);


G_END_DECLS

#endif /* __LIGMA_DRAWABLE_H__ */
