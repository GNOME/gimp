; Test cases for string ports of kind output

; A port has a bifurcated API:
;    input API
;    output API.
; Some ports support both.
; The input API has write, but not read method.

; Some ports support byte and char operations.

; A string port is-a port.

; !!! write and read methods take or return Scheme objects
; i.e. strings, symbols, atoms, etc.

; A string port is of kind: input or output.
; A string port does not have all the methods of the port API:
;    kind output has write method, but not read.
;    kind input has read method, but not write.

; A string output port stores its contents in memory (unlike device ports).
; A get-output-string returns contents previously written.
; A string port is practically infinite.

; A string port is like a string.
; A sequence of writes are like a sequence of appends to a string,
; except the things written are objects, not just strings.

; You can only get the entire string.
; A get does not have the side effect of advancing a cursor in the string.

; write-byte is discouraged on an output string-port.
; Complicated by fact that strings are UTF-8 encoded.

; !!! The port object does not own a string object.
; The "string" internally is in a UTF-8 encoded C allocated chunk
; of memory, but not in a Scheme cell for a Scheme string.
; It survives garbage collection.
; Closing a port frees memory to C, but few cells to Scheme.

; Closing a port leaves the symbol defined until it goes out of scope,
; but the symbol no longer is bound to a port object
; i.e. operations on it fail.




; setup
; Some tests use new ports, not the setup one.

; This port is unlimited, should grow
(define aStringPort (open-output-string))



; tests

(test! "open-output-string yields a port")
(assert `(port? ,aStringPort))

(test! "open-output-string yields a port of kind output")
(assert `(output-port? ,aStringPort))

(test! "open-output-string yields a port NOT of kind input")
(assert `(not (input-port? ,aStringPort)))

(test! "read method fails on an output string-port")
(assert-error `(read ,aStringPort)
              "read: argument 1 must be: input port")

(test! "read-byte method fails on an output string-port")
(assert-error `(read-byte ,aStringPort)
              "read-byte: argument 1 must be: input port")




(test! "string get from port equals string write to port")
; !!! with escaped double quote
(assert `(string=?
           (let* ((aStringPort (open-output-string)))
              (write "foo" aStringPort)
              (get-output-string aStringPort))
           "\"foo\""))

(test! "string get from port equals string repr of symbol written to port")
; !!! without escaped double quote
(assert `(string=?
           (let* ((aStringPort (open-output-string)))
              ; !!! 'foo is-a symbol whose repr is three characters: foo
              ; write to a port writes the repr
              (write 'foo aStringPort)
              (get-output-string aStringPort))
           (symbol->string 'foo)))

(test! "get-output-string called twice returns the same string")
; Can get the same string twice
(assert `(string=?
           (begin
              (write "foo" ,aStringPort)
              (get-output-string ,aStringPort)
              (get-output-string ,aStringPort))
           "\"foo\""))

(test! "port contents survive garbage collection")
; using aStringPort whose contents are "foo"
(assert `(string=?
           (begin
              (gc)
              (get-output-string ,aStringPort))
           "\"foo\""))



; tests of the form (open-output-string <initial string>)

; Some Schemes have an optional argument a string that is the initial contents?
; Guile does not. Racket does not, but takes a name for the port. MIT does not.
;
; The initial string is now ignored, Gimp v3
; In v2, the initial string was always overwritten, but used for the contents.
; Only the size of the initial string ed, not the contents.
; Also, see test9.scm, which tests this using a string whose scope is larger
; and so does not get garbage collected.

(define aLimitedStringPort (open-output-string "initial"))

(test! "initial contents string is ignored")
; get-output-string returns empty string, not the initial contents.
(assert `(string=? (get-output-string ,aLimitedStringPort)
                    ""))

; This is a v2 test.
; (test! "writing to output string-port w initial contents may truncate")
; Only 7 chars are written, and a double quote char takes one
;(assert `(string=?
;           (begin
;              (write "INITIALPLUS" ,aLimitedStringPort)
;              (get-output-string ,aLimitedStringPort))
;           "\"INITIA"))


;(test! "port contents survive garbage collection")
; This is v3 test.
; using aStringPort whose contents are "INITIAL"
(assert `(string=?
           (begin
              (write "INITIAL",aLimitedStringPort)
              (gc)
              (get-output-string ,aLimitedStringPort))
           "\"INITIAL\""))





; write bytes

; initial contents "foo"

(test! "write-byte on output-string, ASCII char")
(assert `(write-byte (integer->byte 72) ,aStringPort))
; write is effective when byte is an ASCII char, valid UTF-8 encoding
; Note that the yield is a repr of a string followed by a repr of a char.
(assert `(string=? (get-output-string ,aStringPort)
                  "\"foo\"H"))

; This test corrupts aStringPort.
; It tests what an author should NOT do: write a byte that is not UTF-8 encoding.
(test! "write-byte on output-string, non ASCII char")
(assert `(write-byte (integer->byte 172) ,aStringPort))
; write yields strange results when byte is not a proper UTF-8 encoding.
; Note that the yield is same as before, and doesn't show the written byte.
(assert `(string=? (get-output-string ,aStringPort)
                  "\"foo\"H"))



; closing

(test! "closing a port")
(assert `(close-port ,aStringPort))

(test! "a closed port cannot be get-output-string")
(assert-error `(get-output-string ,aStringPort)
              "get-output-string: argument 1 must be: output port")

(test! "a closed port cannot be written")
(assert-error `(write 'foo ,aStringPort)
              "write: argument 2 must be: output port ")


; closing not affect prior gotten contents
(test! "closing output port not affect prior gotten contents")
; setup
(define aStringPort (open-output-string))
(write "foo" aStringPort)
(define contents (get-output-string aStringPort))
(close-port aStringPort)
(gc)
(assert `(string=? ,contents
                    "\"foo\""))




; What is read equals the string written.
; Edge case: writing more than 256 characters in two tranches
; where second write crosses end boundary of 256 char buffer.

; issue #9495
(assert '(string=?
           (let* ((aStringPort (open-output-string)))
              (write (string->symbol (make-string 250 #\A)) aStringPort)
              (write (string->symbol (make-string 7 #\B)) aStringPort)
              (get-output-string aStringPort))
           (string-append
              (make-string 250 #\A)
              (make-string 7 #\B))))




; read/write are opposites

; !!! Note in this case lack of escaped quotes on what is read

(test! "read's of a get-output-string return what was write'd before")
; setup
(define aOutStringPort (open-output-string))
(write "foo" aOutStringPort)
(write "bar" aOutStringPort)
(define aInStringPort (open-input-string (get-output-string aOutStringPort)))
(close-port aOutStringPort)
(gc)
; aInStringPort is open having contents "\"foo\"\"bar\""
; test the original strings can be read consecutively
(assert `(string=? (read ,aInStringPort)
                    "foo"))
(assert `(string=? (read ,aInStringPort)
                    "bar"))


