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

#include <gegl.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "gimpbasetypes.h"

#include "gimpbase-private.h"
#include "gimpparamspecs.h"
#include "gimpunit.h"

#include "libgimp/libgimp-intl.h"


enum
{
  PROP_0,
  PROP_ID,
  PROP_NAME,
  PROP_FACTOR,
  PROP_DIGITS,
  PROP_SYMBOL,
  PROP_ABBREVIATION,
};

struct _GimpUnit
{
  GObject   parent_instance;

  gint      id;
  gchar    *name;

  gboolean  delete_on_exit;
  gdouble   factor;
  gint      digits;
  gchar    *symbol;
  gchar    *abbreviation;
};


typedef struct
{
  gdouble   factor;
  gint      digits;
  gchar    *identifier;
  gchar    *symbol;
  gchar    *abbreviation;
} GimpUnitDef;

/*  these are the built-in units
 */
static const GimpUnitDef _gimp_unit_defs[GIMP_UNIT_END] =
{
  /* pseudo unit */
  {
    0.0,  0,
    NC_("unit-plural", "pixels"),
    "px", "px",
  },

  /* standard units */
  {
    1.0,  2,
    NC_("unit-plural", "inches"),
    "''", "in",
  },

  {
    25.4, 1,
    NC_("unit-plural", "millimeters"),
    "mm", "mm",
  },

  /* professional units */
  {
    72.0, 0,
    NC_("unit-plural", "points"),
    "pt", "pt",
  },

  {
    6.0,  1,
    NC_("unit-plural", "picas"),
    "pc", "pc",
  }
};

/*  not a unit at all but kept here to have the strings in one place
 */
static const GimpUnitDef _gimp_unit_percent_def =
{
  0.0,  0,
  NC_("unit-plural", "percent"),
  "%",  "%",
};


static void       gimp_unit_constructed       (GObject             *object);
static void       gimp_unit_finalize          (GObject             *object);
static void       gimp_unit_set_property      (GObject             *object,
                                               guint                property_id,
                                               const GValue        *value,
                                               GParamSpec          *pspec);
static void       gimp_unit_get_property      (GObject             *object,
                                               guint                property_id,
                                               GValue              *value,
                                               GParamSpec          *pspec);

static gint       print                       (gchar               *buf,
                                               gint                 len,
                                               gint                 start,
                                               const gchar         *fmt,
                                               ...) G_GNUC_PRINTF (4, 5);


G_DEFINE_TYPE (GimpUnit, gimp_unit, G_TYPE_OBJECT)

#define parent_class gimp_unit_parent_class


static void
gimp_unit_class_init (GimpUnitClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructed  = gimp_unit_constructed;
  object_class->finalize     = gimp_unit_finalize;
  object_class->set_property = gimp_unit_set_property;
  object_class->get_property = gimp_unit_get_property;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_FACTOR,
                                   g_param_spec_double ("factor", NULL, NULL,
                                                        0.0, G_MAXDOUBLE, 1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_DIGITS,
                                   g_param_spec_int ("digits", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_SYMBOL,
                                   g_param_spec_string ("symbol", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_ABBREVIATION,
                                   g_param_spec_string ("abbreviation", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /*klass->id_table = gimp_id_table_new ();*/
}

static void
gimp_unit_init (GimpUnit *unit)
{
  unit->name         = NULL;
  unit->symbol       = NULL;
  unit->abbreviation = NULL;
}

static void
gimp_unit_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
gimp_unit_finalize (GObject *object)
{
  GimpUnit *unit = GIMP_UNIT (object);

  g_free (unit->name);
  g_free (unit->symbol);
  g_free (unit->abbreviation);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_unit_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpUnit *unit = GIMP_UNIT (object);

  switch (property_id)
    {
    case PROP_ID:
      unit->id = g_value_get_int (value);
      break;
    case PROP_NAME:
      unit->name = g_value_dup_string (value);
      break;
    case PROP_FACTOR:
      unit->factor = g_value_get_double (value);
      break;
    case PROP_DIGITS:
      unit->digits = g_value_get_int (value);
      break;
    case PROP_SYMBOL:
      unit->symbol = g_value_dup_string (value);
      break;
    case PROP_ABBREVIATION:
      unit->abbreviation = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_unit_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpUnit *unit = GIMP_UNIT (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, unit->id);
      break;
    case PROP_NAME:
      g_value_set_string (value, unit->name);
      break;
    case PROP_FACTOR:
      g_value_set_double (value, unit->factor);
      break;
    case PROP_DIGITS:
      g_value_set_int (value, unit->digits);
      break;
    case PROP_SYMBOL:
      g_value_set_string (value, unit->symbol);
      break;
    case PROP_ABBREVIATION:
      g_value_set_string (value, unit->abbreviation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* public functions */


/**
 * gimp_unit_get_id:
 * @unit: The unit you want to know the integer ID of.
 *
 * The ID can be used to retrieve the unit with [func@Unit.get_by_id].
 *
 * Note that this ID will be stable within a single session of GIMP, but
 * you should not expect this ID to stay the same across multiple runs.
 *
 * Returns: The unit's ID.
 **/
gint
gimp_unit_get_id (GimpUnit *unit)
{
  g_return_val_if_fail (GIMP_IS_UNIT (unit), -1);

  return unit->id;
}

/**
 * gimp_unit_get_name:
 * @unit: The unit you want to know the name of.
 *
 * This function returns the usual name of the unit (e.g. "inches").
 * It can be used as the long label for the unit in the interface.
 * For short labels, use [method@Unit.get_abbreviation].
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's name.
 **/
const gchar *
gimp_unit_get_name (GimpUnit *unit)
{
  g_return_val_if_fail (GIMP_IS_UNIT (unit), NULL);

  return unit->name;
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
gimp_unit_get_factor (GimpUnit *unit)
{
  g_return_val_if_fail (GIMP_IS_UNIT (unit), 1.0);

  return unit->factor;
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
 * necessary provide enough precision on high-resolution units.
 * When the information is needed for a specific unit, the use of
 * gimp_unit_get_scaled_digits() may be more appropriate.
 *
 * Returns 0 for @unit == GIMP_UNIT_PIXEL.
 *
 * Returns: The suggested number of digits.
 **/
gint
gimp_unit_get_digits (GimpUnit *unit)
{
  g_return_val_if_fail (GIMP_IS_UNIT (unit), 0);

  return unit->digits;
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
 * the @resolution may not correspond to the unit's horizontal or
 * vertical resolution, but instead to the result of:
 * `distance_in_pixel / distance_in_inch`.
 *
 * Returns: The suggested number of digits.
 **/
gint
gimp_unit_get_scaled_digits (GimpUnit *unit,
                             gdouble   resolution)
{
  gint digits;

  g_return_val_if_fail (GIMP_IS_UNIT (unit), 0);

  digits = ceil (log10 (1.0 /
                        gimp_pixels_to_units (1.0, unit, resolution)));

  return MAX (digits, gimp_unit_get_digits (unit));
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
gimp_unit_get_symbol (GimpUnit *unit)
{
  g_return_val_if_fail (GIMP_IS_UNIT (unit), NULL);

  return unit->symbol;
}

/**
 * gimp_unit_get_abbreviation:
 * @unit: The unit you want to know the abbreviation of.
 *
 * This function returns the abbreviation of the unit (e.g. "in" for
 * inches).
 * It can be used as a short label for the unit in the interface.
 * For long labels, use [method@Unit.get_name].
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's abbreviation.
 **/
const gchar *
gimp_unit_get_abbreviation (GimpUnit *unit)
{
  g_return_val_if_fail (GIMP_IS_UNIT (unit), NULL);

  return unit->abbreviation;
}

/**
 * gimp_unit_get_deletion_flag:
 * @unit: The unit you want to know the @deletion_flag of.
 *
 * Returns: The unit's @deletion_flag.
 **/
gboolean
gimp_unit_get_deletion_flag (GimpUnit *unit)
{
  g_return_val_if_fail (GIMP_IS_UNIT (unit), TRUE);

  if ((unit->id >= 0 && unit->id < GIMP_UNIT_END) ||
      unit->id == GIMP_UNIT_PERCENT)
    return FALSE;

  if (_gimp_unit_vtable.get_deletion_flag != NULL)
    /* This code path will only happen in libgimp. */
    return _gimp_unit_vtable.get_deletion_flag (unit);
  else
    return unit->delete_on_exit;
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
gimp_unit_set_deletion_flag (GimpUnit *unit,
                             gboolean  deletion_flag)
{
  g_return_if_fail (GIMP_IS_UNIT (unit));

  if ((unit->id >= 0 && unit->id < GIMP_UNIT_END) ||
      unit->id == GIMP_UNIT_PERCENT)
    return;

  unit->delete_on_exit = deletion_flag;

  if (_gimp_unit_vtable.set_deletion_flag != NULL)
    /* This code path will only happen in libgimp. */
    _gimp_unit_vtable.set_deletion_flag (unit, deletion_flag);
}

/**
 * gimp_unit_get_by_id:
 * @unit_id: The unit id.
 *
 * Returns the unique [class@Unit] object corresponding to @unit_id,
 * which is the integer identifier as returned by [method@Unit.get_id].
 *
 * Returns: (transfer none): the #GimpUnit object with ID @unit_id.
 *
 * Since: 3.0
 **/
GimpUnit *
gimp_unit_get_by_id (gint unit_id)
{
  GimpUnit *unit = NULL;

  if (unit_id < 0)
    return NULL;

  if (G_UNLIKELY (! _gimp_units))
    _gimp_units = g_hash_table_new_full (g_direct_hash,
                                         g_direct_equal,
                                         NULL,
                                         (GDestroyNotify) g_object_unref);

  unit = g_hash_table_lookup (_gimp_units, GINT_TO_POINTER (unit_id));

  if (! unit)
    {
      if (unit_id < GIMP_UNIT_END)
        {
          GimpUnitDef def = _gimp_unit_defs[unit_id];

          unit = g_object_new (GIMP_TYPE_UNIT,
                               "id",           unit_id,
                               "name",         def.identifier,
                               "factor",       def.factor,
                               "digits",       def.digits,
                               "symbol",       def.symbol,
                               "abbreviation", def.abbreviation,
                               NULL);
          unit->delete_on_exit = FALSE;
        }
      else if (unit_id == GIMP_UNIT_PERCENT)
        {
          unit = g_object_new (GIMP_TYPE_UNIT,
                               "id",           unit_id,
                               "name",         _gimp_unit_percent_def.identifier,
                               "factor",       _gimp_unit_percent_def.factor,
                               "digits",       _gimp_unit_percent_def.digits,
                               "symbol",       _gimp_unit_percent_def.symbol,
                               "abbreviation", _gimp_unit_percent_def.abbreviation,
                               NULL);
          unit->delete_on_exit = FALSE;
        }
      else if (_gimp_unit_vtable.get_data != NULL)
        {
          /* This code path should never happen in app/ where get_data()
           * is NULL, because non built-in units are created in app/
           * whereas they are only queried in libgimp.
           */
          gchar   *identifier   = NULL;
          gdouble  factor;
          gint     digits;
          gchar   *symbol       = NULL;
          gchar   *abbreviation = NULL;

          identifier = _gimp_unit_vtable.get_data (unit_id,
                                                   &factor,
                                                   &digits,
                                                   &symbol,
                                                   &abbreviation);

          if (identifier != NULL)
            unit = g_object_new (GIMP_TYPE_UNIT,
                                 "id",           unit_id,
                                 "name",         identifier,
                                 "factor",       factor,
                                 "digits",       digits,
                                 "symbol",       symbol,
                                 "abbreviation", abbreviation,
                                 NULL);

          g_free (identifier);
          g_free (symbol);
          g_free (abbreviation);
        }
      else if (_gimp_unit_vtable.get_user_unit != NULL)
        {
          /* This code path should never happen in libgimp, only in app/. */

          unit = _gimp_unit_vtable.get_user_unit (unit_id);

          if (unit != NULL)
            g_object_ref (unit);
        }

      if (unit != NULL)
        g_hash_table_insert (_gimp_units, GINT_TO_POINTER (unit_id), unit);
    }

  return unit;
}

/**
 * gimp_unit_pixel:
 *
 * Returns the unique object representing pixel unit.
 *
 * This procedure returns the unit representing pixel. The returned
 * object is unique across the whole run.
 *
 * Returns: (transfer none): The unique pixel unit.
 *
 * Since: 3.0
 **/
GimpUnit *
gimp_unit_pixel (void)
{
  return gimp_unit_get_by_id (GIMP_UNIT_PIXEL);
}

/**
 * gimp_unit_inch:
 *
 * Returns the unique object representing inch unit.
 *
 * This procedure returns the unit representing inch. The returned
 * object is unique across the whole run.
 *
 * Returns: (transfer none): The unique inch unit.
 *
 * Since: 3.0
 **/
GimpUnit *
gimp_unit_inch (void)
{
  return gimp_unit_get_by_id (GIMP_UNIT_INCH);
}

/**
 * gimp_unit_mm:
 *
 * Returns the unique object representing millimeter unit.
 *
 * This procedure returns the unit representing millimeter. The
 * returned object is unique across the whole run.
 *
 * Returns: (transfer none): The unique millimeter unit.
 *
 * Since: 3.0
 **/
GimpUnit *
gimp_unit_mm (void)
{
  return gimp_unit_get_by_id (GIMP_UNIT_MM);
}

/**
 * gimp_unit_point:
 *
 * Returns the unique object representing typographical point unit.
 *
 * This procedure returns the unit representing typographical points.
 * The returned object is unique across the whole run.
 *
 * Returns: (transfer none): The unique point unit.
 *
 * Since: 3.0
 **/
GimpUnit *
gimp_unit_point (void)
{
  return gimp_unit_get_by_id (GIMP_UNIT_POINT);
}

/**
 * gimp_unit_pica:
 *
 * Returns the unique object representing Pica unit.
 *
 * This procedure returns the unit representing Picas.
 * The returned object is unique across the whole run.
 *
 * Returns: (transfer none): The unique pica unit.
 *
 * Since: 3.0
 **/
GimpUnit *
gimp_unit_pica (void)
{
  return gimp_unit_get_by_id (GIMP_UNIT_PICA);
}

/**
 * gimp_unit_percent:
 *
 * Returns the unique object representing percent dimensions relatively
 * to an image.
 *
 * This procedure returns the unit representing typographical points.
 * The returned object is unique across the whole run.
 *
 * Returns: (transfer none): The unique percent unit.
 *
 * Since: 3.0
 **/
GimpUnit *
gimp_unit_percent (void)
{
  return gimp_unit_get_by_id (GIMP_UNIT_PERCENT);
}

/**
 * gimp_unit_is_built_in:
 * @unit: the unit.
 *
 * Returns whether the unit is built-in.
 *
 * This procedure returns @unit is a built-in unit. In particular the
 * deletion flag cannot be set on built-in units.
 *
 * Returns: Whether @unit is built-in.
 *
 * Since: 3.0
 **/
gboolean
gimp_unit_is_built_in (GimpUnit *unit)
{
  g_return_val_if_fail (GIMP_IS_UNIT (unit), FALSE);

  return (unit->id >= 0 && unit->id < GIMP_UNIT_END) || unit->id == GIMP_UNIT_PERCENT;
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
gimp_unit_is_metric (GimpUnit *unit)
{
  gdouble factor;

  if (unit == gimp_unit_mm ())
    return TRUE;

  factor = gimp_unit_get_factor (unit);

  if (factor == 0.0)
    return FALSE;

  return ((ABS (factor -  0.0254) < 1e-7) || /* m  */
          (ABS (factor -  0.254)  < 1e-6) || /* dm */
          (ABS (factor -  2.54)   < 1e-5) || /* cm */
          (ABS (factor - 25.4)    < 1e-4));  /* mm */
}

/**
 * gimp_unit_format_string:
 * @format: A printf-like format string which is used to create the unit
 *          string.
 * @unit:   A unit.
 *
 * The @format string supports the following percent expansions:
 *
 * * `%n`: Name (long label)
 * * `%a`: Abbreviation (short label)
 * * `%%`: Literal percent
 * * `%f`: Factor (how many units make up an inch)
 * * `%y`: Symbol (e.g. `''` for `GIMP_UNIT_INCH`)
 *
 * Returns: (transfer full): A newly allocated string with above percent
 *          expressions replaced with the resp. strings for @unit.
 *
 * Since: 2.8
 **/
gchar *
gimp_unit_format_string (const gchar *format,
                         GimpUnit    *unit)
{
  gchar buffer[1024];
  gint  i = 0;

  g_return_val_if_fail (GIMP_IS_UNIT (unit), NULL);
  g_return_val_if_fail (format != NULL, NULL);

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

            case 'n': /* full name */
              i += print (buffer, sizeof (buffer), i, "%s",
                          gimp_unit_get_name (unit));
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
gimp_pixels_to_units (gdouble   pixels,
                      GimpUnit *unit,
                      gdouble   resolution)
{
  g_return_val_if_fail (gimp_unit_pixel != NULL, 0.0);

  if (unit == gimp_unit_pixel ())
    return pixels;

  return pixels * gimp_unit_get_factor (unit) / resolution;
}

/**
 * gimp_units_to_pixels:
 * @value:      value in units
 * @unit:       unit of @value
 * @resolution: resolution in DPI
 *
 * Converts a @value specified in @unit to pixels.
 *
 * Returns: @value converted to pixels.
 *
 * Since: 2.8
 **/
gdouble
gimp_units_to_pixels (gdouble   value,
                      GimpUnit *unit,
                      gdouble   resolution)
{
  g_return_val_if_fail (gimp_unit_pixel != NULL, 0.0);

  if (unit == gimp_unit_pixel ())
    return value;

  return value * resolution / gimp_unit_get_factor (unit);
}

/**
 * gimp_units_to_points:
 * @value:      value in units
 * @unit:       unit of @value
 * @resolution: resolution in DPI
 *
 * Converts a @value specified in @unit to points.
 *
 * Returns: @value converted to points.
 *
 * Since: 2.8
 **/
gdouble
gimp_units_to_points (gdouble   value,
                      GimpUnit *unit,
                      gdouble   resolution)
{
  g_return_val_if_fail (gimp_unit_pixel != NULL, 0.0);
  g_return_val_if_fail (gimp_unit_point != NULL, 0.0);

  if (unit == gimp_unit_point ())
    return value;

  if (unit == gimp_unit_pixel ())
    return (value * gimp_unit_get_factor (gimp_unit_point ()) / resolution);

  return (value *
          gimp_unit_get_factor (gimp_unit_point ()) / gimp_unit_get_factor (unit));
}


/*
 * GIMP_TYPE_PARAM_UNIT
 */

#define GIMP_PARAM_SPEC_UNIT(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_UNIT, GimpParamSpecUnit))

typedef struct _GimpParamSpecUnit GimpParamSpecUnit;

struct _GimpParamSpecUnit
{
  GimpParamSpecObject  parent_instance;

  gboolean             allow_pixel;
  gboolean             allow_percent;
};

static void         gimp_param_unit_class_init  (GimpParamSpecObjectClass *klass);
static void         gimp_param_unit_init        (GParamSpec               *pspec);
static GParamSpec * gimp_param_unit_duplicate   (GParamSpec               *pspec);
static gboolean     gimp_param_unit_validate    (GParamSpec               *pspec,
                                                 GValue                   *value);

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
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GimpParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_unit_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecUnit),
        0,
        (GInstanceInitFunc) gimp_param_unit_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_OBJECT,
                                     "GimpParamUnit", &info, 0);
    }

  return type;
}

static void
gimp_param_unit_class_init (GimpParamSpecObjectClass *klass)
{
  GParamSpecClass *pclass = G_PARAM_SPEC_CLASS (klass);

  klass->duplicate          = gimp_param_unit_duplicate;

  pclass->value_type        = GIMP_TYPE_UNIT;
  pclass->value_validate    = gimp_param_unit_validate;
}

static void
gimp_param_unit_init (GParamSpec *pspec)
{
  GimpParamSpecUnit   *uspec = GIMP_PARAM_SPEC_UNIT (pspec);
  GimpParamSpecObject *ospec = GIMP_PARAM_SPEC_OBJECT (pspec);

  uspec->allow_pixel    = TRUE;
  uspec->allow_percent  = TRUE;
  ospec->_default_value = g_object_ref (G_OBJECT (gimp_unit_inch ()));
  ospec->_has_default   = TRUE;
}

static GParamSpec *
gimp_param_unit_duplicate (GParamSpec *pspec)
{
  GParamSpec        *duplicate;
  GimpParamSpecUnit *uspec;

  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_UNIT (pspec), NULL);

  uspec = GIMP_PARAM_SPEC_UNIT (pspec);
  duplicate = gimp_param_spec_unit (pspec->name,
                                    g_param_spec_get_nick (pspec),
                                    g_param_spec_get_blurb (pspec),
                                    uspec->allow_pixel,
                                    uspec->allow_percent,
                                    GIMP_UNIT (gimp_param_spec_object_get_default (pspec)),
                                    pspec->flags);

  return duplicate;
}

static gboolean
gimp_param_unit_validate (GParamSpec *pspec,
                          GValue     *value)
{
  GimpParamSpecUnit *uspec = GIMP_PARAM_SPEC_UNIT (pspec);
  GObject            *unit = value->data[0].v_pointer;

  if (unit == NULL                                                                 ||
      (! uspec->allow_percent && value->data[0].v_pointer == gimp_unit_percent ()) ||
      (! uspec->allow_pixel   && value->data[0].v_pointer == gimp_unit_pixel ()))
    {
      g_clear_object (&unit);
      value->data[0].v_pointer = g_object_ref (gimp_param_spec_object_get_default (pspec));
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_param_spec_unit:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @allow_pixel:   Whether "pixels" is an allowed unit.
 * @allow_percent: Whether "percent" is an allowed unit.
 * @default_value: Unit to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a units param.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 2.4
 **/
GParamSpec *
gimp_param_spec_unit (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     allow_pixel,
                      gboolean     allow_percent,
                      GimpUnit    *default_value,
                      GParamFlags  flags)
{
  GimpParamSpecUnit *uspec;

  g_return_val_if_fail (GIMP_IS_UNIT (default_value), NULL);

  uspec = g_param_spec_internal (GIMP_TYPE_PARAM_UNIT,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (uspec, NULL);

  uspec->allow_pixel   = allow_pixel;
  uspec->allow_percent = allow_percent;
  gimp_param_spec_object_set_default (G_PARAM_SPEC (uspec), G_OBJECT (default_value));

  return G_PARAM_SPEC (uspec);
}

/**
 * gimp_param_spec_unit_pixel_allowed:
 * @pspec: a #GParamSpec to hold an #GimpUnit value.
 *
 * Returns: %TRUE if the [func@Gimp.Unit.pixel] unit is allowed.
 *
 * Since: 3.0
 **/
gboolean
gimp_param_spec_unit_pixel_allowed (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_UNIT (pspec), FALSE);

  return GIMP_PARAM_SPEC_UNIT (pspec)->allow_pixel;
}

/**
 * gimp_param_spec_unit_percent_allowed:
 * @pspec: a #GParamSpec to hold an #GimpUnit value.
 *
 * Returns: %TRUE if the [func@Gimp.Unit.percent] unit is allowed.
 *
 * Since: 3.0
 **/
gboolean
gimp_param_spec_unit_percent_allowed (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_UNIT (pspec), FALSE);

  return GIMP_PARAM_SPEC_UNIT (pspec)->allow_percent;
}

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
