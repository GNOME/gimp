#!/usr/bin/env gimp-script-fu-interpreter-3.0

; Test byte, file/string ports and string handling.

(define temp-path
  (string-append (car (gimp-gimprc-query "temp-path")) "/"))

(define (plugin-tmp-filepath name)
  (string-append temp-path "script-fu-test9-" name ".txt"))

; ---------- Helper functions ----------

(define (make-testresult success error-message)
  (list success error-message))
(define (testresult-success x) (car x))
(define (testresult-error x) (cadr x))

(define (displayln msg)
  (display msg)
  (newline))


(define (write-all-bytes port bytes)
  (if (null? bytes)
    '()
    (begin
      (write-byte (car bytes) port)
      (write-all-bytes port (cdr bytes)))))

; !!! bytes->string works for any byte except NUL 0x00
; In that case, it returns a string the prefix of the sequence of bytes up to the NUL.
; Also, the returned string might not be properly UTF-8 encoded
; and therefore might not display correctly.
(define (bytes->string bytes)
   (call-with-output-string
      (lambda (port) (map (lambda (b) (write-byte b port)) bytes))))

; Returns a testresult, a list of (success errmsg)
; Calls open-function to open a port on str
; Then calls function, which is a test function taking a port.
; The test function reads from the port.
; The test function returns a test-result object.
;
; We don't absolutely need this except that it captures failure to open string-port.
; Instead we could just call-with-input-string
(define (call-test-with-string str function)
  (let ((port (open-input-string str)))
    (if (port? port)
      (function port)
      (begin
        (gimp-message "Fail open string-port")
        (make-testresult #f "Failed to open string for string port!")))))


; any->string, call-with-input-string, and call-with-output-string are in init.scm

; Loops from i to n-1.
; Using recursion.
(define (loop i n function)
  (if (< i n)
    (begin
      (function i)
      (loop (+ i 1) n function))
    #t))

(define (assert code)
  (let* ((old-error-hook *error-hook*)
         (exceptions '())
         (append-exception
           (lambda (x)
             (if (null? exceptions)
               (set! exceptions "Exception: ")
               '())
             (set! exceptions (string-append exceptions " " (any->string x)))))
         (assert-error-hook
           (lambda (xs)
             (map append-exception xs)
             (old-error-hook xs)))
         (result #f))
    (set! *error-hook* assert-error-hook)
    (catch '() (set! result (eval code)))
    (set! *error-hook* old-error-hook)
    (if (and (null? exceptions)
             result)
      (make-testresult result '())
      (make-testresult #f
        (if (null? exceptions)
          (string-append "Assertion failed: " (any->string code))
          exceptions)))))

; ---------- Test data ----------

(define test-data-1byte
  (map integer->byte (list 65))) ; 65 = A

(define test-data-256bytes
  (let ((result '()))
    (loop 0 256 (lambda (i) (set! result (cons i result))))
  (reverse (map integer->byte result))))

; all bytes except NUL
; returns list of bytes in range 1..255
(define test-data-255bytes
  (let ((result '()))
    (loop 1 256 (lambda (i) (set! result (cons i result))))
  (reverse (map integer->byte result))))

(define test-data-1char
  (map integer->byte (list 228 189 160))) ; 你 (UTF-8)

(define test-data-2chars
  (map integer->byte
    (list 228 189 160    ; 你 (UTF-8)
          229 165 189))) ; 好 (UTF-8)

; ---------- Tests start here ---------

; Each test function should be individually executable or
; have a wrapper function that can be individually executed.

; ----- Test byte functions -----

; Ensure all integers with values in the range 0-255
; can be converted to a byte and then back successfully.
(define (test-byte-conversion)
  (let* ((errors '())
         (failed
           (lambda (error)
             (if (null? errors)
               (set! errors "")
               '())
             (set! errors (string-append errors error))))
         (test-conversion
           (lambda (i)
             (let ((result (assert `(= (byte->integer (integer->byte ,i)) ,i))))
               (if (not (testresult-success result))
                 (failed (testresult-error result))
                 '())))))
    (loop 0 256 test-conversion)
    (make-testresult (null? errors) errors)))

; Ensure byte? returns true with bytes.
(define (test-byte?-byte)
  (assert '(byte? (integer->byte 10))))

; Ensure byte? returns false with integers.
(define (test-byte?-integer)
  (assert '(not (byte? 10))))

; Ensure byte? returns false with characters.
(define (test-byte?-char)
  (assert '(not (byte? #\A))))

; Ensure atom? works with bytes.
(define (test-byte-atom?)
  (assert '(atom? (integer->byte 128))))

; Ensure atom->string works with bytes.
(define (test-byte-atom->string)
  (assert '(string=? (atom->string (integer->byte 65)) "A")))

; ----- Read tests for ports -----

; The same tests are used for file and string ports,
; as they must behave identically. These do not have to be
; individually executable, as they require the port to be set up.

; Ensure that we can read a single byte.
; Test data: test-data-1byte
(define (test-read-byte-single port)
  (assert `(= (byte->integer (read-byte ,port)) 65))) ; 65 = A

; Ensure peek-byte returns the correct value and does not remove bytes from the port.
; Test data: test-data-1byte
(define (test-read-byte-peek port)
  (assert
    `(and (= (byte->integer (peek-byte ,port)) 65) ; 65 = A
          (not (eof-object? (peek-byte ,port))))))

; Ensure every single possible byte value can be read.
; Test data: test-data-256bytes
(define (test-read-byte-all-values port)
  (let* ((errors '())
         (failure (lambda () ))
         (try
           (lambda (i)
             (let ((result (assert `(= (byte->integer (read-byte ,port)) ,i))))
               (if (not (testresult-success result))
                 (failure (testresult-error result))
                 '())))))
    (loop 0 256 try)
    (make-testresult (null? errors) errors)))

; This is improper test function.
; It aborts testing by call error instead of continuing
; An earlier version using loop suffered runaway memory exhaustion.
(define (test-read-byte-all-values-except-NUL port)
  (let* ((errors '())
         (try
           (lambda (i)
             (let ((result (assert `(= (byte->integer (read-byte ,port)) ,i))))
               (if (not (testresult-success result))
                 (error "failed test-read-byte-all-values-except-NUL")
                 '())))))
    (do ((i 1 (+ i 1)))
      ((< i 256) i)
      (try i))
    (make-testresult (null? errors) errors)))

; Ensure that we can read a single char (not multi-byte).
; Test data: test-data-1byte
(define (test-read-char-single-ascii port)
  (assert `(= (char->integer (read-char ,port)) 65))) ; 65 = A

; Ensure that we can read a single multi-byte char.
; Note: char->integer returns the integer value of a gunichar,
; which is a UTF-32 or UCS-4 character code.
; Test data: test-data-1char
(define (test-read-char-single port)
  (assert `(= (char->integer (read-char ,port)) 20320))) ; 20320 = 你 (UTF-32)

; Ensure peek-char returns the correct value and does not
; remove chars from the port.
; Test data: test-data-1char
(define (test-read-char-peek port)
  (assert
    `(and (= (char->integer (peek-char ,port)) 20320) ; 20320 = 你 (UTF-32)
          (not (eof-object? (peek-char ,port))))))

; Ensure that we can read multiple multi-byte chars from a file.
; Test data: test-data-2chars
(define (test-read-char-multiple port)
  (assert
    `(and (= (char->integer (read-char ,port)) 20320)    ; 20320 = 你 (UTF-32)
          (= (char->integer (read-char ,port)) 22909)))) ; 22909 = 好 (UTF-32)

; Ensure read-byte can not read past EOF.
; Test data: test-data-1byte
(define (test-read-byte-overflow port)
  (assert `(begin (read-byte ,port) (eof-object? (read-byte ,port)))))

; Ensure read-char can not read past EOF.
; Test data: test-data-1char
(define (test-read-char-overflow port)
  (assert `(begin (read-char ,port) (eof-object? (read-char ,port)))))

; ----- Write tests for ports -----

; These tests come in pairs, we write to a port and then read from it to verify.

(define (test-write-byte-single port)
  (assert `(begin (write-byte (integer->byte 77) ,port) #t))) ; 77 == M
(define (test-write-byte-single-verify port)
  (assert `(= (byte->integer (read-byte ,port)) 77))) ; 77 == M

(define (test-write-char-single port)
  (assert `(begin (write-char (car (string->list "你")) ,port) #t)))
(define (test-write-char-single-verify port)
  (assert `(= (char->integer (read-char ,port)) 20320))) ; 20320 = 你 (UTF-32)

; ----- String port tests -----

; Wrapper functions for the port read and write tests.

(define (test-input-string-port test-data test-function)
  (let ((test-string (bytes->string test-data)))
    (call-with-input-string test-string test-function)))
; WAS
;  (call-with-input-string (bytes->string test-data) test-function))

(define (test-string-port-read-byte-single)
  (test-input-string-port test-data-1byte test-read-byte-single))

(define (test-string-port-read-byte-peek)
  (test-input-string-port test-data-1byte test-read-byte-peek))

(define (test-string-port-read-byte-all-values-except-NUL)
  (test-input-string-port test-data-255bytes test-read-byte-all-values-except-NUL))

(define (test-string-port-read-char-single-ascii)
  (test-input-string-port test-data-1byte test-read-char-single-ascii))

(define (test-string-port-read-char-single)
  (test-input-string-port test-data-1char test-read-char-single))

(define (test-string-port-read-char-peek)
  (test-input-string-port test-data-1char test-read-char-peek))

(define (test-string-port-read-char-multiple)
  (test-input-string-port test-data-2chars test-read-char-multiple))

(define (test-string-port-read-byte-overflow)
  (test-input-string-port test-data-1byte test-read-byte-overflow))

(define (test-string-port-read-char-overflow)
  (test-input-string-port test-data-1char test-read-char-overflow))

; On a string port,
; check that what first function writes to port
; is what second function reads from port.
;
; test-data is not used, the verify function knows expected string.
;
; Either the write or read may fail but this version loses the write failure,
; so you should also test the write by itself.
(define (test-string-port-write test-data write-test verify-write-test)
; With SF v2 semantics for call-with-output-string
;  (let* ((str (make-string (length test-data)))
;         (write-result (call-with-output-string "" write-test))
;         (read-result (call-with-input-string str verify-write-test)))
 (let* ((written-str (call-with-output-string write-test))
        (write-result (make-testresult #t written-str))
        (read-result (call-test-with-string written-str verify-write-test)))
    (if (and (testresult-success write-result)
             (testresult-success read-result))
      (make-testresult #t '())
      (make-testresult #f
        (string-append
          "write-error: " (any->string (testresult-error write-result)) ", "
         "read-error: " (any->string (testresult-error read-result)))))))

(define (test-string-port-write-byte-single)
  (test-string-port-write test-data-1byte test-write-byte-single test-write-byte-single-verify))

(define (test-string-port-write-char-single)
  (test-string-port-write test-data-1char test-write-char-single test-write-char-single-verify))

; ----- File port tests -----

; Wrapper functions for the port read and write tests.

(define (test-input-file-port test-data test-function)
  (let ((filepath (plugin-tmp-filepath "fileport")))
    (call-with-output-file filepath (lambda (port) (write-all-bytes port test-data)))
    (call-with-input-file filepath test-function)))

(define (test-file-port-read-byte-single)
  (test-input-file-port test-data-1byte test-read-byte-single))

(define (test-file-port-read-byte-peek)
  (test-input-file-port test-data-1byte test-read-byte-peek))

(define (test-file-port-read-byte-all-values)
  (test-input-file-port test-data-256bytes test-read-byte-all-values))

(define (test-file-port-read-char-single-ascii)
  (test-input-file-port test-data-1byte test-read-char-single-ascii))

(define (test-file-port-read-char-single)
  (test-input-file-port test-data-1char test-read-char-single))

(define (test-file-port-read-char-peek)
  (test-input-file-port test-data-1char test-read-char-peek))

(define (test-file-port-read-char-multiple)
  (test-input-file-port test-data-2chars test-read-char-multiple))

(define (test-file-port-read-byte-overflow)
  (test-input-file-port test-data-1byte test-read-byte-overflow))

(define (test-file-port-read-char-overflow)
  (test-input-file-port test-data-1char test-read-char-overflow))

(define (test-file-port-write test-data write-test verify-write-test)
  (let* ((filepath (plugin-tmp-filepath "fileport"))
         (write-result (call-with-output-file filepath write-test))
         (read-result (call-with-input-file filepath verify-write-test)))
    (if (and (testresult-success write-result)
             (testresult-success read-result))
      (make-testresult #t '())
      (make-testresult #f
        (string-append
          "write-error: " (any->string (testresult-error write-result)) ", "
          "read-error: " (any->string (testresult-error read-result)))))))

(define (test-file-port-write-byte-single)
  (test-file-port-write
    test-data-1byte test-write-byte-single test-write-byte-single-verify))

(define (test-file-port-write-char-single)
  (test-file-port-write
    test-data-1char test-write-char-single test-write-char-single-verify))

; ----- Generic string tests -----

; Ensure basic string functions work.

(define (test-string-length)
  (assert '(= (string-length "Hello") 5)))

(define (test-string-length-multibyte)
  (assert '(= (string-length "你好") 2)))

(define (test-string->list-length)
  (assert '(= (length (string->list "Hello")) 5)))

(define (test-string->list-length-multibyte)
  (assert '(= (length (string->list "你好")) 2)))

(define (test-string-first-char)
  (assert '(= (char->integer (car (string->list "Hello"))) 72))) ; 72 = H

(define (test-string-first-char-multibyte)
  (assert '(= (char->integer (car (string->list "你好"))) 20320))) ; 20320 = 你 (UTF-32)

(define (test-string-overflow)
  (assert '(null? (cdr (string->list "H")))))

(define (test-string-overflow-multibyte)
  (assert '(null? (cdr (string->list "你")))))

; ----- Generic string tests on strings created using string port -----

; Test string functions on strings which are created by writing bytes into
; a string port.

; Write byte sequence of 你 into a string-port
; and ensure string-length of string from the port returns 1.
(define (test-string-port-string-count)
  (let* ((port (open-output-string)))
  (begin
    ; 你 = E4 BD A0 = 228 189 160
    (write-byte (integer->byte 228) port)
    (write-byte (integer->byte 189) port)
    (write-byte (integer->byte 160) port)
    ; not close port, it goes out of scope and is gc
    (assert
      `(and (= (char->integer (car (string->list (get-output-string ,port)))) 20320) ; 20320 = 你 (UTF-32)
            (= (string-length (get-output-string ,port)) 1))))))

; ---------- Test Execution ----------

; Count test results.
(define tests-succeeded 0)
(define tests-failed 0)

(define (test-succeeded)
  (set! tests-succeeded (+ tests-succeeded 1))
  (display "SUCCESS")
  (newline))
(define (test-failed msg)
  (set! tests-failed (+ tests-failed 1))
  (display "FAILED") (newline)
  (display msg) (newline))

(define (run-test test)
  ; log the test to Gimp Error Console when executed.
  ; So severe errors leave a trace.
  ; And until we fix SF hijacking stdout.
  (gimp-message (symbol->string test))
  (display test) (display ": ")
  (let ((result ((eval test))))
    (if (car result)
      (test-succeeded)
      (test-failed (cdr result)))))

(define (run-tests . tests)
  (map run-test tests))

(define (run-byte-tests)
  (run-tests
    'test-byte-conversion
    'test-byte?-byte
    'test-byte?-integer
    'test-byte?-char
    'test-byte-atom?
    'test-byte-atom->string))

(define (run-string-port-tests)
  (run-tests
    'test-string-port-read-byte-single
    'test-string-port-read-byte-peek
    'test-string-port-read-byte-all-values-except-NUL
    'test-string-port-read-char-single-ascii
    'test-string-port-read-char-single
    'test-string-port-read-char-peek
    'test-string-port-read-char-multiple
    'test-string-port-read-byte-overflow
    'test-string-port-read-char-overflow
    'test-string-port-write-byte-single
    'test-string-port-write-char-single))

(define (run-file-port-tests)
  (run-tests
    'test-file-port-read-byte-single
    'test-file-port-read-byte-peek
    'test-file-port-read-byte-all-values
    'test-file-port-read-char-single-ascii
    'test-file-port-read-char-single
    'test-file-port-read-char-peek
    'test-file-port-read-char-multiple
    'test-file-port-read-byte-overflow
    'test-file-port-read-char-overflow
    'test-file-port-write-byte-single
    'test-file-port-write-char-single))

(define (run-string-tests)
  (run-tests
    'test-string-length
    'test-string-length-multibyte
    'test-string->list-length
    'test-string->list-length-multibyte
    'test-string-first-char
    'test-string-first-char-multibyte
    'test-string-overflow
    'test-string-overflow-multibyte))

(define (run-string-tests-string-port)
  (run-tests
    'test-string-port-string-count))

(define (run-string-tests-string-port)
  (run-test 'test-string-port-string-count))

; SF hijacks stdout so all calls to display just write to SF (as an error msg)
; so it usually disappears.
; This is a hack, writing a few lines to Gimp Error Console using gimp-message.
; We should not need to redirect display to a string-port
; using with-log-to-gimp-message see below.
(define (run-all-tests)
  (gimp-message "Begin all tests")
  (displayln "========== Information ==========")
  (displayln "To run a single test individually, specify the name of the test.")
  (displayln (string-append "Temporary files with format 'script-fu-test9-*.txt' can be found in: " temp-path))
  (newline)
  (displayln "========== Testing byte functions ==========")
  (run-byte-tests)
  (newline)
  (displayln "========== Testing string port ==========")
  (run-string-port-tests)
  (newline)
  (displayln "========== Testing string functions ==========")
  (run-string-tests)
  (newline)
  (displayln "========== Testing string functions on strings created using string ports ==========")
  (run-string-tests-string-port)
  (newline)
  (displayln "========== Testing file port ==========")
  ; All file port tests will fail if writing to a file doesn't work properly,
  ; as test data is written to a temporary file. This was done so that the test
  ; data only exists in one place (in this file as list of bytes).
  (run-file-port-tests)
  (newline)
  (if (= tests-failed 0)
    ;(displayln "ALL tests passed!")
    (gimp-message "ALL tests passed!")
    ;(displayln
    (gimp-message
      (string-append
        "Test 9: " (number->string tests-failed)
        " tests FAILED. Run tests in Script-Fu console for details."))))

; wrapper that redirects display to gimp-message.
; Not used because buffering messages makes it hard to debug this test program.
; FUTURE: fix ScriptFu to not hijack the output port
; and display function will write to stdout as it should.
(define (with-log-to-gimp-message function)
    (gimp-message (call-with-output-string
                    (lambda (port)
                      (set-output-port port)
                      (function)))))

(define (name->function name)
  (eval (call-with-input-string (string-append "'" name) read)))

(define (script-fu-test9 testname)
  ; OLD (with-log-to-gimp-message (select-run-function testname)))
  ; NEW log to gimp later, at each test
  (gimp-message testname)
  (if (> (string-length testname) 0)
    (run-test (name->function testname))
    (run-all-tests)))

; ---------- Script registration ----------

(script-fu-register
  "script-fu-test9"
  "Test TinyScheme byte and UTF-8 char handling."
  "Logs tests to Gimp Error Console and stdout."
  "Richard Szibele"
  "Copyright (C) 2022, Richard Szibele"
  "2022"
  ""
  SF-STRING "Test name (optional)" ""
)

(script-fu-menu-register "script-fu-test9" "<Image>/Filters/Development/Demos")
