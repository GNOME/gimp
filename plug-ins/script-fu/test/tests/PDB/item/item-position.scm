; test Item methods for positioning in stack

; This tests that all calls minimally work.
; TODO test stack semantics i.e. algebra of stacking operations.

; All the calls named for layer, channel, vectors are deprecated
; in favor of these more generic "item" calls



; setup
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage ))
                                  0))




; position of single layer is index zero
(assert `(= (car (gimp-image-get-item-position ,testImage ,testLayer))
            0))


; Single layer is both at top and at bottom of stack and cannot be raised or lowered
(assert-error `(gimp-image-raise-item ,testImage ,testLayer)
              "Procedure execution of gimp-image-raise-item failed: Layer cannot be raised higher. ")

(assert-error `(gimp-image-lower-item ,testImage ,testLayer)
              "Procedure execution of gimp-image-lower-item failed: Layer cannot be lowered more. ")



; Can raise to top and bottom without throwing error, but no effect
(assert `(gimp-image-raise-item-to-top ,testImage ,testLayer))

(assert `(gimp-image-lower-item-to-bottom ,testImage ,testLayer))


; FIXME API naming inconsistent: should image-reorder-item => image-item-reorder

; Cannot reorder an item to make parent itself
(assert-error `(gimp-image-reorder-item ,testImage ,testLayer ,testLayer 0)
              "Procedure execution of gimp-image-reorder-item failed on invalid input arguments:"  )
; " Item 'Background' (2) cannot be used because it is not a group item"




