
"""
Python scripts to test GimpPalette class of libgimp API.

See comments about usage and setup in test-resource-class.py

Setup: "Bears" palette is selected.

Testing context_get_palette is in test_resource_class.py
"""


"""
Create artifacts needed for testing.
"""
success, foreground_color = Gimp.context_get_foreground()
assert success


"""
Ensure setup was done.
"""
if Gimp.context_get_palette().get_id() != "Bears" :
    print("\n\n Setup improper: test requires palette Bears selected \n\n")


"""
Test a known, stock palette "Bears"
!!! Test numeric literals must change if the palette is changed.
"""
palette_bears  = Gimp.context_get_palette()
assert palette_bears.get_id() == "Bears"
assert palette_bears.get_color_count() == 256
assert palette_bears.get_columns() == 0
colors = palette_bears.get_colors()
assert len(colors) == 256
# color of first entry is as expected
success, color = palette_bears.get_entry_color(0)
assert success
assert color.r == 0.03137254901960784
assert color.g == 0.03137254901960784
assert color.b == 0.03137254901960784


"""
Test permissions on stock palette.
"""
# Stock palette is not editable
assert not palette_bears.is_editable()
# Stock palette cannot set columns
assert not palette_bears.set_columns(4)
# Stock palette cannot add entry
success, index = palette_bears.add_entry("foo", foreground_color)
assert success == False
# Stock palette cannot delete entry
assert not palette_bears.delete_entry(0)


"""
Test new palette.
"""
palette_new = Gimp.Palette.new("Test New Palette")

# A new palette has the requested name.
# Assuming it does not exist already because the tester followed setup correctly
assert palette_new.get_id() == "Test New Palette"

# a new palette is editable
assert palette_new.is_editable() == True

# a new palette has zero colors
assert palette_new.get_color_count() == 0

# a new palette has an empty array of colors
colors =  palette_new.get_colors()
assert len(colors) == 0

# a new palette displays with the default count of columns
assert palette_new.get_columns() == 0

# You can set columns even when empty of colors
success = palette_new.set_columns(12)
assert success == True
assert palette_new.get_columns() == 12

# Cannot set columns > 64
# Expect a gimp error dialog "Calling error...out of range"
success = palette_new.set_columns(65)
assert not success
# after out of range, still has the original value
assert palette_new.get_columns() == 12

# Cannot set columns to negative number
# TODO

"""
Test add entry
"""

# You can add entries to a new palette
success, index = palette_new.add_entry("", foreground_color)
assert success == True
# The first index is 0
assert index == 0
# After adding an entry, color count is incremented
assert palette_new.get_color_count() == 1
# The color of the new entry equals what we passed
success, color = palette_new.get_entry_color(0)
assert success == True
assert foreground_color.r == color.r


# You can add the same color twice
success, index = palette_new.add_entry("", foreground_color)
assert success == True
assert index == 1
assert palette_new.get_color_count() == 2

# An added entry for which the provided name was empty string is empty string
# TODO C code seems to suggest that it should be named "Untitled"
success, name = palette_new.get_entry_name(1)
assert success
assert name == ""

"""
Test delete entry
"""
assert palette_new.delete_entry(1) == True
# Delete entry decrements the color count
assert palette_new.get_color_count() == 1
# A deleted entry no longer is gettable
success, name = palette_new.get_entry_name(1)
assert not success
assert name == None


"""
Test that an instance that we created in previous test runs persists.
"""
palette_persisted = Gimp.Palette()
palette_persisted.set_property("id", "Test New Palette")
assert palette_persisted.get_color_count() == 1


"""
Test invalid palette

Rare.  We do not envision plugins to be constructing Gimp.Palette using the class constructor.
(Only using the new() method.)
We only envision plugins getting a Gimp.Palette from a dialog or from Gimp.Context.

Expect error dialog "A plugin is trying to use a resource that is no longer installed."
"""
palette_invalid = Gimp.Palette()    # class constructor in Python
palette_invalid.set_property("id", "invalid name")
assert palette_invalid.is_editable() == True
