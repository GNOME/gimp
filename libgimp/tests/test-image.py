#!/usr/bin/env python3

NEW_IMAGE_WIDTH=1920
NEW_IMAGE_HEIGHT=2001

image = Gimp.Image.new(NEW_IMAGE_WIDTH, NEW_IMAGE_HEIGHT, Gimp.ImageBaseType.RGB)
gimp_assert('Gimp.Image.new()', type(image) == Gimp.Image)

images = Gimp.get_images()
gimp_assert('Gimp.get_images()', len(images) == 1 and images[0] == image)

text_layer = Gimp.TextLayer.new(image, "hello world", Gimp.context_get_font(), 20, Gimp.Unit.point())
gimp_assert('Gimp.TextLayer.new() with point unit', type(text_layer) == Gimp.TextLayer)

gimp_assert('Gimp.Image.InsertLayer()', image.insert_layer(text_layer, None, 0))

text_layer = Gimp.TextLayer.new(image, "hello world", Gimp.context_get_font(), 20, Gimp.Unit.pixel())
gimp_assert('Gimp.TextLayer.new() with pixel unit', type(text_layer) == Gimp.TextLayer)
gimp_assert('Gimp.Image.InsertLayer()', image.insert_layer(text_layer, None, 0))
