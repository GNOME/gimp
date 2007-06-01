/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GimpColorManaged interface
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_COLOR_MANAGED_H__
#define __GIMP_COLOR_MANAGED_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_COLOR_MANAGED               (gimp_color_managed_interface_get_type ())
#define GIMP_IS_COLOR_MANAGED(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_MANAGED))
#define GIMP_COLOR_MANAGED(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_MANAGED, GimpColorManaged))
#define GIMP_COLOR_MANAGED_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_COLOR_MANAGED, GimpColorManagedInterface))


typedef struct _GimpColorManagedInterface GimpColorManagedInterface;

struct _GimpColorManagedInterface
{
  GTypeInterface  base_iface;

  /*  virtual functions  */
  const guint8 * (* get_icc_profile) (GimpColorManaged *managed,
                                      gsize            *len);

  /*  signals  */
  void           (* profile_changed) (GimpColorManaged *managed);
};


GType          gimp_color_managed_interface_get_type (void) G_GNUC_CONST;

const guint8 * gimp_color_managed_get_icc_profile    (GimpColorManaged *managed,
                                                      gsize            *len);
void           gimp_color_managed_profile_changed    (GimpColorManaged *managed);


G_END_DECLS

#endif  /* __GIMP_COLOR_MANAGED_IFACE_H__ */
