/* LIBGIMP - The GIMP Library
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_RESOURCE_H__
#define __GIMP_RESOURCE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_RESOURCE (gimp_resource_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpResource, gimp_resource, GIMP, RESOURCE, GObject)


struct _GimpResourceClass
{
  GObjectClass parent_class;

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

gint32         gimp_resource_get_id      (GimpResource *resource);
GimpResource * gimp_resource_get_by_id   (gint32        resource_id);
GimpResource * gimp_resource_get_by_name (GType         resource_type,
                                          const gchar  *resource_name);

gboolean       gimp_resource_is_valid    (GimpResource *resource);
gboolean       gimp_resource_is_brush    (GimpResource *resource);
gboolean       gimp_resource_is_pattern  (GimpResource *resource);
gboolean       gimp_resource_is_gradient (GimpResource *resource);
gboolean       gimp_resource_is_palette  (GimpResource *resource);
gboolean       gimp_resource_is_font     (GimpResource *resource);


G_END_DECLS

#endif /* __GIMP_RESOURCE_H__ */
