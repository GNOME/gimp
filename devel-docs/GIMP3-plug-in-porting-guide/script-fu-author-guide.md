# Guide to changes to ScriptFu v3 for script authors

## About

The audience is authors of Scriptfu plugins.
This discusses how to edit v2 scripts so they will run in GIMP 3.
It also discusses enhancements to ScriptFu and its language.

## Quickstart

A lucky few existing scripts may work in GIMP v3 without change.

### Required changes

Some changes are *required* in existing plugins:

  - many PDB procedures are obsolete or renamed, or their signature changed
  - the symbol SF-VALUE is obolete

Once you edit a script for these changes, the script won't work in GIMP v2.

Most scripts will require edits due to changed PDB API.

### Optional changes

Other changes to scripts are *optional*.
Such changes use improved features of ScriptFu.

In approximate order of importance:

  - you can install scripts like plugins in other languages
  - you can use new, specific registration functions instead of the generic script-fu-register
  - scripts can use the new mult-layer selection feature of GIMP
  - scripts can use a v3 dialect having more natural binding of return values from PDB calls
  - the symbolic constants TRUE and FALSE are deprecated, except for SF-TOGGLE
  - a script can *quit* with an error code
  - a script can call *display* to print to the terminal

Enhancements to ScriptFu might not affect an existing plugin.
You need only understand those enhancements when you want to use them.

### Why these changes now?

GIMP 3 is a major version change with new features.
A major version allows breaking API,
meaning old plugins and scripts might not work.

The GIMP developers understand these changes may be onerous.
Script developers might need to maintain two different versions of their scripts.
Some users will stick with GIMP 2 for a while and some will switch to GIMP 3.

A major version is necessary to move forward.
The situation is similar to the disruption caused by the move from Python 2 to 3.

## Changed PDB API

*You must edit your scripts to conform to version 3 of the PDB API.*

The GIMP PDB (Programmers Data Base), that you can call in scripts, has changed in GIMP version 3.
That also may require you to edit scripts.

  - For changes in signatures of PDB procedures,
    see devel-docs/GIMP3-plug-in-porting-guide/porting_scriptfu_scripts.md
  - For added, removed, and replaced PDB procedures,
    see devel-docs/GIMP3-plug-in-porting-guide/removed_functions.md

Some of the changes are for the multi-layer select feature of GIMP 3.
Many PDB procedures now take a container (a Scheme vector) of drawables
instead of a single drawable.

## SF-VALUE type of argument is obsolete

*You must edit v2 scripts and replace the symbol SF-VALUE.*

In v2, SF-VALUE declared a formal argument that is an unquoted, arbitrary string.
Usually, SF-VALUE was used for an integer valued argument.
In the dialog for a script, ScriptFu showed a text entry widget.
Usually the widget showed a default integer literal,
but the widget let you enter any text into the string.

You usually will replace it with an SF-ADJUSTMENT kind of formal argument,
where the "digits" field of the SF-ADJUSTMENT is 0,
meaning no decimal places, i.e. integer valued.
You must also provide other fields, e.g. the lower and upper limits.

A script that has SF-VALUE replaced with SF-ADJUSTMENT
will remain compatible with GIMP 2.

Example:

    SF-VALUE      "Font size"  "50"
    =>
    SF-ADJUSTMENT "Font size" '(50 1 1000 1 10 0 SF-SPINNER)

Here, in the seven-tuple, the 0 denotes: no decimal places.

Another example, where you formerly
used SF-VALUE to declare a formal argument that is float valued:

    SF-VALUE      "Lighting (degrees)"  "45.0"
    =>
    SF-ADJUSTMENT "Lighting (degrees)" '(45.0 0 360 5 10 1 SF-SLIDER)

Here, the 1 denotes: show 1 decimal place, for example "45.0",
in the dialog widget.

### Use SF-STRING for some use cases

In v2, a SF-VALUE argument let a user enter executable Scheme code,
say "'(1 g 1)", which is a list literal,
to be injected into a Scheme call to a plugin.
That use is no longer possible.
If you must do that, use SF_STRING to get a string,
and then your plugin can eval the string.

### Arbitrary precision floats

In v2, a SF-VALUE argument let a user enter a float with arbitrary precision,
e.g. "0.00000001"

That is no longer possible.  You as a script author must use SF-ADJUSTMENT
and specify the maximum precision that makes sense.  The user won't be able to
enter a value with more precision (more digits after the decimal point.)
You should understand the math of your algorithm and know what precision
is excess in terms of visible results.

Example:

    SF-ADJUSTMENT "Lighting (degrees)" '(45.0 0 360 5 10 4 SF-SLIDER)

Here, the user will only be able to enter four decimal places,
for example by typing "0.0001" into the widget.

If you actually need arbitrary precision, use SF_STRING to get a string,
and then your plugin can eval the string to get a Scheme numeric
of the maximum precision that Scheme supports.

### Rationale

Formerly, a SF-VALUE argument let a user enter garbage for an argument,
which caused an error in the script.
SF-ADJUSTMENT or SF-STRING is more user-friendly.

## You can optionally install scripts like plugins in other languages

*We recommend this for new plugins.*

In v3 you can install ScriptFu scripts to a /plug-ins directory,
like plugins in other languages.
This helps isolate plugins.

You must edit the script to include a shebang in the first line:

    #!/usr/bin/env gimp-script-fu-interpreter-3.0
    ;!# Close comment started on first line.

If the script has translatable strings, it should also have a second line
as above, to accomodate parsing by gettext.
(Note that ScriptFu doesn't actually support multi-line comments using #!,
and this is a trick to fool gettext, which parses a script as if
it did support multi-line comments.)


In v2 all ScriptFu scripts were installed in a /scripts directory.
In v3 you may install ScriptFu scripts with a shebang
in a subdirectory of a /plug-ins directory.

Installation of scripts with a shebang must follow rules for interpreted plugins.
A script file must:

- have a shebang on the first line
- be in a directory having the same base name
- have executable permission
- have a file suffix corresponding to an interpreter

An example path to a script:

    ~/.config/GIMP/2.99/plug-ins/script-fu-my/script-fu-my.scm

The advantage of installing a script in the /plug-ins directory is that
such a script will execute in its own process.
If it crashes, it doesn't affect GIMP or other scripts.

Conversely, in v2, all scripts in the /scripts directory
are executed by the long-lived process "extension-script-fu."
If one of those scripts crash, menu items implemented by ScriptFu dissappear
from the GIMP app, and you should restart GIMP.

It is only a convention to name scripts starting with "script-fu-".

## Using new registration functions

*We recommend this.*

New registration functions let a plugin dialog have a new look-and-feel
and improved settings.
ScriptFu "registration" functions let you declare a plugin PDB procedure,
and how it's dialog will look.

ScriptFu v3 has two new registration functions:

* *script-fu-register-procedure*
* *script-fu-register-filter*

Terminology: you *declare* a plugin's attributes and signature using a registration function.
You *define* a run func with a similar signature.
ScriptFu *registers* the plugin in the PDB.

ScriptFu v2 has only *script-fu-register*.
It is a generic function used to declare procedures that are either:

* generic procedures (operating without an image.)
* filters/renderers (operating on an image)

### Registration function *script-fu-register* is now deprecated.

**You should not use script-fu-register in new ScriptFu scripts.**

In the future, the GIMP project might obsolete *script-fu-register*.

While deprecated, *script-fu-register* should still work.
But the plugin dialog will:

* have an old look-and-feel.
* will be missing some advantages, such as the handling of settings.

Also, using *script-fu-register*,
you can only declare a plugin taking one drawable (i.e. layer or mask).
But GIMP v3 now calls such plugins passing a vector of drawables.
GIMP v2 called such plugins passing one drawable.
That is, the declared and actual signatures are incongruous.
GIMP enables the menu item for such deprecated scripts,
declared to take an image and drawable,
whenever a user selects at least one drawable.

### Improved Settings for ScriptFu plugins

GIMP 3 now handles plugin settings better.
(Using more powerful machinery in GIMP that is common to plugins in all languages,
not by limited machinery in ScriptFu that is specific to Scheme language plugins.)

Scripts declared with new registration functions have settings that persist
within and between Gimp sessions.
That is, the next time a user invokes the script,
the dialog will show the same settings as the last time they chose the filter.
This is not true for v2 *script-fu-register*,
where settings persist only during a GIMP session.

The dialog for a script declared with the new registration functions
will also have buttons for resetting to initial or factory values of settings,
and buttons for saving and restoring a set of settings, by name.

### script-fu-register-procedure

Use *script-fu-register-procedure* to declare PDB procedures that are not filters
or renderers.
"Procedure" denotes the general case.
Plugins that are filters or renerers,
taking an image and drawables, are special
and you instead declare them using *script-fu-register-filter.*

*Script-fu-register-filter* declares a script that:

* is always enabled
* can save its settings between sessions

You don't declare "image types"
as you do with *script-fu-register*.
GIMP enables your plugin regardless of the mode of any image a user has selected.

You don't declare "multilayer capability"
as you do with *script-fu-register-filter*.
Your plugin doesn't necessarily need an image and drawables.
(On your behalf, ScriptFu declares to the PDB that it requires no drawables.)

The run func that you define in your script has the same signature
as you declare in the registration function.
(The registration functaion also declares the procedure signature in the PDB,
and it will be similar, but in terms of the C language
and having an extra argument for run-mode.
As in ScriptFu v2, and unlike plugins in other languages,
the run-mode argument is hidden by ScriptFu.)

#### Example

Here is an abbreviated example:

    (define script-fu-my-plugin (radius)
        body)

    (script-fu-register-procedure "script-fu-my-plugin"
      "My plugin..."
      "Example plugin."
      "author/copyright holder"
      "copyright dates"
      SF-ADJUSTMENT "Radius" (list 100 1 5000 1 10 0 SF-SPINNER)
    )

#### Another example: plugins using the context

A plugin may define a menu item appearing in a context menu.
This is a common use case for *script-fu-register-procedure*.
The context is the set of choices in GIMP that affect drawing operations,
for example, the current Brush.

Note that the plugin does *NOT* receive the choice in context
but must get it from the context before operating with/on it.

Using the same registration as above:

    (define script-fu-my-plugin (radius)
        ; do something with the current brush
        (gimp-context-get-brush))

    (script-fu-register-procedure "script-fu-my-brush-plugin"
       ...same registration as above...)

    (script-fu-menu-register "script-fu-my-brush-plugin"
                         "<Brushes>/Brushes Menu")

In this example, the menu item "My plugin..."
appears in the context menu (pops up with right mouse button)
in the *Brushes* dockable window,
and is always enabled.
The script run func gets the current brush
and does something to or with it.

#### Another example: plugin using an unopened file

A plugin may want to use a file that is *not open as an image.*

    (define script-fu-my-plugin (filename)
        ; open and do something with the file
        )

    (script-fu-register-procedure "script-fu-my-plugin"
      "My plugin..."
      "Example plugin."
      "author/copyright holder"
      "copyright dates"
      SF-FILENAME "Image to do something with" ""
    )

    (script-fu-menu-register "script-fu-my-plugin"
                         "<Image>/File")

In this example, the menu item *My plugin...*
appears in the *File* menu on the menubar.
It is always enabled.

When a user chooses the menu item,
a dialog appears that lets a user choose a file.
When the user clicks *OK*,
the plugin opens the file and does something with it.

### script-fu-register-filter

Use *script-fu-register-filter* to declare a PDB procedure that take an image and drawable components of images.

*Script-fu-register-filter* declares a script that:

* is a filter or renderer: taking a user selected image and its selected components
* is multi-layer capable, processing one or more drawables
* can save its settings between sessions
* has a menu item that is enabled/sensitized when a user selects image components

You don't specify the first two arguments "image" and "drawables"
as you do with *script-fu-register* in v2.
Those arguments are implicit.
As a convenience, ScriptFu registers those arguments in the PDB for you.

The run func that you define in your script
must have those formal arguments.  For example:

    (define script-fu-my-plugin (image drawables arg1 arg2) body)

ScriptFu passes a Scheme vector of drawables, not just one,
to a script
registering with script-fu-register-filter.

GIMP enables the menu item of your script
when the user has selected an image of the declared image mode, e.g. *Grayscale*,
and has selected as many drawables
(layers/masks/channels)
as you have declared for multi-layer capability.

#### Declaring multi-layer capabilily

*Script-fu-register-filter* has an argument "multilayer-capability".
(Some documents may refer to the argument as "drawable arity.")
The argument declares the capability of the plugin
for processing multiple drawables (usually layers.)
The argument follows the "image types" argument,
which declares the capability of the plugin to process
various image modes.

Here is an example:

    (script-fu-register-filter "script-fu-test-sphere-v3"
      "Sphere v3..."
      "Test script-fu-register-filter: needs 2 selected layers."
      "authors"
      "copyright holders"
      "copyright dates"
      "*"  ; image modes: any
      SF-TWO-OR-MORE-DRAWABLE  ; multi-layer capability argument
      SF-ADJUSTMENT "Radius" (list 100 1 5000 1 10 0 SF-SPINNER)
    )

The "multilayer-capability" argument can have the following values:

    SF-ONE-DRAWABLE           expects exactly one drawable
    SF-ONE-OR-MORE-DRAWABLE   expects and will process one or more drawables
    SF-TWO-OR-MORE-DRAWABLE   expects and will process two or more drawables

This is only a declaration; whether your defined run func does what it promises is another matter.

A script declaring SF-ONE-DRAWABLE still receives a vector of drawables,
but the vector should be of length one.

These do not specify how the script will process the drawables.
Typically, SF-ONE-OR-MORE-DRAWABLE means a script will filter
the given drawables independently and sequentially.
Typically, SF-TWO-OR-MORE-DRAWABLE means a script will
combine the given drawables, say into another drawable by a binary operation.

The "multilayer-capability" argument tells GIMP to enable the script's menu item
when a user has selected the appropriate count of drawables.

#### A well-written script should check how many drawables were passed

A well-written script should throw an error when it is not passed
the declared number of drawables, either more or fewer than declared.

Starting with GIMP 3,
a plugin that takes an image takes a container of possibly many drawables.
This is the so-called "multi-layer selection" feature.
Existing plugins that don't are deprecated,
and may become obsolete in a future version of GIMP.

Plugins should declare how many drawables they can process,
also called the "multi-layer capability" or "drawable arity" of the algorithm.

The declared drawable arity
only describes how many drawables the algorithm is able to process.
The declared drawable arity does not denote the signature of the PDB procedure.
Well-written image procedures always receive a container of drawables.

For plugins invoked by a user interactively, the drawable arity describes
how many drawables the user is expected to select.
GIMP disables/enables the menu item for a plugin procedure
according to its declared drawable arity.
So a plugin procedure invoked directly by a user should never receive
a count of drawables that the plugin can't handle.

*But PDB procedures are also called from other PDB procedures.*
A call from another procedure may in fact
pass more drawables than declared for drawable arity.
That is a programming error in the calling procedure.

A well-written called plugin that is passed more drawables than declared
should return an error instead of processing any of the drawables.
Similarly for fewer than declared.

## Using the v3 dialect for PDB return values

*We recommend this for new scripts.*

ScriptFu in GIMP version 3 will optionally interpret a v3 dialect.
The dialect is more natural to the Scheme language.
The dialect affects the binding of return values from calls to the PDB.

A script opts into the v3 dialect at runtime by calling script-fu-use-v3
at the beginning of the run func..

The advantage of the v3 dialect is that scripts are smaller,
with fewer calls to car.
Also, you can more naturally examine boolean values.

For more information, see the document in the GIMP source /plugins/script-fu/docs/using-v3-binding.md

## FALSE and TRUE symbols are deprecated

*We recommend not using TRUE and FALSE in new scripts, except for an SF-TOGGLE argument.*

FALSE and TRUE symbols are deprecated in the ScriptFu language.
They never were in the Scheme language.

Instead, you should use the v3 dialect
and use the Scheme language symbols #f and #t.

In ScriptFu, FALSE is 0
and TRUE is 1.
But FALSE is not equivalent to #f.

You must use the = operator to compare to FALSE and TRUE.
FALSE and TRUE are numeric.
The = operator in Scheme is a numeric operator, not a logical operator.

In Scheme, all values are truthy except for #f.
The empty list is truthy.
The numeric 0 is truthy.
Only #f is not truthy.

In the v3 dialect, a PDB procedure returning a single Boolean (a predicate)
returns one element, for example #f or #t.

In the v2 dialect, it still returns (0) or (1),
that is, numeric 0 or 1 wrapped in a list.

*You still need to compare boolean SF_TOGGLE arguments to a script with TRUE and FALSE.
They are not #t and #f*.

#### Rationale

The ScriptFu language is simpler and smaller; TRUE and FALSE duplicated concepts already in the Scheme language.

#### Example changes

Declaring a script's SF_TOGGLE arguments can still use TRUE and FALSE.

    SF-TOGGLE     "Gradient reverse"   FALSE

Comparing a boolean argument to a script *must* remain as in v2 dialect:

    (if (= should-invert TRUE)
        ; do invert )

Calling a PDB procedure taking a boolean:

    (gimp-context-set-feather TRUE)
    =>
    (gimp-context-set-feather #t)

Logically examining a variable for truth:

    (if (= shadow TRUE) ...
    =>
    (if shadow ...

## Using the quit function to abort a script

*We recommend using quit to declare errors* along with gimp-message.

The function "quit" is in Scheme.
It causes a script to exit i.e. return to the system.
The function can be used anywhere in a script,
like you would a "return" statement in other languages.

In ScriptFu v3 it causes the interpreter to stop interpreting a script
and yield an error of type GimpPDBStatus.
That is, it immediately returns an error to the GIMP app or other caller.
The GIMP app will show an error message including the phrase
"script quit with error code x."

When the caller is another PDB procedure (a plugin or script)
the caller should check the result of a call to the script
and propagate the error.
ScriptFu itself always checks the result of a call to the PDB
and propagates the error,
concatenating error message strings.

In ScriptFu v2, a call such as (quit -1) did abort the script,
but did not return an error to the caller
(usually the GIMP app)
and did not return the error code.

### A script doesn't return a value

In ScriptFu, a script returns void.
It is not possible to define a plugin script that returns useful values.
Note that script registration functions do not declare return values.
Scripts only have side effects.

The last expression in a script plugin is *not* the value of the plugin,
as it is in Scheme.

As a PDB procedure, a script can succeed or fail.
But success does not depend on the value of the last evaluated expression.
To make a script return failure, you should call the quit function.

#### Rationale

Formerly, scripts usually called gimp-message on errors,
without yielding an error to the caller.
It was easy for a user to overlook the error message.

You should still call gimp-message just before calling quit,
with an informative error message.
The numeric code that quit prints is not informative to users,
only to the script author.

#### Example

This script quits early:

    (define (script-fu-my-plugin)
        ...
      (gimp-message "Too many drawables.")
      (quit -1)
      (display "this never evaluated")
    )
    ...

## Using the display function to write to the terminal

*We recommend using the display function to help debug a script or to log,
but not to prompt for user input.*

Display is a Scheme function, in the R5RS standard.
It should write a string to any open terminal (e.g stdout on Linux.)

In ScriptFu v3, it does.
You will see the output in any terminal in which the GIMP app was started
(which is optional, and not usually done on a graphical desktop.)

In ScriptFu v2, it did not write to a terminal.
Display wrote to a stream which was returned to the GIMP app, and usually discarded (unless an error occurred, and then the stream
was the prefix of the error message.)

Example:

    (display "The script is here") (newline)

Note that displayln is not in ScriptFu.