; test resource methods of Context

; !!! This requires a fresh install of GIMP,
; so that the context is the default,
; and not the user's last choice from prior session.

(script-fu-use-v3)



; function to test methods on Resource for a valid Resource ID.
; Tests methods: get-name, id-is-valid, get-identifiers, is-editable.
(define (test-resource-methods resource)

  ; a resource is an int ID in ScriptFu
  (assert `(integer? ,resource))

  ; get-name returns a string
  (assert `(string? (gimp-resource-get-name ,resource)))

  ; id-is-valid returns truth
  (assert `(gimp-resource-id-is-valid ,resource))


  ; since 3.0rc3 resource-get-identifiers is private to libgimp
  ; gimp-resource-get-identifiers succeeds
  ; it returns a triplet
  ;(assert `(gimp-resource-get-identifiers ,resource))

  ; gimp-resource-get-identifiers returns numeric for is-internal
  ; Some of the fresh gimp active resource are internal, some not !!!
  ;(assert `(number? (gimp-resource-get-identifiers ,resource)))

  ; name from get-identifiers is same as from gimp-resource-get-name
  ; name is second field of triplet i.e. cadr
  ;(assert `(string=? (cadr (gimp-resource-get-identifiers ,resource))
  ;                  (gimp-resource-get-name ,resource)))


  ; gimp-resource-is-editable succeeds
  ; Returns a wrapped boolean
  ; !!! Checking the result.
  ; Since the test should be run with context the fresh default,
  ; all should be resources owned by GIMP and not editable.
  (assert `(not (gimp-resource-is-editable ,resource)))

  ; The fresh gimp active resources are all system resources i.e. not editable
  ; returns 0 for #f
  (assert `(not (gimp-resource-is-editable ,resource)))
)

;                  "Test Parasite") ; name
;               "Procedure execution of gimp-resource-get-parasite failed")



; Since 3.0rc2 context-get-resource is private
; It returns active resource of given className
; (define testGradient (gimp-context-get-resource "GimpGradient"))
; context-get-resource requires a valid type name
; (assert-error `(gimp-context-get-resource "InvalidTypeName")
;              "Procedure execution of gimp-context-get-resource failed")

; This is setup, not asserted.

(test! "gimp-context-get-<resource>")

(define testBrush    (gimp-context-get-brush))
(define testFont     (gimp-context-get-font))
(define testGradient (gimp-context-get-gradient))
(define testPalette  (gimp-context-get-palette))
(define testPattern  (gimp-context-get-pattern))
; FUTURE Dynamics and other Resource subclasses


(test! "methods on active resource ID's")

(test-resource-methods testBrush)
(test-resource-methods testFont)
(test-resource-methods testGradient)
(test-resource-methods testPalette)
(test-resource-methods testPattern)



(test! "resource-id-is-foo methods")

; the resource IDs from setup work with the specific id-is-foo methods
(assert `(gimp-resource-id-is-brush ,testBrush))
(assert `(gimp-resource-id-is-font ,testFont))
(assert `(gimp-resource-id-is-gradient ,testGradient))
(assert `(gimp-resource-id-is-palette ,testPalette))
(assert `(gimp-resource-id-is-pattern ,testPattern))




(script-fu-use-v2)
