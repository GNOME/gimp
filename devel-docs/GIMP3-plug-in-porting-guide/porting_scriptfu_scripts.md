

## About this document

This describes *some* changes needed to port a Scriptfu script to GIMP 3:
- changes in types
- changes in PDB signatures for multi-layer support
- changes in error messages
- changes in logging

It does *not* document:
- PDB procedures whose names have changed (see pdb-calls.md)
- PDB procedures that have been removed (see removed_functions.md)
- PDB procedures that have been added
- other changes in signature where arguments are reordered or changed in number

## Changes in types of PDB signatures

Calls from a script to GIMP are calls to PDB procedures.
PDB procedures are documented in terms of C and GLib types.

This table summarizes the changes:

| Purpose        | Old C type            | New C type            | Old Scheme type | New Scheme type       |
| ---------------|-----------------------|-----------------------| ----------------|-----------------------|
| Pass file name | gchar*, gchar*        | GFile                 | string string   | string                |
| Recv file name | gchar*                | GFile                 | string          | string                |
| pass drawable  | GimpDrawable          | gint, GimpObjectArray | int   (an ID)   | int (a length) vector |
| Pass obj array | gint, GimpInt32Array  | gint, GimpObjectArray | int  vector     | int vector            |
| Recv obj array | gint, GimpInt32Array  | gint, GimpObjectArray | int  vector     | int vector            |
| Pass set of str | gint, GimpStringArray | GStrv                 | int  list       | list                  |
| Recv set of str | gint, GimpStringArray | GStrv                 | int  list       | list                  |

(Where "obj" means an object of a GIMP type such as GimpDrawable or similar.)

### Use one string for a filename instead of two.

Formerly a PDB procedure taking a filename (usually a full path) required two strings (two gchar* .)
Now such PDB procedures require a GFile.

In Scheme, where formerly you passed two strings, now pass one string.

Formerly a script passed the second string for a URI, to specify a remote file.
Formerly, in most cases you passed an empty second string.
Now, the single string in a script can be either a local file path or a remote URI.

Example:

(gimp-file-load RUN-NONINTERACTIVE "/tmp/foo" "")
=> (gimp-file-load RUN-NONINTERACTIVE "/tmp/foo")


### PDB procedures still return a string for a filename

All PDB procedures returning a filename return a single string to Scheme scripts.
That is unchanged.

Formerly a PDB signature for a procedure returning a filename
specifies a returned type gchar*, but now specifies a returned type GFile.
But a Scheme script continues to receive a string.

The returned string is either a local file path or a URI.


### Use a vector of drawables for PDB procedures that now take an array of drawables

Formerly, some PDB procedures took a single GimpDrawable,
but now they take an array of GimpDrawable ( type GimpObjectArray.)
(Formerly, no PDB procedure took an array of drawables.
Some that formerly took a single drawable still take a single drawable.
See the list below. )

For such PDB procedures, in Scheme pass a numeric length and a vector of numeric drawable ID's.

These changes support a user selecting multiple layers for an operation.

Example:

(gimp-edit-copy drawable)  => (gimp-edit-copy 1 (vector drawable))

(gimp-edit-copy 2)  => (gimp-edit-copy 1 '#(2))

### The PDB procedures which formerly took single Drawable and now take GimpObjectArray

- Many of the file load/save procedures.
- gimp-color-picker
- gimp-edit-copy
- gimp-edit-cut
- gimp-edit-named-copy
- gimp-edit-named-cut
- gimp-file-save
- gimp-image-pick-color
- gimp-selection-float
- gimp-xcf-save


### Receiving an array of drawables

Formerly a PDB procedure returning an array of drawables (or other GIMP objects)
had a signature specifying a returned gint and GimpInt32Array.
Now the signature specifies a returned gint and GimpObjectArray.
A script receives an int and a vector.
The elements of the vector are numeric ID's,
but are opaque to scripts
(a script can pass them around, but should not for example use arithmetic on them.)

No changes are needed to a script.


Example:

(gimp-image-get-layers image)

Will return a list whose first element is a length,
and whose second element is a vector of drawables (Scheme numerics for drawable ID's)

In the ScriptFu console,

(gimp-image-get-layers (car (gimp-image-new 10 30 1)))

would print:

(0 #())

Meaning a list length of zero, and an empty vector.
(Since a new image has no layers.)

### Passing or receiving a set of strings

Formerly, you passed an integer count of strings, and a list of strings.
Now you only pass the list.
ScriptFu converts to/from the C type GStrv
(which is an object knowing its own length.)
An example is the PDB procedure file-gih-export.

Formerly, you received an integer count of strings, and a list of strings.
Now you only receive the list
(and subsequently get its length using "(length list)").
Examples are the many PDB procedures whose name ends in "-list".
Remember that the result of a call to the PDB is a list of values,
in this case the result is a list containing a list,
and for example you get the list of strings like "(car (gimp-fonts-get-list ".*"))"

## Changes in error messages

ScriptFu is now more forgiving.

Formerly, ScriptFu would not accept a call construct where the argument count was wrong,
except for the case when you provided one argument to a PDB procedure
that took zero arguments (sometimes called a nullary function.)

Now, when a script has the wrong count of arguments to a PDB procedure:

- too many actual arguments: ScriptFu will give a warning to the console
  and call the PDB procedure with a prefix of the actual arguments.
  This is now true no matter how many arguments the PDB procedure takes.
  Extra arguments in the script are ignored by Scriptfu,
  not evaluated and not passed to the PDB.

- too few actual arguments: ScriptFu will give a warning to the console
  and call the PDB procedure with the given actual arguments.
  The warning will say the expected Scheme formal type of the first missing actual argument.
  Usually the PDB procedure will fail and return its own error message.

When you suspect errors in a script,
it is now important to run GIMP from a console to see warnings.


## ScriptFu logging

ScriptFu now does some logging using GLib logging.
When you define in the environment "G_MESSAGES_DEBUG=scriptfu"
ScriptFu will print many messages to the console.

This is mostly useful for GIMP developers.
