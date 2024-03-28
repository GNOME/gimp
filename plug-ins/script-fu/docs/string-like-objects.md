# String-like objects in ScriptFu's TinyScheme

!!! Work in progress.  This documents what should be, and not what is actually implemented.  There are bugs in the current implementation.

## About

This is a language reference for the string-like features of
Script-fu's TinyScheme language.

This may differ from other Scheme languages.  TinyScheme is a subset of R5RS Scheme, and ScriptFu has more than the original TinyScheme because it has been modified to support unichars and bytes.  Both string-ports, bytes, and unichar are recent additions to Scheme, and not standardized among Schemes.

This is not a tutorial, but a technical document intended to be testable.

Terminology.  We use "read method" to denote the function whose name is "read".
We occasionally use "read" to mean "one of the functions: read, read-byte, or read-char.

## The problem of specification

TinyScheme is a loose subset of the R5RS specification.

These are not part of R5RS, but optional implementations:
  - string-ports
  - unichar
  - byte operations

Racket is a Scheme language that also implements the above.
See Racket for specifications, examples, and tests,
but ScriptFu's TinyScheme may differ.

SRFI-6 is one specification for string-port behavior.
SRFI-6:
  - has a reference implementation.
  - does not describe testable behavior.
  - does not discuss unichar or byte operations

## Overview

Script-fu's TinyScheme has these string-like objects:

  - string
  - string-port

Both are:

  - sequences of chars
  - practically infinite
  - UTF-8 encoded, i.e. the chars are unichars of multiple bytes

They are related, you can:

  - initialize an input string-port from a string
  - get a string from an output string-port

However, the passed string is not owned by a string-port but is a separate object with its own lifetime.

Differences are:

  - string-ports implement the port API (open, read, write, and close)
  - string-ports have byte methods but strings do not
  - strings have a length method but string-ports do not
  - strings have indexing and substring methods but string-ports do not
  - write to a string-port is less expensive than an append to a string

Note that read and write methods to a string-port traffic in objects, not chars.
In other words, they serialize or deserialize, representing objects by text, and parsing text into objects.

Symbols also have string representations, but no string-like methods besides conversion to and from strings.

## ScriptFu's implementation is in C

This section does not describe the language but helps to ground the discussion.

ScriptFu does not use the reference implementation of SRFI-6.
The reference implementation is in Scheme.
The reference implementation is on top of strings.

ScriptFu's implementation of *string-ports is not on top of strings.*
ScriptFu's implementation is in C language.
(The reason is not recorded, but probably for performance.)

The reference implementation of SRFI-6 requires that READ and WRITE be redefined to use a redefined READ-CHAR etc. that
are string-port aware.
TinyScheme does something similar: inbyte() and backbyte() dispatch on the kind of port.

Internally, TinyScheme terminates all strings and string-ports with the NUL character (the 0 byte.)  This is not visible in Scheme.

## Allocations

A main concern of string-like objects is how they are allocated and their lifetimes.

All string-like objects are allocated from the heap and with one cell from TinyScheme's cell pool (which is separately allocated from the heap.)

A string-port and any string used with a string-port are separate objects with separate lifetimes.

The length of string-like objects is limited by the underlying allocator (malloc) and the OS.

### String allocations

Strings and string literals are allocated.

They are allocated exactly.

Any append to a string causes a reallocation.

Any substring of a string causes a new allocation and returns a new string instance.

String literals are allocated but are immutable.

### String-port allocations

String-ports of kind output have an allocated, internal buffer.
A buffer has a "reserve" or free space.

The buffer can sometimes accomodate writes without a reallocation.
Writes to an output string-port can be less expensive (higher performing)
than appends to a string, which always reallocates.
But note that writes are not the same as appending (see below.)

The write method can write larger than the size that is pre-allocated for the buffer (256 bytes.)

A string-port of kind input is not a buffer.
It is allocated once.
It's size is fixed when opened.

## Byte, char, and object methods

### String methods

Strings are composed of characters.
The method string-ref accesses a character component.

Strings have no byte methods.
Characters can be converted to integers and then to bytes.
See "Support for byte type" at the Gimp developer web site.

Strings have no object methods: read and write.

### Port methods

Ports, and thus string-ports, have byte and char methods:

    read-byte, read-char
    write-byte, write-char

Ports also have methods trafficing in objects:

    read, write


### Methods and port kinds

String-ports are of two kinds:
  - input
  - output

*There is also an input-output kind of string-port in TinyScheme,
but this document does not describe it and any use of it is not supported in ScriptFu*

You should only use the "read" methods on a string-port of kind input.
You should only use the "write" methods on a string-port of kind output.
A call of a read method on a string-port of kind output returns an error, and vice versa.


### Mixing byte, char, and object methods

*You should not mix byte methods with char methods, unless you are careful.*
You must understand UTF-8 encoding to do so.

*You should not mix char methods with read/write methods, unless you are careful.*
You must understand parsing and representation of Scheme to do so.

### The NUL character and byte

Internally, TinyScheme terminates all strings and string-ports
with the NUL character (the 0 byte.)

*You should not write the NUL character to strings or string-ports.  The result can be surprising, and is not described here.*
You must understand the role of NUL bytes in C strings.

You cannot read the NUL character or byte from a string or string-port since the interpreter always sees it as a terminator.

Note that a string escape sequence for NUL, which is itself a string without a NUL character, can be inside a string or string-port.

You can read and write the NUL character to file ports that you are treating as binary files and not text files.

## Length methods

### Strings

The length of a string is in units of characters.  Remember that each character in UTF-8 encoding may comprise many bytes.

    (string-length "foo") => 3

### Ports

Ports have no methods for obtaining the length, either in characters or byte units.  Some other Schemes have such methods.

## String-port and initial strings

### Input ports

The method open-input-port takes an initial string.

The initial string can be large, limited only by malloc.

TinyScheme copies the initial string to the port.
Subsequently, these have no effect on the port:

  - the initial string going out of scope
  - an append to the initial string

Subsequent calls to read methods progress along the port contents,
until finally EOF is returned.

The initial string can be the empty string and then the first read will return the EOF object.

There are no methods for appending to an input string-port after it is opened.

### Output ports

*The method open-output-port optionally takes an initial string but it is ignored.*

In other Schemes, any initial string is the name of the port.

In version 2 of Script-Fu's TinyScheme, the initial string was loosely speaking the buffer for the string-port.
This document does not describe the version 2 behavior.

An output string-port is initially empty and not the initial string.

The initial string may go out of scope without effect on an output string-port.

You can write more to an output port than the length of the initial string.

## The get-output-string method

A string-port of kind output is a stream that can be written to
but that can be read only by getting its entire contents.

    (get-output-string port)

Returns a string that is the accumulation of all prior writes to the output string-port.  This is a loose definition.  Chars, bytes, and objects can be written, and it is the representation of objects that accumulate.

The port must be open at the time of the call.

A get-output-string call on a newly opened empty port returns the empty string.

Consecutive calls to get-output-string return two different string objects, but they are equivalent.

The returned string is a distinct object from the port.
These subsequent events have no effect on the returned string:

  - writes to the port
  - closing the port
  - the port subsequently going out of scope

Again, *you should not mix write-byte, write-char, and write to an output string-port, without care*


## Writing and reading strings to a string-port

These are different:
 - write a string to an output string-port
 - append a string to a string

A string written to an output string-port writes escaped quotes into the string-port:

```
> (define aPort (open-output-string))
> (write "foo" aPort)
> (get-output-string aPort)
"\"foo\""
```

That is, writing a string to an output string-port
writes seven characters, three for foo and two pairs of
backslash quote.
```
\"foo\"
1234567
```
This is a string, which in the REPL prints surrounded by quotes:
```
"\"foo\""
```




