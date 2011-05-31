/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitadjustment.h
 * Copyright (C) 2011 Enrico Schröder <enni.schroeder@gmail.com>
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

#ifndef __GIMP_UNIT_ADJUSTMENT_H__
#define __GIMP_UNIT_ADJUSTMENT_H__

#include "libgimpbase/gimpbase.h"

G_BEGIN_DECLS

/**
 * boiler-plate
 **/
#define GIMP_TYPE_UNIT_ADJUSTMENT            (gimp_unit_adjustment_get_type ())
#define GIMP_UNIT_ADJUSTMENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNIT_ADJUSTMENT, GimpUnitAdjustment))
#define GIMP_UNIT_ADJUSTMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNIT_ADJUSTMENT, GimpUnitAdjustmentClass))
#define GIMP_IS_UNIT_ADJUSTMENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_UNIT_ADJUSTMENT))
#define GIMP_IS_UNIT_ADJUSTMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNIT_ADJUSTMENT))
#define GIMP_UNIT_ADJUSTMENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNIT_ADJUSTMENT, GimpUnitAdjustmentClass))

typedef struct _GimpUnitAdjustment       GimpUnitAdjustment;
typedef struct _GimpUnitAdjustmentClass  GimpUnitAdjustmentClass;

struct _GimpUnitAdjustment
{
  GtkAdjustment parent_instance;

  /* flag set when unit has been changed externally */
  gboolean      unitChanged; 

  /* private */
  /* TODO move private fields into own struct? */
  GimpUnit  unit;           /* the unit our value is in */
  gdouble   resolution;     /* resolution in dpi */
};

struct _GimpUnitAdjustmentClass
{
  GtkAdjustmentClass parent_class;

  /* signals */
  guint sig_unit_changed_id;
};

/**
 * prototypes
 **/
GType   gimp_unit_adjustment_get_type (void);
GObject *gimp_unit_adjustment_new (void);

/* sets unit of adjustment, does conversion if neccessary */
void    gimp_unit_adjustment_set_unit (GimpUnitAdjustment *adj, GimpUnit unit);
/* sets/gets the value of an adjustment */
void    gimp_unit_adjustment_set_value (GimpUnitAdjustment *adj, gdouble value);
gdouble gimp_unit_adjustment_get_value (GimpUnitAdjustment *adj);
gdouble gimp_unit_adjustment_get_value_in_unit    (GimpUnitAdjustment *adj, GimpUnit unit);
void    gimp_unit_adjustment_set_resolution (GimpUnitAdjustment *adj, gdouble res);
gdouble gimp_unit_adjustment_get_resolution (GimpUnitAdjustment *adj);
/* get string in format "value unit" */
gchar*  gimp_unit_adjustment_to_string (GimpUnitAdjustment *adj);
gchar*  gimp_unit_adjustment_to_string_in_unit (GimpUnitAdjustment *adj, GimpUnit unit);
/* connects adjustment to another adjustment */
void    gimp_unit_adjustment_connect (GimpUnitAdjustment *adj, GimpUnitAdjustment *target);

G_END_DECLS

#endif /*__GIMP_UNIT_ADJUSTMENT_H__*/