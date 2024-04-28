; Complete test of TinyScheme

; This does NOT KNOW the directory organization of the test files in repo.

; When you add a test file, also add it to meson.build,
; which DOES KNOW the dirs of the repo, but flattens into /test.

; Name clash must be avoided on the leaf filenames.


; test the testing framework itself
(testing:load-test "testing.scm")

(testing:load-test "cond-expand.scm")
(testing:load-test "atom2string.scm")
(testing:load-test "integer2char.scm")

(testing:load-test "string-escape.scm")

(testing:load-test "file-port.scm")
(testing:load-test "string-port-output.scm")
(testing:load-test "string-port-input.scm")
; WIP
; (testing:load-test "string-port-unichar.scm")

(testing:load-test "sharp-expr.scm")
(testing:load-test "sharp-expr-char.scm")
(testing:load-test "sharp-expr-unichar.scm")

; test unichar without using sharp char expr
(testing:load-test "unichar.scm")

(testing:load-test "vector.scm")

(testing:load-test "no-memory.scm")

; report the result
(testing:report)

; yield the session result
(testing:all-passed?)

