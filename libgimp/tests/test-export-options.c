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
  GimpImage         **images;
  GimpLayer         **layers;
  GeglBufferIterator *iter;
  GeglBuffer         *buffer1;
  GeglBuffer         *buffer2;
  const Babl         *format;
  GimpImage          *new_image;
  GimpImage          *original_image;
  GimpTextLayer      *text_layer;
  GimpLayer          *layer;
  GimpLayer          *export_layer;
  GimpExportOptions  *options;
  GimpExportReturn    delete;
  gint                n_images;
  gint                n_layers;
  gboolean            identical_buffers;

  new_image = gimp_image_new (NEW_IMAGE_WIDTH, NEW_IMAGE_HEIGHT, GIMP_RGB);
  text_layer = gimp_text_layer_new (new_image, "hello world", gimp_context_get_font (),
                                    20, gimp_unit_point ());
  gimp_image_insert_layer (new_image, GIMP_LAYER (text_layer), NULL, 0);
  text_layer = gimp_text_layer_new (new_image, "annyeong uju", gimp_context_get_font (),
                                    20, gimp_unit_point ());
  gimp_image_insert_layer (new_image, GIMP_LAYER (text_layer), NULL, 0);

  options = g_object_new (GIMP_TYPE_EXPORT_OPTIONS,
                          "capabilities", GIMP_EXPORT_CAN_HANDLE_RGB | GIMP_EXPORT_CAN_HANDLE_ALPHA,
                          NULL);

  GIMP_TEST_START("Verify start state (1)");
  images = gimp_get_images (&n_images);
  GIMP_TEST_END(n_images == 1 && images[0] == new_image);
  g_free (images);

  GIMP_TEST_START("Verify start state (2)");
  layers = gimp_image_get_layers (new_image, &n_layers);
  GIMP_TEST_END(n_layers == 2);
  g_free (layers);

  original_image = new_image;

  GIMP_TEST_START("gimp_export_options_get_image() created a new image");
  delete = gimp_export_options_get_image (options, &new_image);
  images = gimp_get_images (&n_images);
  GIMP_TEST_END(delete == GIMP_EXPORT_EXPORT && n_images == 2 && new_image != original_image);
  g_free (images);

  GIMP_TEST_START("The new image has a single layer");
  layers = gimp_image_get_layers (new_image, &n_layers);
  GIMP_TEST_END(n_layers == 1);

  export_layer = layers[0];
  g_free (layers);

  layer = gimp_image_merge_visible_layers (original_image, GIMP_CLIP_TO_IMAGE);

  GIMP_TEST_START("Compare export buffer with original image's merged buffer");
  buffer1 = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
  buffer2 = gimp_drawable_get_buffer (GIMP_DRAWABLE (export_layer));
  format  = gegl_buffer_get_format (buffer1);
  iter = gegl_buffer_iterator_new (buffer1, NULL, 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add (iter, buffer2, NULL, 0, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  identical_buffers = TRUE;
  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *src1   = (const gfloat *) iter->items[0].data;
      const gfloat *src2   = (const gfloat *) iter->items[1].data;
      gint          count  = iter->length;

      if (memcmp (src1, src2, count * babl_format_get_bytes_per_pixel (format)) != 0)
        {
          identical_buffers = FALSE;
          break;
        }
    }
  GIMP_TEST_END(identical_buffers == TRUE);


  GIMP_TEST_RETURN
}
