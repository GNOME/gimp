/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpframe.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_FRAME_H__
#define __GIMP_FRAME_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_FRAME            (gimp_frame_get_type ())
#define GIMP_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FRAME, GimpFrame))
#define GIMP_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FRAME, GimpFrameClass))
#define GIMP_IS_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FRAME))
#define GIMP_IS_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FRAME))
#define GIMP_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FRAME, GimpFrameClass))


typedef struct _GimpFramePrivate GimpFramePrivate;
typedef struct _GimpFrameClass   GimpFrameClass;

struct _GimpFrame
{
  GtkFrame          parent_instance;

  GimpFramePrivate *priv;
};

struct _GimpFrameClass
{
  GtkFrameClass  parent_class;

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


GType       gimp_frame_get_type  (void) G_GNUC_CONST;
GtkWidget * gimp_frame_new       (const gchar *label);


G_END_DECLS

#endif /* __GIMP_FRAME_H__ */
