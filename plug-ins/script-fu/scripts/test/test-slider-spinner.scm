#!/usr/bin/env gimp-script-fu-interpreter-3.0

; Test slider and spinnner widgets i.e. SF-ADJUSTMENT
; re author's declaration of step, page, and digits.
; test case for #16532

; Expect "radius" widget to be a spinner with step 2, page 5, 
; and 3 digits after decimal point.
; I.E. an entry box with + and - buttons.
; LMB in + button increments by 2, LMB in - button decrements by 2.
; Pressing PageUp increments by 5, PageDown decrements by 5, when widget has focus.
; Entry box should show 3 digits after decimal point, e.g. 1020.000

; Expect "lighting" widget to be a slider with step 2, page 5, 
; and 3 digits after decimal point.
; I.E. a horizontal slider with a draggable thumb and + and - buttons.
; Expect similar behavior as above.

; See issue #16532 which is the origin of this test.
; The range is only 40, which is small enough to exhibit the issue.



(define (script-fu-test-slider-spinner radius light)
; does nothing, only testing GUI
)

(script-fu-register-procedure "script-fu-test-slider-spinner"
  _"Test slider and spinner widgets..."  ; menu item
  ""  ; tooltip
  "L. Konneker"
  "2026"
  SF-ADJUSTMENT "Radius"   (list 1020 1000 1040 2 5 3 SF-SPINNER)
  SF-ADJUSTMENT "Lighting" (list 1020 1000 1040 2 5 3 SF-SLIDER))

(script-fu-menu-register "script-fu-test-slider-spinner"
                         "<Image>/Filters/Development/Demos"))

; No translation
