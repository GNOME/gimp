/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmachannel.h
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

#ifndef __LIGMA_CHANNEL_H__
#define __LIGMA_CHANNEL_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

#include <libligma/ligmadrawable.h>


#define LIGMA_TYPE_CHANNEL (ligma_channel_get_type ())
G_DECLARE_DERIVABLE_TYPE (LigmaChannel, ligma_channel, LIGMA, CHANNEL, LigmaDrawable)


struct _LigmaChannelClass
{
  LigmaDrawableClass parent_class;

  /* Padding for future expansion */
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
  void (*_ligma_reserved5) (void);
  void (*_ligma_reserved6) (void);
  void (*_ligma_reserved7) (void);
  void (*_ligma_reserved8) (void);
};


LigmaChannel * ligma_channel_get_by_id (gint32         channel_id);

LigmaChannel * ligma_channel_new       (LigmaImage     *image,
                                      const gchar   *name,
                                      guint          width,
                                      guint          height,
                                      gdouble        opacity,
                                      const LigmaRGB *color);


G_END_DECLS

#endif /* __LIGMA_CHANNEL_H__ */
