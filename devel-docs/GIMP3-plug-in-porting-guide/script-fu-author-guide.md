# Guide to changes to ScriptFu v3 for script authors

*Draft, until GIMP 3 is final.  FIXME: rearrange and rename the cited documents*

## About

The audience is authors of Scriptfu plugins.
This discusses how to edit v2 scripts so they will run in GIMP 3.

This is only about changes to ScriptFu proper.
The GIMP PDB, which you can call in scripts, has also changed.
That also may require you to edit scripts.

  - For changes in signatures of PDB procedures,
    see devel-docs/GIMP3-plug-in-porting-guide/porting_scriptfu_scripts.md
  - For added, removed, and replaced PDB procedures,
    see devel-docs/GIMP3-plug-in-porting-guide/removed_functions.md

## Quickstart

A lucky few existing scripts may work in GIMP v3.

Some changes are most likely to break an existing plugin.:

  - SF-VALUE is obsolete
  - TRUE and FALSE are obsolete
  - many PDB procedures are obsolete or renamed, or their signature changed

Once you edit a script for these changes, the script won't work in GIMP v2.

Other changes:

  - you can install scripts like plugins in other languages
  - scripts can use the new mult-layer selection feature of GIMP
  - a script can abort with an error message
  - a script's settings are more fully saved

Those changes might not affect an existing plugin.
You need only understand those changes when you want to use new features of GIMP.

A word of explanation: the GIMP developers understand these changes may be onerous.
Script developers might need to maintain two different versions of their scripts.
Some users will stick with GIMP 2 for a while and some will switch to GIMP 3.
But GIMP 3 is a major version change with new features.
A clean break is necessary to move forward with improvements.
The situation is similar to the disruption caused by the move from Python 2 to 3.


### SF-VALUE kind of argument is obsolete

The symbol SF-VALUE is obsolete.
You can edit v2 scripts and replace that symbol.

In v2, SF-VALUE declared a formal argument that is an unquoted, arbitrary string.
Usually, SF-VALUE was used for an integer valued argument.
In the dialog for a script, ScriptFu showed a text entry widget.
Usually the widget showed a default integer literal,
but the widget let you enter any text into the string.

You usually will replace it with an SF-ADJUSTMENT kind of formal argument,
where the "digits" field of the SF-ADJUSTMENT is 0,
meaning no decimal places, i.e. integer valued.
You must also add the other fields, e.g. the lower and upper limits.

A script that has been edited to replace SF-VALUE with SF-ADJUSTMENT
will remain compatible with GIMP 2.

Example:

    SF-VALUE      "Font size (pixels)"  "50"
    =>
    SF-ADJUSTMENT "Font size (pixels)" '(50 1 1000 1 10 0 SF-SPINNER)

Here, in the seven-tuple, the 0 denotes: no decimal places.

Another example, where you formerly
used SF-VALUE to declare a formal argument that is float valued:

    SF-VALUE      "Lighting (degrees)"  "45.0"
    =>
    SF-ADJUSTMENT "Lighting (degrees)" '(45.0 0 360 5 10 1 SF-SLIDER)

Here, the 1 denotes: show 1 decimal place, for example "45.0",
in the dialog widget.

#### Use SF-STRING for some use cases

In v2, a SF-VALUE argument let a user enter executable Scheme code,
say "'(1 g 1)", which is a list literal,
to be injected into a Scheme call to a plugin.
That use is no longer possible.
If you must do that, use SF_STRING to get a string,
and then your plugin can eval the string.

#### Arbitrary precision floats

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

#### Rationale

Formerly, a SF-VALUE argument let a user enter garbage for an argument,
which caused an error in the script.
SF-ADJUSTMENT is more user-friendly.

### FALSE and TRUE symbols obsolete

FALSE and TRUE symbols are no longer defined symbols in the ScriptFu language.
They never were in the Scheme language.
Instead, the Scheme language has symbols #f and #t.

In ScriptFu v2, FALSE was equivalent to 0
and TRUE was equivalent to 1.
But FALSE was not equivalent to #f.

Formerly, you could use the = operator to compare to FALSE and TRUE.
The = operator in Scheme is a numeric operator, not a logical operator.
Now you can use the eq? or eqv? operators.

In Scheme, all values are truthy except for #f.
The empty list is truthy.
The numeric 0 is truthy.
Only #f is not truthy.

A PDB procedure returning a single Boolean (a predicate)
returns a list containing one element, for example (#f) or (#t).

#### Rationale

The ScriptFu language is simpler and smaller; TRUE and FALSE duplicated concepts already in the Scheme language.

#### Example changes

Registering a script:

    SF-TOGGLE     "Gradient reverse"   FALSE
    =>
    SF-TOGGLE     "Gradient reverse"   #f

Calling a PDB procedure taking a boolean:

    (gimp-context-set-feather TRUE)
    =>
    (gimp-context-set-feather #t)

Logically examining a variable for truth:

    (if (= shadow TRUE) ...
    =>
    (if shadow ...

### In scripts, calls to PDB procedures that return boolean yield (#t) or (#f)

In ScriptFu v2, PDB procedures returning a boolean returned 1 or 0 to a script,
that is, numeric values.
Those were equal to the old TRUE and FALSE values.

Remember that a call to the PDB returns a list to the script.
So in ScriptFu v3,
a PDB procedure that returns a single boolean (a predicate function)
returns (#t) or (#f), a list containing one boolean element.

#### Rationale

#t and #f are more precise representations of boolean values.
0 and 1 are binary, but not strictly boolean.

The ScriptFu language is smaller if concepts of truth are not duplicated.

#### Example changes

Calling a PDB procedure that is a predicate function:

    (if (= FALSE (car (gimp-selection-is-empty theImage))) ...
    =>
    (if (car (gimp-selection-is-empty theImage)) ...

Here, the call to the PDB returns a list of one element.
The "car" function returns that element.
The "if" function evaluates that element for truthy.

Note that to evaluate the result of a PDB call for truth,
you should just use as above, and not use "eq?" or "eqv?".
Such a result is always a list, and a list is truthy,
but not equivalent to #t.
In the ScriptFu console:

    >(eq? #t '())
    #f
    >(eqv? #t '())
    #f

### Use script-fu-script-abort to throw an error

The function "script-fu-script-abort" is new to ScriptFu v3.

It causes the interpreter to stop evaluating a script
and yield an error of type GimpPDBStatus.
That is, it immediately returns an error to the caller.
It is similar to the "return" statement in other languages,
but the Scheme language has no "return" statement.

The function takes an error message string.

When the caller is the GIMP app,
the GIMP app will show an error dialog
having the message string.

When the caller is another PDB procedure (a plugin or script)
the caller must check the result of a call to the PDB
and propagate the error.
ScriptFu itself always checks the result of a call to the PDB
and propagates the error,
concatenating error message strings.

The function can be used anywhere in a script,
like you would a "return" statement in other languages.

Alternatively, a script can yield #f to yield a PDB error.
See below.

#### Rationale

Formerly, scripts usually called gimp-message on errors,
without yielding an error to the caller.
It was easy for a user to overlook the error message.
An abort shows an error message that a user must acknowledge
by choosing an OK button.

#### Example

This script defines a PDB procedure that aborts:

    (define (script-fu-abort)
      (script-fu-script-abort "Too many drawables.")
      (gimp-message "this never evaluated")
    )
    ...

### A script can yield #f to throw an error

Here we use the word "yield" instead of the word "return".
Neither  "yield" nor "return" are reserved words in the Scheme language.

A Scheme text evaluates to, or yields, the value of its last expression.
Any value other than #f, even the empty list or the list containing #f,
is truthy.

A ScriptFu plugin
(the PDB procedure that a script defines in its run func)
whose last evaluated expression is #f
will yield an error of type GimpPDBStatus.

If you don't want a ScriptFu plugin to yield an error,
it must not evaluate to #f.
Most existing plugins won't, since their last evaluated expression
is usually a call to the PDB yielding a list, which is not equivalent to #f.

*Remember that ScriptFu does not yet let you register PDB procedures
that return values to the caller.
That is, you can only register a void procedure, having only side effects.
So to yield #f does not mean to return a boolean to the caller.*

*Also, you can define Scheme functions internal to a script
that yield #f but that do not signify errors.
It is only the "run func" that defines a PDB procedure that,
yielding #f, yields a PDB error to the caller.*


#### Examples

    (define (script-fu-always-fail)
      (begin
        ; this will be evaluated and show a message in GIMP status bar
        (gimp-message "Failing")
        ; since last expression, is the result, and will mean error
        #f
      )
    )

### You can optionally install scripts like plugins in other languages

In v3 you can install ScriptFu scripts to a /plug-ins directory.
You must edit the script to include a shebang in the first line:

    #!/usr/bin/env gimp-script-fu-interpreter-3.0

In v2 all ScriptFu scripts were usually installed in a /scripts directory.
In v3 you may install ScriptFu scripts with a shebang
in a subdirectory of a /plug-ins directory.

Installation of scripts with a shebang must follow rules for interpreted plugins.
A script file must:

- have a shebang on the first line
- be in a directory having the same base name
- have executable permission
- have a file suffix corresponding to an interpreter

An example path to a script:

    ~/.config/GIMP/2.99/plug-ins/myScript/myScript.scm

Such a script will execute in its own process.
If it crashes, it doesn't affect GIMP or other scripts.
In v2, all scripts in the /scripts directory are executed by the long-lived
process "extension-script-fu."
If one of those scripts crash, menu items implemented by ScriptFu dissappear
from the GIMP app, and you should restart GIMP.

### Use script-fu-register-filter to register PDB procedures that take images

The function *script-fu-register-filter* is new to v3.
It lets you declare a script that:

    - is multi-layer capable filter, taking an image and many drawables
    - can save its settings between sessions

You don't specify the first two arguments "image" and "drawable"
as you do with script-fu-register in v2.
Those arguments are implicit.
As a convenience, ScriptFu and GIMP registers those arguments in the PDB for you.

The run func that you define in your script
must have those formal arguments.  For example:

    (define script-fu-my-plugin (image drawables arg1 arg2) body)

ScriptFu passes a Scheme vector of drawables, not just one, to a script
registering with script-fu-register-filter.

#### Multi-layer capabilily

script-fu-register-filter has an argument "multilayer-capability".
Some documents may refer to the argument as "drawable arity."
The argument follows the "image types" argument
and precedes the argument triples that declare formally the "other" arguments
of your plugin.

Here is an abbreviated example:

    (script-fu-register-filter "script-fu-test-sphere-v3"
      "Sphere v3..."
      "Test script-fu-register-filter: needs 2 selected layers."
      "authors"
      "copyright holders"
      "copyright dates"
      "*"  ; image types any
      SF-TWO-OR-MORE-DRAWABLE  ; multi-layer capability argument
      SF-ADJUSTMENT "Radius (in pixels)" (list 100 1 5000 1 10 0 SF-SPINNER)

The "multilayer-capability" argument can have the following values:

    SF_ONE_DRAWABLE           expects exactly one drawable
    SF_ONE_OR_MORE_DRAWABLE   expects and will process one or more drawables
    SF_TWO_OR_MORE_DRAWABLE   expects and will process two or more drawables

This is only a declaration; whether your run func does what it promises is another matter.

A script declaring SF_ONE_DRAWABLE still receives a vector of drawables,
but the vector should be of length one.

These do not specify how the script will process the drawables.
Typically, SF_ONE_OR_MORE_DRAWABLE means a script will filter
the given drawables independently and sequentially.
Typically, SF_TWO_OR_MORE_DRAWABLE means a script will
combine the given drawables, say into another drawable by a binary operation.

The "multilayer-capability" argument tells GIMP to enable the script's menu item
when a user has selected the appropriate count of drawables.

#### Settings are handled by GIMP, not ScriptFu

Scripts declared with script-fu-register-filter have settings that are persisted
within and between Gimp sessions.  That is, the next time a user chooses the filter,
the dialog will show the same settings as the last time they chose the filter.
This is not true for v2 script-register-filter,
where settings are only kept during a GIMP session.

The dialog for a script declared with script-fu-register-filter
will also have buttons for resetting to initial or factory values of settings.

#### A script should check how many drawables were passed

A well-written script should throw an error if a caller does not pass the expected number of drawables, either more or fewer than declared.  See below.

### Deprecated: using script-fu-register-filter to register PDB procedures that take images

Existing scripts that use script-fu-register to declare a procedure
that takes an image and single drawable,
are deprecated.
They will still work and have a correct dialog in v3.
In some future version of GIMP,
such scripts may become obsolete.
All newly written scripts taking an image and one or more drawables
should use script-fu-register-filter.

GIMP enables the menu item for such deprecated scripts if and only if a user
selects exactly one drawable (layer or other.)

### ScriptFu plugins are expected to throw errors for improper count of drawables

Starting with GIMP 3,
a plugin that takes an image takes a container of possibly many drawables.
This is the so-called "multi-layer selection" feature.
Existing plugins that don't are deprecated,
and may become obsoleted in a future version of GIMP.

Plugins should declare how many drawables they can process,
also called the "multi-layer capability" or "drawable arity" of the algorithm.

The declared drawable arity
only describes how many drawables the algorithm is able to process.
The declared drawable arity does not denote the signature of the PDB procedure.
Well-written image procedures always receive a container of drawables.

For calls invoked by a user, the drawable arity describes
how many drawables the user is expected to select.
GIMP disables/enables the menu item for a plugin procedure
according to its declared drawable arity.
So a plugin procedure invoked directly by a user should never receive
a count of drawables that the plugin can't handle.

*But PDB procedures are also called from other PDB procedures.*
A call from another procedure may in fact
pass more drawables than declared for drawable arity.
That is a programming error on behalf of the caller.

A well-written callee plugin that is passed more drawables than declared
should return an error instead of processing any of the drawables.
Similarly for fewer than declared.

A ScriptFu plugin can use script-fu-script-abort to declare an error
when passed an improper count of drawables.
