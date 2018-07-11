/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppixmap.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
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

#ifndef GIMP_DISABLE_DEPRECATED

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_PIXMAP_H__
#define __GIMP_PIXMAP_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PIXMAP            (gimp_pixmap_get_type ())
#define GIMP_PIXMAP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PIXMAP, GimpPixmap))
#define GIMP_PIXMAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PIXMAP, GimpPixmapClass))
#define GIMP_IS_PIXMAP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PIXMAP))
#define GIMP_IS_PIXMAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PIXMAP))
#define GIMP_PIXMAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PIXMAP, GimpPixmapClass))


typedef struct _GimpPixmapClass  GimpPixmapClass;

struct _GimpPixmap
{
  GtkImage   parent_instance;

  gchar    **xpm_data;
};

struct _GimpPixmapClass
{
  GtkImageClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_pixmap_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_pixmap_new      (gchar      **xpm_data);

void        gimp_pixmap_set      (GimpPixmap  *pixmap,
                                  gchar      **xpm_data);


G_END_DECLS

#endif /* __GIMP_PIXMAP_H__ */

#endif /* GIMP_DISABLE_DEPRECATED */

