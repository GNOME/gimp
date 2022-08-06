#!/usr/bin/env gimp-script-fu-interpreter-3.0

; Basic test that a second .scm file is also queried.
; Expect "Test>Test SF interpreter 1" in the menus
; Expect when chosen, message on GIMP message bar.

; Also tests that one .scm file can define two PDB procedures
; File is queried once, yielding two names.
; Two separate procedures created.


(define (script-fu-test1)
  (gimp-message "Hello script-fu-test1")
)

(script-fu-register "script-fu-test1"
  "Test SF interpreter 01"
  "Just gives a message from Gimp"
  "lkk"
  "lkk"
  "2022"
  ""  ; all image types
)

(script-fu-menu-register "script-fu-test1" "<Image>/Test")




(define (script-fu-test2)
  (gimp-message "Hello script-fu-test2")
)

(script-fu-register "script-fu-test2"
  "Test SF interpreter 02"
  "Just gives a message from Gimp"
  "lkk"
  "lkk"
  "2022"
  ""  ; all image types
)

(script-fu-menu-register "script-fu-test2" "<Image>/Test")
