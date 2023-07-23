; test Layer methods of PDB
; where methods are operations


;        setup
; Image owns Layers

; method new from fresh GIMP state returns ID
;(define testImage 1)
;(set! testImage (car (gimp-image-new 21 22 RGB)))
;(displayln testImage)

(assert '(= (car (gimp-image-new 21 22 RGB))
            8 ))



; new method yields layer ID 10
(assert '(= (car (gimp-layer-new
                    8
                    21
                    22
                    RGB-IMAGE
                    "LayerNew#2"
                    50.0
                    LAYER-MODE-NORMAL))
            10))
; assert layer is not inserted in image


;               errors when layer not in image

; resize fails
(assert-error '(gimp-layer-resize 10 23 24 0 0)
              (string-append
                "Procedure execution of gimp-layer-resize failed on invalid input arguments: "
                "Item 'LayerNew#2' (10) cannot be used because it has not been added to an image"))

; scale fails
(assert-error '(gimp-layer-scale 10
                  23 24 ; width height
                  0)    ; is local origin?
              (string-append
                "Procedure execution of gimp-layer-scale failed on invalid input arguments: "
                "Item 'LayerNew#2' (10) cannot be used because it has not been added to an image"))

; gimp-layer-resize-to-image-size fails
; TODO

; gimp-layer-remove-mask fails when layer has no mask
(assert-error '(gimp-layer-remove-mask
                  10
                  MASK-APPLY)
              (string-append
                "Procedure execution of gimp-layer-remove-mask failed on invalid input arguments: "
                "Item 'LayerNew#2' (10) cannot be used because it has not been added to an image"))



;              alpha operations

; add-alpha succeeds
(assert '(gimp-layer-add-alpha 10))

; and is effective
; Note method on superclass Drawable
(assert '(= (car (gimp-drawable-has-alpha 10))
            1))

; flatten succeeds
(assert '(gimp-layer-flatten 10))

; flatten was effective: no longer has alpha
; flatten a layer means "remove alpha"
(assert '(= (car (gimp-drawable-has-alpha 10))
            0))




;              delete

; delete succeeds
(assert '(gimp-layer-delete 10))

; delete second time fails
(assert-error '(gimp-layer-delete 10)
              "runtime: invalid item ID")

; Error for flatten:
; "Procedure execution of gimp-layer-delete failed on invalid input arguments: "
; "Procedure 'gimp-layer-delete' has been called with an invalid ID for argument 'layer'. "
; "Most likely a plug-in is trying to work on a layer that doesn't exist any longer."))

; delete layer when image already deleted fails
; TODO


