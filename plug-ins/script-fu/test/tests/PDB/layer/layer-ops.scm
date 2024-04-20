; test Layer methods of PDB
; where methods are operations


(script-fu-use-v3)



;        setup

(define testImage (gimp-image-new 21 22 RGB))

(define testLayer (gimp-layer-new
                    testImage
                    21
                    22
                    RGB-IMAGE
                    "LayerNew#2"
                    50.0
                    LAYER-MODE-NORMAL))
; assert layer is not inserted in image


(test! "errors when layer not in image")

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

; UNTESTED gimp-layer-resize-to-image-size fails when layer not in image

; gimp-layer-remove-mask fails when layer not in image
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
; returns #t
(assert `(gimp-drawable-has-alpha ,testLayer))

; flatten succeeds
(assert `(gimp-layer-flatten ,testLayer))

; flatten was effective: no longer has alpha
; flatten a layer means "remove alpha"
; returns #f
(assert `(not (gimp-drawable-has-alpha ,testLayer)))




(test! "layer-delete")

; gimp-layer-delete is deprecated

; succeeds
(assert `(gimp-item-delete ,testLayer))

; delete second time fails
(assert-error `(gimp-item-delete ,testLayer)
              "runtime: invalid item ID")

; Error for flatten:
; "Procedure execution of gimp-layer-delete failed on invalid input arguments: "
; "Procedure 'gimp-layer-delete' has been called with an invalid ID for argument 'layer'. "
; "Most likely a plug-in is trying to work on a layer that doesn't exist any longer."))


(script-fu-use-v2)
