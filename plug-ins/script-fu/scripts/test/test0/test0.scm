#!/usr/bin/env gimp-script-fu-interpreter-3.0

; Basic test of a .scm file interpreted by script-fu-interpreter
;
; Setup: copy this file w/ executable permission, and its parent dir to /plug-ins
; Example: to ~/.gimp-2.99/plug-ins/test0/test0.scm
; (That is custom to one user.)

; Expect "Test>Test SF interpreter 0" in the menus
; Expect when chosen, message on GIMP message bar.

; Also, remove the execute permission.
; Then expect not appear in GIMP menus (not queried.)

; Also, make the name different from its parent dir.
; Then expect not appear in GIMP menus (not queried.)

(define (script-fu-test0)
  (gimp-message "Hello script-fu-test0")
)

(script-fu-register "script-fu-test0"
  "Test SF interpreter 0"
  "Just gives a message from Gimp"
  "lkk"
  "lkk"
  "2022"
  ""  ; all image types
)

(script-fu-menu-register "script-fu-test0" "<Image>/Test")
