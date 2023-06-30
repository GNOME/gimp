; Test cases for sharp char expr for unicode chars outside ASCII range

; See sharp-expr-char.scm for sharp char expr inside ASCII range.

; See unichar.scm for tests of unichar without using sharp char expr

; This file also documents cases that the testing framework can't test
; since they are syntax errors
; or otherwise throw error in way that testing framework can't catch.
; Such cases are documented with pairs of comments in re-test format:
; First line starting with "; (" and next line "; <expected REPL display>"

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
; char=? also works
(assert `(char=? (integer->char 955) #\λ))

; a sharp char expr unicode is-a char
(assert (char? #\λ))

; sharp char hex expr is same as sharp char expr
(assert (equal? #\x3bb  #\λ))
; sharp char hex expr is-a char
(assert '(char? #\x3bb))

; Unichar extracted from string equals sharp char expr unicode
(assert (equal? (string-ref "λ" 0) #\λ))



;             Edge cases for sharp char expressions: hex: Unicode

; see also integer2char.scm
; where same cases are tested


;       extended ASCII 128-255

; 128 the euro sign
(assert #\x80)
; 255
(assert #\xff)

; 159 \xao is non-breaking space, not visible in most editors

; 256, does not fit in one byte
(assert #\x100)


; outside the Basic Multilingual Plane
; won't fit in two bytes

; Least outside: 65536
(assert #\x10000)

; max valid codepoint #\x10ffff
(assert #\x10ffff)

; Any 32-bit value greater than x10ffff yields a syntax error:
; syntax: illegal sharp constant expression
; and not testable in testing framework



;         extra tests of sharp char expr in other constructed expressions

; sharp char expr unicode passed to string function
(assert (string=? (string #\λ) "λ"))

; sharp char expr unicode in a list
(assert (equal? (list (string-ref "λ" 0)) '(#\λ)))

; sharp char expr unicode in vector
(assert (equal? (vector (string-ref "λ" 0)) '#(#\λ)))

; atom->string
(assert `(string=? (atom->string #\λ)
                   "λ"))



;             Quoted unichar

; quoted unichar is not type char
(assert `(not (char? 'λ)))
; quoted unichar is type symbol
(assert `(symbol? 'λ))




;                 unichar tested in REPL

; What follows are tests that can't be tested by the "testing" framework
; but can be tested by the "re-test" framework
; Testing framework can't test side effects on display.



; re-test display unichar
; (display (string-ref "λ" 0))
; λ#t

; re-test
; (begin (display "Unicode lambda char: ") (string-ref "λ" 0))
; Unicode lambda char: #\λ



;             Unicode character can be passed to error function and displayed

; Seems to be flaw in testing framework,
; this can't be tested:
;(assert-error `(error "Error λ")
;               "Error: Error λ")

; re-test
; (error "Error λ")
; Error: Error: λ



;               syntax errors in sharp char hex expr

; Syntax errors are not testable in testing framework.


; exceeding max range of int 32 codepoint
; longer than 8 hex digits \xf87654321
; > (assert '#\xf87654321

; re-test
; (null? #\xf87654321 )
; syntax: illegal sharp constant expression
; Also prints warning "Hex literal larger than 32-bit" to stderr


; A codepoint that fits in 32 bits but invalid UTF-8 encoding

; re-test
; (null? '#\xd800)
; syntax: illegal sharp constant expression
; Also prints warning "Failed make character from invalid codepoint." to stderr


; Edge error case: first invalid codepoint greater than max valid

; re-test
; (null? '#\x110000)
; syntax: illegal sharp constant expression
; Also prints warning "Failed make character from invalid codepoint." to stderr
