#!/usr/bin/env gimp-script-fu-interpreter

; Test a .scm file with an invalid shebang
; Note "-3.0" missing above.

; The test depends on platform and env and .interp
; Must not be a file system link from gimp-script-fu-interpreter to gimp-script-fu-interpreter-3.0
; Must not be a .interp file having  "gimp-script-fu-interpreter=gimp-script-fu-interpreter-3.0"

; Expect in the console: "/usr/bin/env: 'script-fu-interpreter': No such file or directory"

(define (script-fu-test5)
  (gimp-message "Hello script-fu-test5")
)

; !!! No call to script-fu-menu-register
