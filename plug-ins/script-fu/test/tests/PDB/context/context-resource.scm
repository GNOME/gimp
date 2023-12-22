; test resource methods of Context



; function to test methods on Resource for a valid Resource ID.
; Tests methods: get-name, id-is-valid, get-identifiers, is-editable.
(define (test-resource-methods resource)

  ; a resource is an int ID in ScriptFu
  (assert `(integer? ,resource))

  ; get-name returns a string
  (assert `(string? (car (gimp-resource-get-name ,resource))))

  ; id-is-valid returns truth
  (assert-PDB-true `(gimp-resource-id-is-valid ,resource))

  ; gimp-resource-get-identifiers succeeds
  ; it returns a triplet
  (assert `(gimp-resource-get-identifiers ,resource))

  ; gimp-resource-get-identifiers returns numeric for is-internal
  ; Some of the fresh gimp active resource are internal, some not !!!
  (assert `(number? (car (gimp-resource-get-identifiers ,resource))))

  ; name from get-identifiers is same as from gimp-resource-get-name
  ; name is second field of triplet i.e. cadr
  (assert `(string=? (cadr (gimp-resource-get-identifiers ,resource))
                     (car (gimp-resource-get-name ,resource))))

  ; gimp-resource-is-editable succeeds
  ; Returns a wrapped boolean
  ; !!! Not checking the result, only that it succeeds.
  (assert `(gimp-resource-is-editable ,resource))

  ; The fresh gimp active resources are all system resources i.e. not editable
  ; returns 0 for #f
  (assert-PDB-false `(gimp-resource-is-editable ,resource))
)

;                  "Test Parasite") ; name
;               "Procedure execution of gimp-resource-get-parasite failed")



; test context-get-resource returns active resource of given className
; This is setup, not asserted.

(define testBrush    (car (gimp-context-get-resource "GimpBrush")))
(define testFont     (car (gimp-context-get-resource "GimpFont")))
(define testGradient (car (gimp-context-get-resource "GimpGradient")))
(define testPalette  (car (gimp-context-get-resource "GimpPalette")))
(define testPattern  (car (gimp-context-get-resource "GimpPattern")))
; FUTURE Dynamics and other Resource subclasses


;              test methods on active resource ID's

(test-resource-methods testBrush)
(test-resource-methods testFont)
(test-resource-methods testGradient)
(test-resource-methods testPalette)
(test-resource-methods testPattern)


;             test more specific context methods return same result
;             as the general context-get-resource

; ID's are numeric
; test equality of numeric IDs
(assert `(= (car(gimp-context-get-brush))
            ,testBrush))
(assert `(= (car(gimp-context-get-font))
            ,testFont))
(assert `(= (car(gimp-context-get-gradient))
            ,testGradient))
(assert `(= (car(gimp-context-get-palette))
            ,testPalette))
(assert `(= (car(gimp-context-get-pattern))
            ,testPattern))


;              test resource-id-is-foo methods

; the resource IDs from setup work with the specific id-is-foo methods

(assert-PDB-true `(gimp-resource-id-is-brush ,testBrush))
(assert-PDB-true `(gimp-resource-id-is-font ,testFont))
(assert-PDB-true `(gimp-resource-id-is-gradient ,testGradient))
(assert-PDB-true `(gimp-resource-id-is-palette ,testPalette))
(assert-PDB-true `(gimp-resource-id-is-pattern ,testPattern))


;             test errors


; context-get-resource requires a valid type name
(assert-error `(gimp-context-get-resource "InvalidTypeName")
              "Procedure execution of gimp-context-get-resource failed")


