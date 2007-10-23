/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpunitstore.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_UNIT_STORE_H__
#define __GIMP_UNIT_STORE_H__


enum
{
  GIMP_UNIT_STORE_UNIT,
  GIMP_UNIT_STORE_UNIT_FACTOR,
  GIMP_UNIT_STORE_UNIT_DIGITS,
  GIMP_UNIT_STORE_UNIT_IDENTIFIER,
  GIMP_UNIT_STORE_UNIT_SYMBOL,
  GIMP_UNIT_STORE_UNIT_ABBREVIATION,
  GIMP_UNIT_STORE_UNIT_SINGULAR,
  GIMP_UNIT_STORE_UNIT_PLURAL,
  GIMP_UNIT_STORE_UNIT_COLUMNS,
  GIMP_UNIT_STORE_FIRST_VALUE = GIMP_UNIT_STORE_UNIT_COLUMNS
};


#define GIMP_TYPE_UNIT_STORE            (gimp_unit_store_get_type ())
#define GIMP_UNIT_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNIT_STORE, GimpUnitStore))
#define GIMP_UNIT_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNIT_STORE, GimpUnitStoreClass))
#define GIMP_IS_UNIT_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_UNIT_STORE))
#define GIMP_IS_UNIT_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNIT_STORE))
#define GIMP_UNIT_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNIT_STORE, GimpUnitStoreClass))


typedef struct _GimpUnitStoreClass  GimpUnitStoreClass;

struct _GimpUnitStoreClass
{
  GObjectClass  parent_class;
};

struct _GimpUnitStore
{
  GObject       parent_instance;

  gint          num_values;
  gdouble      *values;
  gdouble      *resolutions;
};


GType           gimp_unit_store_get_type (void) G_GNUC_CONST;

GimpUnitStore * gimp_unit_store_new              (gint           num_values);

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
gdouble         gimp_unit_store_get_value        (GimpUnitStore *store,
                                                  GimpUnit       unit,
                                                  gint           index);
void            gimp_unit_store_get_values       (GimpUnitStore *store,
                                                  GimpUnit       unit,
                                                  gdouble       *first_value,
                                                  ...);


#endif  /* __GIMP_UNIT_STORE_H__ */
