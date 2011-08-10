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
#define DEFAULT_UNIT                    GIMP_UNIT_INCH
#define DEFAULT_RESOLUTION              72.0
#define DEFAULT_UPPER                   GIMP_MAX_IMAGE_SIZE

enum 
{ 
  UNIT_CHANGED,
  RESOLUTION_CHANGED,
  LAST_SIGNAL
};

typedef struct
{
  GimpUnit  unit;           /* the unit our value is in */
  gdouble   resolution;     /* resolution in dpi */
} GimpUnitAdjustmentPrivate;

#define GIMP_UNIT_ADJUSTMENT_GET_PRIVATE(obj) \
  ((GimpUnitAdjustmentPrivate *) ((GimpUnitAdjustment *) (obj))->private)

G_DEFINE_TYPE (GimpUnitAdjustment, gimp_unit_adjustment, GTK_TYPE_ADJUSTMENT);

static void gimp_unit_adjustment_convert_to_unit             (GimpUnitAdjustment *adj, GimpUnit unit);
static void gimp_unit_adjustment_other_unit_changed_handler  (GimpUnitAdjustment *adj, gpointer userData);

static guint unit_adjustment_signals[LAST_SIGNAL] = {0};

static void
gimp_unit_adjustment_init (GimpUnitAdjustment *unit_adjustment)
{
  GimpUnitAdjustmentPrivate *private;

  unit_adjustment->private = G_TYPE_INSTANCE_GET_PRIVATE (unit_adjustment,
                                                          GIMP_TYPE_UNIT_ADJUSTMENT,
                                                          GimpUnitAdjustmentPrivate);

  private = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (unit_adjustment);                                                        

  /* set default values */
  gtk_adjustment_set_upper (&unit_adjustment->parent_instance, DEFAULT_UPPER);
  gtk_adjustment_set_step_increment (&unit_adjustment->parent_instance, 1.0);
  gtk_adjustment_set_page_increment (&unit_adjustment->parent_instance, 10.0);

  /* default unit, resolution */
  private->unit        = DEFAULT_UNIT;
  private->resolution  = DEFAULT_RESOLUTION;
}

static void
gimp_unit_adjustment_class_init (GimpUnitAdjustmentClass *klass)
{
  g_type_class_add_private (klass, sizeof (GimpUnitAdjustmentPrivate));

  unit_adjustment_signals [UNIT_CHANGED] = 
    g_signal_new ("unit-changed",
                  GIMP_TYPE_UNIT_ADJUSTMENT,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, 
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 
                  0);

  unit_adjustment_signals [RESOLUTION_CHANGED] = 
    g_signal_new ("resolution-changed",
                  GIMP_TYPE_UNIT_ADJUSTMENT,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, 
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 
                  0);
}

GimpUnitAdjustment *
gimp_unit_adjustment_new (void)
{
  return GIMP_UNIT_ADJUSTMENT (g_object_new (GIMP_TYPE_UNIT_ADJUSTMENT, NULL));
}

static void
gimp_unit_adjustment_other_unit_changed_handler (GimpUnitAdjustment *adj,
                                                 gpointer            user_data)
{
  GimpUnitAdjustment *other_adj  = GIMP_UNIT_ADJUSTMENT (user_data);
  GimpUnit           unit        = gimp_unit_adjustment_get_unit (other_adj);

  gimp_unit_adjustment_convert_to_unit (adj, unit);
}

/* connects adjustment to another adjustment */
void   
gimp_unit_adjustment_follow_unit_of (GimpUnitAdjustment *adj, 
                                     GimpUnitAdjustment *other)
{
  g_signal_connect_swapped (other,
                           "unit-changed",
                            G_CALLBACK (gimp_unit_adjustment_other_unit_changed_handler),
                            (gpointer*) adj); 
}

/* converts value from one unit to another */
static void
gimp_unit_adjustment_convert_to_unit (GimpUnitAdjustment *adj,
                                      GimpUnit            unit)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adj);
  gdouble                    new_value = 0;
  gdouble                    lower;
  gdouble                    upper;

  if (private->unit == unit)
    return;

  DEBUG   (("GimpUnitAdjustment: changing unit from %s to %s\n",
           gimp_unit_get_abbreviation (private->unit),
           gimp_unit_get_abbreviation (unit));)

  /* convert value to new unit */
  new_value = gimp_units_to_pixels (gtk_adjustment_get_value (&(adj->parent_instance)),
                                    private->unit,
                                    private->resolution);
  new_value = gimp_pixels_to_units (new_value,
                                    unit, 
                                    private->resolution);

  /* also convert bounds */
  upper = gimp_units_to_pixels (gtk_adjustment_get_upper (GTK_ADJUSTMENT (adj)),
                                private->unit,
                                private->resolution);
  lower = gimp_units_to_pixels (gtk_adjustment_get_lower (GTK_ADJUSTMENT (adj)),
                                private->unit,
                                private->resolution);

  upper = gimp_pixels_to_units (upper,
                                unit, 
                                private->resolution);
  lower = gimp_pixels_to_units (lower,
                                unit, 
                                private->resolution);

  gtk_adjustment_set_upper (GTK_ADJUSTMENT (adj), upper);
  gtk_adjustment_set_lower (GTK_ADJUSTMENT (adj), lower);

  /* set the new unit */
  private->unit = unit;

  gimp_unit_adjustment_set_value (adj, new_value);
}

/* sets unit of adjustment, does conversion if neccessary */
void    
gimp_unit_adjustment_set_unit (GimpUnitAdjustment *adj, 
                               GimpUnit            unit)
{
  gimp_unit_adjustment_convert_to_unit (adj, unit);

  /* emit "unit-changed" */
  g_signal_emit(adj, unit_adjustment_signals [UNIT_CHANGED], 0);
}

void
gimp_unit_adjustment_set_value (GimpUnitAdjustment *adj, 
                                gdouble             value)
{
  DEBUG (("set_value: %f", value);)

  gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), value);
}

void
gimp_unit_adjustment_set_value_in_unit (GimpUnitAdjustment *adj, 
                                        gdouble             value, 
                                        GimpUnit            unit)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adj);

  /* convert from given unit to adjustments unit */
  value = gimp_units_to_pixels (value, unit, private->resolution);
  value = gimp_pixels_to_units (value, private->unit, private->resolution);

  gimp_unit_adjustment_set_value (adj, value);
}

gdouble 
gimp_unit_adjustment_get_value (GimpUnitAdjustment *adj)
{
  gdouble value;

  value = gtk_adjustment_get_value (GTK_ADJUSTMENT (adj));

  return value;
}

gdouble 
gimp_unit_adjustment_get_value_in_unit (GimpUnitAdjustment *adj, 
                                        GimpUnit            unit)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adj);
  gdouble                    value     = gimp_unit_adjustment_get_value (adj);

  value = gimp_units_to_pixels (value, private->unit, private->resolution);
  value = gimp_pixels_to_units (value, unit, private->resolution);

  return value;
}

void    
gimp_unit_adjustment_set_resolution (GimpUnitAdjustment *adj, 
                                     gdouble             res)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adj);

  if (private->resolution != res) 
  {
    private->resolution = res;
    /* emit "resolution-changed" */
    g_signal_emit(adj, unit_adjustment_signals [RESOLUTION_CHANGED], 0);
  }
}

gdouble 
gimp_unit_adjustment_get_resolution (GimpUnitAdjustment *adj)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adj);

  return private->resolution;
}

GimpUnit 
gimp_unit_adjustment_get_unit (GimpUnitAdjustment *adj)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adj);

  return private->unit;
}

void
gimp_unit_adjustment_set_bounds (GimpUnitAdjustment *adj, 
                                 GimpUnit            unit, 
                                 gdouble             lower, 
                                 gdouble             upper)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adj);
  gdouble                    temp;

  /* switch lower and upper bounds if neccessary
     (older revisions used gimp_unit_adjustment_set_bounds (adj, unit, upper, lower)) */
  if (lower > upper) 
  {
    temp = upper;
    upper = lower;
    lower = temp;    
  }

  /* convert bounds from given unit to current unit */
  upper = gimp_units_to_pixels (upper,
                                unit,
                                private->resolution);
  lower = gimp_units_to_pixels (lower,
                                unit,
                                private->resolution);

  upper = gimp_pixels_to_units (upper,
                                private->unit, 
                                private->resolution);
  lower = gimp_pixels_to_units (lower,
                                private->unit, 
                                private->resolution);

  /* set bounds */
  gtk_adjustment_set_upper (GTK_ADJUSTMENT (adj), upper);
  gtk_adjustment_set_lower (GTK_ADJUSTMENT (adj), lower);
}

