# The version 3 dialect of the ScriptFu language

## About

This describes a new dialect of ScriptFu,
used when a script calls: (script-fu-use-v3).
The new dialect is more like Scheme and makes scripts shorter.
The dialect only affects calls to the PDB.

The audience is script authors and other developers.

This describes the new dialect and how to port old scripts to the new dialect.


## Quick Start

A script that calls:
```
(script-fu-use-v3)
```
binds to certain PDB calls differently:

1. PDB procedures that return single values return just that single value,
*not wrapped in a list.*
Formerly, every PDB call returned a list (possibly nesting more lists.)

2. You can use #t and #f as arguments to PDB calls taking a boolean.

3. PDB calls returning a boolean return #t or #f, not TRUE or FALSE (1 or 0.)

## Script-Fu Console

The Script-Fu Console always starts in the v2 dialect.

You can call *script-fu-use-v3* in the console.
Then, the console interprets the v3 dialect.
It continues to interpret the v3 dialect in that session,
until you call *script-fu-use-v2.*

## Where to call *script-fu-use-v3* in scripts

Call *script-fu-use-v3* early.
This sets the dialect version for the remaining interpretation of the script.
Call *script-fu-use-v3* at the beginning of the run function.

!!! Do not call *script-fu-use-v3* at the top level of a script.
This has no effect, since it is only executed in the query phase.
The interpreter starts at the run function during the run phase.

*The interpreter always starts interpreting each script in the v2 dialect.
This is true even for extension-script-fu, the long-running interpreter.*
There is no need to call *script-fu-use-v2* before returning from a script,
to ensure that the next script is interpreted in v2 dialect.

Example:

```
(define (script-fu-testv3 img drawables )
  (script-fu-use-v3) ; <<<
  (let* (
        ...
```
### Technically speaking

The dialect version has "execution scope" versus "lexical scope."
Setting the dialect version is effective even for
other functions defined in the same script but lexically
outside the function where the dialect is set.


## Don't call v2 scripts from v3 scripts

When using the v3 dialect,
you can't call plugin scripts or other library scripts that depend on the v2 dialect.
And vice versa.
(When a script calls a PDB procedure that is a script,
a new interpreter process is *NOT* started.)

For example, a new plugin script should not call the PDB procedure
script-fu-add-bevel because it is implemented in ScriptFu Scheme
and for example has:

```
(width (car (gimp-drawable-get-width pic-layer)))
```
which is v2 dialect and would throw an error such as:
```
Error: car requires a pair.
```

*It is rare that a script calls another plugin script.*
A script usually calls the PDB,
but rarely calls a plugin script of the PDB.

There are very few, obscure library scripts that call the PDB using v2 dialect.
These are in scripts/script-fu-util.scm.

## Pure Scheme is unaffected

The dialect only affects calls the to PDB.

This means you can usually call most library scripts when using v3,
since most library scripts are pure Scheme,
that is, with no calls to the GIMP PDB.

## TRUE and FALSE

TRUE and FALSE are still in v3 dialect and are still numbers 1 and 0.
But we recommend not using them.

You can still pass them as arguments to PDB calls taking a boolean,
and they are still converted to the C notion of boolean truth.

FALSE which is 0 is truthy in Scheme!!!
It converts to the C notion of false only in a call the the PDB.
In the ScriptFu Console:
```
>(equal? FALSE #t)
#t
```

TRUE and FALSE symbols may become obsolete in the future.

## Plans for the future

This dialect is shorter and more natural for Scheme programmers.

The long-term goal is for the v3 dialect to be the only dialect of ScriptFu.
For the short term, for backward compatibility,
the default dialect of ScriptFu is the v2 dialect.

You should write any new scripts in the v3 dialect,
and call *script-fu-use-v3*.

You should plan on porting existing scripts to the v3 dialect,
since eventually ScriptFu might not support v2 dialect.

## Example conversions from v2 to v3

### A call to a PDB procedure returning a single value

```
(set! (width (car (gimp-drawable-get-width pic-layer))))
```
*must* become
```
(set! (width (gimp-drawable-get-width pic-layer)))
```
The PDB call returns a single integer,
so no *car* is needed.

### A call to a PDB procedure returning boolean
```
(if (= (gimp-image-is-rgb image) TRUE) ...
```
*must* become:
```
(if (gimp-image-is-rgb image) ...
```
The PDB procedure returns a boolean,
which is bound to #t or #f,
the usual symbols for Scheme truth.

### A call to a PDB procedure taking a boolean

```
(gimp-context-set-antialias TRUE)
```
*should* become
```
(gimp-context-set-antialias #t)
```
This doesn't *require* conversion because TRUE is 1 and is truthy in Scheme.

!!! But FALSE is 0 and 0 is truthy in Scheme
```
(gimp-context-set-antialias FALSE)
```
This *should* be converted, for clarity, but doesn't *require* conversion.
For now, ScriptFu still binds Scheme 0 to C 0.
So it still does what the script intends.

### A call to a PDB procedure returning a list

```
(set! brushes (car (gimp-brushes-get-list)))
```
*must* become:
```
(set! brushes (gimp-brushes-get-list))
```
Formerly, the PDB procedure returned a list wrapped in a list,
i.e. ((...))
In the v3 dialect, it returns just a list, i.e. (...)
which is a single value, a single container.

### A call to a PDB procedure returning a list, getting the first element

```
(set! first-brush (caar (gimp-brushes-get-list)))
```
Gets the first brush in GIMP.
This *must* become:
```
(set! first-brush (car (gimp-brushes-get-list)))
```
The call to *caar* is consecutive calls to *car*,
and you must eliminate the first call to *car*.


## Knowing what constructs need conversion

You *should* (but are not required to)
eliminate all uses of symbols TRUE and FALSE from a script
using v3 dialect.

You should eliminate many, *but not all*, uses of the *car* function wrapping a call to the PDB,
where the PDB procedure returns a single value.
There are several hundred such PDB procedures in the PDB.
Examine the signature of a PDB procedure using the PDB Browser.

For example, gimp-brush-get-angle:
```
Return Values
     angle gdouble ....
Additional Information
```
This returns a single value so no need to use car.

For example, gimp-brushes-get-list:
```
Return Values
     brush-list GStrv
Additional Information
```
This also returns a single value.
The single value is a list i.e. container.
In the v2 dialect, this returned a list wrapped in a list,
for example (("foo" "bar")).

See "Example conversions from v2 to v3"

### Does not require changes to calls to PDB procedures returning void

You should not need to change scripts on calls to PDB procedures returning C void because a script should not be examining the result.

Some scripts might examine the result of a call to a void PDB procedure, thinking that (#t) is the success of the call,
but that is a misconception and should be fixed.

Calls to PDB procedures that return C void return (#t) in v2 and  the empty list (), sometimes called nil, in v3 dialect.

## Details of the implementation

There are new functions in the dialect:
```
script-fu-use-v3
script-fu-use-v2
```
These functions have side-effects on the state of the interpreter.
They always return the empty list ().

The effect is immediate.
The interpreter interprets a dialect from then on,
until the script returns,
or until the script changes the dialect again.

A call to *script-fu-use-v3* sets a flag in the state of the interpreter.
When the flag is set, the interpreter binds arguments to PDB calls slightly differently, as described.
Binding of args to PDB calls is done at run time.

Similarly, a call to *script-fu-use-v2* clears the flag
and restores the interpreter state to binding using the v2 dialect.

When in the v3 state,
any PDB call constructs using v2 binding will yield errors,
and vice versa.

Note that the difference in interpretation is only in binding, but both to and from the PDB:

1. Single return values *from* the PDB are not wrapped in lists.
2. Boolean return values *from* the PDB are bound to #t and #f.
3. Boolean values *to* the PDB can be #t and #f.
