#!/usr/bin/env gimp-script-fu-interpreter-3.0

; A script to test calls to Scheme function: (quit 1)
;
; Setup: copy this file w/ executable permission, and its parent dir to /plug-ins
; Example: to ~/.gimp-2.99/plug-ins/test-quit/test-quit.scm

; Expect "Filters>Dev>Script-Fu>Test>Quit with code" in the menus

; Test interactive:
; Choose "Quit with code".  Expect the plugin's dialog.
; Choose OK.
; Expect:
;   1. a message in stderr
;   2. an error dialog in GIMP that must be OK'd
;      OR a message in Gimp Error Console when it is open.)
; !!! FIXME: this fails now, for reasons unrelated to (quit)

; Repeat, but enter 0.
; Expect:
;   No error in stderr OR Gimp

; Test non-interactive:
; Enter "(script-fu-test-quit 1)" in SF Console
; Expect:
;    1. a message in stderr
     2. SF Console to print the error message.

; In both test case, the error message is like:
; "Execution error for 'Quit with code': script quit with code: 1"

(define (script-fu-test-quit code)
  (quit code)
)

(script-fu-register "script-fu-test-quit"
  "Quit with code"
  "Expect error in Gimp, or PDB execution error when called by another"
  "lkk"
  "lkk"
  "2023"
  ""  ; requires no image
  ; The argument is an integer, defaulting to 1, that the script will call quit with.
  SF-ADJUSTMENT "Return code" '(1 -5 5 1 2 0 0)
)

(script-fu-menu-register "script-fu-test-quit"
                         "<Image>/Filters/Development/Script-Fu/Test")

