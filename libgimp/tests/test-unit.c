#define N_DEFAULT_USER_UNITS 6

typedef struct
{
  gdouble   factor;
  gint      digits;
  gchar    *name;
  gchar    *symbol;
  gchar    *abbreviation;
} GimpUnitDef;

static const GimpUnitDef _gimp_unit_defs[GIMP_UNIT_END] =
{
  /* pseudo unit */
  { 0.0,  0, "pixels",      "px", "px", },

  /* standard units */
  { 1.0,  2, "inches",      "''", "in", },

  { 25.4, 1, "millimeters", "mm", "mm", },

  /* professional units */
  { 72.0, 0, "points",      "pt", "pt", },

  { 6.0,  1, "picas",       "pc", "pc", },
};

static GimpValueArray *
gimp_c_test_run (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *image,
                 gint                  n_drawables,
                 GimpDrawable        **drawables,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  GimpUnit *unit;
  GimpUnit *unit2;
  gint      n_user_units = 0;
  gint      i;

  GIMP_TEST_START("gimp_unit_inch()");
  unit = gimp_unit_inch ();
  GIMP_TEST_END(GIMP_IS_UNIT (unit));

  GIMP_TEST_START("gimp_unit_inch() always returns an unique object");
  unit2 = gimp_unit_inch ();
  GIMP_TEST_END(GIMP_IS_UNIT (unit2) && unit == unit2);

  for (i = 0; i < GIMP_UNIT_END; i++)
    {
      gchar *test_name = g_strdup_printf ("Testing built-in unit %d", i);

      GIMP_TEST_START(test_name);
      g_free (test_name);

      unit = gimp_unit_get_by_id (i);

      GIMP_TEST_END(GIMP_IS_UNIT (unit) &&
                    g_strcmp0 (gimp_unit_get_name (unit), _gimp_unit_defs[i].name) == 0                 &&
                    g_strcmp0 (gimp_unit_get_symbol (unit), _gimp_unit_defs[i].symbol) == 0             &&
                    g_strcmp0 (gimp_unit_get_abbreviation (unit), _gimp_unit_defs[i].abbreviation) == 0 &&
                    gimp_unit_get_factor (unit) == _gimp_unit_defs[i].factor                            &&
                    gimp_unit_get_digits (unit)  == _gimp_unit_defs[i].digits);

      if (i == GIMP_UNIT_INCH)
        {
          GIMP_TEST_START("gimp_unit_inch() is the same as gimp_unit_get_by_id (GIMP_UNIT_INCH)");
          GIMP_TEST_END(unit == unit2);
        }
    }

  GIMP_TEST_START("Counting default user units");
  unit = gimp_unit_get_by_id (GIMP_UNIT_END);
  while (GIMP_IS_UNIT (unit))
    unit = gimp_unit_get_by_id (GIMP_UNIT_END + ++n_user_units);
  /* A bare config contains 6 defaults units in /etc/. */
  GIMP_TEST_END(n_user_units == N_DEFAULT_USER_UNITS);

  GIMP_TEST_START("gimp_unit_new()");
  unit2 = gimp_unit_new ("name", 2.0, 1, "symbol", "abbreviation");
  GIMP_TEST_END(GIMP_IS_UNIT (unit2));

  GIMP_TEST_START("Verifying the new user unit's ID");
  GIMP_TEST_END(gimp_unit_get_id (unit2) == GIMP_UNIT_END + n_user_units);

  GIMP_TEST_START("Verifying the new user unit's unicity");
  GIMP_TEST_END(unit2 == gimp_unit_get_by_id (GIMP_UNIT_END + n_user_units));

  GIMP_TEST_START("Counting again user units");
  unit = gimp_unit_get_by_id(GIMP_UNIT_END);
  n_user_units = 0;
  while (GIMP_IS_UNIT (unit))
    unit = gimp_unit_get_by_id (GIMP_UNIT_END + ++n_user_units);
  GIMP_TEST_END(n_user_units == N_DEFAULT_USER_UNITS + 1);

  GIMP_TEST_RETURN
}
