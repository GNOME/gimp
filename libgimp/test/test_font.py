
"""
Python scripts to test GimpFont class of libgimp API.

See comments about usage and setup in test-resource-class.py

Setup: "Sans-serif" font is selected.

There are few methods on font.
Font has no other getter/setters and is not editable.
Tests of context_set_font and a few other class methods
are in test_resource_class.py
"""

"""
Test a known, stock font "Sans-serif"
"""
font_sans_serif = Gimp.context_get_font()
assert font_sans_serif.get_id() == "Sans-serif"
