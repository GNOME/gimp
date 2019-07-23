/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GimpColorManaged interface
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_COLOR_H_INSIDE__) && !defined (GIMP_COLOR_COMPILATION)
#error "Only <libgimpcolor/gimpcolor.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_MANAGED_H__
#define __GIMP_COLOR_MANAGED_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_COLOR_MANAGED               (gimp_color_managed_get_type ())
#define GIMP_IS_COLOR_MANAGED(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_MANAGED))
#define GIMP_COLOR_MANAGED(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_MANAGED, GimpColorManaged))
#define GIMP_COLOR_MANAGED_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_COLOR_MANAGED, GimpColorManagedInterface))


typedef struct _GimpColorManagedInterface GimpColorManagedInterface;

/**
 * GimpColorManagedInterface:
 * @base_iface: The parent interface
 * @get_icc_profile: Returns the ICC profile of the pixels managed by
 *                   the object
 * @profile_changed: This signal is emitted when the object's color profile
 *                   has changed
 * @get_color_profile: Returns the #GimpColorProfile of the pixels managed
 *                     by the object
 **/
struct _GimpColorManagedInterface
{
  GTypeInterface  base_iface;

  /*  virtual functions  */
  const guint8     * (* get_icc_profile)   (GimpColorManaged *managed,
                                            gsize            *len);

  /*  signals  */
  void               (* profile_changed)   (GimpColorManaged *managed);

  /*  virtual functions  */
  GimpColorProfile * (* get_color_profile) (GimpColorManaged *managed);
};


GType              gimp_color_managed_get_type           (void) G_GNUC_CONST;

GIMP_DEPRECATED_FOR (gimp_color_managed_get_type)
GType              gimp_color_managed_interface_get_type (void) G_GNUC_CONST;

const guint8     * gimp_color_managed_get_icc_profile    (GimpColorManaged *managed,
                                                          gsize            *len);
GimpColorProfile * gimp_color_managed_get_color_profile  (GimpColorManaged *managed);

void               gimp_color_managed_profile_changed    (GimpColorManaged *managed);


G_END_DECLS

#endif  /* __GIMP_COLOR_MANAGED_IFACE_H__ */
