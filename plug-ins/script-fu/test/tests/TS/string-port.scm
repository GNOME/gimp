; Test cases for string ports

; a string port is-a port (having read and write methods).
; a string port stores its contents in memory (unlike device ports).
; A read returns contents previously written.
; A string port is practically infinite.

; a string port is like a string
; a sequence of writes are like a sequence of appends to a string


; Note that each assert is in its own environment,
; so we can't define a global port outside????
; Why shouldn't this work?
; (define aStringPort (open-output-string))
; (assert `(port? aStringPort))


; open-output-string yields a port
(assert '(port? (open-output-string)))

; string read from port equals string written to port
; !!! with escaped double quote
(assert '(string=?
           (let* ((aStringPort (open-output-string)))
              (write "foo" aStringPort)
              (get-output-string aStringPort))
           "\"foo\""))

; string read from port equals string repr of symbol written to port
; !!! without escaped double quote
(assert '(string=?
           (let* ((aStringPort (open-output-string)))
              ; !!! 'foo is-a symbol whose repr is three characters: foo
              ; write to a port writes the repr
              (write 'foo aStringPort)
              (get-output-string aStringPort))
           (symbol->string 'foo)))

; What is read equals the string written.
; For edge case: writing more than 256 characters in two tranches
; where second write crosses end boundary of 256 char buffer.

; issue #9495
; Failing
;(assert '(string=?
;           (let* ((aStringPort (open-output-string)))
;              (write (string->symbol (make-string 250 #\A)) aStringPort)
;              (write (string->symbol (make-string 7 #\B)) aStringPort)
;              (get-output-string aStringPort))
;           (string-append
;              (make-string 250 #\A)
;              (make-string 7 #\B))))



