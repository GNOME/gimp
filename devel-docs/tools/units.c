
#include "config.h"

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpbase-private.h"

#include "units.h"


typedef struct
{
  gdouble      factor;
  gint         digits;
  const gchar *identifier;
  const gchar *symbol;
  const gchar *abbreviation;
  const gchar *singular;
  const gchar *plural;
} GimpUnitDef;

static const GimpUnitDef unit_defs[] =
{
  {  0.0, 0, "pixels",      "px", "px", "pixel",      "pixels"      },
  {  1.0, 2, "inches",      "''", "in", "inch",       "inches"      },
  { 25.4, 1, "millimeters", "mm", "mm", "millimeter", "millimeters" }
};


static gint
units_get_number_of_units (void)
{
  return G_N_ELEMENTS (unit_defs);
}

static gint
units_get_number_of_built_in_units (void)
{
  return G_N_ELEMENTS (unit_defs);
}

static gdouble
units_unit_get_factor (GimpUnit unit)
{
  return unit_defs[unit].factor;
}

static gint
units_unit_get_digits (GimpUnit unit)
{
  return unit_defs[unit].digits;
}

static const gchar *
units_unit_get_identifier (GimpUnit unit)
{
  return unit_defs[unit].identifier;
}

static const gchar *
units_unit_get_symbol (GimpUnit unit)
{
  return unit_defs[unit].symbol;
}

static const gchar *
units_unit_get_abbreviation (GimpUnit unit)
{
  return unit_defs[unit].abbreviation;
}

static const gchar *
units_unit_get_singular (GimpUnit unit)
{
  return unit_defs[unit].singular;
}

static const gchar *
units_unit_get_plural (GimpUnit unit)
{
  return unit_defs[unit].plural;
}

void
units_init (void)
{
  GimpUnitVtable vtable;

  vtable.unit_get_number_of_units          = units_get_number_of_units;
  vtable.unit_get_number_of_built_in_units = units_get_number_of_built_in_units;
  vtable.unit_new                          = NULL;
  vtable.unit_get_factor                   = units_unit_get_factor;
  vtable.unit_get_digits                   = units_unit_get_digits;
  vtable.unit_get_identifier               = units_unit_get_identifier;
  vtable.unit_get_symbol                   = units_unit_get_symbol;
  vtable.unit_get_abbreviation             = units_unit_get_abbreviation;
  vtable.unit_get_singular                 = units_unit_get_singular;
  vtable.unit_get_plural                   = units_unit_get_plural;

  gimp_base_init (&vtable);
}
