#define GIMP_TEST_PALETTE      "Bears"
#define GIMP_TEST_PALETTE_SIZE 256

static GimpValueArray *
gimp_c_test_run (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *image,
                 gint                  n_drawables,
                 GimpDrawable        **drawables,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  GimpPalette     *palette;
  GimpPalette     *palette2;
  GeglColor      **colors;
  gint             n_colors;
  GimpValueArray  *retvals;

  GIMP_TEST_START("gimp_palette_get_by_name()")
  palette = gimp_palette_get_by_name (GIMP_TEST_PALETTE);
  GIMP_TEST_END(GIMP_IS_PALETTE (palette))

  GIMP_TEST_START("gimp_palette_get_by_name() is unique")
  palette2 = gimp_palette_get_by_name (GIMP_TEST_PALETTE);
  GIMP_TEST_END(GIMP_IS_PALETTE (palette2) && palette == palette2)

  GIMP_TEST_START("gimp_palette_get_colors()")
  colors = gimp_palette_get_colors (palette);
  GIMP_TEST_END(colors != NULL && gimp_color_array_get_length (colors) == GIMP_TEST_PALETTE_SIZE)

  GIMP_TEST_START("gimp_palette_get_color_count()")
  n_colors = gimp_palette_get_color_count (palette);
  GIMP_TEST_END(n_colors == gimp_color_array_get_length (colors))

  /* Run the same tests through PDB. */

  GIMP_TEST_START("gimp-palette-get-by-name")
  retvals = gimp_procedure_run (gimp_pdb_lookup_procedure (gimp_get_pdb (), "gimp-palette-get-by-name"),
                                "name", GIMP_TEST_PALETTE, NULL);
  GIMP_TEST_END(g_value_get_enum (gimp_value_array_index (retvals, 0)) == GIMP_PDB_SUCCESS)

  GIMP_TEST_START("gimp-palette-get-by-name and gimp_palette_get_by_name() get identical result")
  palette2 = g_value_get_object (gimp_value_array_index(retvals, 1));
  GIMP_TEST_END(GIMP_IS_PALETTE (palette2) && palette == palette2)

  gimp_value_array_unref (retvals);

  GIMP_TEST_START("gimp-palette-get-colors")
  retvals = gimp_procedure_run (gimp_pdb_lookup_procedure (gimp_get_pdb (), "gimp-palette-get-colors"),
                                "palette", palette, NULL);
  GIMP_TEST_END(g_value_get_enum (gimp_value_array_index (retvals, 0)) == GIMP_PDB_SUCCESS)

  GIMP_TEST_START("gimp-palette-get-colors returns GimpColorArray")
  colors = g_value_get_boxed (gimp_value_array_index (retvals, 1));
  GIMP_TEST_END(colors != NULL && gimp_color_array_get_length (colors) == GIMP_TEST_PALETTE_SIZE)

  GIMP_TEST_START("gimp_value_array_get_color_array()")
  colors = gimp_value_array_get_color_array (retvals, 1);
  GIMP_TEST_END(colors != NULL && gimp_color_array_get_length (colors) == GIMP_TEST_PALETTE_SIZE)

  gimp_value_array_unref (retvals);

  GIMP_TEST_START("gimp-palette-get-color-count")
  retvals = gimp_procedure_run (gimp_pdb_lookup_procedure (gimp_get_pdb (), "gimp-palette-get-color-count"),
                                "palette", palette, NULL);
  GIMP_TEST_END(g_value_get_enum (gimp_value_array_index (retvals, 0)) == GIMP_PDB_SUCCESS)

  GIMP_TEST_START("gimp-palette-get-color-count returns the right number of colors")
  n_colors = g_value_get_int (gimp_value_array_index (retvals, 1));
  GIMP_TEST_END(n_colors == GIMP_TEST_PALETTE_SIZE)

  gimp_value_array_unref (retvals);

  GIMP_TEST_RETURN
}
