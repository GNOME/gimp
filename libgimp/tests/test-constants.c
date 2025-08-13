/* Test for constants in our Gobject Introspection API to make sure they are
 * available.
 */

static GimpValueArray *
gimp_c_test_run (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *image,
                 GimpDrawable        **drawables,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  gint value;


  /* Since this is really about the gobject-introspection data, I'm not
   * adding all constants here.
   */
  GIMP_TEST_START("GIMP_MAJOR_VERSION");
  value = GIMP_MAJOR_VERSION;
  GIMP_TEST_END(value == GIMP_MAJOR_VERSION);

  GIMP_TEST_RETURN
}
