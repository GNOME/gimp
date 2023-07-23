; Test cases for sharp char expr for unicode chars outside ASCII range

; See sharp-expr-char.scm for sharp char expr inside ASCII range.

; See unichar.scm for tests of unichar without using sharp char expr

; This is NOT a test of the REPL: ScriptFu Console.
; A REPL displays using obj2str,
; or internal atom2str() which this doesn't test.

; ScriptFu Console (the REPL) displays a "sharp char expression" to represent
; all atoms which are characters, e.g. #\a .
; A "sharp hex char expression" also
; represents a character, e.g. #\x32.
; But the REPL does not display that representation.


; conversion from number to character equal sharp char expr unicode
(assert `(equal? (integer->char 955) #\λ))

; hex sharp char is same as sharp char
(assert (equal? #\x3bb  #\λ))
(assert '(char? #\x3bb))

; Unichar extracted from string equals sharp char expr unicode
(assert (equal? (string-ref "λ" 0) #\λ))


; Omitted, a test of the REPL
; quoted Unichar is equal to its usual representation
(assert `(equal? ,λ #\λ))

; a sharp char expr unicode is-a char
(assert (char? #\λ))


; sharp char expr unicode passed to string function
(assert (equal? (string #\λ) "λ")

; sharp char expr unicode in a list
(assert (equal? (list (string-ref "λ" 0)) '(#\λ))

; sharp char expr unicode in vector
(assert (equal? (vector (string-ref "λ" 0)) '#(#\λ))


; Omitted: a test of REPL
; display unichar
; > (display (string-ref "λ" 0))
; λ#t


; This is 0x7 BEL
; TODO for greek lambda unicode char
; string function takes sequence of chars
(assert (equal? (string #\) ""))







;                 unichar in context of other expressions

; unichar evaluated after evaluating display.
; Prints side effect of display
; followed by value of the last expression, a unichar
;> (begin (display "Unicode lambda char: ") (string-ref "λ" 0))
;Unicode lambda char: #\λ

; Single Unicode character outside of string can be displayed in error
;>(display "Unicode lambda char: ")(error "testUnicodeKo1: " (string-ref "λ" 0))
;Unicode lambda char: #tError: testUnicodeKo1:  #\λ
; TODO this test seems to infinite loop
; Maybe the problems is catting string to char
;(assert-error `(error "Error: " (string-ref "λ" 0))
;              "Error: λ")
;(assert-error `(error "Error: " "λ")
 ;              "Error: λ")
;(assert-error `(error "Error λ")
;               "Error: λ")
; Seems to be flaw in testing framework



; Edge case: first invalid codepoint greater than max valid
; '#\x110000
; TODO

; A codepoint that fits in 32 bits but invalid UTF-8 encoding
; '#\xd800
; TODO

; sharp constant hex exceeding max range of int 32 codepoint
; longer than 8 hex digits \xf87654321

; FIXME currently this fails to pass or fail
; Yields #\! in the REPL ????
; (assert '#\xf87654321 )

; FIXME This passes but it should not.
; It should be a syntax or other error
; If syntax, not testable here.
(assert `(not (null? #\xf87654321 )))