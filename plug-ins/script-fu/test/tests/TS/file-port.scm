; Test cases for file port



; setup
; Some tests use new ports, not the setup one.


(define aOutFilePort (open-output-file "testFile"))

; Note the file is in the current working directory

; Note read and write are in terms of objects,
; so a string in the file will have quotes.


; tests

; Output port

(test! "open-output-file yields a port")
(assert `(port? ,aOutFilePort))

; Note the file is overwritten if it already exists.

(test! "open-output-file yields a port of kind output")
(assert `(output-port? ,aOutFilePort))
(test! "open-output-file yields a port NOT of kind input")
(assert `(not (input-port? ,aOutFilePort)))

(test! "write succeeds on an output file-port")
(assert `(write "bar" ,aOutFilePort))

(test! "closing a port succeeds")
(assert `(close-port ,aOutFilePort))

(test! "closing a port a second time throws an error")
; FIXME Not tested.  The testing framework doesn't catch, and stops.
; Note it actually is still a port, but it is not open any more.
;(assert-error `(close-port ,aOutFilePort)
;              "Error: not a port")

; aOutFilePort is still in scope and shoud not be garbage collected.

(test! "A closed port is still a port")
(assert `(port? ,aOutFilePort))

(test! "A closed port has no direction")
(assert `(not (output-port? ,aOutFilePort)))
(assert `(not (input-port? ,aOutFilePort)))

(test! "a closed port cannot be written")
; Note it doesn't say "must be open"
(assert-error `(write "foo" ,aOutFilePort)
              "write: argument 2 must be: output port")


; Input port

(test! "a closed output file port can be then opened for input")
(define aInFilePort (open-input-file "testFile"))

(test! "open-input-file yields a port of kind input")
(assert `(input-port? ,aInFilePort))
(test! "open-input-file yields a port NOT of kind output")
(assert `(not (output-port? ,aInFilePort)))

(test! "write always fails on an input file-port")
(assert-error `(write "bar" ,aInFilePort)
              "write: argument 2 must be: output port")
(test! "write-char  always fails on an input file-port")
(assert-error `(write-char #\a ,aInFilePort)
              "write-char: argument 2 must be: output port")
(test! "write-byte always fails on an input file-port")
(assert-error `(write-byte (integer->byte 72),aInFilePort)
              "write-byte: argument 2 must be: output port")

(test! "string read from input file port equals what we wrote earlier")
(assert `(string=?
            (read ,aInFilePort)
            "bar"))

(test! "next read from input file port equals EOF")
(assert `(eof-object? (read ,aInFilePort)))

(test! "closing a input port")
(assert `(close-port ,aInFilePort))


(test! "a closed input port cannot be read")
; Note it doesn't say "must be open"
(assert-error `(read ,aInFilePort)
              "read: argument 1 must be: input port")




; input-output file port
(define aInOutFilePort (open-input-output-file "testFile"))

; Note the file is not overwritten if it already exists.

(test! "open-input-output-file yields a port")
(assert `(port? ,aInOutFilePort))

(test! "open-input-output-file yields a port of kind output")
(assert `(output-port? ,aInOutFilePort))
(test! "open-input-output-file yields a port also of kind input")
(assert `(input-port? ,aInOutFilePort))

(test! "string read from input file port equals what we wrote earlier")
(assert `(string=?
            (read ,aInOutFilePort)
            "bar"))

(test! "next read from input-output file port equals EOF")
(assert `(eof-object? (read ,aInOutFilePort)))

(test! "write-char space succeeds on an input-output file-port")
(assert `(write-char #\space ,aInOutFilePort))

(test! "write succeeds on an input-output file-port")
(assert `(write "zed" ,aInOutFilePort))

; The model is NOT of independent read and write cursors.
; See the following tests.

; This fails w string-length: argument 1 must be: string (/usr/local/share/gimp/3.0/tests/file-port.scm : 107)
;(test! "string read from input-output file port equals what we just wrote")
; read cursor points before what we just wrote
;(assert `(string=?
;            (read ,aInOutFilePort)
;            "zed"))

(test! "next read from input-output file port after a write equals EOF")
(assert `(eof-object? (read ,aInOutFilePort)))



; TODO
; close-port is generic for any port
; close-input-port on a input-output port should only close it for read
; and leave it open for write?

; TODO port with unichar contents






