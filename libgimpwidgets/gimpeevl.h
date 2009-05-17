/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpeevl.h
 * Copyright (C) 2008-2009 Fredrik Alstromer <roe@excu.se>
 * Copyright (C) 2008-2009 Martin Nordholts <martinn@svn.gnome.org>
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

#ifndef __GIMP_EEVL_H__
#define __GIMP_EEVL_H__

G_BEGIN_DECLS


/**
 * GimpEevlQuantity:
 * @value:     In reference units.
 * @dimension: in has a dimension of 1, in^2 has a dimension of 2 etc
 */
typedef struct
{
  gdouble value;
  gint dimension;
} GimpEevlQuantity;


/**
 * GimpEevlUnitResolverProc:
 * @identifier: Identifier of unit to resolve or %NULL if default unit
 *              should be resolved.
 * @result:     Units per reference unit. For example, in GIMP the
 *              reference unit is inches so resolving "mm" should
 *              return 25.4 since there are 25.4 millimeters per inch.
 * @data:       Data given to gimp_eevl_evaluate().
 *
 * Returns: If the unit was successfully resolved or not.
 *
 */
typedef gboolean (* GimpEevlUnitResolverProc) (const gchar      *identifier,
                                               GimpEevlQuantity *result,
                                               gpointer          data);

gboolean gimp_eevl_evaluate (const gchar               *string,
                             GimpEevlUnitResolverProc   unit_resolver_proc,
                             GimpEevlQuantity          *result,
                             gpointer                   data,
                             const gchar              **error_pos,
                             GError                   **error);


G_END_DECLS

#endif /* __GIMP_EEVL_H__ */
