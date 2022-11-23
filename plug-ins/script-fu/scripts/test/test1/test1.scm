#!/usr/bin/env ligma-script-fu-interpreter-3.0

; Basic test that a second .scm file is also queried.
; Expect "Test>Test SF interpreter 1" in the menus
; Expect when chosen, message on LIGMA message bar.

; Also tests that one .scm file can define two PDB procedures
; File is queried once, yielding two names.
; Two separate procedures created.


(define (script-fu-test1)
  (ligma-message "Hello script-fu-test1")
)

(script-fu-register "script-fu-test1"
  "Test SF interpreter 01"
  "Just gives a message from Ligma"
  "lkk"
  "lkk"
  "2022"
  ""  ; all image types
)

(script-fu-menu-register "script-fu-test1" "<Image>/Test")




(define (script-fu-test2)
  (ligma-message "Hello script-fu-test2")
)

(script-fu-register "script-fu-test2"
  "Test SF interpreter 02"
  "Just gives a message from Ligma"
  "lkk"
  "lkk"
  "2022"
  ""  ; all image types
)

(script-fu-menu-register "script-fu-test2" "<Image>/Test")
