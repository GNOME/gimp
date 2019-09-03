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


#define GIMP_TYPE_CHANNEL            (gimp_channel_get_type ())
#define GIMP_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CHANNEL, GimpChannel))
#define GIMP_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CHANNEL, GimpChannelClass))
#define GIMP_IS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CHANNEL))
#define GIMP_IS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CHANNEL))
#define GIMP_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CHANNEL, GimpChannelClass))


typedef struct _GimpChannelClass   GimpChannelClass;
typedef struct _GimpChannelPrivate GimpChannelPrivate;

struct _GimpChannel
{
  GimpDrawable        parent_instance;

  GimpChannelPrivate *priv;
};

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


GType         gimp_channel_get_type  (void) G_GNUC_CONST;

GimpChannel * gimp_channel_get_by_id (gint32         channel_id);

GimpChannel * gimp_channel_new       (GimpImage     *image,
                                      const gchar   *name,
                                      guint          width,
                                      guint          height,
                                      gdouble        opacity,
                                      const GimpRGB *color);


G_END_DECLS

#endif /* __GIMP_CHANNEL_H__ */
