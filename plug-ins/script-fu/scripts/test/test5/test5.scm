#!/usr/bin/env ligma-script-fu-interpreter

; Test a .scm file with an invalid shebang
; Note "-3.0" missing above.

; The test depends on platform and env and .interp
; Must not be a file system link from ligma-script-fu-interpreter to ligma-script-fu-interpreter-3.0
; Must not be a .interp file having  "ligma-script-fu-interpreter=ligma-script-fu-interpreter-3.0"

; Expect in the console: "/usr/bin/env: 'script-fu-interpreter': No such file or directory"

(define (script-fu-test5)
  (ligma-message "Hello script-fu-test5")
)

; !!! No call to script-fu-menu-register
