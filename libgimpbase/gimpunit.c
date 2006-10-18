/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunit.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gimpbasetypes.h"

#include "gimpbase-private.h"
#include "gimpunit.h"


static void   unit_to_string (const GValue *src_value,
                              GValue       *dest_value);
static void   string_to_unit (const GValue *src_value,
                              GValue       *dest_value);

GType
gimp_unit_get_type (void)
{
  static GType unit_type = 0;

  if (! unit_type)
    {
      const GTypeInfo type_info = { 0, };

      unit_type = g_type_register_static (G_TYPE_INT, "GimpUnit",
                                          &type_info, 0);

      g_value_register_transform_func (unit_type, G_TYPE_STRING,
                                       unit_to_string);
      g_value_register_transform_func (G_TYPE_STRING, unit_type,
                                       string_to_unit);
    }

  return unit_type;
}

static void
unit_to_string (const GValue *src_value,
                GValue       *dest_value)
{
  GimpUnit unit = (GimpUnit) g_value_get_int (src_value);

  g_value_set_string (dest_value, gimp_unit_get_identifier (unit));
}

static void
string_to_unit (const GValue *src_value,
                GValue       *dest_value)
{
  const gchar *str;
  gint         num_units;
  gint         i;

  str = g_value_get_string (src_value);

  if (!str || !*str)
    goto error;

  num_units = gimp_unit_get_number_of_units ();

  for (i = GIMP_UNIT_PIXEL; i < num_units; i++)
    if (strcmp (str, gimp_unit_get_identifier (i)) == 0)
      break;

  if (i == num_units)
    {
      if (strcmp (str, gimp_unit_get_identifier (GIMP_UNIT_PERCENT)) == 0)
        i = GIMP_UNIT_PERCENT;
      else
        goto error;
    }

  g_value_set_int (dest_value, i);
  return;

 error:
  g_warning ("Can't convert string to GimpUnit.");
}


/**
 * gimp_unit_get_number_of_units:
 *
 * Returns the number of units which are known to the #GimpUnit system.
 *
 * Returns: The number of defined units.
 **/
gint
gimp_unit_get_number_of_units (void)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_get_number_of_units != NULL,
                        GIMP_UNIT_END);

  return _gimp_unit_vtable.unit_get_number_of_units ();
}

/**
 * gimp_unit_get_number_of_built_in_units:
 *
 * Returns the number of #GimpUnit's which are hardcoded in the unit system
 * (UNIT_INCH, UNIT_MM, UNIT_POINT, UNIT_PICA and the two "pseudo unit"
 *  UNIT_PIXEL).
 *
 * Returns: The number of built-in units.
 **/
gint
gimp_unit_get_number_of_built_in_units (void)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_get_number_of_built_in_units
                        != NULL, GIMP_UNIT_END);

  return _gimp_unit_vtable.unit_get_number_of_built_in_units ();
}

/**
 * gimp_unit_new:
 * @identifier: The unit's identifier string.
 * @factor: The unit's factor (how many units are in one inch).
 * @digits: The unit's suggested number of digits (see gimp_unit_get_digits()).
 * @symbol: The symbol of the unit (e.g. "''" for inch).
 * @abbreviation: The abbreviation of the unit.
 * @singular: The singular form of the unit.
 * @plural: The plural form of the unit.
 *
 * Returns the integer ID of the new #GimpUnit.
 *
 * Note that a new unit is always created with it's deletion flag
 * set to %TRUE. You will have to set it to %FALSE with
 * gimp_unit_set_deletion_flag() to make the unit definition persistent.
 *
 * Returns: The ID of the new unit.
 **/
GimpUnit
gimp_unit_new (gchar   *identifier,
               gdouble  factor,
               gint     digits,
               gchar   *symbol,
               gchar   *abbreviation,
               gchar   *singular,
               gchar   *plural)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_new != NULL, GIMP_UNIT_INCH);

  return _gimp_unit_vtable.unit_new (identifier, factor, digits,
                                     symbol, abbreviation, singular, plural);
}

/**
 * gimp_unit_get_deletion_flag:
 * @unit: The unit you want to know the @deletion_flag of.
 *
 * Returns: The unit's @deletion_flag.
 **/
gboolean
gimp_unit_get_deletion_flag (GimpUnit unit)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_get_deletion_flag != NULL, FALSE);

  return _gimp_unit_vtable.unit_get_deletion_flag (unit);
}

/**
 * gimp_unit_set_deletion_flag:
 * @unit: The unit you want to set the @deletion_flag for.
 * @deletion_flag: The new deletion_flag.
 *
 * Sets a #GimpUnit's @deletion_flag. If the @deletion_flag of a unit is
 * %TRUE when GIMP exits, this unit will not be saved in the users's
 * "unitrc" file.
 *
 * Trying to change the @deletion_flag of a built-in unit will be silently
 * ignored.
 **/
void
gimp_unit_set_deletion_flag (GimpUnit unit,
                             gboolean deletion_flag)
{
  g_return_if_fail (_gimp_unit_vtable.unit_set_deletion_flag != NULL);

  _gimp_unit_vtable.unit_set_deletion_flag (unit, deletion_flag);
}

/**
 * gimp_unit_get_factor:
 * @unit: The unit you want to know the factor of.
 *
 * A #GimpUnit's @factor is defined to be:
 *
 * distance_in_units == (@factor * distance_in_inches)
 *
 * Returns 0 for @unit == GIMP_UNIT_PIXEL.
 *
 * Returns: The unit's factor.
 **/
gdouble
gimp_unit_get_factor (GimpUnit unit)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_get_factor != NULL, 1.0);

  return _gimp_unit_vtable.unit_get_factor (unit);
}

/**
 * gimp_unit_get_digits:
 * @unit: The unit you want to know the digits.
 *
 * Returns the number of digits an entry field should provide to get
 * approximately the same accuracy as an inch input field with two digits.
 *
 * Returns 0 for @unit == GIMP_UNIT_PIXEL.
 *
 * Returns: The suggested number of digits.
 **/
gint
gimp_unit_get_digits (GimpUnit unit)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_get_digits != NULL, 2);

  return _gimp_unit_vtable.unit_get_digits (unit);
}

/**
 * gimp_unit_get_identifier:
 * @unit: The unit you want to know the identifier of.
 *
 * This is an unstranslated string and must not be changed or freed.
 *
 * Returns: The unit's identifier.
 **/
const gchar *
gimp_unit_get_identifier (GimpUnit unit)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_get_identifier != NULL, NULL);

  return _gimp_unit_vtable.unit_get_identifier (unit);
}

/**
 * gimp_unit_get_symbol:
 * @unit: The unit you want to know the symbol of.
 *
 * This is e.g. "''" for UNIT_INCH.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's symbol.
 **/
const gchar *
gimp_unit_get_symbol (GimpUnit unit)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_get_symbol != NULL, NULL);

  return _gimp_unit_vtable.unit_get_symbol (unit);
}

/**
 * gimp_unit_get_abbreviation:
 * @unit: The unit you want to know the abbreviation of.
 *
 * For built-in units, this function returns the translated abbreviation
 * of the unit.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's abbreviation.
 **/
const gchar *
gimp_unit_get_abbreviation (GimpUnit unit)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_get_abbreviation != NULL, NULL);

  return _gimp_unit_vtable.unit_get_abbreviation (unit);
}

/**
 * gimp_unit_get_singular:
 * @unit: The unit you want to know the singular form of.
 *
 * For built-in units, this function returns the translated singular form
 * of the unit's name.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's singular form.
 **/
const gchar *
gimp_unit_get_singular (GimpUnit unit)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_get_singular != NULL, NULL);

  return _gimp_unit_vtable.unit_get_singular (unit);
}

/**
 * gimp_unit_get_plural:
 * @unit: The unit you want to know the plural form of.
 *
 * For built-in units, this function returns the translated plural form
 * of the unit's name.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's plural form.
 **/
const gchar *
gimp_unit_get_plural (GimpUnit unit)
{
  g_return_val_if_fail (_gimp_unit_vtable.unit_get_plural != NULL, NULL);

  return _gimp_unit_vtable.unit_get_plural (unit);
}


/*
 * GIMP_TYPE_PARAM_UNIT
 */

#define GIMP_PARAM_SPEC_UNIT(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_UNIT, GimpParamSpecUnit))

typedef struct _GimpParamSpecUnit GimpParamSpecUnit;

struct _GimpParamSpecUnit
{
  GParamSpecInt parent_instance;

  gboolean      allow_percent;
};

static void      gimp_param_unit_class_init     (GParamSpecClass *class);
static gboolean  gimp_param_unit_value_validate (GParamSpec      *pspec,
                                                 GValue          *value);

/**
 * gimp_param_unit_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a unit param object
 *
 * Since: GIMP 2.4
 **/
GType
gimp_param_unit_get_type (void)
{
  static GType spec_type = 0;

  if (! spec_type)
    {
      const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_unit_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecUnit),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_INT,
                                          "GimpParamUnit",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_unit_class_init (GParamSpecClass *class)
{
  class->value_type     = GIMP_TYPE_UNIT;
  class->value_validate = gimp_param_unit_value_validate;
}

static gboolean
gimp_param_unit_value_validate (GParamSpec *pspec,
                                GValue     *value)
{
  GParamSpecInt     *ispec = G_PARAM_SPEC_INT (pspec);
  GimpParamSpecUnit *uspec = GIMP_PARAM_SPEC_UNIT (pspec);
  gint               oval  = value->data[0].v_int;

  if (uspec->allow_percent && value->data[0].v_int == GIMP_UNIT_PERCENT)
    {
      value->data[0].v_int = value->data[0].v_int;
    }
  else
    {
      value->data[0].v_int = CLAMP (value->data[0].v_int,
                                    ispec->minimum,
                                    gimp_unit_get_number_of_units () - 1);
    }

  return value->data[0].v_int != oval;
}

/**
 * gimp_param_spec_unit:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief desciption of param.
 * @allow_pixels:  Whether "pixels" is an allowed unit.
 * @allow_percent: Whether "perecent" is an allowed unit.
 * @default_value: Unit to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a units param.
 * See g_param_spec_internal() for more information.
 *
 * Returns: a newly allocated #GParamSpec instance
 *
 * Since: GIMP 2.4
 **/
GParamSpec *
gimp_param_spec_unit (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     allow_pixels,
                      gboolean     allow_percent,
                      GimpUnit     default_value,
                      GParamFlags  flags)
{
  GimpParamSpecUnit *pspec;
  GParamSpecInt     *ispec;

  pspec = g_param_spec_internal (GIMP_TYPE_PARAM_UNIT,
                                 name, nick, blurb, flags);

  ispec = G_PARAM_SPEC_INT (pspec);

  ispec->default_value = default_value;
  ispec->minimum       = allow_pixels ? GIMP_UNIT_PIXEL : GIMP_UNIT_INCH;
  ispec->maximum       = GIMP_UNIT_PERCENT - 1;

  pspec->allow_percent = allow_percent;

  return G_PARAM_SPEC (pspec);
}

