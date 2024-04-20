#define DBL(c) ((gdouble)(c) / 255.0)
#define EPSILON 1e-6

typedef struct
{
  const gchar   *str;
  gboolean       fail;
  const gdouble  r;
  const gdouble  g;
  const gdouble  b;
  const gdouble  a;
} ColorSample;

static const ColorSample samples[] =
{
  /* sample                  fail   red       green     blue     alpha */

  { "#000000",               FALSE, 0.0,      0.0,      0.0,      1.0 },
  { "#FFff00",               FALSE, 1.0,      1.0,      0.0,      1.0 },
  { "#6495ed",               FALSE, DBL(100), DBL(149), DBL(237), 1.0 },
  { "#fff",                  FALSE, 1.0,      1.0,      1.0,      1.0 },
  /* Very unsure about this one. Our code has some support for values on 3 or 4
   * hexa per channel, but this doesn't exist in CSS specs.
   * On the other hand, we should add support for alpha.
   * And in any case, the result of the below should not be (1, 0, 0).
   * See: https://drafts.csswg.org/css-color/#hex-notation
   */
  /*{ "#64649595eded",         FALSE, 1.0,      1.0,      0.0,      1.0 },*/
  { "rgb(0,0,0)",            FALSE, 0.0,      0.0,      0.0,      1.0 },
  { "rgb(100,149,237)",      FALSE, DBL(100), DBL(149), DBL(237), 1.0 },
  { "rgba(100%,0,100%,0.5)", FALSE, 1.0,      0.0,      1.0,      0.5 },
  { "rgb(100%,149,20%)",     FALSE, 1.0,      DBL(149), 0.2,      1.0 },
  { "rgb(foobar)",           TRUE,  0.0,      0.0,      0.0,      1.0 },
  { "rgb(100,149,237",       TRUE,  0.0,      0.0,      0.0,      1.0 },
  { "rED",                   FALSE, 1.0,      0.0,      0.0,      1.0 },
  { "cornflowerblue",        FALSE, DBL(100), DBL(149), DBL(237), 1.0 },
  { "    red",               FALSE, 1.0,      0.0,      0.0,      1.0 },
  { "red      ",             FALSE, 1.0,      0.0,      0.0,      1.0 },
  { "red",                   FALSE, 1.0,      0.0,      0.0,      1.0 },
  { "red  blue",             TRUE,  0.0,      0.0,      0.0,      0.0 },
  { "transparent",           FALSE, 0.0,      0.0,      0.0,      0.0 },
  { "23foobar",              TRUE,  0.0,      0.0,      0.0,      0.0 },
  { "",                      TRUE,  0.0,      0.0,      0.0,      0.0 }
};

static gint
check_failure (const ColorSample *sample,
               GeglColor         *color)
{
  gdouble red;
  gdouble green;
  gdouble blue;
  gdouble alpha;

  if (color && sample->fail)
    {
      gegl_color_get_rgba_with_space (color, &red, &green, &blue, &alpha,
                                      babl_format ("R'G'B'A double"));
      g_printerr ("\n\tParser succeeded for sample \"%s\" but should have failed!\n"
                  "\t  parsed color: (%g, %g, %g, %g)\n",
                  sample->str, red, green, blue, alpha);
      return 1;
    }
  else if (! color && ! sample->fail)
    {
      g_printerr ("\n\tParser failed for sample \"%s\" but should have succeeded!\n"
                  "\t  expected color: (%g, %g, %g, %g)\n",
                  sample->str, sample->r, sample->g, sample->b, sample->a);
      return 1;
    }
  else if (! color && sample->fail)
    {
      return 0;
    }

  gegl_color_get_rgba_with_space (color, &red, &green, &blue, &alpha,
                                  babl_format ("R'G'B'A double"));
  if (fabs (red - sample->r) > EPSILON  || fabs (green - sample->g) > EPSILON ||
      fabs (blue - sample->b) > EPSILON || fabs (alpha - sample->a) > EPSILON)
    {
      g_printerr ("\nParser succeeded for sample \"%s\" but found the wrong values!\n"
                  "  parsed color:   (%g, %g, %g, %g)\n"
                  "  expected color: (%g, %g, %g, %g)\n",
                  sample->str, red, green, blue, alpha,
                  sample->r, sample->g, sample->b, sample->a);
      return 1;
    }

  return 0;
}

static GimpValueArray *
gimp_c_test_run (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *image,
                 gint                  n_drawables,
                 GimpDrawable        **drawables,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  gint i;

  g_print ("\nTesting the GIMP color parser ...\n");

  for (i = 0; i < G_N_ELEMENTS (samples); i++)
    {
      GeglColor *color = NULL;
      gchar     *test;

      test = g_strdup_printf ("Parsing [%d] \"%s\" (should %s)",
                              i, samples[i].str,
                              samples[i].fail ? "fail" : "succeed");
      GIMP_TEST_START(test)
      color = gimp_color_parse_css (samples[i].str);

      GIMP_TEST_END(check_failure (samples + i, color) == 0)
      g_free (test);
    }

  GIMP_TEST_RETURN
}
