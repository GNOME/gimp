; test string ports with unichar

; WIP: requires changes to string escapes: four hex digits
; Not currently in the test suite

; This tests unichars in strings work nicely with string-ports.
; Algebraic tests combining two different areas of the code.
; The concern is that bytes and chars are counted correctly.

; Also tests hex escape sequences, which are required to express
; edge case unichars otherwise not possible to put in a text.

; The edge cases of unichar:
;  - simple non-ASCII unichar character named LAMBDA
;  - edge of UTF-8 U+FFFF
;  - edge of NUL-terminated strings: ASCII character named NUL


; TODO testing of string-ports in string-port.scm could contain this





; input-string

(test! "string containing unichar as symbol parses")

; open-input-string yields string without double quotes
; read from that string is-a list containing a symbol
(assert `(list? (read (open-input-string "'(λ)"))))
; car of that list is-a symbol
(assert `(symbol? (car (read (open-input-string "'(λ)")))))


(test! "string containing unichar as character parses into symbol")
; \x3bb is the hex escaped for the LAMBDA character
; but it is in the input string as a symbol
(assert `(symbol? (read (open-input-string "\x3bb"))))

(test! "string containing string embedding a unichar parses into string")
; string inside string via escaped double quotes
(assert `(string? (read (open-input-string "\"\x3bb\""))))

(test! "string containing string embedding a unprintable unichar parses into string")
; string inside string via escaped double quotes
(assert `(string? (read (open-input-string "\"\xFFFF\""))))


; output-string

; open-output-string takes an optional "initial contents string"

(test! "get-output-string from output port equals string written to port")
(test! "simple LAMBDA")
; !!! with escaped double quote
(assert '(string=?
           (let* ((aStringPort (open-output-string)))
              (write "λ" aStringPort)
              (get-output-string aStringPort))
           "\"λ\""))

(test! "U+FFFF")
(assert '(string=?
           (let* ((aStringPort (open-output-string)))
              (write "\xFFFF" aStringPort)
              (get-output-string aStringPort))
           "\"\xFFFF\""))

(test! "writing embedded NUL to output port shortens the string")
; !!! NUL written as \x0000 since \x00af is a hex escape sequence

; I don't think this leaks memory, the internal C string
; is correctly managed if the code always uses gunichar functions
; to calculate the internal strlength in unichars ?

(assert '(string=?
           (let* ((aStringPort (open-output-string)))
              (write "before\x0000after" aStringPort)
              (get-output-string aStringPort))
           "\"before\""))

(test! "initial contents to a new output port, with unichar")
(assert '(string=?
           (let* ((aStringPort (open-output-string "λ")))
              (get-output-string aStringPort))
           "λ"))


; input-output-string

; open-input-output-string is non-standard Scheme, even not R5RS.
; And it is poorly documented.
; I suppose it is supposed to be a pipe, where read() consumes,
; and get-output-string returns the contents without consuming.

; !!! read is different from get-output-string
; read forms a Scheme object
; Here, the Scheme object is a symbol.

; FIXME fails ??? but because it is unichar, or because it fails with ASCII?

(test! "string read from input-output port equals string written to port")
(assert '(string=?
           (let* ((aStringPort (open-input-output-string "foo")))
              (write "λ" aStringPort)
              (symbol->string (read aStringPort)))
           "\"fooλ\""))