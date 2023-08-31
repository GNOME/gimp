; test Layer methods of PDB
; where methods are operations


;        setup

(define testImage (car (gimp-image-new 21 22 RGB)))

(define
  testLayer (car (gimp-layer-new
                    testImage
                    21
                    22
                    RGB-IMAGE
                    "LayerNew#2"
                    50.0
                    LAYER-MODE-NORMAL)))
; assert layer is not inserted in image


;               errors when layer not in image

; resize fails
(assert-error `(gimp-layer-resize ,testLayer 23 24 0 0)
              (string-append
                "Procedure execution of gimp-layer-resize failed on invalid input arguments: "))
                ;"Item 'LayerNew#2' (10) cannot be used because it has not been added to an image"))

; scale fails
(assert-error `(gimp-layer-scale ,testLayer
                  23 24 ; width height
                  0)    ; is local origin?
              (string-append
                "Procedure execution of gimp-layer-scale failed on invalid input arguments: "))
                ;"Item 'LayerNew#2' (10) cannot be used because it has not been added to an image"))

; gimp-layer-resize-to-image-size fails
; TODO

; gimp-layer-remove-mask fails when layer has no mask
(assert-error `(gimp-layer-remove-mask
                  ,testLayer
                  MASK-APPLY)
              (string-append
                "Procedure execution of gimp-layer-remove-mask failed on invalid input arguments: "))
                ; "Item 'LayerNew#2' (10) cannot be used because it has not been added to an image"))



;              alpha operations

; add-alpha succeeds
(assert `(gimp-layer-add-alpha ,testLayer))

; and is effective
; Note method on superclass Drawable
(assert `(= (car (gimp-drawable-has-alpha ,testLayer))
            1))

; flatten succeeds
(assert `(gimp-layer-flatten ,testLayer))

; flatten was effective: no longer has alpha
; flatten a layer means "remove alpha"
(assert `(= (car (gimp-drawable-has-alpha ,testLayer))
            0))




;              delete

; delete succeeds
(assert `(gimp-layer-delete ,testLayer))

; delete second time fails
(assert-error `(gimp-layer-delete ,testLayer)
              "runtime: invalid item ID")

; Error for flatten:
; "Procedure execution of gimp-layer-delete failed on invalid input arguments: "
; "Procedure 'gimp-layer-delete' has been called with an invalid ID for argument 'layer'. "
; "Most likely a plug-in is trying to work on a layer that doesn't exist any longer."))

; delete layer when image already deleted fails
; TODO


