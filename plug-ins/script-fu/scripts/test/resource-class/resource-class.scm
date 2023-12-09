#!/usr/bin/env gimp-script-fu-interpreter-3.0

; A script that tests resource classes in GIMP
; Tests the marshalling of parameters and return values in ScriptFu
;
; Setup: copy this file w/ executable permission, and its parent dir to /plug-ins
; Example: to ~/.gimp-2.99/plug-ins/always-fail/always-fail.scm

; Delete .config/GIMP so that resources are in a standard state.

; Expect various resource names in the console
; Expect no "Fail" in the console


(define (script-fu-test-resource-class)

  (define (expect expression
                  expected-value )
      ; use equal?, don't use eq?
      (if (equal? expression expected-value)
          #t
          (gimp-message "Fail")
      )
  )

  ; redirect messages to the console
  (gimp-message-set-handler 1)

  (let* (
        ; Test as a return value
        ; These calls return a list with one element, use car
        (brush    (car (gimp-context-get-brush)))
        (font     (car (gimp-context-get-font)))
        (gradient (car (gimp-context-get-gradient)))
        (palette  (car (gimp-context-get-palette)))
        (pattern  (car (gimp-context-get-pattern)))

        ; font and pattern cannot be new(), duplicate(), delete()

        ; new() methods
        (brushnew    (car (gimp-brush-new    "Brush New")))
        (gradientnew (car (gimp-gradient-new "Gradient New")))
        (palettenew  (car (gimp-palette-new  "Palette New")))

        ; copy() methods
        ; copy method is named "duplicate"
        ; Takes an existing brush and a desired name
        (brushcopy    (car (gimp-brush-duplicate    brushnew    "brushcopy")))
        (gradientcopy (car (gimp-gradient-duplicate gradientnew "gradientcopy")))
        (palettecopy  (car (gimp-palette-duplicate  palettenew  "palettecopy")))

        ; See below, we test rename later
        )

    ; write names to console
    (gimp-message brush)
    (gimp-message font)
    (gimp-message gradient)
    (gimp-message palette)
    (gimp-message pattern)

    (gimp-message brushnew)
    (gimp-message gradientnew)
    (gimp-message palettenew)

    (gimp-message brushcopy)
    (gimp-message gradientcopy)
    (gimp-message palettecopy)

    ; Note equal? works for strings, but eq? and eqv? do not
    (gimp-message "Expect resources from context have de novo installed GIMP names")
    (expect (equal? brush "2. Hardness 050")   #t)
    (expect (equal? font "Sans-serif")         #t)
    (expect (equal? gradient "FG to BG (RGB)") #t)
    (expect (equal? palette "Color History")   #t)
    (expect (equal? pattern "Pine") #t)

    (gimp-message "Expect new resource names are the names given when created")
    (expect (equal? brushnew    "Brush New")    #t)
    (expect (equal? gradientnew "Gradient New") #t)
    (expect (equal? palettenew  "Palette New")  #t)

    (gimp-message "Expect copied resources have names given when created")
    ; !!! TODO GIMP appends " copy" and does not use the given name
    ; which contradicts the docs for the procedure
    (expect (equal? brushcopy    "Brush New copy")    #t)
    (expect (equal? gradientcopy "Gradient New copy") #t)
    (expect (equal? palettecopy  "Palette New copy")  #t)

    ; rename() methods
    ; Returns new resource proxy, having possibly different name than requested
    ; ScriptFu marshals to a string
    ; !!! Must assign it to the same var,
    ; else the var becomes an invalid reference since it has the old name
    (set! brushcopy    (car (gimp-brush-rename    brushcopy    "Brush Copy Renamed")))
    (set! gradientcopy (car (gimp-gradient-rename gradientcopy "Gradient Copy Renamed")))
    (set! palettecopy  (car (gimp-palette-rename  palettecopy  "Palette Copy Renamed")))

    ; write renames to console
    (gimp-message brushcopy)
    (gimp-message gradientcopy)
    (gimp-message palettecopy)

    (gimp-message "Expect renamed have new names")
    (expect (equal? brushcopy    "Brush Copy Renamed")    #t)
    (expect (equal? gradientcopy "Gradient Copy Renamed") #t)
    (expect (equal? palettecopy  "Palette Copy Renamed")  #t)

    (gimp-message  "Expect class method id_is_valid of the GimpResource class")
    ; the class method takes a string.
    ; ScriptFu already has a string var, and marshalling is trivial
    ; For now, returns (1), not #t
    (expect (car (gimp-brush-id-is-valid    brush))    1)
    (expect (car (gimp-font-id-is-valid     font))     1)
    (expect (car (gimp-gradient-id-is-valid gradient)) 1)
    (expect (car (gimp-palette-id-is-valid  palette))  1)
    (expect (car (gimp-pattern-id-is-valid  pattern))  1)

    (gimp-message "Expect class method id_is_valid for invalid name")
    ; Expect false, but no error dialog from GIMP
    ; Returns (0), not #f
    (expect (car (gimp-brush-id-is-valid    "invalid_name")) 0)
    (expect (car (gimp-font-id-is-valid     "invalid_name")) 0)
    (expect (car (gimp-gradient-id-is-valid "invalid_name")) 0)
    (expect (car (gimp-palette-id-is-valid  "invalid_name")) 0)
    (expect (car (gimp-pattern-id-is-valid  "invalid_name")) 0)

    (gimp-message "Expect as a parameter to context works")
    ; Pass each resource class instance back to Gimp
    (gimp-context-set-brush    brush)
    (gimp-context-set-font     font)
    (gimp-context-set-gradient gradient)
    (gimp-context-set-palette  palette)
    (gimp-context-set-pattern  pattern)

    (gimp-message "Expect delete methods work without error")
    ; call superclass method
    (gimp-resource-delete brushnew)
    (gimp-resource-delete gradientnew)
    (gimp-resource-delete palettenew)

    (gimp-message "Expect var holding deleted resource is still defined, but is invalid reference")
    ;  Returns (0), not #f
    (expect (car (gimp-brush-id-is-valid    brushnew))    0)
    (expect (car (gimp-gradient-id-is-valid gradientnew)) 0)
    (expect (car (gimp-palette-id-is-valid  palettenew))  0)

    ; We don't test the specialized methods of the classes here, see elsewhere
  )
)

(script-fu-register "script-fu-test-resource-class"
  "Test resource classes of Gimp"
  "Expect no errors in the console"
  "lkk"
  "lkk"
  "2022"
  ""  ; requires no image
  ; no arguments or dialog
)

(script-fu-menu-register "script-fu-test-resource-class" "<Image>/Test")
