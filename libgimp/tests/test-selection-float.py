#!/usr/bin/env python3

image = Gimp.Image.new(32,32,Gimp.ImageBaseType.RGB)
gimp_assert('Gimp.Image.new', image is not None)

layer1 = Gimp.Layer.new(image, "layer1", 20, 10,
                        Gimp.ImageType.RGBA_IMAGE, 100.0,
                        Gimp.LayerMode.NORMAL)
image.insert_layer(layer1,None,0)

group1 = Gimp.GroupLayer.new(image, None)
image.insert_layer(group1,None,-1)

layer2 = Gimp.Layer.new(image, "layer2", 10, 20,
                        Gimp.ImageType.RGBA_IMAGE, 100.0,
                        Gimp.LayerMode.NORMAL)

gimp_assert('insert layer inside group', image.insert_layer(layer2,group1,-1) == True)

# Make floating selection

# 1. Fail with no selection
gimp_assert('Gimp.Selection.float - no selection',
            Gimp.Selection.float(image,[layer1],10,10) is None)

# 2. Fail on a group layer
gimp_assert('Gimp.Selection.float - group layer',
            Gimp.Selection.float(image,[group1],10,10) is None)

# Create a selection
image.select_rectangle(Gimp.ChannelOps.REPLACE, 5, 5, 20, 20)

# 3. Succeed on a normal layer
gimp_assert('take selected layers: layer1',
            image.take_selected_layers([layer1]) is True)
float1 = Gimp.Selection.float(image,[layer1],10,10)
gimp_assert('Gimp.Selection.float - normal layer',
            float1 is not None)
gimp_assert('Remove float1 layer from image',
            Gimp.floating_sel_remove(float1) is True)

# Create a selection
image.select_rectangle(Gimp.ChannelOps.REPLACE, 5, 5, 20, 20)

# 4. Succeed on a layer inside a group
gimp_assert('take selected layers: layer2',
            image.take_selected_layers([layer2]) is True)
sel_drawables = image.get_selected_drawables()
gimp_assert('selected drawables', sel_drawables is not None)
float2 = Gimp.Selection.float(image,sel_drawables,10,10)
gimp_assert('Gimp.Selection.float - layer inside group', float2 is not None)
gimp_assert('Remove float2 layer from image',
            Gimp.floating_sel_remove(float2) is True)
