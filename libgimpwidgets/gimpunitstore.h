/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitstore.h
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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_UNIT_STORE_H__
#define __GIMP_UNIT_STORE_H__

G_BEGIN_DECLS


enum
{
  GIMP_UNIT_STORE_UNIT,
  GIMP_UNIT_STORE_UNIT_FACTOR,
  GIMP_UNIT_STORE_UNIT_DIGITS,
  GIMP_UNIT_STORE_UNIT_NAME,
  GIMP_UNIT_STORE_UNIT_SYMBOL,
  GIMP_UNIT_STORE_UNIT_ABBREVIATION,
  GIMP_UNIT_STORE_UNIT_SHORT_FORMAT,
  GIMP_UNIT_STORE_UNIT_LONG_FORMAT,
  GIMP_UNIT_STORE_UNIT_COLUMNS,
  GIMP_UNIT_STORE_FIRST_VALUE = GIMP_UNIT_STORE_UNIT_COLUMNS
};


#define GIMP_TYPE_UNIT_STORE (gimp_unit_store_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpUnitStore, gimp_unit_store, GIMP, UNIT_STORE, GObject)

struct _GimpUnitStoreClass
{
  GObjectClass  parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved0) (void);
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
  void (*_gimp_reserved8) (void);
  void (*_gimp_reserved9) (void);
};


GimpUnitStore * gimp_unit_store_new              (gint           num_values);

void            gimp_unit_store_set_has_pixels   (GimpUnitStore *store,
                                                  gboolean       has_pixels);
gboolean        gimp_unit_store_get_has_pixels   (GimpUnitStore *store);

void            gimp_unit_store_set_has_percent  (GimpUnitStore *store,
                                                  gboolean       has_percent);
gboolean        gimp_unit_store_get_has_percent  (GimpUnitStore *store);

void            gimp_unit_store_set_pixel_value  (GimpUnitStore *store,
                                                  gint           index,
                                                  gdouble        value);
void            gimp_unit_store_set_pixel_values (GimpUnitStore *store,
                                                  gdouble        first_value,
                                                  ...);
void            gimp_unit_store_set_resolution   (GimpUnitStore *store,
                                                  gint           index,
                                                  gdouble        resolution);
void            gimp_unit_store_set_resolutions  (GimpUnitStore *store,
                                                  gdouble        first_resolution,
                                                  ...);
gdouble         gimp_unit_store_get_nth_value    (GimpUnitStore *store,
                                                  GimpUnit      *unit,
                                                  gint           index);
void            gimp_unit_store_get_values       (GimpUnitStore *store,
                                                  GimpUnit      *unit,
                                                  gdouble       *first_value,
                                                  ...);

void            _gimp_unit_store_sync_units      (GimpUnitStore *store);


G_END_DECLS

#endif  /* __GIMP_UNIT_STORE_H__ */
