
static GimpValueArray *
gimp_c_test_run (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *image,
                 GimpDrawable        **drawables,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  GimpImage     *img;
  GimpDrawable  *layer1;
  GimpDrawable  *layer2;
  GimpDrawable  *group1;
  GimpLayer     *float_layer;

  /* Setup */

  GIMP_TEST_START("gimp_image_new()")
  img = gimp_image_new (32, 32, GIMP_RGB);
  GIMP_TEST_END(GIMP_IS_IMAGE (img))

  layer1 = GIMP_DRAWABLE (gimp_layer_new (img, "layer1", 20, 10,
                          GIMP_RGBA_IMAGE, 100.0, GIMP_LAYER_MODE_NORMAL));
  layer2 = GIMP_DRAWABLE (gimp_layer_new (img, "layer2", 10, 20,
                          GIMP_RGBA_IMAGE, 100.0, GIMP_LAYER_MODE_NORMAL));
  group1 = GIMP_DRAWABLE (gimp_group_layer_new (img, NULL));

  GIMP_TEST_START("insert layer")
  GIMP_TEST_END(gimp_image_insert_layer (img, GIMP_LAYER (layer1), NULL, 0))

  GIMP_TEST_START("insert group layer")
  GIMP_TEST_END(gimp_image_insert_layer (img, GIMP_LAYER (group1), NULL, -1))

  GIMP_TEST_START("insert layer inside group")
  GIMP_TEST_END(gimp_image_insert_layer (img, GIMP_LAYER (layer2),
                GIMP_LAYER (group1), -1))


  /* Floating selection tests */

  /* 1. Fail with no selection */
  GIMP_TEST_START("Gimp.Selection.float - no selection")
  GIMP_TEST_END(gimp_selection_float (img, (GimpDrawable *[2]) { layer1, NULL }, 10, 10) == NULL)

  /* 2. Fail on a group layer */
  GIMP_TEST_START("Gimp.Selection.float - group layer")
  GIMP_TEST_END(gimp_selection_float (img, (GimpDrawable *[2]) { group1, NULL }, 10, 10) == NULL)

  /* 3. Succeed on a normal layer */
  gimp_image_select_rectangle (img, GIMP_CHANNEL_OP_REPLACE, 5, 5, 20, 20);

  GIMP_TEST_START("gimp_selection_float - normal layer")
  float_layer = gimp_selection_float (img, (GimpDrawable *[2]) { layer1, NULL }, 10, 10);
  GIMP_TEST_END(GIMP_IS_LAYER (float_layer))

  GIMP_TEST_START("gimp_floating_sel_remove")
  GIMP_TEST_END(gimp_floating_sel_remove (float_layer) == TRUE);

  /* 4. Succeed on a layer inside a group */
  gimp_image_select_rectangle (img, GIMP_CHANNEL_OP_REPLACE, 5, 5, 20, 20);

  GIMP_TEST_START("gimp_selection_float - layer inside group")
  float_layer = gimp_selection_float (img, (GimpDrawable *[2]) { layer2, NULL }, 10, 10);
  GIMP_TEST_END(GIMP_IS_LAYER (float_layer))

  GIMP_TEST_START("gimp_floating_sel_remove")
  GIMP_TEST_END(gimp_floating_sel_remove (float_layer) == TRUE);


  /* Teardown */
  gimp_image_delete (img);

  GIMP_TEST_RETURN
}
