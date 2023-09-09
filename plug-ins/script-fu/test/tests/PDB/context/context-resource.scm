; test resource methods of Context



; function to test methods on Resource
; for a valid Resource ID
(define (test-resource-methods resource)

  ; a resource is an int ID in ScriptFu
  (assert `(integer? ,resource))

  ; get-name returns a string
  (assert `(string? (car (gimp-resource-get-name ,resource))))

  ; id-is-valid returns truth
  ; (1) FUTURE #t
  (assert `(car (gimp-resource-id-is-valid ,resource)))

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
  (assert `(gimp-resource-is-editable ,resource))

  ; The fresh gimp active resources are all system resources i.e. not editable
  ; returns 0 for #f
  (assert `(= (car(gimp-resource-is-editable ,resource))
              0))
)

;                  "Test Parasite") ; name
;               "Procedure execution of gimp-resource-get-parasite failed")



; test context-get-resource returns active resource of given className
; Setup. Not assert.

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

(assert `(= (car(gimp-resource-id-is-brush ,testBrush))
            1))
(assert `(= (car(gimp-resource-id-is-font ,testFont))
            1))
(assert `(= (car(gimp-resource-id-is-gradient ,testGradient))
            1))
(assert `(= (car(gimp-resource-id-is-palette ,testPalette))
            1))
(assert `(= (car(gimp-resource-id-is-pattern ,testPattern))
            1))


;             test errors


; invalid type name
(assert-error `(gimp-context-get-resource "InvalidTypeName")
              "Procedure execution of gimp-context-get-resource failed")

; invalid numeric ID
; -1 is out of range
(assert-error `(gimp-resource-get-name -1)
              "Procedure execution of gimp-resource-get-name failed on invalid input arguments:")
; 12345678 is in range but invalid
(assert-error `(gimp-resource-get-name 12345678)
              "Procedure execution of gimp-resource-get-name failed on invalid input arguments:")

