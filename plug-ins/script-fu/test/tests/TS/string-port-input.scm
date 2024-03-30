; Test cases for string ports of kind input

; See general discussion of string ports at string-port-output.scm


; You read objects from a string.
; A read has the side effect of advancing a cursor in the string.

; read-byte is discouraged on an output string-port.
; Complicated by fact that strings are UTF-8 encoded.

; read-char is a method also

; !!! The input port object does not own a string object.
; The "string" internally is a C pointer to a Scheme cell for a Scheme string.
; The port does not have a cell referring to the cell for the string.
; It does NOT survive garbage collection.
; Closing a port frees memory to C, but few cells to Scheme.

; Closing a port leaves the symbol defined until it goes out of scope,
; but the symbol no longer is bound to a port object
; i.e. operations on it fail.




; setup
; Some tests use new ports, not the setup one.

; Note initial contents is a sequence of alphabetic chars,
; which reads as one symbol object.
(define aStringPort (open-input-string "foo"))



; tests

(test! "open-input-string yields a port")
(assert `(port? ,aStringPort))

(test! "open-input-string yields a port of kind input")
(assert `(input-port? ,aStringPort))

(test! "open-input-string yields a port NOT of kind output")
(assert `(not (output-port? ,aStringPort)))

(test! "write always fails on an input string-port")
(assert-error `(write "bar" ,aStringPort)
              "write: argument 2 must be: output port")

(test! "write-char  always fails on an input string-port")
(assert-error `(write-char #\a ,aStringPort)
              "write-char: argument 2 must be: output port")

(test! "write-byte always fails on an input string-port")
(assert-error `(write-byte (integer->byte 72),aStringPort)
              "write-byte: argument 2 must be: output port")

(test! "get-output-string always fails on an input string-port")
(assert-error `(get-output-string ,aStringPort)
              "get-output-string: argument 1 must be: output port")



; read

; refresh the port
(define aStringPort (open-input-string "foo"))

(test! "string read from input-string equals initial contents of port, one symbol")
; yields a symbol whose repr is "foo"
; ??? This seems to fail sometimes, possibly due to gc, see below?
(assert `(string=?
            (symbol->string (read ,aStringPort))
            "foo"))

(test! "next read from input-string equals EOF")
(assert `(eof-object? (read ,aStringPort)))

; Note now the port is empty and for testing we must make another




; port with unichar contents

(define aStringPort (open-input-string "λ"))

; issue #11040 was returning EOF where it should return a unichar char as a symbol
(test! "read from input-string with unichar content equals that unichar as symbol")
; yields a symbol whose repr is "λ"
(assert `(string=?
            (symbol->string (read ,aStringPort))
            "λ"))



; port with escape sequence for NUL char
(define aStringPort (open-input-string "a\x00b"))

(test! "read from input-string with escape sequence for NUL is truncated")
; yields a symbol whose repr is "a"
(assert `(string=?
            (symbol->string (read ,aStringPort))
            "a"))



; read multiple objects

(test! "read two symbols")
; two symbols in the in port, separated by a delimiter space
(define aStringPort (open-input-string "λ bar"))
(assert `(string=?
            (begin
              ; read and discard
              (read ,aStringPort)
              (symbol->string (read ,aStringPort)))
            "bar"))


; garbage collection

(define aStringPort (open-input-string "foo"))
; using aStringPort whose contents read as a symbol "foo"

(test! "input string-port with literal contents MAY NOT survive garbage collection")
; FIXME this test fails, either the test is wrong, or a bug in test framework, or there is a bug in TS
; FIXME the test is v2.  In v3, a literal should survive gc
; !!! We wrote "foo" but assert that "foo" is NOT THE CONTENTS
; This test corrupts the port.
; This test is of a random result and may fail.
; After gc, a C pointer of the port implementation
; is pointing to the garbage collected string,
; some memory whose contents are undefined.
; Usually a symbol is returned and it is not "foo".
; But it could still be "foo".
;(assert `(not
;            (string=?
;             (symbol->string
;               (begin
;                  (define aStringPort (open-input-string "foo"))
;                  (gc)
;                  (gimp-message "here")
;                  (read aStringPort)))
;             "foo")))





;    read-char and read-byte


(define aStringPort (open-input-string "foo"))

(test! "read-char on input string-port, ASCII")
; read-char works, but discouraged from mixing with read
; since read parses a Scheme object, and the read char might
; be syntax.
(assert `(equal?
            (read-char ,aStringPort)
            #\f ))


(define aStringPort (open-input-string "λ"))

(test! "read-char on input string-port, unichar")
(assert `(equal?
            (read-char ,aStringPort)
            #\λ ))


; Example code for getting char from byte from read-byte
; (integer->char (byte->integer (read-byte port)))



; read-byte
;
; read-byte should not be mixed with read-char or read, without care.

(define aStringPort (open-input-string "foo"))

(test! "read-byte to EOF on input-string, ASCII chars")
; The first byte is the single byte UTF-8 encoding of f char,
; then two bytes each the o char, then EOF
(assert `(eof-object?
           (begin
              (read-byte ,aStringPort)
              (read-byte ,aStringPort)
              (read-byte ,aStringPort)
              (read-byte ,aStringPort))))


(define aStringPort (open-input-string "λa"))

(test! "read-byte then read-char on input-string, two-byte UTF-8 encoded char")
; The first byte of the lambda char is 0xce 206, the next 0xbb 187, code point is 0x3bb
; Expect this leaves the port in condtion for a subsequent read-char or read
(assert `(= (byte->integer (read-byte ,aStringPort))
            206))
(assert `(= (byte->integer (read-byte ,aStringPort))
            187))
(assert `(equal? (read-char ,aStringPort)
                  #\a))




; closing

(define aStringPort (open-input-string "foo"))

(test! "closing a port")
(assert `(close-port ,aStringPort))

(test! "a closed port cannot be read")
(assert-error `(read ,aStringPort)
              "read: argument 1 must be: input port")

