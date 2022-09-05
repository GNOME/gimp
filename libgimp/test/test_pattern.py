
"""
Python to test GimpPattern class of libgimp API.

See comments about usage and setup in test-resource-class.py
Paste into the Python Console and expect no exceptions.

Setup: "Pine" Pattern is selected.

Testing some class methods is in test_resource_class.py

Class has no setters.
Object is not editable.
Class has no is_editable() method.
"""

"""
Test a known, stock pattern "Pine"
!!! Test numeric literals must change if the pattern is changed.
"""
pattern_pine  = Gimp.context_get_pattern()
assert pattern_pine.get_id() == "Pine"

"""
Test getters
"""
success, width, height, bpp = pattern_pine.get_info()
print( width, height, bpp)
assert success
assert width  == 64
assert height == 56
assert bpp    == 3

success, width, height, bpp, pixels = pattern_pine.get_pixels()
print( len(pixels) )
assert success
assert width  == 64
assert height == 56
assert bpp    == 3
assert len(pixels) == 10752

# Not testing the pixels content
