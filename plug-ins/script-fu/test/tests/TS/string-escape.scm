; test string escape sequences

; An "escape sequence" is a sequence of characters that,
; when parsing a string, yields a single character.
; All escape sequences start with the backslash.

; TS is unicode: lengths are in unichars, not bytes

; 0xff is the C language notation for a hex constant

; Many tests are lax using string-length



; We can't test certain errors since they terminate
;  - Doublequote without trailing doublequote
;  - buffer overflow
;  - short hex escapes (<2 hex digits)





(test! "escaped doublequote")
(assert `(= (string-length "\"")   1))

; escaped newline, tab, carriage return
(assert `(= (string-length "\n")   1))
(assert `(= (string-length "\t")   1))
(assert `(= (string-length "\r")   1))

(test! "escaped backslash")
; escaped backslash, stands for itself
(assert `(= (string-length "\\")   1))

(test! "escaped other chars, ASCII")
; any other escaped char, that is not an octal digit, stands for itself
(assert `(= (string-length "\a")   1))

(test! "escaped other chars, unichar")
(assert `(= (string-length "\λ")   1))


; !!! Note that readable sequences for sharp constants for control chars
; are not suitable in strings.
; #\tab is not a sharp constant expression, and \tab is not a string escape
(assert `(= (string-length "\tab")     3))



; octal escape sequences
; FUTURE obsolete these: we don't need to support both hex and octal.

(test! "octal escapes")

(test! "octal NUL")
; one digit octal sequence
; NUL character, a zero byte, yields a string, but empty
(assert `(= (string-length "\0")   0))

; two digit octal sequence
; 0o11 is tab
(assert `(string=? "\11"  "\t"))

(test! "octal escaped characters match non-escaped ASCII characters")
; A is 65 is 0o101
(assert `(string=? "\101"  "A"))





; Three digit octal sequences that don't fit in a byte.
; Comments in the code says it should yield an error.
; So < 255, which is 0o377, should work.

(test! "octal 377")
; Yields a sequence of bytes that is not proper UTF-8 encoded, string length 0
(assert `(= (string-length "\377")   0))


; (test! "octal 400 yields error")
; In v2 the max value is 255, that fits in a byte.
; FIXME: the code comments says 0x400 should yield an error
;(assert-error `(string-length "\400")
;                "Error: Error reading string")

; !!! But in UTF-8 0x377==255 is encoded in two bytes
; and yields LATIN SMALL LETTER Y WITH DIAERESIS
; !!! length in chars is 1, length in bytes is 2.
; FUTURE (assert `(= (string-length "\377")   1))

; FUTURE: if we don't obsolete octal escapes altogether,
; then three or four octal digits should be allowed.
;(test! "octal 777")
; 0o777 is 0x1ff
; 1 char, encoded as 2 bytes.
; (assert `(= (string-length "\777")   1))
; TODO test the string is two-bytes
; we don't have string-length-bytes function

; four octal digits yields two char and three bytes.
; (assert `(= (string-length "\3777")   2))
; TODO test the second char is '7'




(test! "hex escapes")

(test! "hex NUL")
; NUL character, a zero byte, yields a string, but empty
(assert `(= (string-length "\x0000")   0))

;(test! "short hex escape")
; TODO Can't be tested, aborts interpreter, parsing fails
; maybe wrapping it in a string-port
; require at least two hex digits
;(assert-error `(string-length "\x")
;             "Error: Error reading string")
;(assert-error `(string-length "\x0")
;             "Error: Error reading string")

(test! "2 digit hex escape, ASCII")
; yields A
(assert `(= (string-length "\x41")   1))

(test! "2 digit hex escape, non-ASCII > 127")
; FIXME, fails string length 0 i.e. returns EOF object
; See scheme.c line 1957 *p++=c is pushing one byte
;
; Yields LATIN SMALL LETTER Y WITH DIAERESIS
; Yields one character of two UTF-8 bytes.
;(assert `(= (string-length "\xff")   1))

; Uppercase \XFF also accepted
; yields LATIN SMALL LETTER Y WITH DIAERESIS
;(assert `(= (string-length "\XFF")   1))


(test! "3 digit hex escape x414 yields two characters")
; This is the current behavior.
; SF parses only two hex digits as part of the hex escape,
; and the third hex digit is parsed as itself.
; FUTURE parse a max of four bytes of hex, like say Racket
; yields A4
(assert `(= (string-length "\x414")   2))

; FUTURE: Now does not accept
;(test! "3 digit hex escape")
; yields one unnamed char, 3 bytes
;(assert `(= (string-length "\xfff")   1))

;(test! "4 digit hex escape")
; yields unnamed char, 3 bytes
;(assert `(= (string-length "\xffff")   1))

;(test! "5 digit hex escape")
; yields 2 chars, the unnamed char, 3 bytes,
; and the char LOWER CASE F, 1 byte
;(assert `(= (string-length "\xfffff")   1))



; Every four digit hex value is a valid codepoint
; meaning it will encode in UTF-8.
; Whether it displays a visible glyph depends on other factors.




(test! "consecutive escape sequences")


(test! "consecutive hex escapes")
; two A chars
(assert `(= (string-length "\x41\x41")   2))

; FIXME fails
;(test! "consecutive hex escapes")
; two CENT chars
;(assert `(= (string-length "\xa2\xa2")   2))

; FIXME fails
; (test! "consecutive octal escapes")
; two CENT chars
; (assert `(= (string-length "\242\242")   2))

(test! "consecutive escaped backslash and hex escape")
; yields 3 characters: BACKSLASH, A, BACKSLASH,
(assert `(= (string-length "\\\x41\\")   3))

; FIXME fails
;(test! "consecutive escaped backslash and hex escape")
; yields 3 characters: BACKSLASH, CENT, BACKSLASH,
;(assert `(= (string-length "\\\xa2\\")   3))








