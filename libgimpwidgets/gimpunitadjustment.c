/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitadjustment.c
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include "libgimpbase/gimpbase.h"
#include "gimpwidgets.h"
#include "gimpunitadjustment.h"

/* debug macro */
//#define UNITADJUSTMENT_DEBUG
#ifdef  UNITADJUSTMENT_DEBUG
#define DEBUG(x) g_debug x 
#else
#define DEBUG(x) /* nothing */
#endif

/* some default values */
#define DEFAULT_UNIT        GIMP_UNIT_INCH
#define DEFAULT_RESOLUTION  72.0

G_DEFINE_TYPE (GimpUnitAdjustment, gimp_unit_adjustment, GTK_TYPE_ADJUSTMENT);

/** 
 * prototypes
 **/
/* converts from one current unit to another */
static void gimp_unit_adjustment_convert_unit (GimpUnitAdjustment *adj, GimpUnit unit);
/* handler for unit-changed signal from other entries */
static void unit_changed_handler (GimpUnitAdjustment *adj, GimpUnit unit, gpointer userData);


static void
gimp_unit_adjustment_init (GimpUnitAdjustment *unitAdjustment)
{
  /* set default values */
  gtk_adjustment_set_upper (&unitAdjustment->parent_instance, 10000000.0);
  gtk_adjustment_set_step_increment (&unitAdjustment->parent_instance, 1.0);
  gtk_adjustment_set_page_increment (&unitAdjustment->parent_instance, 10.0);

  /* default unit, resolution */
  unitAdjustment->unit        = DEFAULT_UNIT;
  unitAdjustment->resolution  = DEFAULT_RESOLUTION;
}

static void
gimp_unit_adjustment_class_init (GimpUnitAdjustmentClass *klass)
{
  klass->sig_unit_changed_id = g_signal_new ("unit-changed",
                                              GIMP_TYPE_UNIT_ADJUSTMENT,
                                              G_SIGNAL_RUN_LAST,
                                              0,
                                              NULL, 
                                              NULL,
                                              g_cclosure_marshal_VOID__INT,
                                              G_TYPE_NONE, 
                                              1, 
                                              G_TYPE_INT);

  klass->sig_resolution_changed_id = g_signal_new ("resolution-changed",
                                                    GIMP_TYPE_UNIT_ADJUSTMENT,
                                                    G_SIGNAL_RUN_LAST,
                                                    0,
                                                    NULL, 
                                                    NULL,
                                                    g_cclosure_marshal_VOID__INT,
                                                    G_TYPE_NONE, 
                                                    1, 
                                                    G_TYPE_DOUBLE);
}

GObject *
gimp_unit_adjustment_new (void)
{
  return g_object_new (GIMP_TYPE_UNIT_ADJUSTMENT, NULL);
}

static void
unit_changed_handler (GimpUnitAdjustment *adj, GimpUnit unit, gpointer userData)
{
  GimpUnitAdjustment *adjustment = GIMP_UNIT_ADJUSTMENT (userData);

  gimp_unit_adjustment_convert_unit (adjustment, unit);
}

/* connects adjustment to another adjustment */
void   
gimp_unit_adjustment_connect (GimpUnitAdjustment *adj, GimpUnitAdjustment *target)
{
  g_signal_connect (target,
                    "unit-changed",
                    G_CALLBACK (unit_changed_handler),
                    (gpointer*) adj); 
}

/* converts value from one unit to another */
static void
gimp_unit_adjustment_convert_unit (GimpUnitAdjustment *adj, GimpUnit unit)
{
  gdouble newValue = 0, lower, upper;
  if (adj->unit != unit)
  {
    DEBUG   (("GimpUnitAdjustment: changing unit from %s to %s\n",
             gimp_unit_get_abbreviation (adj->unit),
             gimp_unit_get_abbreviation (unit));)

    /* convert value to new unit */
    newValue = gimp_units_to_pixels (gtk_adjustment_get_value (&(adj->parent_instance)),
                                     adj->unit,
                                     adj->resolution);
    newValue = gimp_pixels_to_units (newValue,
                                     unit, 
                                     adj->resolution);

    /* also convert bounds */
    upper = gimp_units_to_pixels (gtk_adjustment_get_upper (GTK_ADJUSTMENT (adj)),
                                  adj->unit,
                                  adj->resolution);
    lower = gimp_units_to_pixels (gtk_adjustment_get_lower (GTK_ADJUSTMENT (adj)),
                                  adj->unit,
                                  adj->resolution);

    upper = gimp_pixels_to_units (upper,
                                  unit, 
                                  adj->resolution);
    lower = gimp_pixels_to_units (lower,
                                  unit, 
                                  adj->resolution);

    gtk_adjustment_set_upper (GTK_ADJUSTMENT (adj), upper);
    gtk_adjustment_set_lower (GTK_ADJUSTMENT (adj), lower);

    /* set the new unit */
    adj->unit  = unit;

    gimp_unit_adjustment_set_value (adj, newValue);
  }
}

/* sets unit of adjustment, does conversion if neccessary */
void    
gimp_unit_adjustment_set_unit (GimpUnitAdjustment *adj, GimpUnit unit)
{
  gimp_unit_adjustment_convert_unit (adj, unit);

  /* emit "unit-changed" */
  g_signal_emit(adj, GIMP_UNIT_ADJUSTMENT_GET_CLASS(adj)->sig_unit_changed_id, 0, unit);
}

/* sets/gets the value of an adjustment */
void
gimp_unit_adjustment_set_value (GimpUnitAdjustment *adj, gdouble value)
{
  DEBUG (("set_value: %f", value);)

  gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), value);
}
gdouble 
gimp_unit_adjustment_get_value (GimpUnitAdjustment *adj)
{
  gdouble value;

  value = gtk_adjustment_get_value (GTK_ADJUSTMENT (adj));

  return value;
}
gdouble 
gimp_unit_adjustment_get_value_in_unit (GimpUnitAdjustment *adj, GimpUnit unit)
{
  gdouble value = gimp_unit_adjustment_get_value (adj);

  value = gimp_units_to_pixels (value, adj->unit, adj->resolution);
  value = gimp_pixels_to_units (value, unit, adj->resolution);

  return value;
}
void    
gimp_unit_adjustment_set_resolution (GimpUnitAdjustment *adj, gdouble res)
{
  if (adj->resolution != res) 
  {
    adj->resolution = res;
    /* emit "resolution-changed" */
    g_signal_emit(adj, 
                  GIMP_UNIT_ADJUSTMENT_GET_CLASS(adj)->sig_resolution_changed_id,
                  0, 
                  res);
  }
}
gdouble 
gimp_unit_adjustment_get_resolution (GimpUnitAdjustment *adj)
{
  return adj->resolution;
}

/* get string in format "value unit" */
gchar* 
gimp_unit_adjustment_to_string (GimpUnitAdjustment *adj)
{
  return gimp_unit_adjustment_to_string_in_unit (adj, adj->unit);
}
gchar*  
gimp_unit_adjustment_to_string_in_unit (GimpUnitAdjustment *adj, GimpUnit unit)
{
  gdouble value;
  gchar *text = g_malloc (sizeof (gchar) * 12);

  value = gimp_unit_adjustment_get_value_in_unit (adj, unit);

  g_sprintf (text, "%.*f %s", 
             gimp_unit_get_digits (unit),
             value,
             gimp_unit_get_abbreviation (unit));

  return text;
}
GimpUnit 
gimp_unit_adjustment_get_unit (GimpUnitAdjustment *adj)
{
  return adj->unit;
}
void
gimp_unit_adjustment_set_bounds (GimpUnitAdjustment *adj, GimpUnit unit, gdouble upper, gdouble lower)
{
  /* convert bounds from given unit to current unit */
  upper = gimp_units_to_pixels (upper,
                                unit,
                                adj->resolution);
  lower = gimp_units_to_pixels (lower,
                                unit,
                                adj->resolution);

  upper = gimp_pixels_to_units (upper,
                                adj->unit, 
                                adj->resolution);
  lower = gimp_pixels_to_units (lower,
                                adj->unit, 
                                adj->resolution);

  /* set bounds */
  gtk_adjustment_set_upper (GTK_ADJUSTMENT (adj), upper);
  gtk_adjustment_set_lower (GTK_ADJUSTMENT (adj), lower);
}

