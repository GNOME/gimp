/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitcache.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_UNIT_CACHE_H__
#define __GIMP_UNIT_CACHE_H__

G_BEGIN_DECLS


G_GNUC_INTERNAL gint          _gimp_unit_cache_get_number_of_units          (void);
G_GNUC_INTERNAL gint          _gimp_unit_cache_get_number_of_built_in_units (void) G_GNUC_CONST;

G_GNUC_INTERNAL GimpUnit      _gimp_unit_cache_new               (gchar   *identifier,
                                                                  gdouble  factor,
                                                                  gint     digits,
                                                                  gchar   *symbol,
                                                                  gchar   *abbreviation,
                                                                  gchar   *singular,
                                                                  gchar   *plural);
G_GNUC_INTERNAL gboolean      _gimp_unit_cache_get_deletion_flag (GimpUnit unit);
G_GNUC_INTERNAL void          _gimp_unit_cache_set_deletion_flag (GimpUnit unit,
                                                                  gboolean deletion_flag);
G_GNUC_INTERNAL gdouble       _gimp_unit_cache_get_factor        (GimpUnit unit);
G_GNUC_INTERNAL gint          _gimp_unit_cache_get_digits        (GimpUnit unit);
G_GNUC_INTERNAL const gchar * _gimp_unit_cache_get_identifier    (GimpUnit unit);
G_GNUC_INTERNAL const gchar * _gimp_unit_cache_get_symbol        (GimpUnit unit);
G_GNUC_INTERNAL const gchar * _gimp_unit_cache_get_abbreviation  (GimpUnit unit);
G_GNUC_INTERNAL const gchar * _gimp_unit_cache_get_singular      (GimpUnit unit);
G_GNUC_INTERNAL const gchar * _gimp_unit_cache_get_plural        (GimpUnit unit);


G_END_DECLS

#endif /*  __GIMP_UNIT_CACHE_H__ */
