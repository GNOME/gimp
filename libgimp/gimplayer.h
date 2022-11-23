/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmalayer.h
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

#ifndef __LIGMA_LAYER_H__
#define __LIGMA_LAYER_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_LAYER (ligma_layer_get_type ())
G_DECLARE_DERIVABLE_TYPE (LigmaLayer, ligma_layer, LIGMA, LAYER, LigmaDrawable)

struct _LigmaLayerClass
{
  LigmaDrawableClass parent_class;

  /*  virtual functions  */
  LigmaLayer * (* copy) (LigmaLayer *layer);

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

LigmaLayer * ligma_layer_get_by_id          (gint32           layer_id);

LigmaLayer * ligma_layer_new                (LigmaImage       *image,
                                           const gchar     *name,
                                           gint             width,
                                           gint             height,
                                           LigmaImageType    type,
                                           gdouble          opacity,
                                           LigmaLayerMode    mode);

LigmaLayer * ligma_layer_new_from_pixbuf    (LigmaImage       *image,
                                           const gchar     *name,
                                           GdkPixbuf       *pixbuf,
                                           gdouble          opacity,
                                           LigmaLayerMode    mode,
                                           gdouble          progress_start,
                                           gdouble          progress_end);
LigmaLayer * ligma_layer_new_from_surface   (LigmaImage       *image,
                                           const gchar     *name,
                                           cairo_surface_t *surface,
                                           gdouble          progress_start,
                                           gdouble          progress_end);

LigmaLayer * ligma_layer_copy               (LigmaLayer       *layer);


G_END_DECLS

#endif /* __LIGMA_LAYER_H__ */
