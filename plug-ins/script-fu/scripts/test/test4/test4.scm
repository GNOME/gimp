#!/usr/bin/env gimp-script-fu-interpreter-3.0

; Test a .scm file that does not call script-fu-menu-register
; The menu will NOT default.
; Expect "Test SF interpreter 4" to NOT EXIST in any menu
; Expect the PDB proc "script-fu-test4" does appear in the PDB Browser

; Two test cases:
;    Alongside an executable script:  plug-ins/test4/test4.scm NOT executable
;    Executable, in its own directory: plug-ins/test1/test4.scm is executable

(define (script-fu-test4)
  (gimp-message "Hello script-fu-test4")
)

(script-fu-register "script-fu-test4"
  "Test SF interpreter 4"
  "Just gives a message from Gimp"
  "lkk"
  "lkk"
  "2022"
  ""  ; all image types
)

; !!! No call to script-fu-menu-register
