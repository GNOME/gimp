#!/usr/bin/env gimp-script-fu-interpreter-3.0

; Calls scheme (display ...)
; Tests it prints to any stdout console where Gimp was started.

; Then calls (error ...)
; Tests Gimp declares an error.

; in v2, (display ...) did not go to the terminal,
; but was prepended to any error message,
; or disappeared when there was no error.



(define (script-fu-test-display)

  ;       test display function

  ; display shows a passed string
  (display "foo")

  ; display shows repr of any atom or list
  (display '(1 2 "bar"))

  ; print is same as display but adds newline
  ; shows repr of a function #<CLOSURE>
  (print gimp-message)

  (gimp-message "Called display: expect foo(1 2 bar)#<CLOSURE> in terminal")



  ;       test error function

  ; Call to error yields:
  ;    dialog when Gimp Error Console not open
  ;    else text in the open Gimp Error Console
  (gimp-message "Called error: expect Gimp dialog, OR error in Gimp Error Console.")
  ; Scheme objects print their representation.
  ; Here gimp-message should print as #<CLOSURE>
  (error "Reason" gimp-message)

  ; Call to error returns to Gimp, this should not be reached.
  (gimp-message "Test failed: unreachable statement was reached.")
)

(script-fu-register "script-fu-test-display"
    "Test scheme display and error functions"
    "Test (display ...) to console, and (error ...) returns err to Gimp"
    "lkk"
    "lkk"
    "2024"
    ""
)

(script-fu-menu-register "script-fu-test-display"
                         "<Image>/Filters/Development/Demos")
