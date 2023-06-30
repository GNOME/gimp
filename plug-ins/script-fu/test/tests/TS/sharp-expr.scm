; Tests of sharp expressions in ScriptFu

; This only tests:
;    miscellaneous sharp expressions
; See also:
;   sharp-expr-char.scm
;   sharp-expr-number.scm

; Some "sharp expressions" e.g. #t and #f might not be explicitly tested,
; but tested "driveby" by other tests.

; Terminology:
; The code says "sharp constant expression".
; A "sharp expression" is text in the language that denotes a "sharp constant."
; A constant is an atom of various types: char, byte, number.
; The expression is distinct from the thing it denotes.

; A "sharp expression" is *recognized* by the interpreter.
; But also *printed* by the interpreter REPL.
; Mostly these are tests of recognition.
; The testing framework cannot test the REPL.

; See scheme.c, the token() function, about line 2000
; and the mk_sharp_constant() function, for sharp character constant

; #( token denotes start of a vector

; #! token denotes start of a comment terminated by newline
; aka shebang or hashbang, a notation that OS shells read

; #t denotes true
; #f denotes false

; #odxb<x> denotes a numeric constant in octal, decimal, hex, binary base
; where <x> are digits of that base

; #\<char> denotes a character constant where <char> is one character
; The one character may be multiple bytes in UTF-8,
; but should appear in the display as a single glyph,
; but may appear as a box glyph for unichar chars outside ASCII.

; #\x<x> denotes a character constant where <x> is a sequence of hex digits
; See mk_sharp_const()

; #\space #\newline #\return and #\tab also denote character constants.

; Note: sharp backslash followed by space/blank parses as a token,

; #U+<x> notation for unichar character constants is not in ScriptFu

; Any sharp character followed by characters not described above
; MAY optionally be a sharp expression when a program
; uses the "sharp hook" by defining symbol *sharp-hook* .


; block quote parses
; Seems only testable in REPL?
; Note there is a newline after foo
;(assert '#! foo
;        )
; but is not testable by the framework

; #t denotes truth
(assert #t)

; #t denotes an atom
(assert (atom? #t))

; #t is type boolean
(assert (boolean? #t))
; #t is neither type number or symbol
(assert (not (number? #t)))
(assert (not (symbol? #t)))

; #t denotes constant, and constant means immutable
; You cannot redefine #t
(assert-error `(define #t 1)
              "variable is not a symbol")
; You cannot set #t
(assert-error `(set! #t 1)
              "set!: unbound variable:")
; error-hook omits suffix: #t

; There is no predicate immutable? in Scheme language?

