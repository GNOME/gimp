#!/usr/bin/env python3

N_DEFAULT_USER_UNITS=6

gimp_unit_defs = [
  # pseudo unit
  {
    'factor':       0.0,
    'digits':       0,
    'name':         "pixels",
    'symbol':       "px",
    'abbreviation': "px"
  },
  # standard units
  {
    'factor':       1.0,
    'digits':       2,
    'name':         "inches",
    'symbol':       "''",
    'abbreviation': "in"
  },
  {
    'factor':       25.4,
    'digits':       1,
    'name':         "millimeters",
    'symbol':       "mm",
    'abbreviation': "mm"
  },
  # professional units
  {
    'factor':       72.0,
    'digits':       0,
    'name':         "points",
    'symbol':       "pt",
    'abbreviation': "pt"
  },
  {
    'factor':       6.0,
    'digits':       1,
    'name':         "picas",
    'symbol':       "pc",
    'abbreviation': "pc"
  }
]

unit = Gimp.Unit.inch()
gimp_assert('Gimp.Unit.inch()', type(unit) == Gimp.Unit)

unit2 = Gimp.Unit.inch()
gimp_assert('Gimp.Unit.inch() always returns an unique object', unit == unit2)

for i in range(len(gimp_unit_defs)):
  unit = Gimp.Unit.get_by_id(i)
  unitdef = gimp_unit_defs[i]
  gimp_assert('Testing built-in unit {}'.format(i),
              type(unit) == Gimp.Unit                            and \
              unit.get_name() == unitdef['name']                 and \
              unit.get_symbol() == unitdef['symbol']             and \
              unit.get_abbreviation() == unitdef['abbreviation'] and \
              unit.get_factor() == unitdef['factor']             and \
              unit.get_digits() == unitdef['digits'])

  if i == Gimp.UnitID.INCH:
    gimp_assert('Gimp.Unit.inch() is the same as Gimp.Unit.get_by_id (Gimp.UnitID.INCH)',
                unit == unit2)

unit = Gimp.Unit.get_by_id(Gimp.UnitID.END)
n_user_units = 0
while unit is not None:
  n_user_units += 1
  unit = Gimp.Unit.get_by_id(Gimp.UnitID.END + n_user_units)
gimp_assert('Counting default user units', n_user_units == N_DEFAULT_USER_UNITS)

unit2 = Gimp.Unit.new ("name", 2.0, 1, "symbol", "abbreviation");
gimp_assert('Gimp.Unit.new()', type(unit2) == Gimp.Unit)

gimp_assert("Verifying the new user unit's ID", unit2.get_id() == Gimp.UnitID.END + n_user_units)
gimp_assert("Verifying the new user unit's unicity", unit2 == Gimp.Unit.get_by_id(Gimp.UnitID.END + n_user_units))

unit = Gimp.Unit.get_by_id(Gimp.UnitID.END)
n_user_units = 0
while unit is not None:
  n_user_units += 1
  unit = Gimp.Unit.get_by_id(Gimp.UnitID.END + n_user_units)
gimp_assert('Counting again user units', n_user_units == N_DEFAULT_USER_UNITS + 1)
