#!/usr/bin/env python3

NEW_IMAGE_WIDTH=1920
NEW_IMAGE_HEIGHT=2001

image = Gimp.Image.new(NEW_IMAGE_WIDTH, NEW_IMAGE_HEIGHT, Gimp.ImageBaseType.RGB)
text_layer = Gimp.TextLayer.new(image, "hello world", Gimp.context_get_font(), 20, Gimp.Unit.point())
image.insert_layer(text_layer, None, 0)
text_layer = Gimp.TextLayer.new(image, "annyeong uju", Gimp.context_get_font(), 20, Gimp.Unit.point())
image.insert_layer(text_layer, None, 0)

images = Gimp.get_images()
layers = image.get_layers()
gimp_assert('Verify start state', len(images) == 1 and images[0] == image and len(layers) == 2)

options = Gimp.ExportOptions()
options.set_property ("capabilities",
                      Gimp.ExportCapabilities.CAN_HANDLE_RGB |
                      Gimp.ExportCapabilities.CAN_HANDLE_ALPHA)

delete, export_image = options.get_image(image)

gimp_assert('Gimp.ExportOptions.get_image() created a new image', delete == Gimp.ExportReturn.EXPORT and export_image != image)
export_layers = export_image.get_layers()
gimp_assert('The new image has a single layer', len(export_layers) == 1)

merged_layer = image.merge_visible_layers(Gimp.MergeType.CLIP_TO_IMAGE)
merged_layer.resize_to_image_size()

buffer1 = merged_layer.get_buffer()
buffer2 = export_layers[0].get_buffer()
l1 = buffer1.get(buffer1.get_extent(), 1.0, None, Gegl.AbyssPolicy.NONE)
l2 = buffer2.get(buffer2.get_extent(), 1.0, None, Gegl.AbyssPolicy.NONE)
gimp_assert("Compare export buffer with original image's merged buffer", l1 == l2)
