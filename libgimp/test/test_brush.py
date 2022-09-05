
"""
Python scripts to test GimpBrush class of libgimp API.

See comments about usage and setup in test-resource-class.py

Setup: "2. Hardness 050" brush is selected.

Testing context_set_brush
and a few other tests of class methods
are in test_resource_class.py

FUTURE: revise comparison of floats to within epsilon
"""

"""
Test a known, stock, parametric brush "2. Hardness 050"
********************************************************************************

Numeric literals might change if gimp devs change defaults.
"""
brush_default = Gimp.context_get_brush()
print( brush_default.get_id() )
assert brush_default.get_id() == "2. Hardness 050"

assert brush_default.is_generated()
assert not brush_default.is_editable()

"""
Test getters of default, stock (not editable), parametric brush
"""
success, width, height, mask_bpp, color_bpp = brush_default.get_info()
print( width, height, mask_bpp, color_bpp )
assert success
assert width     == 51
assert height    == 51
assert mask_bpp  == 1
# !!! Since parametric, not pixmap and color_bpp is zero
assert color_bpp == 0

success, width, height, mask_bpp, mask,  color_bpp, pixels = brush_default.get_pixels()
print( width, height, mask_bpp, len(mask), color_bpp, len(pixels) )
assert success
assert width     == 51
assert height    == 51
assert mask_bpp  == 1
# !!! Since parametric, not pixmap and color_bpp is zero
assert color_bpp == 0
assert len(mask)   == 2601
assert len(pixels) == 0

# !!! spacing does not return success, only a value
spacing = brush_default.get_spacing()
print(spacing)
assert spacing == 10

# shape, radius, spikes, hardness, aspect_ratio, angle
success, shape = brush_default.get_shape()
assert success
print(shape)
assert shape == Gimp.BrushGeneratedShape.CIRCLE

success, radius = brush_default.get_radius()
assert success
print(radius)
assert radius == 25.0

success, spikes = brush_default.get_spikes()
assert success
print(spikes)
assert spikes == 2

success, hardness = brush_default.get_hardness()
assert success
print(hardness)
assert hardness == 0.5

success, aspect_ratio = brush_default.get_aspect_ratio()
assert success
print(aspect_ratio)
assert aspect_ratio == 1.0

success, angle = brush_default.get_angle()
assert success
print(angle)
assert angle == 0.0


"""
Test set_spacing fails on not editable brush.
"""
success = brush_default.set_spacing(1)
assert not success
# spacing did not change
assert brush_default.get_spacing() == 10


"""
Test setters fail on non-editable brush.

When they fail, they return a value that is nullish
"""
success, returned_shape = brush_default.set_shape(Gimp.BrushGeneratedShape.DIAMOND)
print(success, returned_shape)
assert not success
assert returned_shape is Gimp.BrushGeneratedShape.CIRCLE

success, returned_radius = brush_default.set_radius(1.0)
print(success, returned_radius)
assert not success
assert returned_radius == 0.0

success, returned_spikes = brush_default.set_spikes(1.0)
print(success, returned_spikes)
assert not success
assert returned_spikes == 0

success, returned_hardness = brush_default.set_hardness(1.0)
print(success, returned_hardness)
assert not success
assert returned_hardness == 0.0

success, returned_aspect_ratio = brush_default.set_aspect_ratio(1.0)
print(success, returned_aspect_ratio)
assert not success

success, returned_angle = brush_default.set_angle(1.0)
assert not success


"""
Test non-parametric brush i.e. raster, and stock i.e. not editable
********************************************************************************

No method exists to get a specific brush.
But we can set the ID of a new brush.
"""
brush_raster = Gimp.Brush()
brush_raster.set_property("id", "Acrylic 01")
assert brush_raster.get_id()  == "Acrylic 01"
assert brush_raster.is_valid()

# raster brush is not parametric
assert not brush_raster.is_generated()

# This brush is not editable
assert not brush_raster.is_editable()

# Raster brush has spacing
spacing = brush_raster.get_spacing()
print(spacing)
assert spacing == 25

# Getters of attributes of parametric brush fail for raster brush
success, shape = brush_raster.get_shape()
assert not success
success, radius = brush_raster.get_radius()
assert not success
success, spikes = brush_raster.get_spikes()
assert not success
success, hardness = brush_raster.get_hardness()
assert not success
success, aspect_ratio = brush_raster.get_aspect_ratio()
assert not success
success, angle = brush_raster.get_angle()
assert not success


"""
Test raster brush of kind color has pixel data.
"""
brush_color = Gimp.Brush()
brush_color.set_property("id", "z Pepper")
assert brush_color.is_valid()
success, width, height, mask_bpp, mask,  color_bpp, pixels = brush_color.get_pixels()
print( width, height, mask_bpp, len(mask), color_bpp, len(pixels) )
assert success
# !!! Since is color and raster, has pixmap and color_bpp is non-zero
assert color_bpp == 3
assert len(pixels) > 0

"""
Test setting spacing on not editable, raster brush
"""
success = brush_raster.set_spacing(1)
assert not success
# spacing did not change
assert brush_raster.get_spacing() == 25


"""
Test setters fails on raster brush.
"""
success, returned_shape = brush_raster.set_shape(Gimp.BrushGeneratedShape.DIAMOND)
print(success, returned_shape)
assert not success
# Returned value is integer 0, corresponding to CIRCLE enum
assert returned_shape is Gimp.BrushGeneratedShape.CIRCLE

success, returned_radius = brush_raster.set_radius(1.0)
assert not success
success, returned_spikes = brush_raster.set_spikes(1.0)
assert not success
success, returned_hardness = brush_raster.set_hardness(1.0)
assert not success
success, returned_aspect_ratio = brush_raster.set_aspect_ratio(1.0)
assert not success
success, returned_angle = brush_raster.set_angle(1.0)
assert not success

"""
Test new brush
********************************************************************************

Parametric and editable
"""

brush_new = Gimp.Brush.new("New Brush")

# is generated and editable
assert brush_new.is_generated()
assert brush_new.is_editable()

"""
Test setters on editable brush.
"""
success = brush_new.set_spacing(20)
assert success == True
spacing = brush_new.get_spacing()
assert spacing == 20

success, returned_shape = brush_new.set_shape(Gimp.BrushGeneratedShape.DIAMOND)
print(success)
assert success == True
assert returned_shape == Gimp.BrushGeneratedShape.DIAMOND
success, shape = brush_new.get_shape()
assert success == True
assert shape == Gimp.BrushGeneratedShape.DIAMOND

success, returned_radius = brush_new.set_radius(4.0)
assert success == True
print(returned_radius)
assert returned_radius == 4.0
success, radius = brush_new.get_radius()
assert success == True
assert radius == 4.0

success, returned_spikes = brush_new.set_spikes(2)
assert success == True
assert returned_spikes == 2
success, spikes = brush_new.get_spikes()
assert success == True
assert spikes == 2

success, returned_hardness = brush_new.set_hardness(0.5)
assert success == True
print(returned_hardness)
assert returned_hardness == 0.5
success, hardness = brush_new.get_hardness()
assert success == True
assert hardness == 0.5

success, returned_aspect_ratio = brush_new.set_aspect_ratio(5.0)
assert success == True
print(returned_aspect_ratio)
assert returned_aspect_ratio == 5.0
success, aspect_ratio = brush_new.get_aspect_ratio()
assert success == True
assert aspect_ratio == 5.0

success, returned_angle = brush_new.set_angle(90.0)
assert success == True
print(returned_angle)
assert returned_angle == 90.0
success, angle = brush_new.get_angle()
assert success == True
assert angle == 90.0

"""
Test invalid enum for shape.
"""
# TODO it yields a TypeError

"""
Test clamping at upper limits.
"""

'''
Omitting this test for now.  Possible bug.
The symptoms are that it takes a long time,
and then GIMP crashes.

success, returned_radius = brush_new.set_radius(40000)
assert success == True
# upper clamp
assert returned_radius == 32767.0
success, radius = brush_new.get_radius()
assert success == True
assert radius == 32767.0
'''

success, returned_spikes = brush_new.set_spikes(22)
assert success == True
assert returned_spikes == 20
success, spikes = brush_new.get_spikes()
assert success == True
assert spikes == 20

success, returned_hardness = brush_new.set_hardness(2.0)
assert success == True
assert returned_hardness == 1.0
success, hardness = brush_new.get_hardness()
assert success == True
assert hardness == 1.0

success, returned_aspect_ratio = brush_new.set_aspect_ratio(2000)
assert success == True
assert returned_aspect_ratio == 1000.0
success, aspect_ratio = brush_new.get_aspect_ratio()
assert success == True
assert aspect_ratio == 1000.0

success, returned_angle = brush_new.set_angle(270)
assert success == True
assert returned_angle == 90
success, angle = brush_new.get_angle()
assert success == True
assert angle == 90



"""
Cleanup
"""
success = brush_new.delete()
assert success == True
