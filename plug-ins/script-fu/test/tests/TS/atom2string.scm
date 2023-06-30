
; test atom->string function

; atom->string is not R5RS
; Instead, it is TinyScheme specific.

; atom->string works for atoms of type: number, char, string, byte, symbol.
; This is not the usual definition of atom.
; Others define atom as anything but list and pair.

; For atom of type number,
; accepts an optional second arg: <base> in [2,8,10,16]
; Meaning arithmetic base binary, octal, decimal, hexadecimal.
; For atoms of other types, passing a base returns an error.


; The REPL uses an internal C function atom2str()
; which is not exposed in the TS language.
; It *DOES* represent every object (all atoms) as strings.
; But the representation is sometimes a string that can
; be turned around and evaluated,
; which is not the same string as atom->string produces.

; !!! Note readstring() internal function
; accepts and reduces C "escaped" string representations
; i.e. \x07 or \t for tab.
; Thus in a test, a double-quoted string enclosing
; an escape sequence can be equivalent to a
; string for a char atom.



;                normal tests (without error)


;                number

; number, integer aka fixnum
(assert `(string=? (atom->string 1)
                    "1"))

; number, float aka flonum
(assert `(string=? (atom->string 1.0)
                    "1.0"))

; FIXME the above is known to fail in German:
; currently prints 1,0.
; To test, set locale to German and retest.

; There are no other numeric types in TinyScheme.
; Refer to discussions of "Lisp numeric tower"



;                 char

; ASCII, i.e. fits in 8-bit byte

; char, ASCII, printing and visible
(assert `(string=? (atom->string 'a)
                    "a"))

; char, ASCII, non-printing, whitespace
(assert `(string=? (atom->string #\space)
                    " "))

; Note the char between quotes is a tab char
; whose display when viewing this source depends on editor.
; Some editors will show just a single white glyph.
;
; Note also that the SF Console will print "\t"
; i.e. this is not a test of the REPL.
(assert `(string=? (atom->string #\tab)
                    "	"))
; Note the char between quotes is a newline char
(assert `(string=? (atom->string #\newline)
                    "
"))
; Note between quotes is an escaped return char,
; which readstring() converts to a single char
; decimal 13, hex 0d
(assert `(string=? (atom->string #\return)
                    "\x0d"))

; char, ASCII, non-printing control
(assert `(string=? (atom->string #\x7)
										""))
; !!! This also passes, because readstring converts
; the \x.. escape sequence to a char.
(assert `(string=? (atom->string #\x7)
                    "\x07"))
; !!! Note the REPL for (atom->string #\x7)
; yields "\x07" which is not a sharp char expr wrapped in quotes
; but is a string that can be turned around and evaluated
; to a string containing one character.



;             multi-byte UTF-8 encoded chars

; see more tests in sharp-expr-unichar.scm

; char, unichar outside the ASCII range
(assert `(string=? (atom->string #\λ)
                   "λ"))



; symbol
(assert `(string=? (atom->string 'gimp-message)
                    "gimp-message"))
; symbol having multibyte char
(assert `(string=? (atom->string 'λ)
                    "λ"))

; string
(assert `(string=? (atom->string "foo")
                    "foo"))
; string having multibyte char
(assert `(string=? (atom->string "λ")
                    "λ"))


; byte

; Note that readstring() accepts and reduces \x.. notation.

; Test against a glyph
(assert `(string=? (atom->string (integer->byte 31))
										""))
;Test for equivalence to reduced string
(assert `(string=? (atom->string (integer->byte 1))
                    "\x01"))
(assert `(string=? (atom->string (integer->byte 255))
                    "\xff"))
; integer->byte truncates a number that does not fit in 8-bits
(assert `(string=? (atom->string (integer->byte 256))
                    "\xff"))

; Note some TinyScheme C code uses printf ("%lu", var) where var is unsigned char,
; and that prints unsigned char in this format.
; The above tests are not a test of that code path.


;             test optional base arg for numeric atom

; binary, octal, decimal, hexadecimal
(assert `(string=? (atom->string 15 2)
                    "1111"))
(assert `(string=? (atom->string 15 8)
                    "17"))
(assert `(string=? (atom->string 15 10)
                    "15"))
(assert `(string=? (atom->string 15 16)
                    "f"))

; passing <base> arg for non-numeric atom is error
(assert-error `(atom->string (integer->byte 255) 2)
                "atom->string: bad base:")





;              tests of abnormality i.e. error messages

; atom->string does not work for [#t, nil, closure, port, list, vector, foreign function]

; foreign function
(assert-error `(atom->string gimp-message)
              "atom->string: not an atom:")
; nil aka '()
(assert-error `(atom->string '() )
              "atom->string: not an atom:")
; #t
(assert-error `(atom->string #t )
              "atom->string: not an atom:")

; TODO port etc.
