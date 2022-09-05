# API changes in libgimp and the PDB for resources

This explains changes to the GIMP API from v2 to v3,
concerning resources.

The audience is plugin authors, and GIMP developers.

### Resources

A resource is a chunk of data that can be installed with GIMP
and is used by painting tools or for other rendering tasks.
Usually known as brush, font, palette, pattern, gradient and so forth.

### Resources are now first class objects

GimpResource is now a class in libgimp.

It has subclasses:

  - Brush
  - Font
  - Gradient
  - Palette
  - Pattern

Formerly, the GIMP API had functions operating on resources by name.
Now, there are methods on resource objects.
Methods take an instance of the object as the first argument,
often called "self."

This means that where you formerly used a string name to refer to a resource object,
now you usually should pass an instance of an object.

### Changes to reference documents

#### libgimp API reference

Shows classes Brush, Font, and so forth.
The classes have instance methods taking the instance as the first argument.

Example:

```
gboolean gboolean gimp_brush_delete(gcharray) => gboolean gimp_brush_delete ( GimpBrush*)
```

The classes may also have class methods still taking string names.

Example:

```
gboolean gimp_brush_id_is_valid (const gchar* id)
```

Is a class method (in the "Functions" section of the class) taking the ID
(same as the name) to test whether such a brush is installed in Gimp core.

#### PDB Browser

Remember the PDB Browser shows the C API.  You must mentally convert
to the API in bound languages.

Shows some procedures that now take type e.g. GimpBrush
where formerly they took type gcharray i.e. strings.

Shows some procedures that take a string name of a brush.
These are usually class methods.

#### Other changes to the API

Many of the Gimp functions dealing with the context
now take or return an instance of a resource.

Example:

```
gcharray* gimp_context_get_brush (void) =>  GimpBrush* gimp_context_get_brush (void)
```

A few functions have even more changed signature:

```
gint gimp_palette_get_info (gcharray) =>
gint gimp_palette_get_color_count (GimpPalette*)
```

The name and description of this function are changed
to accurately describe that the function only returns an integer
(formerly, the description said it also returned the name of the palette.)

### New resource objects

FUTURE

Formerly there were no methods in the libgimp API or the PDB for objects:

  - Dynamics
  - ColorProfile
  - ToolPreset

These classes exist primarily so that plugins can let a user choose an instance,
and pass the instance on to other procedures.

### Traits

Informally, resources can have these traits:

  - Nameable
  - Creatable/Deleable
  - Cloneable (Duplicatable)
  - Editable

Some resource subclasses don't have all traits.

### ID's and names

The ID and name of a resource are currently the same.
(Some documents and method names may use either word, inconsistently.)

You usually use resource instances instead of their IDs.
This will insulate your code from changes to GIMP re ID versus name.

A plugin should not use a resource's ID.
A plugin should not show the ID/name to a user.
The GIMP app shows the names of resources as a convenience to users,
but usually shows resources visually, that is, iconically.
An ID is opaque, that is, used internally by GIMP.

FUTURE: the ID and name of a resource are distinct.
Different resource instances may have the same name.
Methods returning lists of resources or resource names may return
lists having duplicate names.

### Resource instances are references to underlying data

A resource instance is a proxy, or reference, to the underlying data.
Methods on the instance act on the underlying data.
The underlying data is in GIMP's store of resources.

It is possible for a resource instance to be "invalid"
that is, referring to underlying data that does not exist,
usually when a user uninstalls the thing.

### Creating Resources

Installing a resource is distinct from creating a resource.

GIMP lets you create some resources.
You can't create fonts in GIMP, you can only install them.
For those resources that GIMP lets you create,
the act of creating it also installs it.

For resources that you can create in GIMP:

  - some you create using menu items
  - some you can create using the API

The API does not let you create a raster brush.

The API does let you create a parametric brush.
For example, in Python:
```
brush = Gimp.Brush.new("Foo")
```
creates a new parametric brush.

Note that the passed name is a proposed name.
If the name is already in use,
the new brush will have a different name.
The brush instance will always be valid.

### Getting Resources by ID

Currently, you usually ask the user to interactively choose a resource.

If you must get a reference to a resource for which you know the ID,
you can new() the resource class and set it's ID property.
See below.

FUTURE Resource classes have get_by_id() methods.

If such a named resource is currently installed,
get_by_id() returns a valid instance of the resource class.
If such a named resource is not currently installed,
the method returns an error.

### Uninitialized or invalid resource instances

You can create an instance of a resource class that is invalid.

For example, in Python:

```
brush = Gimp.Brush()
brush.set_property("id", "Foo")
```
creates an instance that is invalid because there is no underlying data in the GIMP store
(assuming a brush named "Foo" is not installed.)

Ordinarily, you would not use such a construct.
Instead, you should use the new() method
(for resource classes where it is defined)
which creates, installs, and returns a valid instance except in dire circumstances (out of memory.)

### Invalid resource instances due to uninstalls

A plugin may have a resource as a parameter.

An interactive plugin may show a chooser widget to let a user choose a resource.
The user's choices may be saved in settings.

In the same sesssion of GIMP, or in a subsequent session,
a user may invoke the plugin again.
Then the saved settings are displayed in the plugin's dialog
(when the second invocation is also interactive).

When, in the meantime (between invocations of the plugin)
a user has uninstalled the reference resource,
the resource, as a reference, is invalid.
A well-written plugin should handle this case.

Resource classes have:

  - is_valid() instance method
  - id_is_valid(char * name) class method

Well-written plugins should use these methods to ensure
that saved (deserialized) resource instances
are valid before subsequently using them.


### Naming and renaming

As mentioned above, currently names must be unique.

For some resources, the method rename(char * name) changes the name.
The method fails if the new name is already used.

When the instance is invalid to start with
(it has an ID that does not refer to any installed data)
renaming it can succeed and then it creates a valid instance.

### Duplicating

Duplicating a resource creates and installs the underlying data,
under a new, generated name.

The duplicate() method on an instance returns a new instance.

### Deleting

You can delete some resources.  This uninstalls them.
You can delete some brushes, palettes, and gradients,
when they are writeable i.e. editable,
which usually means that a user previously created them.
You can't delete fonts and patterns.

You can delete using the delete() instance method

When you delete a resource, the instance (the proxy in a variable) continues to exist, but is invalid.

### Resource lists

Some functions in GIMP return lists of resource names,
representing the set of resources installed.

For example: gimp_brushes_get_list.

This returns a list of strings, the ID's of the resources.
The list will have no duplicates.

FUTURE: this will return a list of resource instances, and their names may have duplicates.
