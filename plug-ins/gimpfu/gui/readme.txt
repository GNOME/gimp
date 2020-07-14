Most of this module goes away when Gimp itself provides a control panel dialog
for plugins.  Which is work in progress in Gimp.

Conversely, it is no big deal to implement that here, using PyGObject and GTK3.

The current status is: throw up a largely non-functional, ugly dialog.
Some widgets may work, most don't.
Since the object is to test other features of Gimpfu, and the default values
are usually adequate for that.
