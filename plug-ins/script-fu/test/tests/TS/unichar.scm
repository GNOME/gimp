; Test cases for unicode chars outside ASCII range

; !!! These tests don't use sharp char expr, but the chars themselves.
; See sharp-expr-unichar.scm for sharp char expr denoting unichars
; outside ASCII range.

; History: we avoid sharp char expr for unicode here
; because of bug #9660.
; Loosely speaking, ScriptFu was handling unichars,
; but not sharp char expr for them.

; Most test cases are for atoms that are type "char",
; meaning a component of a string.
; ScriptFu implementation uses a C type: gunichar,
; which holds a UTF-8 encoding of any Unicode code point.)
; A unichar is as many as four bytes, not always one byte.

; This is NOT a test of the REPL: ScriptFu Console.
; A REPL displays using obj2str,
; or internal atom2str() which this doesn't test.

; ScriptFu Console (the REPL) displays a "sharp char expression" to represent
; all atoms of type char, e.g. #\a .
; A "sharp hex char expression" also
; represents a character, e.g. #\x32.
; But the REPL does not display that representation.


; conversion from number to character equal sharp char
(assert `(equal? (integer->char 955)
                 (string-ref "λ" 0)))


; Unichar itself (a wide character) can be in the script
; but is unbound
(assert-error `(eval λ) "eval: unbound variable:")
; Note the error message is currently omitting the errant symbol


; Unichar in a string
(assert (string=? (string (string-ref "λ" 0)) "λ"))


; Omitted: a test of REPL
; display unichar
; > (display (string-ref "λ" 0))
; λ#t


; Quoted unichar
; These test that the script can contain unichars
; versus test that a script can process unichars.

; quoted unichar is not type char
(assert `(not (char? 'λ)))

; quoted unichar is type symbol
(assert (symbol? 'λ))
