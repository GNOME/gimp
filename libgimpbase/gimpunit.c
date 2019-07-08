/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunit.c
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
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <math.h>
#include <string.h>

#include <glib-object.h>

#include "gimpbasetypes.h"

#include "gimpbase-private.h"
#include "gimpunit.h"


/**
 * SECTION: gimpunit
 * @title: gimpunit
 * @short_description: Provides a collection of predefined units and
 *                     functions for creating user-defined units.
 * @see_also: #GimpUnitMenu, #GimpSizeEntry.
 *
 * Provides a collection of predefined units and functions for
 * creating user-defined units.
 **/


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
  g_warning ("Can't convert string '%s' to GimpUnit.", str);
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
 * Note that a new unit is always created with its deletion flag
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
 * Returns the number of digits set for @unit.
 * Built-in units' accuracy is approximately the same as an inch with
 * two digits. User-defined units can suggest a different accuracy.
 *
 * Note: the value is as-set by defaults or by the user and does not
 * necessary provide enough precision on high-resolution images.
 * When the information is needed for a specific image, the use of
 * gimp_unit_get_scaled_digits() may be more appropriate.
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
 * gimp_unit_get_scaled_digits:
 * @unit: The unit you want to know the digits.
 * @resolution: the resolution in PPI.
 *
 * Returns the number of digits a @unit field should provide to get
 * enough accuracy so that every pixel position shows a different
 * value from neighboring pixels.
 *
 * Note: when needing digit accuracy to display a diagonal distance,
 * the @resolution may not correspond to the image's horizontal or
 * vertical resolution, but instead to the result of:
 * `distance_in_pixel / distance_in_inch`.
 *
 * Returns: The suggested number of digits.
 **/
gint
gimp_unit_get_scaled_digits (GimpUnit unit,
                             gdouble  resolution)
{
  gint digits;

  g_return_val_if_fail (_gimp_unit_vtable.unit_get_digits != NULL, 2);

  digits = ceil (log10 (1.0 /
                        gimp_pixels_to_units (1.0, unit, resolution)));

  return MAX (digits, gimp_unit_get_digits (unit));
}

/**
 * gimp_unit_get_identifier:
 * @unit: The unit you want to know the identifier of.
 *
 * This is an untranslated string and must not be changed or freed.
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

static gint print (gchar       *buf,
                   gint         len,
                   gint         start,
                   const gchar *fmt,
                   ...) G_GNUC_PRINTF (4, 5);

static gint
print (gchar       *buf,
       gint         len,
       gint         start,
       const gchar *fmt,
       ...)
{
  va_list args;
  gint printed;

  va_start (args, fmt);

  printed = g_vsnprintf (buf + start, len - start, fmt, args);
  if (printed < 0)
    printed = len - start;

  va_end (args);

  return printed;
}

/**
 * gimp_unit_format_string:
 * @format: A printf-like format string which is used to create the unit
 *          string.
 * @unit:   A unit.
 *
 * The @format string supports the following percent expansions:
 *
 * <informaltable pgwide="1" frame="none" role="enum">
 *   <tgroup cols="2"><colspec colwidth="1*"/><colspec colwidth="8*"/>
 *     <tbody>
 *       <row>
 *         <entry>% f</entry>
 *         <entry>Factor (how many units make up an inch)</entry>
 *        </row>
 *       <row>
 *         <entry>% y</entry>
 *         <entry>Symbol (e.g. "''" for GIMP_UNIT_INCH)</entry>
 *       </row>
 *       <row>
 *         <entry>% a</entry>
 *         <entry>Abbreviation</entry>
 *       </row>
 *       <row>
 *         <entry>% s</entry>
 *         <entry>Singular</entry>
 *       </row>
 *       <row>
 *         <entry>% p</entry>
 *         <entry>Plural</entry>
 *       </row>
 *       <row>
 *         <entry>%%</entry>
 *         <entry>Literal percent</entry>
 *       </row>
 *     </tbody>
 *   </tgroup>
 * </informaltable>
 *
 * Returns: A newly allocated string with above percent expressions
 *          replaced with the resp. strings for @unit.
 *
 * Since: 2.8
 **/
gchar *
gimp_unit_format_string (const gchar *format,
                         GimpUnit     unit)
{
  gchar buffer[1024];
  gint  i = 0;

  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (unit == GIMP_UNIT_PERCENT ||
                        (unit >= GIMP_UNIT_PIXEL &&
                         unit < gimp_unit_get_number_of_units ()), NULL);

  while (i < (sizeof (buffer) - 1) && *format)
    {
      switch (*format)
        {
        case '%':
          format++;
          switch (*format)
            {
            case 0:
              g_warning ("%s: unit-menu-format string ended within %%-sequence",
                         G_STRFUNC);
              break;

            case '%':
              buffer[i++] = '%';
              break;

            case 'f': /* factor (how many units make up an inch) */
              i += print (buffer, sizeof (buffer), i, "%f",
                          gimp_unit_get_factor (unit));
              break;

            case 'y': /* symbol ("''" for inch) */
              i += print (buffer, sizeof (buffer), i, "%s",
                          gimp_unit_get_symbol (unit));
              break;

            case 'a': /* abbreviation */
              i += print (buffer, sizeof (buffer), i, "%s",
                          gimp_unit_get_abbreviation (unit));
              break;

            case 's': /* singular */
              i += print (buffer, sizeof (buffer), i, "%s",
                          gimp_unit_get_singular (unit));
              break;

            case 'p': /* plural */
              i += print (buffer, sizeof (buffer), i, "%s",
                          gimp_unit_get_plural (unit));
              break;

            default:
              g_warning ("%s: unit-menu-format contains unknown format "
                         "sequence '%%%c'", G_STRFUNC, *format);
              break;
            }
          break;

        default:
          buffer[i++] = *format;
          break;
        }

      format++;
    }

  buffer[MIN (i, sizeof (buffer) - 1)] = 0;

  return g_strdup (buffer);
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
 * Since: 2.4
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

  if (!(uspec->allow_percent && value->data[0].v_int == GIMP_UNIT_PERCENT))
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
 * @blurb:         Brief description of param.
 * @allow_pixels:  Whether "pixels" is an allowed unit.
 * @allow_percent: Whether "percent" is an allowed unit.
 * @default_value: Unit to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a units param.
 * See g_param_spec_internal() for more information.
 *
 * Returns: a newly allocated #GParamSpec instance
 *
 * Since: 2.4
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

/**
 * gimp_pixels_to_units:
 * @pixels:     value in pixels
 * @unit:       unit to convert to
 * @resolution: resolution in DPI
 *
 * Converts a @value specified in pixels to @unit.
 *
 * Returns: @pixels converted to units.
 *
 * Since: 2.8
 **/
gdouble
gimp_pixels_to_units (gdouble  pixels,
                      GimpUnit unit,
                      gdouble  resolution)
{
  if (unit == GIMP_UNIT_PIXEL)
    return pixels;

  return pixels * gimp_unit_get_factor (unit) / resolution;
}

/**
 * gimp_units_to_pixels:
 * @value:      value in units
 * @unit:       unit of @value
 * @resolution: resloution in DPI
 *
 * Converts a @value specified in @unit to pixels.
 *
 * Returns: @value converted to pixels.
 *
 * Since: 2.8
 **/
gdouble
gimp_units_to_pixels (gdouble  value,
                      GimpUnit unit,
                      gdouble  resolution)
{
  if (unit == GIMP_UNIT_PIXEL)
    return value;

  return value * resolution / gimp_unit_get_factor (unit);
}

/**
 * gimp_units_to_points:
 * @value:      value in units
 * @unit:       unit of @value
 * @resolution: resloution in DPI
 *
 * Converts a @value specified in @unit to points.
 *
 * Returns: @value converted to points.
 *
 * Since: 2.8
 **/
gdouble
gimp_units_to_points (gdouble  value,
                      GimpUnit unit,
                      gdouble  resolution)
{
  if (unit == GIMP_UNIT_POINT)
    return value;

  if (unit == GIMP_UNIT_PIXEL)
    return (value * gimp_unit_get_factor (GIMP_UNIT_POINT) / resolution);

  return (value *
          gimp_unit_get_factor (GIMP_UNIT_POINT) / gimp_unit_get_factor (unit));
}

/**
 * gimp_unit_is_metric:
 * @unit: The unit
 *
 * Checks if the given @unit is metric. A simplistic test is used
 * that looks at the unit's factor and checks if it is 2.54 multiplied
 * by some common powers of 10. Currently it checks for mm, cm, dm, m.
 *
 * See also: gimp_unit_get_factor()
 *
 * Returns: %TRUE if the @unit is metric.
 *
 * Since: 2.10
 **/
gboolean
gimp_unit_is_metric (GimpUnit unit)
{
  gdouble factor;

  if (unit == GIMP_UNIT_MM)
    return TRUE;

  factor = gimp_unit_get_factor (unit);

  if (factor == 0.0)
    return FALSE;

  return ((ABS (factor -  0.0254) < 1e-7) || /* m  */
          (ABS (factor -  0.254)  < 1e-6) || /* dm */
          (ABS (factor -  2.54)   < 1e-5) || /* cm */
          (ABS (factor - 25.4)    < 1e-4));  /* mm */
}
