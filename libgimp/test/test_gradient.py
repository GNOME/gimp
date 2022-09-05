
"""
Python scripts to test GimpGradient class of libgimp API.

See comments about usage and setup in test-resource-class.py

Setup: "FG to BG (RGB)" gradient is selected.

Testing context_get_foo is in test_resource_class.py
"""


"""
Create artifacts needed for testing.
"""
success, foreground_color = Gimp.context_get_foreground()
assert success
success, background_color = Gimp.context_get_background()
assert success


"""
Test a known, stock resource named 'FG to BG (RGB)'

!!! Test numeric literals must change if GIMP developers change the stock gradient.
"""
gradient_FG_BG  = Gimp.context_get_gradient()
print(gradient_FG_BG.get_id())
assert gradient_FG_BG.get_id() == 'FG to BG (RGB)'
assert gradient_FG_BG.is_valid() == True

print( gradient_FG_BG.get_number_of_segments() )
assert gradient_FG_BG.get_number_of_segments() == 1

print( gradient_FG_BG.segment_get_blending_function(0) )
success, blending_function = gradient_FG_BG.segment_get_blending_function(0)
assert success
assert blending_function == Gimp.GradientSegmentType.LINEAR

print( gradient_FG_BG.segment_get_coloring_type(0) )
success, coloring_type = gradient_FG_BG.segment_get_coloring_type(0)
assert success
assert coloring_type == Gimp.GradientSegmentColor.RGB



"""
Test stock gradient permissions:
    is not editable
    setters fail
    delete fails
"""
# Stock gradient is not editable
assert not gradient_FG_BG.is_editable()

# Stock gradient is not editable so set color fails and has no effect
# The initial opacity is 100.0 percent
success, left_color, opacity = gradient_FG_BG.segment_get_left_color(0)
assert opacity == 100.0
# Attempt to change opacity to 0 fails
assert not gradient_FG_BG.segment_set_left_color(0, background_color, 0.0)
# opacity is unchanged
success, left_color, opacity = gradient_FG_BG.segment_get_left_color(0)
assert opacity == 100.0

# all other setters return error
assert not gradient_FG_BG.segment_set_right_color(0, background_color, 0.0)

success, position = gradient_FG_BG.segment_set_left_pos(0, 0.0)
assert not success
success, position = gradient_FG_BG.segment_set_right_pos(0, 0.0)
assert not success
success, position = gradient_FG_BG.segment_set_middle_pos(0, 0.0)
assert not success

# There is no setter for a single segment, use a range
assert not gradient_FG_BG.segment_range_set_coloring_type    (0, 0, Gimp.GradientSegmentColor.RGB)
assert not gradient_FG_BG.segment_range_set_blending_function(0, 0, Gimp.GradientSegmentType.LINEAR)

# Cannot delete stock gradient
assert not gradient_FG_BG.delete()


"""
Test sampling of gradient.
"""
success, samples = gradient_FG_BG.get_uniform_samples(3, False)
print(samples)
assert success
# Each sample is a color quadruple RGBA
assert len(samples) == 12

# list of 3 sample positions, reverse the gradient
success, samples = gradient_FG_BG.get_custom_samples( [0.0, 0.5, 1.0], True)
assert success
# Each sample is a color quadruple RGBA
assert len(samples) == 12


"""
Test segment methods getters.
"""
success, left_color, opacity = gradient_FG_BG.segment_get_left_color(0)
print(left_color.r, left_color.g, left_color.b, left_color.a)
assert success
# Not supported: assert left_color == foreground_color
# black 0,0,0,1 opacity 1.0
assert left_color.r == 0.0
assert left_color.g == 0.0
assert left_color.b == 0.0
assert left_color.a == 1.0
assert opacity == 100.0

success, right_color, opacity = gradient_FG_BG.segment_get_right_color(0)
print(right_color.r, right_color.g, right_color.b, right_color.a)
assert success
# white 1,1,1,1  opacity  1.0
assert right_color.r == 1.0
assert right_color.g == 1.0
assert right_color.b == 1.0
assert right_color.a == 1.0
assert opacity == 100.0

success, left_pos = gradient_FG_BG.segment_get_left_pos(0)
print(left_pos)
assert left_pos == 0.0

success, right_pos = gradient_FG_BG.segment_get_right_pos(0)
print(right_pos)
assert right_pos == 1.0

success, middle_pos = gradient_FG_BG.segment_get_middle_pos(0)
print(middle_pos)
assert middle_pos == 0.5

success, blending_function = gradient_FG_BG.segment_get_blending_function(0)
assert success
assert blending_function == Gimp.GradientSegmentType.LINEAR

success, coloring_type = gradient_FG_BG.segment_get_coloring_type (0)
assert success
assert coloring_type == Gimp.GradientSegmentColor.RGB

"""
Test new gradient.
"""
gradient_new = Gimp.Gradient.new("New Gradient")

# A new gradient has the requested name.
# Assuming it does not exist already because the tester followed setup correctly
assert gradient_new.get_id() == "New Gradient"

# a new gradient is editable
assert gradient_new.is_editable() == True

# a new gradient has one segment
assert gradient_new.get_number_of_segments() == 1

# attributes of a new gradient are defaulted.
success, left_color, opacity = gradient_FG_BG.segment_get_left_color(0)
print(left_color.r, left_color.g, left_color.b, left_color.a, opacity)

success, right_color, opacity = gradient_FG_BG.segment_get_right_color(0)
print(right_color.r, right_color.g, right_color.b, right_color.a, opacity)


"""
Test segment methods setters.
On the new gradient, which is editable.
"""

# setter succeeds
assert gradient_new.segment_set_left_color(0, background_color, 50.0)
# it changed the color from foreground black to background white
success, left_color, opacity = gradient_new.segment_get_left_color(0)
print(left_color.r, left_color.g, left_color.b, left_color.a)
assert left_color.r == 1.0
assert left_color.g == 1.0
assert left_color.b == 1.0
assert left_color.a == 0.5  # !!! opacity changes the alpha
assert opacity == 50.0

assert gradient_new.segment_set_right_color(0, foreground_color, 50.0)
success, right_color, opacity = gradient_new.segment_get_right_color(0)
print(right_color.r, right_color.g, right_color.b, right_color.a)
assert right_color.r == 0.0
assert right_color.g == 0.0
assert right_color.b == 0.0
assert right_color.a == 0.5
assert opacity == 50.0

success, left_pos = gradient_new.segment_set_left_pos(0, 0.01)
print(left_pos)
assert left_pos == 0.0

success, right_pos = gradient_new.segment_set_right_pos(0, 0.99)
print(right_pos)
assert right_pos == 1.0

success, middle_pos = gradient_new.segment_set_middle_pos(0, 0.49)
print(middle_pos)
assert middle_pos == 0.49

# There is no setter for a single segment, use a range
# Change to a different enum from existing
assert gradient_new.segment_range_set_coloring_type    (0, 0, Gimp.GradientSegmentColor.HSV_CW)
success, coloring_type = gradient_new.segment_get_coloring_type (0)
assert success
assert coloring_type == Gimp.GradientSegmentColor.HSV_CW

assert gradient_new.segment_range_set_blending_function(0, 0, Gimp.GradientSegmentType.CURVED)
success, blending_function = gradient_new.segment_get_blending_function (0)
assert success
assert blending_function ==  Gimp.GradientSegmentType.CURVED


"""
Test segment range methods for splitting and flipping.

!!! Not fully testing the effect, whether segments where shuffled properly.
TODO test a range was flipped by testing for different color types
"""
assert gradient_new.segment_range_split_midpoint(0, 0)
# split one into two
assert gradient_new.get_number_of_segments() == 2

assert gradient_new.segment_range_flip(0, 1)
# flipping does not change count of segments
assert gradient_new.get_number_of_segments() == 2

assert gradient_new.segment_range_replicate(0, 1, 2)
# replicate two segments, first and second.
# replicate each into two, into four total
print( gradient_new.get_number_of_segments() )
assert gradient_new.get_number_of_segments() == 4

assert gradient_new.segment_range_split_midpoint(3, 3)
# We split last one of four, now have 5
assert gradient_new.get_number_of_segments() == 5

assert gradient_new.segment_range_split_midpoint(0, 0)
# We split first one of five, now have 6
assert gradient_new.get_number_of_segments() == 6

assert gradient_new.segment_range_split_uniform(1, 1, 3)
# We split second one of six into 3 parts, now have 8
assert gradient_new.get_number_of_segments() == 8

assert gradient_new.segment_range_delete(6, 6)
# We deleted seventh one of 8, not have seven
assert gradient_new.get_number_of_segments() == 7

# Move segment one left (minus), not compressing
actual_delta = gradient_new.segment_range_move(1, 1, -1.0, False)
print(actual_delta)
assert actual_delta == -0.0637499999
# Moving does not change the count of segments
assert gradient_new.get_number_of_segments() == 7


"""
Test the segment range functions that operate across the range.

TODO test the effect
"""
assert gradient_new.segment_range_redistribute_handles(0, 5)

assert gradient_new.segment_range_blend_colors(1, 4)

assert gradient_new.segment_range_blend_opacity(2, 3)


"""
Test segment methods setters: out of range.
"""
# Segment indexes that do not exist.
assert not gradient_new.segment_set_left_color(9, background_color, 50.0)
# This yields gimp message, so don't always test
# assert not gradient_new.segment_set_left_color(-1, background_color, 50.0)


"""
Test delete gradient

Also cleans up artifacts of testing: the new gradient.
Otherwise it persists and interferes with repeated testing.
"""
gradient_new.delete()
assert not gradient_new.is_valid()

"""
Test that an instance that we created in previous test runs persists.
"""
# TODO fix this by creating it for the next test ession
#gradient_persisted = Gimp.Gradient()
#gradient_persisted.set_property("id", "My Persisted Gradient")
#assert gradient_persisted.get_color_count() == 1


"""
Test invalid gradient

Rare.  We do not envision plugins to be constructing Gimp.Gradient using the class constructor.
(Only using the new() method.)
We only envision plugins getting a Gimp.Gradient from a dialog or from Gimp.Context.

Expect error dialog "A plugin is trying to use a resource that is no longer installed."

Commented out because it is tested in test-resource-class.py
and it requires interaction during testing.
"""
#gradient_invalid = Gimp.Gradient()    # class constructor in Python
#gradient_invalid.set_property("id", "invalid name")
#assert gradient_invalid.is_editable() == True
