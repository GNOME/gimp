/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpchannel.h
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_CHANNEL_H__
#define __GIMP_CHANNEL_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

#include <libgimp/gimpdrawable.h>


#define GIMP_TYPE_CHANNEL (gimp_channel_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpChannel, gimp_channel, GIMP, CHANNEL, GimpDrawable)


struct _GimpChannelClass
{
  GimpDrawableClass parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
  void (*_gimp_reserved8) (void);
};


GimpChannel * gimp_channel_get_by_id (gint32       channel_id);

GimpChannel * gimp_channel_new       (GimpImage   *image,
                                      const gchar *name,
                                      guint        width,
                                      guint        height,
                                      gdouble      opacity,
                                      GeglColor   *color);


G_END_DECLS

#endif /* __GIMP_CHANNEL_H__ */
