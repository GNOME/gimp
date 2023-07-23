; test integer->char function

; Is R5RS, but with unicode


; General test strategy:
; Generate char atom using integer->char.
; Convert each such char atom to string.
; In all cases where it was possible to create such a string, test length is 1.

; See also number->string which is similar to (atom->string (integer->char <foo>) <base>)


; integer->char takes only unsigned (positive or zero) codepoints
; -1 in twos complement is out of range of UTF-8
(assert-error `(integer->char -1)
              "integer->char: argument 1 must be: non-negative integer")



;            ASCII NUL character.

; 0 is a valid codepoint
; whose repr in the REPL is #\x0
; > (integer->char 0)
; "#\x0"

; Converting the atom to string yields an empty string
(assert `(string=? (atom->string (integer->char 0))
                   ""))

; ASCII null terminates the string early, so to speak.

; You can also represent as escaped hex "x\00"
(assert `(string=? "\x00"
                   ""))
; Escaped hex must have more than one hex digit.
; Can't be tested: (assert-error `(string? "\x0") "Error reading string ")
; In REPL:
; > "\x0"
; Error: Error reading string


;           the first non-ASCII character;

; TODO (integer->char 128),


;           first Unicode character outside the 8-bit range;

; evaluates without complaint
(assert (integer->char 256))

; length of converted string is 1
; The length is count of characters, not the count of bytes.
(assert `(= (string-length (atom->string (integer->char 256)))
            1))

; converted string is equivalent to a string literal that displays
(assert `(string=? (atom->string (integer->char 256))
                   "Ā"))


;      first Unicode character outside the Basic Multilingual Plane

(assert (integer->char 65536))

(assert `(= (string-length (atom->string (integer->char 65536)))
            1))

; The usual glyph in some editors is a wide box with these digits inside:
;   010
;   000
; Other editors may display a small empty box.
(assert `(string=? (atom->string (integer->char 65536))
                   "𐀀"))

; !!! Note the SF Console REPL yields #\
; which is not the correct representation.
; FIXME