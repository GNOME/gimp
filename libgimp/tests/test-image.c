/* Sometimes odd dimensions may create weird situations. It's usually a
 * good idea to test both even and odd dimensions.
 */
#define NEW_IMAGE_WIDTH  1920
#define NEW_IMAGE_HEIGHT 2001

static GimpValueArray *
gimp_c_test_run (GimpProcedure        *procedure,
                 GimpRunMode           run_mode,
                 GimpImage            *image,
                 gint                  n_drawables,
                 GimpDrawable        **drawables,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  GimpImage     **images;
  GimpImage      *new_image;
  GimpTextLayer  *text_layer;
  gint            n_images;

  GIMP_TEST_START("gimp_image_new()");
  new_image = gimp_image_new (NEW_IMAGE_WIDTH, NEW_IMAGE_HEIGHT, GIMP_RGB);
  GIMP_TEST_END(GIMP_IS_IMAGE (new_image));

  GIMP_TEST_START("gimp_get_images()");
  images = gimp_get_images (&n_images);
  GIMP_TEST_END(n_images == 1 && images[0] == new_image);
  g_free (images);

  GIMP_TEST_START("gimp_text_layer_new() with point unit");
  text_layer = gimp_text_layer_new (new_image, "hello world", gimp_context_get_font (),
                                    20, gimp_unit_point ());
  GIMP_TEST_END(GIMP_IS_TEXT_LAYER (text_layer));

  GIMP_TEST_START("gimp_image_insert_layer()");
  GIMP_TEST_END(gimp_image_insert_layer (new_image, GIMP_LAYER (text_layer), NULL, 0));

  GIMP_TEST_START("gimp_text_layer_new() with pixel unit");
  text_layer = gimp_text_layer_new (new_image, "hello world", gimp_context_get_font (),
                                    20, gimp_unit_pixel ());
  GIMP_TEST_END(GIMP_IS_TEXT_LAYER (text_layer));

  GIMP_TEST_START("gimp_image_insert_layer()");
  GIMP_TEST_END(gimp_image_insert_layer (new_image, GIMP_LAYER (text_layer), NULL, 0));

  GIMP_TEST_RETURN
}
