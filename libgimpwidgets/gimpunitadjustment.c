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

/**
 * SECTION: gimpunitadjustment
 * @title: GimpUnitAdjustment
 * @short_description: A #GtkAdjustment subclass for holding a value
 * and the corresponding unit.
 * @see_also: #GimpUnitEntry, #GimpUnit
 *
 * #GimpUnitAdjustment is a subclass of #GtkAdjustment. The GimpUnitAdjustment
 * oject represents a value and its unit and resolution. It works basically
 * the same way as GtkAdjustment does, but has a few additional unit-related
 * getters and setters and the ability to be connected to other GimpUnitAdjustments,
 * so that the change of one of the adjustment's unit automatically
 * changes the unit of all other connected adjustments.
 *
 * It is used by #GimpUnitEntry.
 **/

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

static void    gimp_unit_adjustment_convert_to_unit             (GimpUnitAdjustment *adjustment, 
                                                                 GimpUnit            unit);
static void    gimp_unit_adjustment_other_unit_changed_handler  (GimpUnitAdjustment *adjustment, 
                                                                 gpointer            userData);
static gdouble gimp_unit_adjustment_value_to_unit               (gdouble             value, 
                                                                 GimpUnit            current_unit,
                                                                 GimpUnit            new_unit, 
                                                                 gdouble             resolution);

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

static void
gimp_unit_adjustment_other_unit_changed_handler (GimpUnitAdjustment *adjustment,
                                                 gpointer            user_data)
{
  GimpUnitAdjustment *other_adj  = GIMP_UNIT_ADJUSTMENT (user_data);
  GimpUnit           unit        = gimp_unit_adjustment_get_unit (other_adj);

  gimp_unit_adjustment_convert_to_unit (adjustment, unit);
}

/**
 * gimp_unit_adjustment_new:
 *
 * Creates a new GimpUnitAdjustment object.
 *
 * Returns: A Pointer to the new #GimpUnitAdjustment object
 **/
GimpUnitAdjustment *
gimp_unit_adjustment_new (void)
{
  return GIMP_UNIT_ADJUSTMENT (g_object_new (GIMP_TYPE_UNIT_ADJUSTMENT, NULL));
}

/**
 * gimp_unit_adjustment_follow_unit_of:
 * @adjustment:         The adjustment you want to adopt the followed adjustments
 *                      unit.
 * @follow_adjustment:  The adjustment you want to follow.
 * 
 * Lets a #GimpUnitAdjustment follow unit changes of another GimpUnitAdjustment.
 *
 * Whenever the followed adjustment changes its unit, the first adjustment is
 * automatically changed to the same unit. Note that the connection is "one-way",
 * if you also want the second adjustment to follow the first one, you have
 * to call this function again for the other direction.
 **/
void   
gimp_unit_adjustment_follow_unit_of (GimpUnitAdjustment *adjustment, 
                                     GimpUnitAdjustment *followed_adjustment)
{
  g_signal_connect_swapped (followed_adjustment,
                           "unit-changed",
                            G_CALLBACK (gimp_unit_adjustment_other_unit_changed_handler),
                            (gpointer*) adjustment); 
}

/* converts value from one unit to another */
static void
gimp_unit_adjustment_convert_to_unit (GimpUnitAdjustment *adjustment,
                                      GimpUnit            unit)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adjustment);
  gdouble                    new_value = 0;
  gdouble                    lower;
  gdouble                    upper;

  if (private->unit == unit)
    return;

  DEBUG   (("GimpUnitAdjustment: changing unit from %s to %s\n",
           gimp_unit_get_abbreviation (private->unit),
           gimp_unit_get_abbreviation (unit));)

  /* convert value to new unit */
  new_value = gimp_unit_adjustment_value_to_unit (gtk_adjustment_get_value (GTK_ADJUSTMENT (adjustment)),
                                                  private->unit,
                                                  unit,
                                                  private->resolution);

  /* also convert bounds */
  upper = gimp_unit_adjustment_value_to_unit (gtk_adjustment_get_upper (GTK_ADJUSTMENT (adjustment)),
                                              private->unit,
                                              unit,
                                              private->resolution);
  lower = gimp_unit_adjustment_value_to_unit (gtk_adjustment_get_lower (GTK_ADJUSTMENT (adjustment)),
                                              private->unit,
                                              unit,
                                              private->resolution);

  gtk_adjustment_set_upper (GTK_ADJUSTMENT (adjustment), upper);
  gtk_adjustment_set_lower (GTK_ADJUSTMENT (adjustment), lower);

  /* set the new unit */
  private->unit = unit;

  gimp_unit_adjustment_set_value (adjustment, new_value);
}

/** 
 * gimp_unit_adjustment_set_unit:
 * @adjustment:     The adjustment you want to change the unit of.
 * @unit:           The new unit.
 *
 * Changes the unit of a #GimpUnitAdjustment.
 *
 * The value and the upper and lower bounds of the adjustment are automatically
 * converted to the new unit and the "unit-changed" signal is emmited.
 **/
void    
gimp_unit_adjustment_set_unit (GimpUnitAdjustment *adjustment, 
                               GimpUnit            unit)
{
  gimp_unit_adjustment_convert_to_unit (adjustment, unit);

  /* emit "unit-changed" */
  g_signal_emit(adjustment, unit_adjustment_signals [UNIT_CHANGED], 0);
}

/**
 * gimp_unit_adjustment_set_value:
 * @adjustment:     The adjustment you want to set the value of.
 * @value:          The new value.
 *
 * Sets the value of a #GimpUnitAdjustment in its current unit.
 **/
void
gimp_unit_adjustment_set_value (GimpUnitAdjustment *adjustment, 
                                gdouble             value)
{
  DEBUG (("set_value: %f", value);)

  gtk_adjustment_set_value (GTK_ADJUSTMENT (adjustment), value);
}

/**
 * gimp_unit_adjustment_set_value_in_unit:
 * @adjustment:     The adjustment you want to set the value of.
 * @value:          The new value.
 * @unit:           The unit the new value is in.
 *
 * Sets the value of a #GimpUnitAdjustment in a specific unit. 
 *
 * The value is converted from the given unit to the adjustments unit. The
 * adjustments unit is not changed!
 **/
void
gimp_unit_adjustment_set_value_in_unit (GimpUnitAdjustment *adjustment, 
                                        gdouble             value, 
                                        GimpUnit            unit)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adjustment);

  /* convert from given unit to adjustments unit */
  value = gimp_unit_adjustment_value_to_unit (value, unit, private->unit, private->resolution);

  gimp_unit_adjustment_set_value (adjustment, value);
}

/**
 * gimp_unit_adjustment_get_value:
 * @adjustment:     The adjustment you want to get the value from.
 *
 * Returns: The value of the #GimpUnitAdjustment in its current unit.
 **/
gdouble 
gimp_unit_adjustment_get_value (GimpUnitAdjustment *adjustment)
{
  gdouble value;

  value = gtk_adjustment_get_value (GTK_ADJUSTMENT (adjustment));

  return value;
}

/**
 * gimp_unit_adjustment_get_value_in_unit:
 * @adjustment:     The adjustment you want to get the value from.
 * @unit:           The unit you want the value to be converted to.
 *
 * Returns: The value of the #GimpUnitAdjustment converted to the given unit.
 **/
gdouble 
gimp_unit_adjustment_get_value_in_unit (GimpUnitAdjustment *adjustment, 
                                        GimpUnit            unit)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adjustment);
  gdouble                    value     = gimp_unit_adjustment_get_value (adjustment);

  value = gimp_unit_adjustment_value_to_unit (value, 
                                              private->unit, 
                                              unit,
                                              private->resolution);

  return value;
}

/**
 * gimp_unit_adjustment_set_resolution:
 * @adjustment:     The adjustment you want to set the value of.
 * @resolution:     The new resolution.
 *
 * Sets the resolution of a #GimpUnitAdjustment in dpi.
 * 
 * Emits the "resolution-changed" signal.
 **/
void    
gimp_unit_adjustment_set_resolution (GimpUnitAdjustment *adjustment, 
                                     gdouble             res)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adjustment);

  if (private->resolution != res) 
  {
    private->resolution = res;
    /* emit "resolution-changed" */
    g_signal_emit(adjustment, unit_adjustment_signals [RESOLUTION_CHANGED], 0);
  }
}

/**
 * gimp_unit_adjustment_get_resolution:
 * @adjustment:     The adjustment you want to get the resolution from.
 *
 * Returns: The resolution of the #GimpUnitAdjustment in dpi.
 **/
gdouble 
gimp_unit_adjustment_get_resolution (GimpUnitAdjustment *adjustment)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adjustment);

  return private->resolution;
}

/**
 * gimp_unit_adjustment_get_unit:
 * @adjustment:     The adjustment you want to get the unit from.
 *
 * Returns: The unit of the #GimpUnitAdjustment.
 **/
GimpUnit 
gimp_unit_adjustment_get_unit (GimpUnitAdjustment *adjustment)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adjustment);

  return private->unit;
}

/**
 * gimp_unit_adjustment_set_bounds:
 * @adjustment:     The adjustment you want to set the bounds of.
 * @unit:           The unit your given bounds are in.
 * @lower:          The lower bound.
 * @upper:          The upper bound.
 *
 * Sets the lower and upper bounds (minimum and maximum values) of a #GimpUnitAdjustment.
 * 
 * The bounds are converted from the given unit to the adjustment's. If the adjustment's
 * value is smaller/bigger than the lower/upper bound, it is automatically changed to be 
 * in bounds. Note that if you change the adjustments unit, the bounds are also
 * converted to that unit.
 **/
void
gimp_unit_adjustment_set_bounds (GimpUnitAdjustment *adjustment, 
                                 GimpUnit            unit, 
                                 gdouble             lower, 
                                 gdouble             upper)
{
  GimpUnitAdjustmentPrivate *private   = GIMP_UNIT_ADJUSTMENT_GET_PRIVATE (adjustment);
  gdouble                    temp;

  /* switch lower and upper bounds if neccessary
     (older revisions used gimp_unit_adjustment_set_bounds (adjustment, unit, upper, lower)) */
  if (lower > upper) 
  {
    temp = upper;
    upper = lower;
    lower = temp;    
  }

  /* convert bounds from given unit to current unit */
  upper = gimp_unit_adjustment_value_to_unit (upper,
                                              unit,
                                              private->unit,
                                              private->resolution);
  lower = gimp_unit_adjustment_value_to_unit (lower,
                                              unit,
                                              private->unit,
                                              private->resolution);

  /* set bounds */
  gtk_adjustment_set_upper (GTK_ADJUSTMENT (adjustment), upper);
  gtk_adjustment_set_lower (GTK_ADJUSTMENT (adjustment), lower);

  /* update value if neccessary */
  temp = gimp_unit_adjustment_get_value_in_unit (adjustment, unit);
  if (temp < lower) 
  {
    gimp_unit_adjustment_set_value (adjustment, lower);
  }
  else if (temp > upper)
  {
    gimp_unit_adjustment_set_value (adjustment, upper);
  }
}

static gdouble 
gimp_unit_adjustment_value_to_unit (gdouble  value, 
                                    GimpUnit current_unit, 
                                    GimpUnit new_unit,
                                    gdouble  resolution)
{
  gdouble new_value;

  new_value = gimp_units_to_pixels (value, current_unit, resolution);

  return gimp_pixels_to_units (new_value, new_unit, resolution);
}                                    
